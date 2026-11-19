#!/usr/bin/env python3
"""Direct original/rebuilt ARM checks for menu and diagnostic entry points.

Actual menu/state manipulation and rasterization run in both images. Only
explicit subordinate interaction, mechanical, storage and network boundaries
are controlled. The entry under comparison is never replaced by a hook.
"""
from ui_test_support import *
from test_native_startup import Layout
import argparse
from collections import Counter

parser = argparse.ArgumentParser()
parser.add_argument('--elf', type=Path, default=ELF)
parser.add_argument('--output', type=Path, default=PROJECT/'analysis/ui-direct-menus-tests.json')
parser.add_argument('--only', help='Development filter: one entry name')
options = parser.parse_args()
layout = Layout(options.elf)
manifest = json.loads((PROJECT/'analysis/recovery/ui.json').read_text())
owned_files = {'src/ui/menus.c', 'src/ui/ui_menus.c', 'src/ui/diagnostics.c',
               'src/ui/still_ui.c', 'src/ui/screens.c'}
# Stable scope: these entries were the uncovered menu/diagnostic entries when
# this suite was introduced. The ledger's verification status may later change.
NAMES = '''pico_menu_initialize pico_menu_action_at pico_menu_add_action
pico_menu_set_actions pico_menu_populate_action_labels
pico_menu_append_wifi_search_actions pico_screen_still_setup_step
pico_menu_populate_in_brew_options pico_ui_draw_menu_items
pico_diagnostic_sensors pico_menu_choose_default_timeout pico_menu_choose
pico_menu_select_action pico_ui_choose_binary_option pico_ui_confirm_still_cleaned
pico_diagnostic_drain_pump pico_diagnostic_steam_pump pico_diagnostic_heater
pico_diagnostic_stepper pico_ui_show_still_communication_failure
pico_ui_show_still_completion pico_ui_show_still_temperature_warning
pico_ui_toggle_stepper_torque pico_ui_choose_compartment pico_diagnostic_shuttle_pump
pico_diagnostic_buzzer pico_diagnostic_rfid pico_diagnostic_run_all
pico_diagnostics_menu pico_ui_confirm_at_row pico_diagnostic_wifi
pico_ui_show_network_addresses pico_ui_service_menu pico_ui_still_fan_menu
pico_utilities_menu'''.split()
entries = {e['replacement_function']: e for e in manifest['entries']
           if e['replacement_function'] in NAMES and e['replacement_file'] in owned_files}
assert set(entries) == set(NAMES)
results, failures = [], []


def argument(arm, index):
    return arm.reg(index) if index < 4 else arm.read_u32(arm.reg('sp') + 4*(index-4))


