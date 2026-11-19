#!/usr/bin/env python3
"""Direct comparisons for previously review-only Pico network/storage entries.

Original Thumb and rebuilt ARM bodies execute independently. Subordinate
services are controlled explicitly; state, returns and ordered effects are
compared. Folded helpers and deliberate repaired inputs are labeled separately.
No socket, radio, flash, or physical brewer is operated.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from fast_arm import ArmFirmware, RebuiltArm
from test_native_startup import Layout
import test_network as base

MAP = base.MAP
U32 = lambda value: value & 0xffffffff
CASES = []
LAYOUT = None
ELF = ROOT / 'build/application-validation.elf'


class Pair:
    def __init__(self):
        self.old = ArmFirmware()
        self.h = base.Harness()
        self.new = self.h.f
        self.events = [[], []]
        self.serial = [bytearray(), bytearray()]
        self.clocks = [100, 100]
        self.tick = 1
        for name, address in MAP.items():
            if name.startswith('Print_'):
                self.old.hook(address, lambda f: 0)
        def text(f, newline=False):
            data = f.cstring(f.reg(1)) + (b'\r\n' if newline else b'')
            self.serial[0].extend(data)
            return len(data)
        self.old.hook(MAP['Print_print_string'], text)
        self.old.hook(MAP['Print_println_string'], lambda f: text(f, True))
        for name in ('Print_print_unsigned', 'Print_print_unsigned_alias_a',
                     'Print_print_unsigned_alias_b', 'Print_print_signed',
                     'Print_print_signed_alias', 'Print_println_signed',
                     'Print_println_unsigned'):
            def number(f, name=name):
                value = f.reg(1)
                if 'signed' in name and 'unsigned' not in name and value & 0x80000000:
                    value -= 0x100000000
                data = str(value).encode() + (b'\r\n' if 'println' in name else b'')
                self.serial[0].extend(data)
                return len(data)
            self.old.hook(MAP[name], number)
        self.old.hook(MAP['pico_log_serial'], lambda f: 0)
        self.old.hook(MAP['pico_assert_nonfatal'], lambda f: 0)
        self.new.hook_symbol('pico_network_log', lambda f: 0)
        self.new.hook_symbol('pico_network_assert', lambda f: 0)
        self.h.hook('serial', lambda f: self.serial[1].extend(f.cstring(f.reg(1))))
        self.old.hook(MAP['millis'], lambda f: self.now(0))
        self.h.hook('millis', lambda f: self.now(1))
        self.old.hook(MAP['delay'], lambda f: self.delay(0, f.reg(0)))
        self.h.hook('delay', lambda f: self.delay(1, f.reg(1)))
        self.old.write(0x2002c2d0, b'Network\0')
        self.old.write(0x2002c290, b'password\0')
        self.old.write(0x2002c2f1, b'\4')
        self.old.write(0x2002da9d, b'0123456789abcdef0123456789abcdef\0')

    def now(self, side):
        value = self.clocks[side]
        self.clocks[side] = U32(value + self.tick)
        return value

    def delay(self, side, value):
        self.clocks[side] = U32(self.clocks[side] + value)
        self.events[side].append(('delay', value))

    def field(self, record, field):
        objects = {'PicoNetwork': self.h.n, 'PicoAppState': self.h.app,
                   'PicoSettings': self.h.settings,
                   'PicoStillConnection': self.h.n + self.h.o[5]}
        return objects[record] + LAYOUT.offset(record, field)

    def service(self, original, rebuilt, label, value=0, old_args=(), new_args=()):
        def call(side, f, args):
            self.events[side].append((label, *(f.reg(i) for i in args)))
            return value
        self.old.hook(original, lambda f: call(0, f, old_args))
        self.hook_new(rebuilt, lambda f: call(1, f, new_args))

    def hook_new(self, name, callback):
        # GCC splits guarded bodies into same-ABI .part helpers. Constant
        # callers may reach that body directly after proving the guard.
        for symbol in self.new.symbols:
            if symbol == name or symbol.startswith(name + '.part.'):
                self.new.hook_symbol(symbol, callback)

    def compare(self, address, replacement, case, old_args=(), new_args=(),
                returns=True, state=None, serial=False, kind='direct_original_vs_rebuilt'):
        old_result = self.old.call(address, list(old_args), max_instructions=3000000)['r0']
        new_result = self.h.call(replacement, *new_args, max_instructions=3000000)
        assert self.events[0] == self.events[1], (hex(address), case, self.events)
        if returns:
            assert old_result == new_result, (hex(address), case, old_result, new_result)
        if state:
            before, after = state()
            assert before == after, (hex(address), case, 'state', before, after)
        if serial:
            assert self.serial[0] == self.serial[1], (hex(address), case, 'serial', self.serial)
        CASES.append({'original_address': hex(address), 'replacement': replacement,
                      'case': case, 'kind': kind, 'return_compared': returns,
                      'state_compared': state is not None, 'serial_compared': serial,
                      'ordered_events': len(self.events[0])})


def storage():
    for address, name, expected in [
            (0x200058dc, 'pico_storage_delete_restart', ['restart.txt']),
            (0x200058e8, 'pico_storage_delete_wifi', ['ssid.txt', 'wifiPass.txt', 'wifiSecurity.txt']),
            (0x2000590c, 'pico_storage_delete_user_files', ['userPref.txt', 'SS.txt',
             '/sys/mcuimg2.bin', '/sys/mcuimg3.bin', 'sys/uid.txt', '/sys/mcubootinfo.bin'])]:
        for result in (0, -1):
            p = Pair()
            p.old.hook(0x200058c8, lambda f: p.events[0].append(('remove', f.cstring(f.reg(0)).decode())) or 1)
            p.h.hook('file_remove', lambda f: p.events[1].append(('remove', f.cstring(f.reg(1)).decode())) or result)
            p.compare(address, name, {'remove_result': result}, returns=False)
            assert [event[1] for event in p.events[0]] == expected
    for opened in (0, 1):
        p = Pair()
        p.old.hook(0x2001822c, lambda f: p.events[0].append(('open', f.cstring(f.reg(1)), f.reg(2))) or opened)
        p.old.hook(0x200182b2, lambda f: p.events[0].append(('close',)) or 1)
        p.h.hook('file_open', lambda f: p.events[1].append(('open', f.cstring(f.reg(1)), f.reg(2))) or opened)
        p.h.hook('file_close', lambda f: p.events[1].append(('close',)) or 1)
        p.compare(0x2000564c, 'pico_storage_recovery_image_exists', {'opened': opened})
    for text, success in [(b'', 0), (b'1:recipe:0:0:1:20:0:0:0:', 1), (b'x' * 100, 1)]:
        p = Pair()
        def read(side, f):
            p.events[side].append(('read', f.cstring(f.reg(0 if side == 0 else 1)),
                                   f.reg(2 if side == 0 else 3) - (side == 1)))
            f.write(f.reg(1 if side == 0 else 2), text + b'\0')
            return success
        p.old.hook(0x2000a538, lambda f: read(0, f))
        p.hook_new('pico_storage_read_text', lambda f: read(1, f))
        p.compare(0x2000a584, 'pico_storage_read_restart', {'bytes': len(text), 'success': success},
                  old_args=[p.old.SCRATCH], new_args=[p.new.SCRATCH],
                  state=lambda: (p.old.cstring(p.old.SCRATCH), p.new.cstring(p.new.SCRATCH)))
    for value, written in itertools.product([0.0, 0.2, 0.9999, -0.125, 12.345], [0, 1]):
        p = Pair()
        p.old.write(0x2002d518, struct.pack('<d', value))
        p.new.write(p.field('PicoAppState', 'controller.candidate_shuttle_scale'), struct.pack('<d', value))
        def write(side, f):
            offset = side
            p.events[side].append(('write', f.cstring(f.reg(offset)), f.cstring(f.reg(offset + 1)), f.reg(offset + 2)))
            return written
        p.old.hook(0x20005678, lambda f: write(0, f))
        p.hook_new('pico_storage_write_text', lambda f: write(1, f))
        p.compare(0x200059f4, 'pico_storage_save_shuttle_scale', {'value': value, 'written': written})
    for text, success in [(b'', 0), (b'0.200', 1), (b'-.125', 1), (b'1e-3', 1), (b'12.345', 1)]:
        p = Pair()
        value = struct.pack('<d', 0.375)
        p.old.write(0x2002d518, value)
        p.new.write(p.field('PicoAppState', 'controller.candidate_shuttle_scale'), value)
        def read(side, f):
            p.events[side].append(('read', f.cstring(f.reg(side)), f.reg(side + 2) - side))
            f.write(f.reg(side + 1), text + b'\0')
            return success
        p.old.hook(0x2000a538, lambda f: read(0, f))
        p.hook_new('pico_storage_read_text', lambda f: read(1, f))
        p.compare(0x2000a5cc, 'pico_storage_load_shuttle_scale', {'text': text.decode(), 'success': success},
                  state=lambda: (p.old.read(0x2002d518, 8), p.new.read(p.field('PicoAppState', 'controller.candidate_shuttle_scale'), 8)))
    for read_ok, hash_ok, write_ok in itertools.product((0, 1), repeat=3):
        p = Pair()
        def read(side, f):
            p.events[side].append(('read', f.cstring(f.reg(side))))
            if read_ok:
                f.write(f.reg(side + 1), b'cached-uid\0')
            return read_ok
        def compute(side, f):
            p.events[side].append(('compute',))
            address = 0x2002da9d if not side else p.field('PicoAppState', 'device_uid')
            f.write(address, b'fedcba9876543210fedcba9876543210\0')
            return hash_ok
        def write(side, f):
            p.events[side].append(('write', f.cstring(f.reg(side)), f.cstring(f.reg(side + 1)), f.reg(side + 2)))
            return write_ok
        p.old.hook(0x2000a538, lambda f: read(0, f))
        p.hook_new('pico_storage_read_text', lambda f: read(1, f))
        p.old.hook(0x20005c00, lambda f: compute(0, f))
        p.hook_new('pico_storage_compute_uid', lambda f: compute(1, f))
        p.old.hook(0x20005678, lambda f: write(0, f))
        p.hook_new('pico_storage_write_text', lambda f: write(1, f))
        p.compare(0x2000a594, 'pico_storage_load_uid', {'read': read_ok, 'hash': hash_ok, 'write': write_ok},
                  state=lambda: (p.old.cstring(0x2002da9d), p.new.cstring(p.field('PicoAppState', 'device_uid'))))
    for success in (0, 1):
        p = Pair()
        # The UI event is a concrete composition boundary separately checked
        # by the UI framebuffer tests. Record its component calls here.
        for address in (0x20004488, 0x20004494):
            p.old.hook(address, lambda f: 0)
        p.old.hook(MAP['pico_ui_draw_header'], lambda f: p.events[0].append(('reset_screen',)) or 0)
        p.old.hook(MAP['pico_display_center_text'], lambda f: 0)
        p.h.hook('event', lambda f: p.events[1].append(('reset_screen',)))
        p.service(0x200059f4, 'pico_storage_save_shuttle_scale', 'save_scale', success)
        p.compare(0x2000a7c0, 'pico_storage_reset_shuttle_scale', {'save_result': success},
                  state=lambda: (p.old.read(0x2002d518, 8), p.new.read(p.field('PicoAppState', 'controller.candidate_shuttle_scale'), 8)))
    for text, scale_ok in itertools.product([None, '00', '12', '01', '10'], [0, 1]):
        p = Pair()
        p.service(0x20008994, 'fixture_file_begin', 'initialize_storage', 1)
        p.old.hook(0x20014a70, lambda f: p.events[0].append(('initialize_storage',)) or 1)
        def read(side, f):
            p.events[side].append(('read_preferences', f.cstring(f.reg(side))))
            if text is not None:
                f.write(f.reg(side + 1), text.encode() + b'\0')
            return int(text is not None)
        p.old.hook(0x2000a538, lambda f: read(0, f))
        p.hook_new('pico_storage_read_text', lambda f: read(1, f))
        for address, symbol, tag, result in [(0x2000c334, 'fixture_language', 'choose_language', 2),
                (0x2000c36c, 'fixture_units', 'choose_units', 1),
                (0x20005888, 'pico_storage_save_display', 'save_display', 1),
                (0x2000a5cc, 'pico_storage_load_shuttle_scale', 'load_scale', scale_ok),
                (0x200059f4, 'pico_storage_save_shuttle_scale', 'save_scale', 1),
                (0x2000a594, 'pico_storage_load_uid', 'load_uid', 1)]:
            p.service(address, symbol, tag, result)
        p.old.hook(0x20004488, lambda f: p.events[0].append(('clear',)) or 0)
        p.h.hook('event', lambda f: p.events[1].append(('clear',)))
        p.old.write(0x2002c164, b'\0');p.old.write(0x2002d9c9, b'\0')
        p.compare(0x2000c390, 'pico_storage_load_preferences', {'text': text, 'scale': scale_ok}, returns=False,
                  state=lambda: ((p.old.read(0x2002c164, 1), p.old.read(0x2002d9c9, 1)),
                    (p.new.read(p.field('PicoSettings', 'language'), 1), p.new.read(p.field('PicoSettings', 'temperature_unit_celsius'), 1))))


def access_points():
    for security, password, replies in itertools.product([0, 4, 5, 6, 7, 255],
            ['password', '0102030405', 'ABCDEF0123', 'x' * 32], [[3], [4, 3], [4, 4]]):
        p = Pair()
        p.old.write(0x2002c290, password.encode() + b'\0')
        p.old.write(0x2002c2f1, bytes([security]))
        p.new.write(p.field('PicoSettings', 'wifi_password'), password.encode() + b'\0')
        p.new.write(p.field('PicoSettings', 'wifi_security'), bytes([security]))
        positions = [0, 0]
        def begin(side, f, mode):
            index = positions[side];positions[side] += 1
            key = b'' if mode == 'open' else f.cstring(f.reg(3 if mode == 'wep' and not side else 2))
            p.events[side].append(('begin', f.cstring(f.reg(1)), key,
                                   'open' if mode == 'open' else 'wep' if security == 5 else 'wpa'))
            return replies[min(index, len(replies)-1)]
        p.old.hook(0x20014e08, lambda f: begin(0, f, 'open'))
        p.old.hook(0x20014e94, lambda f: begin(0, f, 'wep'))
        p.old.hook(0x20014f38, lambda f: begin(0, f, 'wpa'))
        p.h.hook('begin', lambda f: begin(1, f, 'open' if security >= 7 else 'wep' if security == 5 else 'wpa'))
        p.compare(0x20008998, 'pico_network_connect_access_point',
                  {'security': security, 'password': password, 'replies': replies})
    scenarios = [dict(ap=[3], status=[3], connecting=False),
        dict(ap=[4, 4, 4], status=[4], connecting=False),
        dict(ap=[4, 4, 3], status=[3], connecting=False),
        dict(ap=[4], status=[0, 3, 3], connecting=True),
        dict(ap=[4, 4, 4], status=[0], connecting=True)]
    for scenario in scenarios:
        p = Pair();p.tick = 5000
        positions = [{}, {}]
        connecting = [scenario['connecting'], scenario['connecting']]
        p.old.write(0x2002dafe, bytes([connecting[0]]))
        def take(side, key):
            index = positions[side].get(key, 0);positions[side][key] = index + 1
            return scenario[key][min(index, len(scenario[key])-1)]
        def ap(side, f):
            p.events[side].append(('access_point',));return take(side, 'ap')
        def disconnect(side, f):
            p.events[side].append(('disconnect',));connecting[side] = False
            if not side:f.write(0x2002dafe, b'\0')
            return 0
        p.old.hook(0x20008998, lambda f: ap(0, f));p.hook_new('pico_network_connect_access_point', lambda f: ap(1, f))
        p.old.hook(0x20014dd4, lambda f: take(0, 'status'));p.h.hook('status', lambda f: take(1, 'status'))
        p.old.hook(0x20014b6c, lambda f: disconnect(0, f));p.h.hook('disconnect', lambda f: disconnect(1, f))
        p.h.hook('connecting', lambda f: int(connecting[1]))
        p.compare(0x20008a8c, 'pico_network_connect_stored', scenario,
                  state=lambda: (p.clocks[0], p.clocks[1]))
    for initialized, initialize_ok, connected in itertools.product((0, 1), repeat=3):
        p = Pair();p.old.write(0x2002daff, bytes([initialized]));p.h.init = bool(initialized)
        p.service(0x20014b6c, 'fixture_disconnect', 'disconnect')
        p.service(0x20014a70, 'fixture_initialize', 'initialize', initialize_ok)
        p.service(0x20008a8c, 'pico_network_connect_stored', 'connect_stored', connected)
        p.compare(0x20008b70, 'pico_network_reconnect', {'initialized': initialized, 'init_ok': initialize_ok, 'connected': connected})
    for connected in (0, 1):
        p = Pair()
        for address in (0x20004488, 0x20005550, 0x20004494, 0x20014584):p.old.hook(address, lambda f: 0)
        p.old.hook(0x20005284, lambda f: p.events[0].append(('connecting', f.cstring(f.reg(2)))) or 0)
        p.h.hook('event', lambda f: p.events[1].append(('connecting', f.cstring(f.reg(2)))))
        p.service(0x20008a8c, 'pico_network_connect_stored', 'connect_stored', connected)
        def state():
            names = ['progress_row', 'progress_start_column', 'progress_end_column', 'progress_glyph_index', 'progress_active']
            return (p.old.read(0x2002bf10, 4) + p.old.read(0x2002bf20, 1),
                    b''.join(p.new.read(p.field('PicoAppState', 'control_signals.' + field), 1) for field in names))
        p.compare(0x20008b1c, 'pico_network_connect_with_screen', {'connected': connected}, state=state)
    for local, first, second in itertools.product((0, 1), repeat=3):
        p = Pair();indices = [0, 0]
        p.service(0x2000dc64, 'pico_network_wait_local_ip', 'wait_local', local, (0, 1), (1, 2))
        def resolve(side, f):
            p.events[side].append(('resolve',));indices[side] += 1
            return first if indices[side] == 1 else second
        p.old.hook(0x20008b98, lambda f: resolve(0, f));p.hook_new('pico_network_resolve_address', lambda f: resolve(1, f))
        p.old.hook(0x20014b14, lambda f: p.events[0].append(('dns', b'\x08' * 4)) or 0)
        p.h.hook('dns', lambda f: p.events[1].append(('dns', struct.pack('<I', f.reg(1)))))
        p.compare(0x2000dd58, 'pico_network_resolve_server', {'local': local, 'first': first, 'second': second})


def logging():
    for level, component, message in [('I', 'WIFI', 'connecting'), ('E', 'STILL', ''),
            ('warning', 'component' * 25, 'bounded prefix'), ('D', 'UTF8', 'caf\u00e9')]:
        p = Pair()
        p.old.unhook(MAP['pico_log_serial'])
        p.new.unhook(p.new.address('pico_network_log'))
        old_args = [];new_args = []
        for index, text in enumerate((level, component, message)):
            old_address = p.old.SCRATCH + index * 0x200
            new_address = p.new.SCRATCH + index * 0x200
            p.old.write(old_address, text.encode() + b'\0')
            p.new.write(new_address, text.encode() + b'\0')
            old_args.append(old_address);new_args.append(new_address)
        p.compare(0x20004a94, 'pico_network_log', {'level': level, 'component_bytes': len(component), 'message': message},
                  old_args, new_args, returns=False, serial=True,
                  state=lambda: (p.clocks[0], p.clocks[1]))
    for condition, counter in itertools.product((0, 1), (0, 17, 0xffffffff)):
        p = Pair();p.old.unhook(MAP['pico_assert_nonfatal'])
        p.new.unhook(p.new.address('pico_network_assert'))
        p.old.write_u32(0x2002c160, counter)
        target = p.field('PicoNetwork', 'assertion_count');p.new.write_u32(target, counter)
        p.old.write(p.old.SCRATCH, b'value != NULL\0');p.new.write(p.new.SCRATCH, b'value != NULL\0')
        p.compare(0x20004b7c, 'pico_network_assert', {'condition': condition, 'counter': counter},
                  [condition, p.old.SCRATCH], [condition, p.new.SCRATCH], returns=False, serial=True,
                  state=lambda: (p.old.read_u32(0x2002c160), p.new.read_u32(target)))


def connection_workflow():
    scenarios = [dict(stored=0, choices=[1]), dict(stored=0, choices=[0, 0, 1]),
                 dict(stored=1, connected=0, choices=[0, 1]),
                 dict(stored=1, connected=1, resolved=0),
                 dict(stored=1, connected=1, resolved=1)]
    for case in scenarios:
        p = Pair();positions = [0, 0]
        for address, event in [(0x20004488, 0), (0x20005550, 1), (0x20009408, 2),
                (0x2000945c, 3), (0x20004494, 4), (0x200093d0, 6)]:
            p.old.hook(address, lambda f, event=event: p.events[0].append(('ui', event)) or 0)
        p.h.hook('event', lambda f: p.events[1].append(('ui', f.reg(1))))
        for address, symbol, tag, result in [(0x2000a5f8, 'pico_storage_load_wifi', 'load_wifi', case['stored']),
                (0x20008b1c, 'pico_network_connect_with_screen', 'connect', case.get('connected', 0)),
                (0x2000dd58, 'pico_network_resolve_server', 'resolve', case.get('resolved', 0)),
                (0x2000fccc, 'fixture_diagnose', 'diagnose', 0),
                (0x2000581c, 'pico_storage_save_wifi', 'save_wifi', 1)]:
            p.service(address, symbol, tag, result)
        def choose(side, f):
            p.events[side].append(('choose',));index = positions[side];positions[side] += 1
            return case['choices'][min(index, len(case['choices'])-1)]
        p.old.hook(0x2000fb30, lambda f: choose(0, f));p.h.hook('choose_wifi', lambda f: choose(1, f))
        p.compare(0x2000fc4c, 'pico_network_ensure_connection', case,
                  state=lambda: (p.clocks[0], p.clocks[1]))


def updates():
    scenarios = [dict(request=0), dict(request=1, connected=0),
                 *[dict(request=1, connected=1, value=value) for value in (ord('T'), ord('F'), 0, -1, 256)],
                 dict(request=1, connected=1, timeout=True),
                 dict(request=1, connected=1, timeout=True, start=0xffffff00)]
    for case in scenarios:
        p = Pair();p.tick = 10001;p.clocks = [case.get('start', 100)] * 2
        def request(side, f):
            p.events[side].append(('request', f.cstring(f.reg(side)), f.reg(side+1), f.reg(side+2)))
            return case['request']
        p.old.hook(0x20010f64, lambda f: request(0, f));p.hook_new('pico_network_call', lambda f: request(1, f))
        p.service(0x200156f4, 'fixture_connected', 'connected', case.get('connected', 0))
        p.service(0x20015218, 'fixture_available', 'available', int('value' in case))
        p.service(0x20015132, 'fixture_read', 'read', case.get('value', 0))
        p.service(0x2000bd10, 'pico_network_close', 'close')
        p.compare(0x20012ea0, 'pico_update_available', case, serial=True,
                  state=lambda: (p.clocks[0], p.clocks[1]))
    for uid_present, load, request_ok, received in itertools.product((0, 1), repeat=4):
        p = Pair();uid = b'0123456789abcdef0123456789abcdef'
        p.old.write(0x2002da9d, (uid if uid_present else b'') + b'\0')
        target = p.field('PicoAppState', 'device_uid')
        p.new.write(target, (uid if uid_present else b'') + b'\0')
        def load_uid(side, f):
            p.events[side].append(('load_uid',))
            if load:f.write(0x2002da9d if side == 0 else target, uid + b'\0')
            return load
        p.old.hook(0x2000a594, lambda f: load_uid(0, f));p.hook_new('pico_storage_load_uid', lambda f: load_uid(1, f))
        def request(side, f):
            p.events[side].append(('request', f.cstring(f.reg(side)), f.reg(side+1), f.reg(side+2)))
            return request_ok
        p.old.hook(0x20010f64, lambda f: request(0, f));p.hook_new('pico_network_call', lambda f: request(1, f))
        p.old.hook(0x20004488, lambda f: p.events[0].append(('clear',)) or 0)
        p.h.hook('event', lambda f: p.events[1].append(('clear',)))
        p.service(0x20012f84, 'pico_update_receive', 'receive', received, (0,), (1,))
        p.compare(0x20013100, 'pico_update_download', {'uid_present': uid_present, 'load': load, 'request': request_ok, 'received': received},
                  state=lambda: (p.old.cstring(0x2002da9d), p.new.cstring(target)))
    for available, downloaded in itertools.product((0, 1), repeat=2):
        p = Pair()
        for address, event in [(0x20004488, 0), (0x20004494, 4), (0x20006c80, 9),
                               (0x20006ccc, 10), (0x20006d18, 11)]:
            p.old.hook(address, lambda f, event=event: p.events[0].append(('ui', event)) or 0)
        p.old.hook(0x20005578, lambda f: 0)
        def clear_rows(f):
            assert (f.reg(0), f.reg(1)) == (4, 7)
            return 0
        p.old.hook(0x200095d8, clear_rows)
        p.h.hook('event', lambda f: p.events[1].append(('ui', f.reg(1))))
        for address, symbol, tag, result, old_args, new_args in [
                (0x20012ea0, 'pico_update_available', 'available', available, (), ()),
                (0x20013100, 'pico_update_download', 'download', downloaded, (), ()),
                (0x20014b6c, 'fixture_disconnect', 'disconnect', 0, (), ()),
                (0x2001839c, 'pico_boot_toggle_application', 'activate', 1, (0,), (1,)),
                (MAP['PRCMMCUReset'], 'fixture_reset', 'reset', 0, (), ()),
                (0x2000dfec, 'pico_api_error', 'error', 1, (0,), (1,)),
                (0x2000c7f8, 'fixture_continue', 'continue', 0, (), ())]:
            p.service(address, symbol, tag, result, old_args, new_args)
        p.compare(0x20013150, 'pico_update_run', {'available': available, 'downloaded': downloaded}, returns=False,
                  state=lambda: (p.clocks[0], p.clocks[1]))


def main():
    global ELF, LAYOUT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ELF)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/network-direct-tests.json')
    options = parser.parse_args();ELF = options.elf
    base.ELF = ELF;base.RebuiltArm = RebuiltArm
    LAYOUT = Layout(ELF)
    for group in (storage, access_points, logging, connection_workflow, updates):
        group();print(group.__name__, 'PASS', len(CASES), flush=True)
    report = {'passed': True, 'total_cases': len(CASES), 'cases': CASES,
              'elf_sha256': hashlib.sha256(ELF.read_bytes()).hexdigest(),
              'reference_sha256': hashlib.sha256((ROOT / 'reference/original-0.1.34.bin').read_bytes()).hexdigest(),
              'scope': __doc__}
    options.output.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
