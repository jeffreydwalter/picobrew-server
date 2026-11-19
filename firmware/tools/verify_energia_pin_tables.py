#!/usr/bin/env python3
"""Compare complete upstream digital-pin tables to the original Pico image.

This compiles host code that emits only fixed-width constant tables. It does
not execute Energia hardware functions or claim that every board setting is
identical to the original Pico firmware.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

IMAGE_SHA256 = 'e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702'
TABLES = ('port_to_base', 'digital_pin_to_pin_num', 'digital_pin_to_port',
          'digital_pin_to_bit_mask', 'digital_pin_to_timer')
EXPECTED = {'port_to_base': '0x200293f0', 'digital_pin_to_pin_num': '0x20029404',
            'digital_pin_to_port': '0x20029456', 'digital_pin_to_bit_mask': '0x200293c5'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    sdk = args.sdk.resolve()
    image = args.image.read_bytes()
    if hashlib.sha256(image).hexdigest() != IMAGE_SHA256:
        raise ValueError('The input is not the analyzed original firmware')
    source = '#define ARDUINO_MAIN\n#include <Energia.h>\n#include <stdio.h>\nint main(void) {\n'
    for table in TABLES:
        source += ('unsigned n_' + table + '=sizeof(' + table + '); fwrite(&n_' + table +
                   ',4,1,stdout);fwrite(' + table + ',1,sizeof(' + table + '),stdout);\n')
    source += 'return 0;}\n'
    records = []
    with tempfile.TemporaryDirectory(prefix='pico-pin-tables-') as temp:
        root = Path(temp)
        (root / 'probe.c').write_text(source)
        for variant in sorted((sdk / 'variants').iterdir()):
            if not (variant / 'pins_energia.h').is_file():
                continue
            command = ['gcc', '-std=gnu11', '-DF_CPU=80000000L',
                       '-I' + str(sdk / 'cores/cc3200'), '-I' + str(sdk / 'system'),
                       '-I' + str(variant), str(root / 'probe.c'), '-o', str(root / 'probe')]
            subprocess.run(command, check=True, capture_output=True)
            raw = subprocess.check_output([str(root / 'probe')])
            position = 0
            record = {'variant': variant.name, 'tables': {}}
            for table in TABLES:
                length = struct.unpack_from('<I', raw, position)[0]
                position += 4
                data = raw[position:position + length]
                position += length
                found, offset = [], 0
                while (offset := image.find(data, offset)) >= 0:
                    found.append(hex(0x20004000 + offset))
                    offset += 1
                record['tables'][table] = {'bytes': length, 'matches': found}
            records.append(record)
    launchpad = next(x for x in records if x['variant'] == 'CC3200-LAUNCHXL')
    passed = all(launchpad['tables'][table]['matches'] == [address]
                 for table, address in EXPECTED.items())
    report = {'passed': passed, 'firmware_sha256': IMAGE_SHA256, 'variants': records,
              'scope': 'Exact full pin-number/port/bit-mask/port-base array matches; both LaunchPad-named variants have these same arrays.',
              'limitation': 'Timer/PWM table is not found in this image; matching these tables does not prove the complete board configuration or all SDK behavior.'}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': passed, 'matched_arrays': len(EXPECTED), 'variant': launchpad['variant']}))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