class ObservedPair(Pair):
    def __init__(self, language=0):
        super().__init__(language)
        self.events = {'old': [], 'new': []}
        self.serial = {'old': bytearray(), 'new': bytearray()}
        self.net = self.new.address('fixture_network')
        self.menu = self.app + layout.offset('PicoAppState', 'menu')
        self.tables = {}
        self.trap_count = 0
        for side, arm in [('old', self.old), ('new', self.new)]:
            arm.hook(MAP['pico_display_flush'] if side == 'old' else arm.address('pico_display_flush'),
                     lambda f, side=side: self.flush(side, f))
        self.hook('pico_ui_wait_for_continue', count=0, frame=True)
        self.hook('pico_ui_wait_for_stop', count=0, frame=True)
        self.hook('pico_display_wait_for_click', count=0, frame=True)

    def frame(self, side):
        arm = self.old if side == 'old' else self.new
        return arm.read(0x2002af60 if side == 'old' else self.fb, 1024)

    def flush(self, side, arm):
        data = self.frame(side)
        (self.old_flush if side == 'old' else self.new_flush).append(data)
        self.events[side].append(('flush', hashlib.sha256(data).hexdigest()))

    def capture(self, side, label, values=(), frame=False):
        event = (label, *values)
        if frame:
            event += (hashlib.sha256(self.frame(side)).hexdigest(),)
        self.events[side].append(event)

    def hook(self, name, original=None, count=0, old_shift=0, new_shift=0,
             returns=0, sequence=None, frame=False, label=None):
        original = original or name
        address = MAP[original] if isinstance(original, str) else original
        indices = {'old': 0, 'new': 0}
        for side, arm in [('old', self.old), ('new', self.new)]:
            shift = old_shift if side == 'old' else new_shift
            def callback(f, side=side, shift=shift):
                values = tuple(argument(f, index+shift) for index in range(count))
                self.capture(side, label or name, values, frame)
                if sequence is None:
                    return returns
                index = indices[side]
                indices[side] += 1
                assert index < len(sequence), (name, 'unexpected additional call', side)
                return sequence[index]
            arm.hook(address if side == 'old' else arm.address(name), callback)
        return indices

    def callback(self, record, field, handler):
        """Install a test callback using compiler-derived table/field offsets."""
        if record not in self.tables:
            owner, owner_type, member, table = (
                (self.ui, 'PicoUi', 'services', 0x2008c000) if record == 'PicoUiServices'
                else (self.net, 'PicoNetwork', 'platform', 0x2008d000))
            location = owner + layout.offset(owner_type, member)
            source = self.new.read_u32(location)
            size = layout.record(record).attributes['DW_AT_byte_size'].value
            self.new.write(table, self.new.read(source, size))
            self.new.write_u32(location, table)
            self.tables[record] = table
        trap = 0x2008e000 + self.trap_count*4
        self.trap_count += 1
        self.new.write(trap, b'\x70\x47')
        self.new.hook(trap, handler)
        self.new.write_u32(self.tables[record] + layout.offset(record, field), trap | 1)

    def service(self, field, original, count=0, returns=0, frame=False):
        old_address = MAP[original] if isinstance(original, str) else original
        self.old.hook(old_address, lambda f: self.service_call(
            'old', field, f, 0, count, returns, frame))
        self.callback('PicoUiServices', field, lambda f: self.service_call(
            'new', field, f, 1, count, returns, frame))

    def service_call(self, side, field, arm, shift, count, returns, frame):
        self.capture(side, field, tuple(argument(arm, shift+i) for i in range(count)), frame)
        return returns

    def clock(self, step=401, start=0):
        counts = {'old': 0, 'new': 0}
        def now(side):
            value = u32(start + step*counts[side])
            counts[side] += 1
            self.capture(side, 'millis', (value,))
            return value
        self.old.hook(MAP['millis'], lambda f: now('old'))
        self.new.hook_symbol('ui_fixture_millis', lambda f: now('new'))
        self.old.hook(MAP['delay'], lambda f: self.capture('old', 'delay', (f.reg(0),)))
        self.new.hook_symbol('ui_fixture_delay', lambda f: self.capture('new', 'delay', (f.reg(1),)))
        return counts

    def clicks(self, after):
        counts = {'old': 0, 'new': 0}
        def click(side):
            counts[side] += 1
            value = 255 if counts[side] >= after else 0
            self.capture(side, 'click', (value,))
            return value
        self.old.hook(MAP['pico_encoder_take_click'], lambda f: click('old'))
        self.new.hook_symbol('pico_encoder_take_click', lambda f: click('new'))

    def menu_state(self, actions, count=None):
        values = list(actions) + [0]*(22-len(actions))
        data = struct.pack('<23I', *values, len(actions) if count is None else count)
        self.old.write(0x2002bc74, data)
        self.new.write(self.menu, data)

    def compare_menu(self):
        assert self.old.read(0x2002bc74, 92) == self.new.read(self.menu, 92), 'menu structure differs'

    def invoke(self, name, args=(), stack=(), new_args=None, new_stack=None,
               returns=False, max_instructions=15000000):
        address = int(entries[name]['original_address'], 16)
        self.old.unhook(address)
        self.new.unhook(self.new.address(name))
        # These calls execute the real function bodies, not the fixture callbacks.
        before = self.old.call(address, args, stack_args=stack, max_instructions=max_instructions)
        after = self.new.call_symbol(name, args if new_args is None else new_args,
                                     stack_args=stack if new_stack is None else new_stack,
                                     max_instructions=max_instructions)
        if returns:
            assert before['r0'] == after['r0'], ('return', before['r0'], after['r0'])
        assert self.events['old'] == self.events['new'], ('ordered effects', self.events)
        assert self.serial['old'] == self.serial['new'], ('serial bytes', self.serial)
        self.check_display()
        self.check_labels()
        return before['r0'], after['r0']


