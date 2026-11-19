#!/usr/bin/env python3
"""Execute the production C++ application constructor without test fixtures.

The ELF must retain symbols and DWARF. Its real initialization functions,
C++ IPAddress/WiFiClient constructors, owned state setup, and service bindings
execute under Unicorn. Only millis and __aeabi_atexit are controlled; no
hardware, UI, network, or application constructor is replaced with a stub.

This starts at the application's .init_array entry after RAM/BSS initialization.
It does not emulate ResetISR, SDK peripheral startup, setup(), or the appliance.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(ROOT / 'tools'))
from elftools.elf.elffile import ELFFile
from rebuilt_arm import RebuiltArm


class Layout:
    """Use actual compiler DWARF, not guessed native C++ object offsets."""
    def __init__(self, elf_path):
        self.handle = Path(elf_path).open('rb')
        self.elf = ELFFile(self.handle)
        if not self.elf.has_dwarf_info():
            raise ValueError('Production ELF must retain DWARF for native layout checks')
        self.dwarf = self.elf.get_dwarf_info()
        self.types = {}
        for cu in self.dwarf.iter_CUs():
            for die in cu.iter_DIEs():
                name = die.attributes.get('DW_AT_name')
                if name and 'DW_AT_byte_size' in die.attributes and die.tag in (
                        'DW_TAG_structure_type', 'DW_TAG_class_type', 'DW_TAG_union_type'):
                    self.types.setdefault(name.value.decode(), die)
        self.functions = set()
        self.symbol_sizes = {}
        self.local_symbols = {}
        source = ''
        for symbol in self.elf.get_section_by_name('.symtab').iter_symbols():
            if symbol['st_info']['type'] == 'STT_FILE':
                source = symbol.name
            if symbol['st_info']['type'] == 'STT_FUNC':
                self.functions.add(symbol['st_value'])
            if symbol.name:
                self.symbol_sizes[symbol.name] = symbol['st_size']
                if symbol['st_info']['bind'] == 'STB_LOCAL':
                    self.local_symbols[source, symbol.name] = symbol['st_value']

    def resolve(self, die):
        while die.tag in ('DW_TAG_typedef', 'DW_TAG_const_type', 'DW_TAG_volatile_type',
                          'DW_TAG_restrict_type', 'DW_TAG_atomic_type'):
            die = die.get_DIE_from_attribute('DW_AT_type')
        if 'DW_AT_declaration' in die.attributes and 'DW_AT_name' in die.attributes:
            # GCC may put only a base-class declaration in one compilation
            # unit while emitting its complete layout in another.
            die = self.types.get(die.attributes['DW_AT_name'].value.decode(), die)
        return die

    def record(self, name):
        try:
            return self.types[name]
        except KeyError as error:
            raise ValueError('Missing complete DWARF type: ' + name) from error

    def member(self, record, name):
        record = self.record(record) if isinstance(record, str) else self.resolve(record)
        for child in record.iter_children():
            member_name = child.attributes.get('DW_AT_name')
            if child.tag == 'DW_TAG_member' and member_name and member_name.value.decode() == name:
                offset = child.attributes.get('DW_AT_data_member_location')
                value = offset.value if offset else 0
                if not isinstance(value, int):
                    raise ValueError('Nonconstant native member offset: ' + name)
                return value, self.resolve(child.get_DIE_from_attribute('DW_AT_type'))
        # C++ base classes and C anonymous unions are represented explicitly.
        for child in record.iter_children():
            if child.tag == 'DW_TAG_inheritance' or (
                    child.tag == 'DW_TAG_member' and 'DW_AT_name' not in child.attributes):
                offset = child.attributes.get('DW_AT_data_member_location')
                value = offset.value if offset else 0
                try:
                    nested, member_type = self.member(child.get_DIE_from_attribute('DW_AT_type'), name)
                    return value + nested, member_type
                except KeyError:
                    pass
        raise KeyError(name)

    def offset(self, record, path):
        offset = 0
        current = self.record(record)
        for part in path.split('.'):
            relative, current = self.member(current, part)
            offset += relative
        return offset

    def size(self, record):
        return self.record(record).attributes['DW_AT_byte_size'].value

    def callbacks(self, record):
        result = []
        for child in self.record(record).iter_children():
            if child.tag != 'DW_TAG_member':
                continue
            name = child.attributes['DW_AT_name'].value.decode()
            _, member_type = self.member(record, name)
            if member_type.tag != 'DW_TAG_pointer_type':
                continue
            if 'DW_AT_type' not in member_type.attributes:
                continue
            target = member_type.get_DIE_from_attribute('DW_AT_type')
            if target is not None and self.resolve(target).tag == 'DW_TAG_subroutine_type':
                result.append(name)
        return result


def inspect_startup(elf_path, layout, ticks, registration_result):
    arm = RebuiltArm(elf_path)
    checks = []
    failures = []
    clock_values = []
    registrations = []
    symbol = arm.address
    word = arm.read_u32
    location = lambda base, record, path: base + layout.offset(record, path)
    u32 = lambda value: value & 0xffffffff

    def check(name, actual, expected):
        if actual != expected:
            failures.append(f'{name}: actual {actual!r}, expected {expected!r}')
        else:
            checks.append(name)

    def scalar(base, record, member, expected, kind='I'):
        address = location(base, record, member)
        actual = struct.unpack('<' + kind, arm.read(address, struct.calcsize(kind)))[0]
        check(record + '.' + member, actual, expected)

    def pointer(base, record, member, expected):
        scalar(base, record, member, expected)

    def millis(runner):
        if len(clock_values) >= len(ticks):
            raise AssertionError('Unexpected extra clock call during native startup')
        value = ticks[len(clock_values)]
        clock_values.append(value)
        return value

    def register_destructor(runner):
        registrations.append(tuple(runner.reg(i) for i in range(3)))
        return registration_result

    arm.hook_symbol('millis', millis)
    arm.hook_symbol('__aeabi_atexit', register_destructor)
    check('production ELF excludes validation entry', 'pico_validation_entry' in arm.symbols, False)
    check('production ELF excludes recipe fixture',
          any(name.startswith('recipe_fixture_') for name in arm.symbols), False)
    entry = symbol('_GLOBAL__sub_I_setup')
    array_start, array_end = symbol('__init_array_start'), symbol('__init_array_end')
    array = struct.unpack('<' + 'I' * ((array_end - array_start) // 4),
                          arm.read(array_start, array_end - array_start))
    check('application constructor occurs once in native init array', array.count(entry), 1)
    bss = layout.elf.get_section_by_name('.bss')
    check('fresh BSS matches reset-stage zero initialization',
          arm.read(bss['sh_addr'], bss['sh_size']), bytes(bss['sh_size']))
    execution = arm.call_symbol('_GLOBAL__sub_I_setup', max_instructions=250000)
    check('two PID constructor clock calls', clock_values, list(ticks))

    application = symbol('_ZL11application')
    native = symbol('_ZL14native_objects')
    initialization = symbol('_ZL14initialization')
    control_runtime = symbol('_ZL15control_runtime')
    network_runtime = symbol('_ZL15network_runtime')
    objects = {name: location(application, 'PicoApplication', name)
               for name in ('state', 'settings', 'network', 'ui', 'device', 'reader',
                            'rfid', 'calibration', 'phase', 'recipe')}
    state = objects['state']
    controller = location(state, 'PicoAppState', 'controller')
    signals = location(state, 'PicoAppState', 'control_signals')
    screen = location(state, 'PicoAppState', 'screen')
    carousel = location(state, 'PicoAppState', 'carousel')
    session = location(state, 'PicoAppState', 'session')
    recipe_state = location(state, 'PicoAppState', 'recipe')
    native_control = location(control_runtime, 'PicoEnergiaControl', 'control')
    actuators = location(control_runtime, 'PicoEnergiaControl', 'actuators')
    settings = objects['settings']
    settings_field = lambda name: location(settings, 'PicoSettings', name)
    http = location(native, 'PicoEnergiaObjects', 'http_client')

    # Actual SDK constructors run. Verify their native vtables and initialized
    # base-class fields using the compiler's inheritance/member descriptions.
    for member in ('still_home_address', 'still_active_address', 'server_address'):
        address = location(native, 'PicoEnergiaObjects', member)
        check(member + ' IPAddress vtable', word(address), symbol('_ZTV9IPAddress') + 8)
        scalar(address, 'IPAddress', '_address', 0)
    check('PicoHttpClient native vtable', word(http), symbol('_ZTV14PicoHttpClient') + 8)
    scalar(http, 'WiFiClient', '_socketIndex', 255)
    scalar(http, 'WiFiClient', 'rx_fillLevel', 0)
    scalar(http, 'WiFiClient', 'rx_currentIndex', 0)
    scalar(http, 'WiFiClient', 'hasRootCA', 0, 'B')
    scalar(http, 'WiFiClient', 'sslVerifyStrict', 0, 'B')
    scalar(http, 'WiFiClient', 'sslLastError', 0, 'i')
    scalar(http, 'WiFiClient', '_timeout', 1000)
    scalar(http, 'WiFiClient', 'write_error', 0, 'i')
    pointer(network_runtime, 'PicoEnergiaNetwork', 'client', http)
    pointer(initialization, 'PicoEnergiaInitialization', 'objects', native)
    pointer(initialization, 'PicoEnergiaInitialization', 'rfid_reader', objects['reader'])
    pointer(initialization, 'PicoEnergiaInitialization', 'rfid_context', 0)
    pointer(initialization, 'PicoEnergiaInitialization', 'construct_rfid',
            symbol('_ZL16construct_readerPvS_hhh'))
    scalar(initialization, 'PicoEnergiaInitialization', 'destructor_registration_result',
           registration_result, 'i')
    check('exactly one native HTTP destructor registration', len(registrations), 1)
    destructor = symbol('_ZL12destroy_httpPv')
    check('native HTTP destructor registration arguments', registrations[0],
          (http, destructor, symbol('__dso_handle')))

    # Owned constructor defaults and internal PID pointers are established by
    # the real state/PID functions, not by this test assigning expected memory.
    scalar(state, 'PicoAppState', 'device_state', 2, 'B')
    scalar(controller, 'PicoBrewController', 'compartment', 2, 'B')
    scalar(controller, 'PicoBrewController', 'phase', 4, 'B')
    scalar(controller, 'PicoBrewController', 'status', 0, 'B')
    scalar(controller, 'PicoBrewController', 'wort_fahrenheit', 70.0, 'd')
    scalar(controller, 'PicoBrewController', 'thermoblock_fahrenheit', 83.0, 'd')
    scalar(controller, 'PicoBrewController', 'thermoblock_target_fahrenheit', 300.0, 'd')
    scalar(controller, 'PicoBrewController', 'active_shuttle_scale', 0.2, 'd')
    scalar(controller, 'PicoBrewController', 'candidate_shuttle_scale', 0.2, 'd')
    scalar(controller, 'PicoBrewController', 'still.temperature2_limit', 95.0, 'd')
    scalar(controller, 'PicoBrewController', 'still.temperature2_margin', 5.0, 'd')
    scalar(controller, 'PicoBrewController', 'still.pressure_limit', 10.0, 'd')
    scalar(controller, 'PicoBrewController', 'still.communication_failure_limit', 5)
    scalar(signals, 'PicoControlSignals', 'enabled', 0, 'B')
    scalar(signals, 'PicoControlSignals', 'progress_active', 0, 'B')
    for index, (member, input_name, output_name, target_name, gains) in enumerate((
            ('wort_pid', 'wort_fahrenheit', 'wort_pid_output', 'wort_target_fahrenheit', (130., 50., 30.)),
            ('thermoblock_pid', 'thermoblock_fahrenheit', 'thermoblock_pid_output',
             'thermoblock_target_fahrenheit', (150., 50., 80.)))):
        pid = location(controller, 'PicoBrewController', member)
        pointer(pid, 'PicoPid', 'input', location(controller, 'PicoBrewController', input_name))
        pointer(pid, 'PicoPid', 'output', location(controller, 'PicoBrewController', output_name))
        pointer(pid, 'PicoPid', 'setpoint', location(controller, 'PicoBrewController', target_name))
        for field, value in zip(('displayedProportionalGain', 'displayedIntegralGain',
                                 'displayedDerivativeGain'), gains):
            scalar(pid, 'PicoPid', field, value, 'd')
        scalar(pid, 'PicoPid', 'lastUpdateMs', u32(ticks[index] - 100))
        scalar(pid, 'PicoPid', 'sampleIntervalMs', 100)
        scalar(pid, 'PicoPid', 'maximumOutput', 3000., 'd')
        scalar(pid, 'PicoPid', 'automatic', 0, 'B')
    pointer(recipe_state, 'PicoRecipe', 'program', session)
    scalar(recipe_state, 'PicoRecipe', 'abv_adjustment', -1, 'i')
    scalar(recipe_state, 'PicoRecipe', 'ibu_adjustment', -1, 'i')
    scalar(recipe_state, 'PicoRecipe', 'step_count', 0, 'B')
    steps = location(recipe_state, 'PicoRecipe', 'steps')
    for index in range(15):
        scalar(steps + index * layout.size('PicoRecipeStep'), 'PicoRecipeStep', 'location', 2, 'B')

    page_names = ['pico_screen_brew_name_page', 'pico_screen_current_step_page',
                  'pico_screen_step_progress_page', 'pico_screen_temperature_page',
                  'pico_screen_recipe_metrics_page']
    expected_pages = [symbol(name) for name in page_names]
    scalar(carousel, 'PicoPageCarousel', 'page_count', 5)
    scalar(carousel, 'PicoPageCarousel', 'interval_ms', 10000)
    pages = struct.unpack('<15I', arm.read(location(carousel, 'PicoPageCarousel', 'pages'), 60))
    check('constructor normal carousel order', list(pages), expected_pages + [0] * 10)
    check('runtime normal carousel table order',
          list(struct.unpack('<5I', arm.read(symbol('recipe_pages'), 20))), expected_pages)

    # Check the application graph after both control and service bindings.
    pointer(application, 'PicoApplication', 'control', native_control)
    pointer(application, 'PicoApplication', 'actuators', actuators)
    pointer(application, 'PicoApplication', 'hardware', symbol('_ZL8hardware'))
    pointer(application, 'PicoApplication', 'hardware_context', 0)
    pointer(control_runtime, 'PicoEnergiaControl', 'state', state)
    pointer(control_runtime, 'PicoEnergiaControl', 'ui', symbol('_ZL10control_ui'))
    pointer(control_runtime, 'PicoEnergiaControl', 'ui_context', application)
    for field, expected in [('controller', controller), ('signals', signals),
                            ('ops', symbol('_ZL11control_ops')), ('context', control_runtime)]:
        pointer(native_control, 'PicoControl', field, expected)
    pointer(actuators, 'PicoActuators', 'ops', symbol('_ZL12actuator_ops'))
    pointer(actuators, 'PicoActuators', 'context', control_runtime)
    for field, expected in [('app', state), ('settings', settings),
                            ('platform', symbol('_ZZ29pico_energia_network_platformvE8platform')),
                            ('platform_context', network_runtime), ('ui', symbol('network_ui')),
                            ('ui_context', application)]:
        pointer(objects['network'], 'PicoNetwork', field, expected)
    for field, expected in [('control', native_control), ('actuators', actuators), ('screen', screen),
                            ('session', session), ('calibration', objects['calibration']),
                            ('language', settings_field('language')), ('ops', symbol('phase_ops')),
                            ('context', application)]:
        pointer(objects['phase'], 'PicoPhaseControl', field, expected)
    for field, expected in [('state', state), ('phase', objects['phase']),
                            ('language', settings_field('language')),
                            ('detailed_status', settings_field('detailed_status')),
                            ('offline', settings_field('offline_mode')),
                            ('legacy_compartment', settings_field('legacy_compartment_mapping')),
                            ('assets', symbol('program_assets')), ('pages', symbol('recipe_pages')),
                            ('ops', symbol('recipe_ops')), ('context', application), ('active_title', 0)]:
        pointer(objects['recipe'], 'PicoRecipeRuntime', field, expected)
    for field, expected in [('app', state), ('network', objects['network']), ('control', native_control),
                            ('actuators', actuators), ('hardware', symbol('ui_hardware')),
                            ('hardware_context', application), ('services', symbol('ui_services')),
                            ('service_context', application)]:
        pointer(objects['ui'], 'PicoUi', field, expected)
    check('UI singleton points at owned application UI', word(symbol('pico_ui_active')), objects['ui'])
    for field, expected in [('state', state), ('actuators', actuators),
                            ('legacy_compartment_mapping', settings_field('legacy_compartment_mapping')),
                            ('home_compartment', settings_field('home_compartment')),
                            ('ops', symbol('device_ops')), ('context', application)]:
        pointer(objects['device'], 'PicoDevice', field, expected)
    for field, expected in [('select_pin', 4), ('shared_select_pin', 18), ('auxiliary_pin', 0)]:
        scalar(objects['reader'], 'PicoRfid', field, expected, 'B')
    pointer(objects['reader'], 'PicoRfid', 'ops', symbol('rfid_ops'))
    pointer(objects['reader'], 'PicoRfid', 'context', application)
    for field, expected in [('reader', objects['reader']), ('ops', symbol('rfid_application_ops')),
                            ('context', application)]:
        pointer(objects['rfid'], 'PicoRfidApplication', field, expected)
    scalar(settings, 'PicoSettings', 'home_compartment', 6, 'B')
    display = location(objects['ui'], 'PicoUi', 'display')
    for field, value, kind in [('width', 128, 'h'), ('height', 64, 'h'), ('text_scale', 1, 'B'),
                                ('wrap', 1, 'B'), ('command_data_pin', 13, 'B'),
                                ('chip_select_pin', 4, 'B'), ('shared_chip_select_pin', 18, 'B')]:
        scalar(display, 'PicoDisplay', field, value, kind)
    encoder = location(objects['ui'], 'PicoUi', 'encoder')
    scalar(encoder, 'PicoEncoder', 'step', 1, 'i')
    scalar(encoder, 'PicoEncoder', 'position', 0, 'i')

    # Every service slot must reference an actual Thumb function in this ELF.
    tables = [('_ZL8hardware', 'PicoApplicationHardware'), ('_ZL10control_ui', 'PicoEnergiaUi'),
              ('_ZL11control_ops', 'PicoControlOps'), ('_ZL12actuator_ops', 'PicoActuatorOps'),
              ('_ZL18initialization_ops', 'PicoInitializationOps'), ('recipe_ops', 'PicoRecipeOps'),
              ('phase_ops', 'PicoPhaseOps'), ('device_ops', 'PicoDeviceOps'), ('rfid_ops', 'PicoRfidOps'),
              ('rfid_application_ops', 'PicoRfidApplicationOps'), ('ui_hardware', 'PicoUiHardware'),
              ('ui_services', 'PicoUiServices'), ('network_ui', 'PicoNetworkUi'),
              ('_ZZ29pico_energia_network_platformvE8platform', 'PicoNetworkPlatform')]
    callback_count = 0
    for table_symbol, record in tables:
        for field in layout.callbacks(record):
            target = word(location(symbol(table_symbol), record, field))
            check(record + '.' + field + ' is a linked Thumb function',
                  bool(target & 1) and target in layout.functions, True)
            callback_count += 1
    for name, width, height in [('pico_ui_asset_program_primary', 128, 14),
                                ('pico_ui_asset_program_secondary', 128, 16),
                                ('pico_ui_asset_sousvide_secondary', 128, 16)]:
        bitmap = symbol(name)
        scalar(bitmap, 'PicoBitmap', 'width', width, 'i')
        scalar(bitmap, 'PicoBitmap', 'height', height, 'i')
        pixels = word(location(bitmap, 'PicoBitmap', 'pixels'))
        check(name + ' contains owned pixel data',
              0x20004000 <= pixels < symbol('__init_array_end'), True)

    # Exercise small, non-actuating callbacks through the bound function
    # pointers, proving context translation into the actual owned objects.
    callback = word(location(symbol('recipe_ops'), 'PicoRecipeOps', 'set_recipe_name'))
    arm.write(arm.SCRATCH, b'Native lifecycle test\0')
    arm.call(callback & ~1, [application, arm.SCRATCH], max_instructions=50000)
    check('bound recipe-name callback reaches owned screen',
          arm.cstring(location(screen, 'PicoScreenState', 'recipe_name')), b'Native lifecycle test')
    callback = word(location(symbol('_ZL12actuator_ops'), 'PicoActuatorOps', 'current_phase'))
    value = arm.call(callback & ~1, [control_runtime], max_instructions=10000)['r0']
    check('bound actuator phase callback reads owned controller', value, 4)
    # Invoke the registered real destructor after all live-object checks. The
    # native client has no socket; no WiFi or file service should be needed.
    destruction = arm.call(destructor & ~1, [http], max_instructions=100000)
    check('native destructor returns with original stack', destruction['sp'], arm.STACK)
    check('callbacks/destructor did not require additional clock stubs', clock_values, list(ticks))
    host = arm.cstring(word(location(objects['network'], 'PicoNetwork', 'server_host'))).decode()
    port = struct.unpack('<H', arm.read(location(objects['network'], 'PicoNetwork', 'server_port'), 2))[0]
    if failures:
        raise AssertionError('Native startup checks failed:\n' + '\n'.join(failures))
    return {'clock_values': list(ticks), 'atexit_result': registration_result,
            'constructor_instructions': execution['instructions'],
            'destructor_instructions': destruction['instructions'],
            'checks_passed': len(checks), 'callback_slots_checked': callback_count,
            'application_address': hex(application), 'native_http_address': hex(http),
            'server_host': host, 'server_port': port, 'carousel_order': page_names,
            'checks': checks}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/firmware/pico-pro-recovered.elf')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/native-startup-tests.json')
    options = parser.parse_args()
    layout = Layout(options.elf)
    cases = []
    for ticks, registration in [((0, 1), 0), ((123456, 123999), 0),
                                 ((0xfffffff0, 0x20), 0), ((1000, 2000), -1)]:
        cases.append(inspect_startup(options.elf, layout, ticks, registration))
    report = {
        'passed': True,
        'total_cases': len(cases),
        'total_checks': sum(case['checks_passed'] for case in cases),
        'production_elf_sha256': hashlib.sha256(options.elf.read_bytes()).hexdigest(),
        'test_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'entry': '_GLOBAL__sub_I_setup',
        'controlled_boundaries': ['millis', '__aeabi_atexit'],
        'hardware_boundaries_intercepted': [],
        'test_fixture_symbols_added': False,
        'scope': 'Actual production application constructor after zeroed BSS and loaded data; real SDK native object construction and destructor; owned state and service graph; no ResetISR, peripheral setup, network session, or complete appliance emulation.',
        'cases': cases,
    }
    options.output.parent.mkdir(parents=True, exist_ok=True)
    options.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS native production startup: {len(cases)} cases, '
          f'{sum(case["checks_passed"] for case in cases)} checks; '
          f'{cases[0]["callback_slots_checked"]} callback slots per case; '
          'only millis/atexit controlled')


if __name__ == '__main__':
    main()
