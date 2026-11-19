#!/usr/bin/env python3
"""Compare portable sensor C and optionally the rebuilt ARM ELF to the original."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from original_arm import ArmFirmware, IMAGE_SHA256


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, help='Also compare newly compiled ARM code')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/sensor_tests.json')
    options = parser.parse_args()
    rebuilt = None
    if options.elf:
        from rebuilt_arm import RebuiltArm
        rebuilt = RebuiltArm(options.elf)
    original = ArmFirmware()
    function_map = json.loads((ROOT / 'reference/function_map.json').read_text())
    addresses = {item['name']: int(item['address'], 16) for item in function_map}
    count_fixture = [0]
    calls = []
    original.hook(addresses['analogRead'], lambda runner: count_fixture[0])
    original.hook(addresses['pinMode'], lambda runner: calls.append((runner.reg(0), runner.reg(1))))
    global_map = json.loads((ROOT / 'reference/global_map.json').read_text())
    session_address = None
    for item in global_map:
        if item.get('name') == 'g_session_type':
            session_address = int(item['address'], 16)
            break
    if session_address is None:
        raise ValueError('Session address absent from reference map')
    groups, mismatches = [], []
    with tempfile.TemporaryDirectory(prefix='pico-sensors-') as temporary:
        so = Path(temporary) / 'sensors.so'
        subprocess.run(['gcc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall', '-Wextra',
                        '-Werror', '-fno-fast-math', '-ffp-contract=off',
                        '-fexcess-precision=standard', '-I' + str(ROOT / 'include'),
                        str(ROOT / 'src/sensors.c'), '-lm', '-o', str(so)], check=True)
        native = ctypes.CDLL(str(so))
        for pin, session in [(26, 1), (25, 1), (25, 2), (25, 4)]:
            name = f'pico_temperature_pin{pin}_from_adc'
            reference_name = f'pico_temperature_read_pin{pin}'
            function = getattr(native, name)
            function.restype = ctypes.c_float
            function.argtypes = [ctypes.c_uint32] + ([ctypes.c_int] if pin == 25 else [])
            original.write_u32(session_address, session)
            maximum_instructions = 0
            failures_before = len(mismatches)
            for adc in range(4096):
                count_fixture[0] = adc
                calls.clear()
                expected = original.call(addresses[reference_name])['r0']
                args = [adc, session] if pin == 25 else [adc]
                actual = struct.unpack('<I', struct.pack('<f', function(*args)))[0]
                if calls != [(pin, 0)]:
                    raise AssertionError(('Original I/O contract changed', pin, calls))
                compiled = None
                if rebuilt:
                    result = rebuilt.call_symbol(name, args=args)
                    compiled = result['r0']
                    maximum_instructions = max(maximum_instructions, result['instructions'])
                if actual != expected or (compiled is not None and compiled != expected):
                    mismatches.append({'pin': pin, 'session': session, 'adc': adc,
                                       'original_bits': hex(expected), 'native_bits': hex(actual),
                                       'rebuilt_bits': None if compiled is None else hex(compiled)})
            groups.append({'pin': pin, 'session_type': session, 'cases': 4096,
                           'mismatches': len(mismatches) - failures_before,
                           'max_rebuilt_arm_instructions': maximum_instructions})
            print(f'Pin {pin}, session {session}: 4096 cases; {groups[-1]["mismatches"]} mismatches', flush=True)
    report = {
        'passed': not mismatches, 'original_sha256': IMAGE_SHA256,
        'cases': sum(item['cases'] for item in groups),
        'native_c_compared': True, 'rebuilt_arm_compared': rebuilt is not None,
        'rebuilt_elf_sha256': hashlib.sha256(options.elf.read_bytes()).hexdigest() if rebuilt else None,
        'groups': groups, 'mismatch_count': len(mismatches), 'sample_mismatches': mismatches[:20],
        'scope': 'Exact returned binary32 bits across ADC 0..4095; pin26 session1 and pin25 sessions1/2/4. Original math instructions and linked rebuilt math library execute.',
        'limits': 'No physical sensor calibration, full appliance, or input outside the documented ADC domain.'
    }
    options.output.parent.mkdir(parents=True, exist_ok=True)
    options.output.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