def case(name, label, run, language=0):
    if options.only and name != options.only:
        return
    try:
        pair = ObservedPair(language)
        run(pair)
        results.append({'function': name, 'original_address': entries[name]['original_address'],
                        'case': label, 'language': language,
                        'observed_effects': len(pair.events['new']),
                        'frame_sha256': hashlib.sha256(pair.frame('new')).hexdigest()})
    except Exception as error:
        failures.append({'function': name, 'case': label, 'language': language, 'error': str(error)})
        print('FAIL', name, label, language, str(error), flush=True)


for fill in (0, 0xa5, 0xff):
    def run(p, fill=fill):
        for arm in (p.old, p.new): arm.write(arm.SCRATCH, bytes([fill])*96)
        p.invoke('pico_menu_initialize', [p.old.SCRATCH])
        assert p.old.read(p.old.SCRATCH, 96) == p.new.read(p.new.SCRATCH, 96)
        assert p.new.read(p.new.SCRATCH, 92) == b'\0'*92
    case('pico_menu_initialize', f'fill={fill}', run)

for index in (-1, 0, 2, 3, 21, 22, 0x7fffffff):
    def run(p, index=index):
        p.menu_state([12, 17, 49])
        p.invoke('pico_menu_action_at', [0x2002bc74, u32(index)],
                 new_args=[p.menu, u32(index)], returns=True)
        p.compare_menu()
    case('pico_menu_action_at', f'index={index}', run)

for count in (0, 21, 22):
    for action in (0, 12, 49, 50, 0xffffffff):
        def run(p, count=count, action=action):
            p.menu_state(range(count), count)
            p.invoke('pico_menu_add_action', [0x2002bc74, action], new_args=[p.menu, action])
            p.compare_menu()
        case('pico_menu_add_action', f'count={count},action={action}', run)

for count, null, invalid in [(0, False, False), (1, False, False), (21, False, False),
                            (22, False, False), (23, False, False), (3, True, False),
                            (-1, False, False), (4, False, True)]:
    def run(p, count=count, null=null, invalid=invalid):
        p.menu_state([49]*22)
        data = struct.pack('<24I', *([12, 50, 14, 0xffffffff] if invalid else [12, 13, 14, 15]), *range(20))
        for arm in (p.old, p.new): arm.write(arm.SCRATCH, data)
        p.invoke('pico_menu_set_actions', [0x2002bc74, 0 if null else p.old.SCRATCH, u32(count)],
                 new_args=[p.menu, 0 if null else p.new.SCRATCH, u32(count)])
        p.compare_menu()
    case('pico_menu_set_actions', f'count={count},null={null},invalid={invalid}', run)

