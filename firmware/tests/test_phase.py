#!/usr/bin/env python3
"""Compare portable lifecycle C against original ARM through real PID/actuator code.

Only GPIO, clocks/interrupts, UI, stepper, settings, logging, and the original
bounded log formatter are controlled boundaries. The candidate runs from a
separate newly linked ARM ELF and cannot call the reference image.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import random
import struct
import sys

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parent if (HERE.parent / 'tools/original_arm.py').exists() else HERE.parents[1] / 'picobrew-application'
sys.path.insert(0, str(PROJECT / 'tools'))
from original_arm import ArmFirmware, IMAGE_SHA256
from rebuilt_arm import RebuiltArm

CONTROLLER = 0x2002d2f8
SIGNALS = (0x2002dabe, 0x2002bc54, 0x2002bf10, 0x2002bf11, 0x2002bf12, 0x2002bf13, 0x2002bf20)
AUX_FIELDS = ((8, 0x2002d568, 4), (12, 0x2002d56c, 1),
              (0x10, 0x2002bc58, 12), (0x1c, 0x2002bc64, 4),
              (0x20, 0x2002bc68, 1), (0x24, 0x2002bc6c, 4),
              (0x28, 0x2002bc70, 1), (0x2c, 0x2002c138, 4),
              (0x30, 0x2002c164, 1))
ENTRIES = (0x2000cde0, 0x2000cf68, 0x2000cf86, 0x2000d2c4, 0x2000d314, 0x20005a20)


def u32(value): return value & 0xffffffff
def w32(buffer, offset, value): struct.pack_into('<I', buffer, offset, u32(value))
def w64(buffer, offset, value): struct.pack_into('<d', buffer, offset, value)
def signed(value): return struct.unpack('<i', struct.pack('<I', u32(value)))[0]


def hash_string(value):
    result = 2166136261
    for byte in value:
        result = u32((result ^ byte) * 16777619)
    return result


def initial_controller():
    raw = bytearray(0x228)
    raw[0x60] = 2
    raw[0x62] = 4
    raw[0x63:0x6b] = b'INITIAL\0'
    raw[0xa8] = 3
    for i, value in enumerate((100., 101., 102., 103., 104., 105.)):
        w64(raw, 0x78 + i * 8, value)
    w32(raw, 0x1a4, 55)
    w32(raw, 0x1a8, 600)
    w32(raw, 0x1ac, 3)
    for offset, value in ((0x1b0, 100.), (0x1b8, 110.), (0x1d0, 150.),
                          (0x1d8, 300.), (0x1e0, 1500.), (0x1e8, 2000.),
                          (0x1f0, 120.), (0x1f8, 300.), (0x218, .7), (0x220, .2)):
        w64(raw, offset, value)
    for offset, value in ((0x200, 1000), (0x204, 80), (0x208, 1000),
                          (0x20c, 25000), (0x210, 789)):
        w32(raw, offset, value)
    for p, input_offset, output_offset, target_offset in ((0xb0, 0x1f0, 0x1e0, 0x1d0),
                                                        (0x128, 0x1f8, 0x1e8, 0x1d8)):
        for offset, value in ((0, 2.), (8, .5), (0x10, .2), (0x18, 2.),
                              (0x20, .05), (0x28, 2.), (0x48, 50.),
                              (0x50, 100.), (0x60, 0.), (0x68, 3000.)):
            w64(raw, p + offset, value)
        for offset, value in ((0x34, CONTROLLER + input_offset),
                              (0x38, CONTROLLER + output_offset),
                              (0x3c, CONTROLLER + target_offset),
                              (0x40, 1000), (0x58, 100)):
            w32(raw, p + offset, value)
        raw[p + 0x70] = 1
    return raw


def initial_aux():
    raw = bytearray(0x40)
    raw[:7] = bytes((1, 0, 2, 3, 8, 0, 0))
    w32(raw, 8, 99)
    raw[12] = 7
    struct.pack_into('<fff', raw, 0x10, .1, .2, .3)
    w32(raw, 0x24, 1000)
    return raw


class Original:
    def __init__(self, image):
        self.arm = ArmFirmware(image) if image else ArmFirmware()
        hooks = {0x20018efc: (1, 0), 0x20019ade: (2, 0), 0x20019ad4: (3, 0),
                 0x200190cc: (4, 1), 0x200190fc: (5, 2),
                 0x20018f20: (6, 1), 0x20018f08: (7, 1),
                 0x20004a94: (8, 3), 0x20009ef4: (9, 2),
                 0x20004488: (10, 0), 0x20004494: (11, 0),
                 0x2000a414: (12, 0), 0x20004870: (13, 0),
                 0x20007494: (14, 1), 0x2000cd4c: (15, 1),
                 0x200059f4: (16, 0)}
        for address, (tag, argc) in hooks.items():
            self.arm.hook(address, lambda runner, tag=tag, argc=argc: self.callback(runner, tag, argc))
        self.arm.hook(0x2001bb1c, self.format_log)

    def format_log(self, arm):
        # The single phase-entry formatter has exactly this fixed call shape.
        assert arm.cstring(arm.reg(2)) == b'%s %d'
        assert arm.cstring(arm.reg(3)) == b'setBrewState'
        message = b'setBrewState ' + str(signed(arm.read_u32(arm.reg('sp')))).encode()
        capacity = arm.reg(1)
        if capacity:
            arm.write(arm.reg(0), message[:capacity - 1] + b'\0')
        return len(message)

    def snapshot(self, tag, values):
        arm = self.arm
        flags = arm.read(CONTROLLER + 0x60, 3)
        complete = arm.read(CONTROLLER + 0x1a0, 1)[0]
        phase_word = flags[2] | (flags[1] << 8) | (flags[0] << 16) | (complete << 24)
        screen = arm.read_u32(0x2002d568)
        signal_word = (arm.read(0x2002d56c, 1)[0] |
                       (arm.read(SIGNALS[0], 1)[0] << 8) |
                       (arm.read(0x2002bc70, 1)[0] << 16) |
                       (arm.read(0x2002bc68, 1)[0] << 24))
        return (tag, *values, phase_word, screen, signal_word,
                arm.read_u32(0x2002bc64), hash_string(arm.cstring(CONTROLLER + 0x63)),
                arm.read_u32(CONTROLLER + 0x20c))

    def callback(self, arm, tag, argc):
        values = [arm.reg(i) for i in range(argc)] + [0] * (3 - argc)
        returned = 0
        if tag == 1:
            returned = self.ticks[min(self.tick_index, len(self.ticks) - 1)]
            self.tick_index += 1
            values = [returned, 0, 0]
        elif tag == 3:
            returned = self.enable_result
        elif tag == 4:
            assert values[0] == 11
            returned = 0 if self.restored else self.readings[min(self.read_index, 1)]
            self.read_index += 1
            values[1] = u32(returned)
        elif tag == 8:
            values = [hash_string(arm.cstring(arm.reg(i))) for i in range(3)]
        elif tag == 14:
            values[0] = hash_string(arm.cstring(arm.reg(0)))
        elif tag == 15:
            returned = 1
        elif tag == 16:
            values[:2] = struct.unpack('<II', arm.read(CONTROLLER + 0x220, 8))
            returned = 1
        self.events.append(self.snapshot(tag, values))
        if tag == 12:
            self.restored = True
            if self.resume_phase >= 0:
                arm.write(CONTROLLER + 0x62, bytes([self.resume_phase & 255]))
            if self.resume_status >= 0:
                arm.write(CONTROLLER + 0x61, bytes([self.resume_status & 255]))
        return returned

    def run(self, operations, raw, aux, ticks, configuration):
        self.ticks = ticks
        self.tick_index = self.read_index = 0
        self.restored = False
        self.enable_result = configuration[1]
        self.readings = configuration[2:4]
        self.resume_phase, self.resume_status = map(signed, configuration[4:6])
        self.events = []
        self.arm.write(CONTROLLER, raw)
        for address, value in zip(SIGNALS, aux):
            self.arm.write(address, bytes([value]))
        for offset, address, size in AUX_FIELDS:
            self.arm.write(address, aux[offset:offset + size])
        self.arm.write(self.arm.SCRATCH, b'Test Program\0')
        returns, instructions = [], 0
        for operation, argument in operations:
            args = [CONTROLLER]
            if operation == 0:
                args.append(u32(argument))
            elif operation == 3:
                args = []
            elif operation == 4:
                args.append(self.arm.SCRATCH)
            result = self.arm.call(ENTRIES[operation], args, max_instructions=250000)
            returns.append(result['r0'] if operation == 5 else 0)
            instructions += result['instructions']
        output_aux = bytearray(aux)
        for offset, address in enumerate(SIGNALS):
            output_aux[offset] = self.arm.read(address, 1)[0]
        for offset, address, size in AUX_FIELDS:
            output_aux[offset:offset + size] = self.arm.read(address, size)
        return self.arm.read(CONTROLLER, len(raw)), bytes(output_aux), returns, self.events, instructions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=PROJECT / 'build/application-validation.elf')
    parser.add_argument('--image', type=Path)
    parser.add_argument('--source-root', type=Path,
                        default=HERE.parent if (HERE.parent / 'src/phase.c').exists() else None)
    parser.add_argument('--output', type=Path, default=HERE / 'phase-validation.json')
    options = parser.parse_args()
    rebuilt = RebuiltArm(options.elf)
    original = Original(options.image)
    records = []
    operation_counts = {str(i): 0 for i in range(6)}

    def check(description, operation=0, new_phase=0, changes=None, aux_changes=None,
              ticks=(1500, 1501, 1502, 1503), readings=(0, 0), enable_result=1,
              resume_phase=-1, resume_status=-1, operations=None):
        raw, aux = initial_controller(), initial_aux()
        for buffer, changes_ in ((raw, changes), (aux, aux_changes)):
            for offset, value in (changes_ or {}).items():
                if isinstance(value, bytes):
                    buffer[offset:offset + len(value)] = value
                elif isinstance(value, float):
                    w64(buffer, offset, value)
                else:
                    w32(buffer, offset, value)
        operations = operations or [(operation, new_phase)]
        configuration = tuple(map(u32, (len(ticks), enable_result, *readings,
                                        resume_phase, resume_status)))
        ticks = tuple(map(u32, ticks))
        scratch = rebuilt.SCRATCH
        rebuilt.write(scratch, raw)
        rebuilt.write(scratch + 0x300, aux)
        rebuilt.write(scratch + 0x400, struct.pack('<' + 'I' * len(ticks), *ticks))
        rebuilt.write(scratch + 0x500, struct.pack('<6I', *configuration))
        rebuilt.call_symbol('phase_fixture_reset', [scratch, scratch + 0x300,
                                                   scratch + 0x400, scratch + 0x500])
        actual_returns = []
        for selected, argument in operations:
            result = rebuilt.call_symbol('phase_fixture_run', [selected, u32(argument)],
                                         max_instructions=250000)
            actual_returns.append(result['r0'])
            operation_counts[str(selected)] += 1
        rebuilt.call_symbol('phase_fixture_result', [scratch + 0x600, scratch + 0x900])
        actual_raw = rebuilt.read(scratch + 0x600, len(raw))
        actual_aux = rebuilt.read(scratch + 0x900, len(aux))
        count = rebuilt.call_symbol('phase_fixture_event_count')['r0']
        pointer = rebuilt.call_symbol('phase_fixture_events')['r0']
        actual_events = [struct.unpack('<10I', rebuilt.read(pointer + i * 40, 40)) for i in range(count)]
        expected_raw, expected_aux, expected_returns, expected_events, instructions = original.run(
            operations, raw, aux, ticks, configuration)
        differences = {}
        if actual_raw != expected_raw:
            differences['controller'] = [{'offset': hex(i), 'rebuilt': a, 'original': b}
                                         for i, (a, b) in enumerate(zip(actual_raw, expected_raw)) if a != b]
        if actual_aux != expected_aux:
            differences['auxiliary'] = [{'offset': hex(i), 'rebuilt': a, 'original': b}
                                        for i, (a, b) in enumerate(zip(actual_aux, expected_aux)) if a != b]
        if actual_returns != expected_returns:
            differences['returns'] = {'rebuilt': actual_returns, 'original': expected_returns}
        if actual_events != expected_events:
            differences['events'] = {'rebuilt': actual_events, 'original': expected_events}
        if differences:
            failure = {'case': description, 'operations': operations, 'differences': differences}
            options.output.parent.mkdir(parents=True, exist_ok=True)
            options.output.with_name('phase-failure.json').write_text(json.dumps(failure, indent=2) + '\n')
            raise AssertionError(f'{description}: {list(differences)}; see phase-failure.json')
        records.append({'case': description, 'operations': operations,
                        'original_instructions': instructions, 'events': len(actual_events)})

    # Every phase, all four label choices, language fallbacks, session special
    # cases, compartment-dependent pump paths, and disabled/error controllers.
    for target in (*range(9), -1, 9, 255, 256, -256, -2147483648, 2147483647):
        for language in (0, 1, 2, 255):
            for session in (0, 3, 5, 6):
                check(f'entry-{target}-language{language}-session{session}', new_phase=target,
                      aux_changes={0x2c: session, 0x30: bytes([language])})
    for target in range(9):
        for prior in (0, 2, 4, 255):
            for readings in ((0, 0), (1, 0), (1, 1)):
                check(f'interlock-entry{target}-prior{prior}-read{readings}', new_phase=target,
                      changes={0x62: bytes([prior])}, readings=readings)
        for compartment in (0, 1, 3, 6, 255):
            check(f'compartment-entry{target}-{compartment}', new_phase=target,
                  changes={0x60: bytes([compartment])})
        for status in (0, 1, 3, 255):
            for automatic in (0, 1):
                check(f'status-entry{target}-{status}-auto{automatic}', new_phase=target,
                      changes={0x61: bytes([status]), 0x120: bytes([automatic]),
                               0x198: bytes([automatic])}, enable_result=automatic)

    for prior in (*range(9), 255):
        for operation in (1, 2):
            for status in (0, 5):
                check(f'op{operation}-prior{prior}-status{status}', operation=operation,
                      changes={0x61: bytes([status]), 0x62: bytes([prior])})
        for resumed in (0, 1, 2, 3, 4, 8, 255):
            check(f'pause-prior{prior}-resume{resumed}', operation=2,
                  changes={0x61: b'\x05', 0x62: bytes([prior])},
                  resume_phase=resumed, resume_status=0)
    for session in (0, 2, 3, 4, 5, 6, 0xffffffff):
        for prior in (0, 4, 8):
            for readings in ((0, 0), (1, 0), (1, 1)):
                check(f'circulation-session{session}-prior{prior}-read{readings}', operation=3,
                      aux_changes={0x2c: session}, changes={0x62: bytes([prior])}, readings=readings)
        for compartment in (0, 1, 2, 3, 6, 255):
            for readings in ((0, 0), (1, 1)):
                check(f'finish-session{session}-compartment{compartment}-read{readings}', operation=4,
                      aux_changes={0x2c: session}, changes={0x60: bytes([compartment]),
                      0x61: b'\x07', 0x62: b'\x02'}, readings=readings)

    # Calibration's active/settling flag transitions and inclusive heater band.
    for complete in (0, 1, 255):
        for temperature in (299.999, 300., 300.001, -math.inf, math.inf, math.nan):
            for active in (0, 1):
                for settled in (0, 1):
                    check(f'cal-flags-{complete}-{temperature}-{active}-{settled}', operation=5,
                          changes={0x1a0: bytes([complete]), 0x1f8: temperature},
                          aux_changes={0x20: bytes([settled]), 0x28: bytes([active])}, ticks=(61001,))
    for heater_time in (0, 22699, 22700, 22701, 26699, 26700, 26701, 0xffffffff):
        for elapsed in (0, 59999, 60000, 60001, 60020, 0xffffffff):
            for epoch in (0, 0xfffffff0):
                check(f'cal-heater{heater_time}-elapsed{elapsed}-epoch{epoch}', operation=5,
                      changes={0x20c: heater_time}, aux_changes={0x20: b'\x01',
                      0x28: b'\x01', 0x24: epoch}, ticks=(u32(epoch + elapsed),))
    for coefficient in (.0, -.0, .2, -.2, 1e-300, 1e300, float(2**54), math.inf, -math.inf, math.nan):
        for heater_time in (0, 25000, 30000):
            for history_count in (0, 1, 2):
                next_coefficient = coefficient + (.04 if heater_time == 0 else -.04)
                try:
                    rounded = struct.unpack('<f', struct.pack('<f', next_coefficient))[0]
                except OverflowError:
                    rounded = math.copysign(math.inf, next_coefficient)
                history = struct.pack('<fff', rounded, .7, .8)
                check(f'cal-history-coeff{coefficient}-heat{heater_time}-count{history_count}', operation=5,
                      changes={0x220: coefficient, 0x20c: heater_time}, aux_changes={
                      0x10: history, 0x1c: history_count, 0x20: b'\x01', 0x28: b'\x01'}, ticks=(61001,))
    for coefficient in (.2, math.nan, math.inf):
        for first in (.1, .2, .3, math.nan, math.inf):
            check(f'cal-history-shift-{coefficient}-{first}', operation=5,
                  changes={0x220: coefficient, 0x20c: 0}, aux_changes={0x1c: 2,
                  0x10: struct.pack('<fff', first, .7, .8), 0x20: b'\x01', 0x28: b'\x01'}, ticks=(61001,))
    check('two-calibration-windows', operations=[(5, 0)] * 6,
          ticks=(0, 60000, 60001, 70000, 130000, 130001))
    check('phase-pause-finish-sequence', operations=[(0, 0), (2, 0), (0, 1), (4, 0)],
          changes={0x120: b'\x00', 0x198: b'\x00'}, ticks=tuple(range(100, 132)))

    rng = random.Random(0x5049434f)
    for index in range(150):
        operation = rng.randrange(6)
        prior, target = rng.randrange(10), rng.randrange(10)
        epoch = rng.getrandbits(32)
        check(f'random-{index}', operation=operation, new_phase=target,
              changes={0x60: bytes([rng.randrange(7)]), 0x61: bytes([rng.choice([0, 0, 1, 7])]),
                       0x62: bytes([prior]), 0x120: bytes([rng.randrange(2)]),
                       0x198: bytes([rng.randrange(2)]), 0x1a0: bytes([rng.randrange(2)]),
                       0x1f8: rng.uniform(200., 400.), 0x20c: rng.randrange(60001),
                       0x220: rng.uniform(-.2, 1.)},
              aux_changes={0x1c: rng.randrange(3), 0x20: bytes([rng.randrange(2)]),
                           0x28: bytes([rng.randrange(2)]), 0x24: epoch,
                           0x2c: rng.randrange(7), 0x30: bytes([rng.randrange(4)])},
              ticks=tuple(u32(epoch + 70000 + i) for i in range(32)),
              readings=rng.choice([(0, 0), (1, 0), (1, 1)]))

    source_root = options.source_root.resolve() if options.source_root else None
    sources = {'phase.c': source_root / 'src/phase.c' if source_root else HERE / 'phase.c',
               'control.c': source_root / 'src/control.c' if source_root else PROJECT / 'src/control.c',
               'pid.c': source_root / 'src/pid.c' if source_root else PROJECT / 'src/pid.c',
               'actuators.c': source_root / 'src/actuators.c' if source_root else PROJECT / 'src/actuators.c'}
    report = {'passed': len(records), 'failed': 0, 'candidate_engine': 'newly built ARM ELF',
              'firmware_sha256': IMAGE_SHA256,
              'elf_sha256': hashlib.sha256(options.elf.read_bytes()).hexdigest(),
              'source_sha256': {name: hashlib.sha256(path.read_bytes()).hexdigest()
                                for name, path in sources.items()},
              'operation_counts': operation_counts,
              'original_instructions': sum(r['original_instructions'] for r in records),
              'scope': 'Exact controller/auxiliary state bytes, meaningful returns, and ordered I/O events with state snapshots. Original and rebuilt phase, control/PID, filter debounce and actuator logic execute.',
              'limits': 'Hardware/UI/stepper/settings/logging boundaries controlled; original fixed log format emulated. No interrupt concurrency or physical appliance test. Corrupt calibration history indices excluded.',
              'cases': records}
    options.output.parent.mkdir(parents=True, exist_ok=True)
    options.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'Phase lifecycle: {len(records)} exact ARM comparisons passed; 0 mismatches; '
          f'{report["original_instructions"]} original instructions')


if __name__ == '__main__':
    main()
