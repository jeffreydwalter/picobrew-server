#!/usr/bin/env python3
"""Demonstrate that editing application C changes the newly compiled ARM code.

The production tree is never modified. A temporary project changes the
session-specific temperature correction from 2.0 to 2.5 degrees Fahrenheit,
then rebuilds and executes the resulting ARM function. This is a build-path
check, not a proposed calibration change for a physical machine.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from rebuilt_arm import RebuiltArm


def as_float(bits):
    return struct.unpack('<f', struct.pack('<I', bits))[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/application-validation.elf')
    parser.add_argument('--arm-prefix', help='Optional override; otherwise use the normal build environment')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/edit-rebuild-tests.json')
    args = parser.parse_args()
    baseline = RebuiltArm(args.elf)
    results = []
    with tempfile.TemporaryDirectory(prefix='pico-edit-rebuild-') as temporary:
        candidate = Path(temporary)
        for name in ('src', 'include', 'platform', 'tools', 'tests', 'dependencies'):
            shutil.copytree(ROOT / name, candidate / name,
                            ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
        source = candidate / 'src/sensors.c'
        original = source.read_text()
        old, new = 'fahrenheit -= 2.0f;', 'fahrenheit -= 2.5f;'
        if original.count(old) != 1:
            raise ValueError('Expected exactly one original session calibration constant')
        source.write_text(original.replace(old, new))
        command = [sys.executable, str(candidate / 'tools/build.py'), '--core-only']
        if args.arm_prefix:
            command += ['--arm-prefix', args.arm_prefix]
        elif (ROOT / '.deps/toolchain/bin/arm-none-eabi-gcc').is_file():
            command += ['--arm-prefix', str(ROOT / '.deps/toolchain/bin/arm-none-eabi-')]
        subprocess.run(command, check=True, capture_output=True, text=True)
        modified_elf = candidate / 'build/application-validation.elf'
        rebuilt = RebuiltArm(modified_elf)
        for adc in (256, 1024, 2048, 3072, 3840):
            for session in (1, 2, 4):
                name = 'pico_temperature_pin25_from_adc'
                before_bits = baseline.call_symbol(name, [adc, session])['r0']
                after_bits = rebuilt.call_symbol(name, [adc, session])['r0']
                before, after = as_float(before_bits), as_float(after_bits)
                expected_difference = -0.5 if session in (2, 4) else 0.0
                if after - before != expected_difference:
                    raise AssertionError((adc, session, before, after, expected_difference))
                results.append({'adc': adc, 'session': session, 'before_f': before,
                                'after_f': after, 'difference_f': after - before})
            name = 'pico_temperature_pin26_from_adc'
            if baseline.call_symbol(name, [adc])['r0'] != rebuilt.call_symbol(name, [adc])['r0']:
                raise AssertionError('Unmodified thermoblock function changed')
        report = {
            'passed': True, 'temperature_comparisons': len(results),
            'unchanged_thermoblock_comparisons': 5,
            'original_source_sha256': hashlib.sha256(original.encode()).hexdigest(),
            'modified_source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'baseline_elf_sha256': hashlib.sha256(args.elf.read_bytes()).hexdigest(),
            'modified_elf_sha256': hashlib.sha256(modified_elf.read_bytes()).hexdigest(),
            'production_source_modified': False, 'results': results,
            'scope': 'Isolated source edit, real ARM recompile/link, and observed changed ARM behavior; not a physical calibration recommendation.'
        }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('Edit/rebuild: 10 intended changes and 10 unchanged controls passed in ARM execution.')


if __name__ == '__main__':
    main()