for language in range(3):
    for status in (0, 1):
        for count in (0, 1, 10, 22):
            def run(p, count=count, status=status, language=language):
                data = struct.pack('<22I', *range(22))
                for arm in (p.old, p.new): arm.write(arm.SCRATCH, data)
                p.invoke('pico_menu_populate_action_labels', [p.old.SCRATCH, count, language, status])
                p.compare_menu()
            case('pico_menu_populate_action_labels', f'count={count},status={status}', run, language)
    for count in (0, 3, 20):
        def run(p, count=count):
            for arm in (p.old, p.new): arm.write_u32(arm.SCRATCH, count)
            p.invoke('pico_menu_append_wifi_search_actions', [p.old.SCRATCH])
            assert p.old.read_u32(p.old.SCRATCH) == p.new.read_u32(p.new.SCRATCH) == count+2
        case('pico_menu_append_wifi_search_actions', f'initial_count={count}', run, language)
    for step in (0, 1, 2, 3, 4, 255):
        def run(p, step=step):
            for arm in (p.old, p.new): arm.write(arm.SCRATCH, bytes([step]))
            p.invoke('pico_screen_still_setup_step', [p.old.SCRATCH])
        case('pico_screen_still_setup_step', f'step={step}', run, language)
    for count in (0, 1, 6, 22):
        def run(p, count=count):
            data = struct.pack('<22I', *range(22))
            for arm in (p.old, p.new): arm.write(arm.SCRATCH, data)
            p.old.write_u32(0x2002c108+0x50, p.old.SCRATCH)
            p.old.write_u32(0x2002c108+0x54, count)
            session = p.app + layout.offset('PicoAppState', 'session')
            p.new.write_u32(session+layout.offset('PicoProgramContext', 'actions'), p.new.SCRATCH)
            p.new.write_u32(session+layout.offset('PicoProgramContext', 'action_count'), count)
            p.old.hook(MAP['Print_println_string'], lambda f: p.serial['old'].extend(f.cstring(f.reg(1))+b'\r\n'))
            p.new.hook_symbol('fixture_serial', lambda f: p.serial['new'].extend(f.cstring(f.reg(1))))
            p.invoke('pico_menu_populate_in_brew_options', returns=True)
        case('pico_menu_populate_in_brew_options', f'count={count}', run, language)

for count, first, selected, visible in [(1, 7, 7, 0), (3, 3, 4, 0), (7, 1, 7, 0),
                                        (22, 3, 21, 17), (22, 3, 24, 17)]:
    def run(p, count=count, first=first, selected=selected, visible=visible):
        data = b''.join((f'Item {i:02d} '.encode()+b'x'*20)[:22] for i in range(22))
        p.old.write(0x2002bf24, data); p.new.write(p.labels, data)
        p.invoke('pico_ui_draw_menu_items', [count, selected, first, visible])
    case('pico_ui_draw_menu_items', f'{count},{first},{selected},{visible}', run)

for name in ('pico_menu_choose_default_timeout', 'pico_menu_choose'):
    for first, count, initial, result in [(3, 7, 2, 4), (7, 1, 0, 0), (0, 22, 21, 21)]:
        def run(p, name=name, first=first, count=count, initial=initial, result=result):
            p.hook('pico_menu_choose_with_timeout', count=7, returns=result)
            if name.endswith('default_timeout'):
                p.invoke(name, [first, count, initial, 1], [count-1, 1], returns=True)
                assert p.events['new'][-1] == ('pico_menu_choose_with_timeout', first, count, initial, 1, count-1, 1, 60)
            else:
                p.invoke(name, [first, count, initial], returns=True)
                assert p.events['new'][-1] == ('pico_menu_choose_with_timeout', first, count, initial, 0, 0, 0, 60)
        case(name, f'row={first},count={count},initial={initial},result={result}', run)

for initial in (-1, 0, 2, 3, 9):
    for selected in (0, 2):
        def run(p, initial=initial, selected=selected):
            p.menu_state([12, 17, 49])
            p.hook('pico_menu_choose', count=3, returns=selected)
            p.invoke('pico_menu_select_action', [3, u32(initial)], returns=True)
            p.compare_menu()
        case('pico_menu_select_action', f'initial={initial},selection={selected}', run)

UTILITY_ACTIONS = [40, 41, 42, 43, 44, 45, 46, 48, 47, 39]
for selected, expected in enumerate(UTILITY_ACTIONS):
    def run(p, selected=selected, expected=expected):
        p.menu_state(UTILITY_ACTIONS)
        p.hook('pico_menu_choose', count=3, returns=selected)
        before, after = p.invoke('pico_menu_select_action', [3, 0], returns=True)
        assert before == after == expected
        p.compare_menu()
    case('pico_menu_select_action', f'utility_index={selected},expected_action={expected}', run)

