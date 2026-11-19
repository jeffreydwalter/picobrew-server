#!/usr/bin/env python3
"""Compare native serial float bytes with the original Pico Pro formatter.

The production PicoNetworkPlatform callback, its double-to-float conversion,
and the actual pinned SDK Print methods execute as ARM instructions. Both
Serial objects are initialized by their real constructors. Only the virtual
HardwareSerial byte writer is replaced with a successful in-memory byte sink.
No Python number formatter supplies either side's expected output.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from fast_arm import ArmFirmware, RebuiltArm
from original_arm import DEFAULT_IMAGE, IMAGE_SHA256
from test_native_startup import Layout
from unicorn import UC_HOOK_CODE

ORIGINAL_SERIAL = 0x20036664
ORIGINAL_CONSTRUCTOR = 0x2001918c
ORIGINAL_BYTE_WRITE = 0x200192a4
ORIGINAL_FLOAT_PRINT = 0x200197a4
NATIVE_BRIDGE = '_ZL18serial_write_floatPvdj'
NATIVE_FLOAT_PRINT = '_ZN5Print10printFloatEfh'
NATIVE_BYTE_WRITE = '_ZN14HardwareSerial5writeEh'


def bits_of(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def value_of(bits):
    return struct.unpack('<f', struct.pack('<I', bits))[0]


def is_nan(bits):
    return bits & 0x7f800000 == 0x7f800000 and bits & 0x007fffff != 0


def scenarios():
    """Sensor/sentinel inputs, adjacent binary32 rounding values, and limits."""
    result = []
    seen = set()

    def add(label, bits, digits=2, category='telemetry'):
        if (bits, digits) not in seen:
            seen.add((bits, digits))
            result.append((label, bits, digits, category))

    for label, bits in [
            ('positive zero', 0), ('negative zero', 0x80000000),
            ('positive minimum subnormal', 1), ('negative minimum subnormal', 0x80000001),
            ('positive maximum subnormal', 0x007fffff),
            ('negative maximum subnormal', 0x807fffff),
            ('positive minimum normal', 0x00800000),
            ('negative minimum normal', 0x80800000),
            ('positive one', 0x3f800000), ('missing-field sentinel', 0xbf800000)]:
        add(label, bits, category='binary32 boundaries and sentinel')
    for value in [-40, -5, 0.01, 0.1, 12.5, 37, 60, 65, 85, 100,
                  123.25, -123.25, 153.7, 203, 212, 999, 10000,
                  16777215, 16777216, 4294967040]:
        add(str(value), bits_of(value))
    for value in [0.005, 0.095, 0.995, 1.005, 10.255, 99.995,
                  153.705, 999.995, 9999.995]:
        center = bits_of(value)
        for neighbor in [-1, 0, 1]:
            for sign in [0, 0x80000000]:
                add(f'{"negative " if sign else ""}{value} binary32 neighbor {neighbor:+d}',
                    (center + neighbor) | sign, category='rounding boundaries')
    for label, bits in [
            ('positive maximum finite', 0x7f7fffff),
            ('negative maximum finite', 0xff7fffff),
            ('positive infinity', 0x7f800000), ('negative infinity', 0xff800000),
            ('positive quiet NaN', 0x7fc00000), ('negative quiet NaN', 0xffc00000)]:
        add(label, bits, category='legacy special-value output')
    for digits in [0, 1, 3, 6]:
        for value in [-1, 0, 153.7, 10.255]:
            add(f'{value} with {digits} digits', bits_of(value), digits,
                category='precision argument forwarding')
    return result


def compare(elf):
    layout = Layout(elf)
    original = ArmFirmware()
    native = RebuiltArm(elf)
    original.call(ORIGINAL_CONSTRUCTOR, [ORIGINAL_SERIAL])
    native_serial = native.address('Serial')
    native.call_symbol('_ZN14HardwareSerialC1Ev', [native_serial])

    # Read the actual callback table using the compiler's member offset.
    table = native.call_symbol('_Z29pico_energia_network_platformv')['r0']
    offset = layout.offset('PicoNetworkPlatform', 'serial_write_float')
    callback = native.read_u32(table + offset)
    assert callback == native.address(NATIVE_BRIDGE), 'Float callback is not bound to the native adapter'
    assert original.read_u32(original.read_u32(ORIGINAL_SERIAL)) == ORIGINAL_BYTE_WRITE | 1
    assert native.read_u32(native.read_u32(native_serial)) == native.address(NATIVE_BYTE_WRITE)

    original_bytes = bytearray()
    native_bytes = bytearray()

    def original_write(machine):
        assert machine.reg(0) == ORIGINAL_SERIAL
        original_bytes.append(machine.reg(1) & 255)
        return 1

    def native_write(machine):
        assert machine.reg(0) == native_serial
        native_bytes.append(machine.reg(1) & 255)
        return 1

    original.hook(ORIGINAL_BYTE_WRITE, original_write)
    native.hook_symbol(NATIVE_BYTE_WRITE, native_write)
    formatter_entries = []

    def observe_formatter(uc, address, size, user_data):
        # This observer does not skip instructions or alter any machine state.
        formatter_entries.append((native.reg(0), native.reg(1), native.reg(2) & 255))

    formatter = native.address(NATIVE_FLOAT_PRINT) & ~1
    native.uc.hook_add(UC_HOOK_CODE, observe_formatter, begin=formatter, end=formatter)

    # These fixed expectations also establish that the shared byte sink records
    # complete, ordinary text rather than just matching empty or partial traces.
    known = {
        (bits_of(0), 2): b'0.00',
        (0x80000000, 2): b'0.00',
        (bits_of(-1), 2): b'-1.00',
        (bits_of(123.25), 2): b'123.25',
        (bits_of(153.7), 2): b'153.70',
        (bits_of(1.005), 2): b'1.00',
    }
    rows = []
    for label, bits, digits, category in scenarios():
        original_bytes.clear()
        native_bytes.clear()
        formatter_entries.clear()
        # Exactly widen the parser's binary32 result to the callback's double
        # ABI; the production adapter must convert it back to binary32.
        low, high = struct.unpack('<II', struct.pack('<d', value_of(bits)))
        returned = original.call(ORIGINAL_FLOAT_PRINT, [ORIGINAL_SERIAL, bits, digits])['r0']
        native.call(callback, [0, 0, low, high], stack_args=[digits])
        expected = bytes(original_bytes)
        actual = bytes(native_bytes)
        errors = []
        if actual != expected:
            errors.append('Native bytes differ from the original firmware')
        if returned != len(expected):
            errors.append('Original Print return does not equal captured byte count')
        entry = formatter_entries[0] if len(formatter_entries) == 1 else None
        # __aeabi_d2f canonicalizes NaN signs/payloads. This is not a value or
        # byte-output difference; require NaN preservation in that case and
        # exact binary32 preservation for every other input (including -0).
        input_matches = entry is not None and (
            is_nan(entry[1]) if is_nan(bits) else entry[1] == bits)
        if not input_matches or entry[0] != native_serial or entry[2] != digits:
            errors.append('Native SDK float formatter received a different value, precision, or object')
        if (bits, digits) in known and expected != known[bits, digits]:
            errors.append('Original bytes differ from the fixed regression expectation')
        rows.append({
            'label': label,
            'category': category,
            'input_binary32': f'0x{bits:08x}',
            'widened_binary64': f'0x{high:08x}{low:08x}',
            'digits': digits,
            'original_output': expected.decode('ascii'),
            'native_output': actual.decode('ascii'),
            'original_byte_count': returned,
            'native_byte_count': len(actual),
            'sdk_formatter_entries': len(formatter_entries),
            'sdk_formatter_input_binary32': f'0x{entry[1]:08x}' if entry else None,
            'passed': not errors,
            'failures': errors,
        })
    return rows, {'platform_table': f'0x{table:08x}',
                  'callback_member_offset_from_dwarf': offset,
                  'registered_callback': f'0x{callback:08x}',
                  'sdk_float_formatter': f'0x{formatter:08x}'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/firmware/pico-pro-recovered.elf')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/native-serial-tests.json')
    args = parser.parse_args()
    rows, binding = compare(args.elf)
    passed = all(row['passed'] for row in rows)
    categories = {}
    for row in rows:
        categories[row['category']] = categories.get(row['category'], 0) + 1
    report = {
        'schema_version': 1,
        'passed': passed,
        'cases': len(rows),
        'passed_cases': sum(row['passed'] for row in rows),
        'failed_cases': sum(not row['passed'] for row in rows),
        'checks': categories,
        'elf': str(args.elf),
        'elf_sha256': hashlib.sha256(args.elf.read_bytes()).hexdigest(),
        'original_image': str(Path(DEFAULT_IMAGE).relative_to(ROOT)),
        'original_image_sha256': IMAGE_SHA256,
        'test_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'binding': binding,
        'original_entries': {
            'constructor': f'0x{ORIGINAL_CONSTRUCTOR:08x}',
            'float_print': f'0x{ORIGINAL_FLOAT_PRINT:08x}',
            'controlled_byte_write': f'0x{ORIGINAL_BYTE_WRITE:08x}',
        },
        'scope': (
            'Direct original ARM Print_print_float versus the registered production '
            'PicoNetworkPlatform.serial_write_float callback and actual pinned SDK '
            'float formatter. Real constructors, C double-to-float ABI conversion, '
            'float arithmetic, integer/string formatting, and virtual byte dispatch '
            'execute. Only HardwareSerial::write(uint8_t) is controlled to accept '
            'each byte. The original return count is checked; the new void callback '
            'has no return-value contract. Precision 2 covers parser diagnostics; '
            'other precisions verify argument forwarding. Non-NaN input bits '
            'are preserved exactly; the SDK double-to-float helper canonicalizes '
            'NaN signs/payloads. Special IEEE values preserve legacy byte output '
            'and do not claim physically meaningful readings. '
            'No UART peripheral or physical device is exercised.'),
        'results': rows,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: value for key, value in report.items() if key != 'results'}, indent=2))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
