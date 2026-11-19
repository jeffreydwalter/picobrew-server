#!/usr/bin/env python3
"""Compare actuator ordering and filter interlocks with the original ARM code."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from original_arm import ArmFirmware, IMAGE_SHA256

NAMES = [
    'pico_actuators_initialize', 'pico_step_filter_is_inserted',
    'pico_actuators_all_off', 'pico_actuators_off_except_drain',
    'pico_steam_and_drain_pumps_off', 'pico_heater_on', 'pico_heater_off',
    'pico_steam_pump_on', 'pico_steam_pump_off', 'pico_drain_pump_on',
    'pico_drain_pump_off', 'pico_shuttle_pump_on', 'pico_shuttle_pump_off',
    'pico_drain_pump_is_on', 'pico_start_drain_and_steam_pumps',
    'pico_pause_outputs_for_missing_filter'
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/actuator_tests.json')
    options = parser.parse_args()
    original = ArmFirmware()
    rebuilt = None
    if options.elf:
        from rebuilt_arm import RebuiltArm
        rebuilt = RebuiltArm(options.elf)
    records = json.loads((ROOT / 'reference/function_map.json').read_text())
    addresses = {x['name']: int(x['address'], 16) for x in records}
    globals_ = json.loads((ROOT / 'reference/global_map.json').read_text())
    phase_address = int(next(x['address'] for x in globals_ if x['name'] == 'g_controller_brew_phase'), 16)
    events, readings = [], [0, 0]
    read_index, restored = [0], [False]

    def read_pin(runner):
        value = 0 if restored[0] else readings[min(read_index[0], 1)]
        read_index[0] += 1
        events.append([1, runner.reg(0), value])
        return value

    def pause_or_prompt(event_id):
        def callback(runner):
            events.append([event_id, 0, 0])
            restored[0] = True
        return callback

    original.hook(addresses['digitalRead'], read_pin)
    original.hook(addresses['digitalWrite'], lambda r: events.append([2, r.reg(0), r.reg(1)]))
    original.hook(addresses['pinMode'], lambda r: events.append([3, r.reg(0), r.reg(1)]))
    original.hook(addresses['delayMicroseconds'], lambda r: events.append([4, r.reg(0), 0]))
    original.hook(addresses['delay'], lambda r: events.append([5, r.reg(0), 0]))
    original.hook(addresses['pico_controller_pause_until_resume'], pause_or_prompt(6))
    original.hook(addresses['pico_prompt_insert_filter_with_alarm'], pause_or_prompt(7))
    cases = []
    for operation in range(16):
        phases = [0, 1, 4, 8, 255] if operation in [7, 9, 11, 14, 15] else [4]
        patterns = [(0, 0), (1, 0), (1, 1)] if operation in [1, 7, 9, 11, 13, 14, 15] else [(0, 0)]
        masks = range(8) if operation == 15 else [0]
        cases.extend((operation, phase, first, second, mask)
                     for phase in phases for first, second in patterns for mask in masks)
    results = []
    with tempfile.TemporaryDirectory(prefix='pico-actuators-') as temporary:
        library = Path(temporary) / 'actuators.so'
        subprocess.run(['gcc', '-std=c11', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra',
                        '-Werror', '-I' + str(ROOT / 'include'), str(ROOT / 'src/actuators.c'),
                        str(ROOT / 'tests/actuator_fixture.c'), '-o', str(library)], check=True)
        native = ctypes.CDLL(str(library))
        native.pico_test_actuator_configure.argtypes = [ctypes.c_uint32] * 3
        native.pico_test_actuator_configure.restype = None
        native.pico_test_actuator_run.argtypes = [ctypes.c_uint32] * 2
        native.pico_test_actuator_run.restype = ctypes.c_uint32
        native.pico_test_actuator_event_count.restype = ctypes.c_uint32
        native.pico_test_actuator_event_word.argtypes = [ctypes.c_uint32] * 2
        native.pico_test_actuator_event_word.restype = ctypes.c_uint32
        for operation, phase, first, second, mask in cases:
            events.clear()
            read_index[0], restored[0] = 0, False
            readings[:] = [first, second]
            original.write(phase_address, bytes([phase]))
            args = [int(bool(mask & 1)), int(bool(mask & 2)), int(bool(mask & 4))] if operation == 15 else []
            expected_return = original.call(addresses[NAMES[operation]], args=args)['r0']
            expected_events = list(events)
            native.pico_test_actuator_configure(phase, first, second)
            actual_return = native.pico_test_actuator_run(operation, mask)
            actual_events = [[native.pico_test_actuator_event_word(i, j) for j in range(3)]
                             for i in range(native.pico_test_actuator_event_count())]
            assert actual_events == expected_events, (cases, actual_events, expected_events)
            if operation in [1, 13, 15]:
                assert actual_return == expected_return, (operation, phase, first, second)
            if rebuilt:
                rebuilt.call_symbol('pico_test_actuator_configure', [phase, first, second])
                arm_return = rebuilt.call_symbol('pico_test_actuator_run', [operation, mask])['r0']
                count = rebuilt.call_symbol('pico_test_actuator_event_count')['r0']
                arm_events = [[rebuilt.call_symbol('pico_test_actuator_event_word', [i, j])['r0']
                               for j in range(3)] for i in range(count)]
                assert arm_events == expected_events, (operation, phase, first, second, arm_events, expected_events)
                if operation in [1, 13, 15]:
                    assert arm_return == expected_return
            results.append({'original_function': NAMES[operation], 'phase': phase,
                            'readings': [first, second], 'resume_mask': mask,
                            'events': expected_events})
    report = {'passed': True, 'cases': len(results), 'original_functions': len(NAMES),
              'mismatches': 0, 'original_sha256': IMAGE_SHA256,
              'native_c_compared': True, 'rebuilt_arm_compared': rebuilt is not None,
              'rebuilt_elf_sha256': hashlib.sha256(options.elf.read_bytes()).hexdigest() if rebuilt else None,
              'scope': 'Ordered GPIO/mode/delay calls and meaningful returns; original filter predicate executes. UI pause/prompt boundaries are controlled and restore the filter.',
              'limits': 'No electrical behavior, interrupts during waits, or complete UI implementation.',
              'results': results}
    options.output.parent.mkdir(parents=True, exist_ok=True)
    options.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'Actuators: {len(results)} scenarios match original ARM; rebuilt ARM compared={rebuilt is not None}')


if __name__ == '__main__':
    main()