for language in range(3):
    for selected in (0, 1, 2):
        def binary(p, selected=selected):
            p.hook('pico_menu_choose', count=3, returns=selected, frame=True)
            p.invoke('pico_ui_choose_binary_option', [p.old.SCRATCH], returns=True)
        case('pico_ui_choose_binary_option', f'selection={selected}', binary, language)
        for row in (3, 7):
            def confirm(p, selected=selected, row=row):
                p.hook('pico_menu_choose', count=3, returns=selected, frame=True)
                p.invoke('pico_ui_confirm_at_row', [row], returns=True)
            case('pico_ui_confirm_at_row', f'row={row},selection={selected}', confirm, language)
    for name in ('pico_ui_confirm_still_cleaned', 'pico_ui_show_still_communication_failure',
                 'pico_ui_show_still_completion', 'pico_ui_show_still_temperature_warning'):
        def run(p, name=name):
            p.hook('pico_menu_choose', count=3, frame=True)
            p.service('alarm_until_press', 'pico_alarm_until_encoder_press', frame=True)
            p.invoke(name)
        case(name, 'complete visible instruction and interaction trace', run, language)


def mechanical(p, clicks=3, clock_step=401):
    p.clock(clock_step); p.clicks(clicks)
    p.service('stepper_move', 'pico_stepper_move_to_compartment', 1, frame=True)
    p.service('restore_idle', 'pico_restore_idle_hardware', frame=True)
    for new, old in [('pico_actuator_drain_on', 'pico_drain_pump_on'),
                     ('pico_actuator_drain_off', 'pico_drain_pump_off'),
                     ('pico_actuator_steam_on', 'pico_steam_pump_on'),
                     ('pico_actuator_steam_off', 'pico_steam_pump_off'),
                     ('pico_actuator_shuttle_on', 'pico_shuttle_pump_on'),
                     ('pico_actuator_shuttle_off', 'pico_shuttle_pump_off')]:
        p.hook(new, old, frame=True)
    p.hook('pico_actuators_pause_for_missing_filter', 0x2000d10c, count=3, new_shift=1, frame=True)
    p.old.hook(MAP['digitalWrite'], lambda f: p.capture('old', 'pin_write', (f.reg(0), f.reg(1))))
    p.new.hook_symbol('ui_fixture_write', lambda f: p.capture('new', 'pin_write', (f.reg(1), f.reg(2))))


for name in ('pico_diagnostic_drain_pump', 'pico_diagnostic_steam_pump', 'pico_diagnostic_stepper',
             'pico_diagnostic_heater', 'pico_diagnostic_shuttle_pump'):
    for clicks in (1, 3, 10):
        def run(p, name=name, clicks=clicks):
            mechanical(p, clicks, 400 if name == 'pico_diagnostic_heater' else 401)
            p.invoke(name)
        case(name, f'click_after={clicks}', run)

for pin in (0, 1):
    for step in (0, 401, 1001):
        def run(p, pin=pin, step=step):
            p.clock(step); p.clicks(4)
            p.old.hook(MAP['digitalRead'], lambda f: (p.capture('old', 'read_pin', (f.reg(0),)) or pin))
            p.new.hook_symbol('ui_fixture_read', lambda f: (p.capture('new', 'read_pin', (f.reg(1),)) or pin))
            for field, old_addr, value in [('wort_fahrenheit', 0x2002d4e8, 153.75),
                                            ('thermoblock_fahrenheit', 0x2002d4f0, 290.25)]:
                put_double(p.old, old_addr, value)
                put_double(p.new, p.app+layout.offset('PicoAppState', 'controller.'+field), value)
            p.invoke('pico_diagnostic_sensors')
        case('pico_diagnostic_sensors', f'filter_pin={pin},clock_step={step}', run)

