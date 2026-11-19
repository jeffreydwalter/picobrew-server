#!/usr/bin/env python3
"""Verify every compiled immutable bitmap/font byte and native descriptor."""
from ui_test_support import *

manifest = json.loads(ASSET_MANIFEST.read_text())
image = (PROJECT / 'reference/original-0.1.34.bin').read_bytes()
assert hashlib.sha256(image).hexdigest() == manifest['image_sha256']
firmware = RebuiltArm(ELF)
results = []
for asset in manifest['assets']:
    address = int(asset['source_address'], 16) - 0x20004000
    expected = image[address:address + asset['size']]
    actual = firmware.read(firmware.address(asset['name']), asset['size'])
    assert actual == expected, asset['name']
    assert hashlib.sha256(actual).hexdigest() == asset['sha256']
    results.append({'kind': 'native_bytes', **asset})
for name, descriptor in manifest['descriptors'].items():
    actual = words(firmware.read(firmware.address('pico_ui_asset_' + name), 12))
    expected = (firmware.address(descriptor['array']), descriptor['width'],
                descriptor['height'])
    assert actual == expected, (name, actual, expected)
    results.append({'kind': 'bitmap_descriptor', 'name': name, **descriptor})
for array, prefix, count in [('pico_ui_digits', 'pico_ui_digit_', 10),
                             ('pico_ui_setup_images', 'pico_ui_setup_', 8)]:
    actual = words(firmware.read(firmware.address(array), 4 * count))
    for index, pointer in enumerate(actual):
        asset = next(item for item in manifest['assets'] if item['name'] == prefix + str(index))
        offset = int(asset['source_address'], 16) - 0x20004000
        expected = image[offset:offset + asset['size']]
        assert firmware.read(pointer, asset['size']) == expected, (array, index)
    if array == 'pico_ui_setup_images':
        # Equal source pixels may share storage; callers use the bytes, not
        # distinct array addresses. These two pairs save 960 application bytes.
        assert actual[6] == actual[5] and actual[7] == actual[1], array
    results.append({'kind': 'pointer_table', 'name': array, 'count': count})
report = {'image_sha256': manifest['image_sha256'],
          'elf_sha256': hashlib.sha256(ELF.read_bytes()).hexdigest(),
          'passed_cases': len(results), 'failed_cases': 0,
          'native_arrays': len(manifest['assets']),
          'native_bytes': sum(asset['size'] for asset in manifest['assets']),
          'cases': results}
write_report(report,'assets-verified.json','ui-assets-tests.json')
print(json.dumps({k: v for k, v in report.items() if k != 'cases'}, indent=2))