for success in (0, 1):
    def run(p, success=success):
        p.service('stepper_torque', 'pico_stepper_set_torque_mode', 1, success, True)
        p.service('flush_compartments', 'pico_flush_compartments', 2, frame=True)
        p.invoke('pico_ui_toggle_stepper_torque')
    case('pico_ui_toggle_stepper_torque', f'torque_success={success}', run)
    def buzzer(p, success=success):
        p.old.hook(MAP['pico_buzzer_on'], lambda f: (p.capture('old', 'buzzer', (1,), True) or 1))
        p.old.hook(MAP['pico_buzzer_off'], lambda f: (p.capture('old', 'buzzer', (0,), True) or success))
        def update(f):
            enabled = f.reg(1)
            p.capture('new', 'buzzer', (enabled,), True)
            return 1 if enabled else success
        p.callback('PicoUiServices', 'buzzer', update)
        p.invoke('pico_diagnostic_buzzer', returns=True)
    case('pico_diagnostic_buzzer', f'final_hardware_status={success}', buzzer)
    for uid in (b'01234567', b'0123456789abcdef0123'):
        def rfid(p, success=success, uid=uid):
            def read(f, side, index):
                pointer = f.reg(index)
                assert f.read(pointer, 21) == b'\0'*21
                if success: f.write(pointer, uid+b'\0')
                p.capture(side, 'read_uid', (success, uid if success else b''))
                return success
            p.old.hook(MAP['pico_rfid_read_uid'], lambda f: read(f, 'old', 0))
            p.callback('PicoUiServices', 'rfid_read_uid', lambda f: read(f, 'new', 1))
            p.invoke('pico_diagnostic_rfid')
        case('pico_diagnostic_rfid', f'read_success={success},length={len(uid)}', rfid)

for selected in range(7):
    def run(p, selected=selected):
        mechanical(p)
        p.hook('pico_menu_choose', count=3, returns=selected, frame=True)
        p.invoke('pico_ui_choose_compartment')
    case('pico_ui_choose_compartment', f'compartment={selected}', run)


def diagnostic_boundaries(p):
    for name in ('pico_diagnostic_shuttle_pump', 'pico_diagnostic_heater', 'pico_diagnostic_sensors',
                 'pico_diagnostic_stepper', 'pico_diagnostic_buzzer', 'pico_diagnostic_rfid',
                 'pico_diagnostic_steam_pump', 'pico_diagnostic_drain_pump', 'pico_diagnostic_run_all',
                 'pico_ui_toggle_stepper_torque', 'pico_ui_choose_compartment', 'pico_screen_show_help_temporarily'):
        p.hook(name, frame=True)
    p.hook('pico_storage_reset_shuttle_scale', 'pico_reset_shuttle_scale', frame=True)
    p.service('flush_compartments_until_press', 'pico_flush_compartments_until_press', frame=True)
    def heat(f, side):
        p.capture(side, 'heat_flush', (f.reg(0 if side == 'old' else 1), f.reg(2), f.reg(3)), True)
    p.old.hook(MAP['pico_heat_flush_compartments'], lambda f: heat(f, 'old'))
    p.callback('PicoUiServices', 'heat_flush_compartments', lambda f: heat(f, 'new'))


def all_diagnostics(p):
    diagnostic_boundaries(p)
    p.invoke('pico_diagnostic_run_all')
case('pico_diagnostic_run_all', 'complete ordered diagnostic chain', all_diagnostics)

for selected in (*range(16), 99):
    def run(p, selected=selected):
        diagnostic_boundaries(p)
        p.hook('pico_menu_choose', count=3, sequence=[selected, 15], frame=True)
        p.invoke('pico_diagnostics_menu')
    case('pico_diagnostics_menu', f'action={selected},then_exit', run)

for selected in (0, 1, 2):
    def run(p, selected=selected):
        p.hook('pico_menu_choose_default_timeout', count=6, returns=selected, frame=True)
        p.hook('pico_still_set_fan', 0x20010560, count=1, new_shift=1, returns=1, frame=True)
        p.invoke('pico_ui_still_fan_menu')
        signals = p.app+layout.offset('PicoAppState', 'control_signals')
        for field, old_addr in [('progress_row', 0x2002bf10), ('progress_start_column', 0x2002bf11),
                                ('progress_end_column', 0x2002bf12), ('progress_glyph_index', 0x2002bf13),
                                ('progress_active', 0x2002bf20)]:
            assert p.old.read(old_addr, 1) == p.new.read(signals+layout.offset('PicoControlSignals', field), 1), field
    case('pico_ui_still_fan_menu', f'selection={selected}', run)


def network(p, statuses):
    ssid = b'Diagnostic WiFi\0'
    p.old.write(0x2002c2d0, ssid)
    p.new.write(p.settings+layout.offset('PicoSettings', 'wifi_ssid'), ssid)
    status_index = {'old': 0, 'new': 0}
    def status(side):
        index = status_index[side]; status_index[side] += 1
        value = statuses[min(index, len(statuses)-1)]
        p.capture(side, 'wifi_status', (value,))
        return value
    p.old.hook(MAP['WiFi_status'], lambda f: status('old'))
    p.new.hook_symbol('fixture_status', lambda f: status('new'))
    p.hook('pico_network_ensure_connection', 'pico_wifi_ensure_connection', returns=1, frame=True)
    p.clock()


for statuses, checked, ip, ping, reported in [([0, 0], 0, b'\0'*4, 0, 0),
    ([0, 3], 3, b'\0'*4, 0, 0), ([3, 3], 9, b'\1\2\3\4', 0, 0),
    ([3, 3], 10, b'\1\2\3\4', 0, 0), ([3, 3], 3, b'\1\2\3\4', 0, 0),
    ([3, 3], 3, b'\1\2\3\4', 5, 1), ([3, 3], 0, b'\xff'*4, 3, 0)]:
    def run(p, statuses=statuses, checked=checked, ip=ip, ping=ping, reported=reported):
        network(p, statuses)
        p.hook('pico_network_check_status', 'pico_wifi_check_status', count=2, new_shift=1, returns=checked)
        def local_old(f):
            f.write(f.reg(0), b'\0'*4+ip); p.capture('old', 'local_ip', (ip.hex(),))
        p.old.hook(MAP['WiFi_get_local_ip'], local_old)
        p.new.hook_symbol('fixture_local', lambda f: (p.capture('new', 'local_ip', (ip.hex(),)) or int.from_bytes(ip, 'little')))
        p.old.write_u32(0x20036ac8, 0)
        p.old.hook(MAP['pico_wifi_ping_public_dns'], lambda f: (p.capture('old', 'ping', (ping,)) or ping))
        p.new.hook_symbol('fixture_ping', lambda f: (p.capture('new', 'ping', (ping,)) or ping))
        p.hook('pico_api_error', 'pico_api_report_error', count=1, new_shift=1, returns=reported)
        p.invoke('pico_diagnostic_wifi')
    case('pico_diagnostic_wifi', f'statuses={statuses},check={checked},ip={ip.hex()},ping={ping},report={reported}', run)

for connected in (0, 3):
    for values in [(b'\0'*4,)*4, (bytes([192,168,1,2]), bytes([8,8,8,8]), bytes([192,168,1,1]), bytes([255,255,255,0]))]:
        def run(p, connected=connected, values=values):
            network(p, [connected])
            ssid=b'Diagnostic WiFi\0'
            p.old.write(0x2002c2d0, ssid)
            p.new.write(p.settings+layout.offset('PicoSettings', 'wifi_ssid'), ssid)
            def config(f, side):
                for index, value in enumerate(values):
                    f.write(argument(f, index+1)+(4 if side == 'old' else 0), value)
                p.capture(side, 'ip_configuration', tuple(v.hex() for v in values))
            p.old.hook(MAP['WiFi_get_ip_configuration'], lambda f: config(f, 'old'))
            p.new.hook_symbol('fixture_ip_config', lambda f: config(f, 'new'))
            p.invoke('pico_ui_show_network_addresses')
        case('pico_ui_show_network_addresses', f'status={connected},values={[v.hex() for v in values]}', run)


def service_boundaries(p):
    for field, original in [('drain_reservoir', 'pico_drain_reservoir'), ('drain_step_filter', 'pico_drain_step_filter'),
                            ('run_circulation', 'pico_run_circulation'), ('run_first_rinse', 'pico_run_first_rinse'),
                            ('run_rinse', 'pico_run_rinse_cycle'), ('run_circulation_program', 'pico_run_circulation_program'),
                            ('run_rack_beer', 'pico_run_rack_beer'), ('run_descale', 'pico_run_descale_program'),
                            ('run_still', 'pico_run_still_program')]:
        p.service(field, original, frame=True)
    p.hook('pico_ui_network_menu', frame=True)
    p.hook('pico_diagnostics_menu', frame=True)


for selected, confirmations, exists in [(0, [], 0), (1, [], 0), (2, [], 0), (3, [], 0), (4, [], 0),
                                        (6, [], 0), (99, [], 0), (5, [0], 0), (5, [1,0], 0),
                                        (5, [1,1], 0), (5, [1,0], 1), (5, [1,1], 1)]:
    for language in range(3):
        def run(p, selected=selected, confirmations=confirmations, exists=exists):
            service_boundaries(p); p.clock()
            p.hook('pico_menu_choose', count=3, sequence=[selected, 6], frame=True)
            p.hook('pico_ui_confirm_at_row', count=1, sequence=confirmations, frame=True)
            p.hook('pico_storage_delete_wifi', 'pico_settings_delete_wifi', frame=True)
            p.hook('pico_storage_delete_user_files', 0x2000590c, frame=True)
            p.hook('pico_storage_recovery_image_exists', 0x2000564c, returns=exists, frame=True)
            p.old.hook(MAP['PRCMMCUReset'], lambda f: p.capture('old', 'reset', (f.reg(0),), True))
            p.new.hook_symbol('fixture_reset', lambda f: p.capture('new', 'reset', (1,), True))
            p.invoke('pico_ui_service_menu')
        case('pico_ui_service_menu', f'action={selected},confirm={confirmations},image_exists={exists}', run, language)

for selected in (39, *range(40,49), 0, 99):
    for language in range(3):
        def run(p, selected=selected):
            service_boundaries(p)
            if selected in UTILITY_ACTIONS:
                # Exercise the actual Menu::actionAt / selectAction path so
                # displayed ordering, returned IDs and dispatch all agree.
                p.hook('pico_menu_choose', count=3,
                       sequence=[UTILITY_ACTIONS.index(selected), 9], frame=True)
            else:
                p.hook('pico_menu_select_action', count=2, sequence=[selected, 39], frame=True)
            p.invoke('pico_utilities_menu')
            p.compare_menu()
            assert words(p.new.read(p.menu, 40)) == tuple(UTILITY_ACTIONS)
        case('pico_utilities_menu', f'action={selected},then_exit', run, language)

counts = Counter(result['function'] for result in results)
missing = sorted(set(NAMES)-set(counts)) if not options.only else []
report = {
    'passed': not failures and not missing,
    'elf_sha256': hashlib.sha256(options.elf.read_bytes()).hexdigest(),
    'reference_sha256': hashlib.sha256((PROJECT/'reference/original-0.1.34.bin').read_bytes()).hexdigest(),
    'passed_cases': len(results), 'failed_cases': len(failures),
    'directly_tested_entries': len(counts),
    'entries': [{'original_address': entries[name]['original_address'], 'function': name,
                 'direct_cases': counts[name]} for name in sorted(counts)],
    'missing_entries': missing, 'failures': failures, 'cases': results,
    'scope': 'Direct original and reconstructed ARM entry execution with actual rasterization and menu/state mutation. Ordered screen, input, actuator, storage, network and diagnostic-dispatch effects are compared at explicitly controlled subordinate boundaries. Synthetic callback tables use compiler DWARF field offsets. No physical peripheral timing, appliance startup or complete brew is exercised.'}
options.output.parent.mkdir(parents=True, exist_ok=True)
options.output.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({k: report[k] for k in ('passed', 'passed_cases', 'failed_cases', 'directly_tested_entries', 'missing_entries')}, indent=2))
if failures or missing:
    raise SystemExit(1)
