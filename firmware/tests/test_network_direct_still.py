#!/usr/bin/env python3
"""Original/rebuilt Still-network comparisons at explicit subordinate boundaries.

The real original and reconstructed control flow, formatting, parsers, owned
state and failure handling execute. Scripted Still replies and SDK/UI services
replace external I/O. Folded IPv4 helpers and malformed-input repairs are
identified separately. No hardware or network session is operated.
"""
from collections import Counter
from pathlib import Path
import argparse
import hashlib
import itertools
import json
import struct
import sys
import test_network_direct as d

ROOT = Path(__file__).resolve().parents[1]
CONN = 0x2002d580
TELEMETRY = 0x2002d2f8
REPAIRS = []
FAILURES = []


def one(label, callback):
    try:
        callback()
    except Exception as error:
        FAILURES.append({'case': label, 'error': str(error)})
        print('FAIL', label, str(error), flush=True)


def record(address, replacement, case, kind, **evidence):
    d.CASES.append({'original_address': hex(address), 'replacement': replacement,
                    'case': case, 'kind': kind, **evidence})


def put_connection(p, home=b'\xc0\xa8\x01\x10', active=b'\xc0\xa8\x04\x01',
                   lan=1, session=0, failures=0, ssid=b'PicoStill_aabbcc', identifier=b'aabbcc'):
    for name, old_offset, value, size in [
            ('home_ip', 4, home, 4), ('active_ip', 16, active, 4),
            ('lan_connected', 8, bytes([lan]), 1), ('session_started', 20, bytes([session]), 1),
            ('ssid', 22, (ssid + b'\0').ljust(32, b'\0'), 32),
            ('identifier', 54, (identifier + b'\0').ljust(16, b'\0'), 16)]:
        assert len(value) == size
        p.old.write(CONN + old_offset, value)
        p.new.write(p.field('PicoStillConnection', name), value)
    p.old.write(CONN + 21, bytes([failures]))
    p.new.write_u32(p.field('PicoStillConnection', 'consecutive_failures'), failures)


def connection_state(p):
    old = (p.old.read(CONN + 4, 4), p.old.read(CONN + 16, 4), p.old.read(CONN + 8, 1),
           p.old.read(CONN + 20, 1), p.old.cstring(CONN + 22), p.old.cstring(CONN + 54),
           p.old.read(CONN + 21, 1)[0])
    new = (p.new.read(p.field('PicoStillConnection', 'home_ip'), 4),
           p.new.read(p.field('PicoStillConnection', 'active_ip'), 4),
           p.new.read(p.field('PicoStillConnection', 'lan_connected'), 1),
           p.new.read(p.field('PicoStillConnection', 'session_started'), 1),
           p.new.cstring(p.field('PicoStillConnection', 'ssid')),
           p.new.cstring(p.field('PicoStillConnection', 'identifier')),
           p.new.read_u32(p.field('PicoStillConnection', 'consecutive_failures')))
    return old, new


def assertions(p):
    for side, arm in enumerate((p.old, p.new)):
        def assertion(f, side=side):
            p.events[side].append(('assert', f.reg(side), f.cstring(f.reg(side + 1))))
        arm.hook(d.MAP['pico_assert_nonfatal'] if not side else arm.address('pico_network_assert'), assertion)


def still_replies(p, replies):
    positions = [0, 0]
    for side, arm in enumerate((p.old, p.new)):
        def call(f, side=side):
            index = positions[side]; positions[side] += 1
            success, body = replies[min(index, len(replies) - 1)]
            capacity = f.reg(side + 2)
            # Old max-content length vs new NUL-inclusive buffer capacity.
            p.events[side].append(('still_call', f.reg(side), capacity - side))
            assert len(body) <= capacity - side
            f.write(f.reg(side + 1), body + b'\0')
            return success
        if side:
            p.hook_new('pico_still_call', call)
        else:
            arm.hook(d.MAP['pico_still_call'], call)
    return positions


def scalar_and_folded():
    for message in [b'', b'StillAction::GetStillIp', b'not a float', b'network failure 123']:
        p = d.Pair()
        for arm in (p.old, p.new): arm.write(arm.SCRATCH, message + b'\0')
        p.old.call(0x20006308, [p.old.SCRATCH])
        p.new.call_symbol('still_error', [p.h.n, p.new.SCRATCH])
        assert p.serial[0] == p.serial[1] == b'ERROR ' + message + b'\r\n'
        record(0x20006308, 'still_error', {'message': message.decode()},
               'direct_original_vs_rebuilt', serial_compared=True)
    for ip in [bytes(4), bytes([1, 2, 3, 4]), bytes([192, 168, 0, 1]), bytes([255]) * 4]:
        p = d.Pair(); put_connection(p, active=ip)
        p.old.call(0x2000632c, [CONN + 12])
        observed = []
        p.h.hook('connect', lambda f: observed.append((f.reg(1), f.reg(2), f.reg(3))) or 0)
        assert p.h.call('pico_still_call', 0, p.new.SCRATCH, 51) == 0
        actual_ip = p.serial[1].split(b'[ip = ', 1)[1].split(b']', 1)[0]
        assert actual_ip == p.serial[0]
        assert observed == [(int.from_bytes(ip, 'little'), 80, 1)]
        record(0x2000632c, 'pico_still_call (folded IPv4 log)', {'octets': list(ip)},
               'direct_original_vs_actual_folded_caller', serial_compared=True,
               caller='pico_still_call', boundary='connect failure after formatted address')
    for failures in [0, 1, 254, 255]:
        p = d.Pair()
        put_connection(p, session=255, lan=255, failures=failures)
        p.old.call(0x20006380, [CONN])
        p.new.call_symbol('pico_still_connection_initialize', [p.h.n + p.h.o[5]])
        old, new = connection_state(p)
        assert old == new == (bytes(4), bytes(4), b'\0', b'\0', b'', b'', failures)
        record(0x20006380, 'pico_still_connection_initialize', {'preserved_failures': failures},
               'direct_original_vs_rebuilt', state_compared=True)
    for ip in [bytes(4), bytes([0, 0, 0, 1]), bytes([1, 0, 0, 0]), bytes([255]) * 4]:
        p = d.Pair(); put_connection(p, home=ip)
        before = p.old.call(0x20006434, [CONN])['r0']
        after = not p.new.call_symbol('pico_network_ipv4_is_zero', [int.from_bytes(ip, 'little')])['r0']
        assert before == after
        record(0x20006434, '!pico_network_ipv4_is_zero(home_ip)', {'octets': list(ip)},
               'direct_original_vs_folded_predicate', return_compared=True)
        for home in [False, True]:
            p = d.Pair(); put_connection(p, home=ip); assertions(p)
            p.compare(0x20006454, 'pico_still_select_network', {'home': home, 'octets': list(ip)},
                      old_args=[CONN, int(home)], new_args=[int(home)], returns=False,
                      state=lambda: connection_state(p), serial=True)


def simple_requests():
    for success, body in [(0, b''), (1, b'0.0.0.0'), (1, b'192.168.1.22'),
                          (1, b'255.255.255.255'), (1, b'256.1.2.3'), (1, b'not an ip'),
                          (1, b'1.2.3'), (1, b'1.2.3.4.5')]:
        p = d.Pair(); put_connection(p); still_replies(p, [(success, body)])
        p.compare(0x2000bf28, 'pico_still_get_ip', {'success': success, 'response': body.decode()},
                  state=lambda: connection_state(p), serial=True)
    for lan, success, body in itertools.product([0, 1], [0, 1], [b'', b'T', b'F', b'True', b'true']):
        p = d.Pair(); put_connection(p, lan=lan); still_replies(p, [(success, body)])
        p.compare(0x2000bf64, 'pico_still_connect_lan',
                  {'initial_lan': lan, 'success': success, 'response': body.decode()},
                  state=lambda: connection_state(p), serial=True)
    for success, body in itertools.product([0, 1], [b'', b'T', b'F', b'True', b'true']):
        p = d.Pair(); still_replies(p, [(success, body)])
        p.compare(0x200102a4, 'pico_still_start', {'success': success, 'response': body.decode()}, serial=True)
    for enabled, success in itertools.product([0, 1], repeat=2):
        p = d.Pair(); still_replies(p, [(success, b'')])
        p.compare(0x20010560, 'pico_still_set_fan', {'enabled': enabled, 'success': success},
                  old_args=[enabled], new_args=[enabled], returns=False, serial=True)
    for session, started in itertools.product([0, 1, 255], [0, 1]):
        p = d.Pair(); put_connection(p, session=session)
        p.service(0x200102a4, 'pico_still_start', 'start', started)
        p.compare(0x200102e8, 'pico_still_start_once', {'active': session, 'start_result': started},
                  old_args=[CONN], state=lambda: connection_state(p), serial=True)
    for session, success in itertools.product([0, 1, 255], [0, 1]):
        p = d.Pair(); put_connection(p, session=session); still_replies(p, [(success, b'F')])
        p.old.hook(0x200065ec, lambda f: p.events[0].append(('ui', 13)) or 0)
        p.old.hook(0x200093d0, lambda f: p.events[0].append(('ui', 6)) or 0)
        p.h.hook('event', lambda f: p.events[1].append(('ui', f.reg(1))))
        p.compare(0x20010340, 'pico_still_stop', {'active': session, 'stop_result': success},
                  old_args=[CONN], returns=bool(session), state=lambda: connection_state(p), serial=True,
                  kind='direct_original_vs_rebuilt' if session else 'inactive_noop_effects_original_serial_count_not_bool_return')
        if not session:
            d.CASES[-1]['return_contract'] = 'Original r0 is incidental Serial.println byte count; reconstructed bool is defined false.'



def float_observer(p):
    # Keep native float formatting as an explicit SDK boundary here. The old
    # call takes float32 in r1; the typed callback widens that same value to an
    # AAPCS-aligned double in r2/r3 and puts precision on the stack.
    def observed(side, f):
        if side == 0:
            bits, digits = f.reg(1), f.reg(2)
        else:
            value = struct.unpack('<d', struct.pack('<II', f.reg(2), f.reg(3)))[0]
            bits = struct.unpack('<I', struct.pack('<f', value))[0]
            digits = f.read_u32(f.reg('sp'))
        p.events[side].append(('serial_float32', bits, digits))
        token = f'<float32:{bits:08x};digits={digits}>'.encode()
        p.serial[side].extend(token + (b'\r\n' if side == 0 else b''))
        return 0
    p.old.hook(d.MAP['Print_println_float'], lambda f: observed(0, f))
    p.h.hook('serial_float', lambda f: observed(1, f))


def parser_logging():
    for value in ['0', '-0', '-1', '0.005', '1.005', '10.255', '99.995', '153.7',
                  '1e-10', '1234567.89', '4294967296', '']:
        p = d.Pair(); float_observer(p)
        response = ','.join([value] * 8).encode()
        initial = bytes((i * 17 + 11) & 255 for i in range(96))
        for arm, location in [(p.old, TELEMETRY), (p.new, p.h.app)]:
            arm.write(location, initial); arm.write(arm.SCRATCH, response + b'\0')
        p.compare(0x200064d8, 'pico_still_parse_response', {'repeated_field': value},
                  old_args=[TELEMETRY, p.old.SCRATCH], new_args=[p.h.app, p.new.SCRATCH],
                  returns=False, serial=True, state=lambda: (p.old.read(TELEMETRY, 96), p.new.read(p.h.app, 96)))


def status():
    replies = [(0, b''), (0, b'partial'), (1, b''),
               (1, b'10.25,20.5,30.75,4,1.25,0,1,0'),
               (1, b',,,,,,,'), (1, b'-1,-2,-3,-4,-0.5,1,0,1')]
    for failures, (success, body) in itertools.product([0, 1, 254, 255], replies):
        p = d.Pair(); put_connection(p, failures=failures); float_observer(p)
        initial = bytes((i * 17 + 11) & 255 for i in range(96))
        p.old.write(TELEMETRY, initial); p.new.write(p.h.app, initial)
        still_replies(p, [(success, body)])
        p.compare(0x2000bf9c, 'pico_still_read_status',
                  {'previous_failures': failures, 'success': success, 'response': body.decode()},
                  old_args=[TELEMETRY], serial=True,
                  state=lambda: ((p.old.read(TELEMETRY, 96), p.old.read(CONN + 21, 1)[0]),
                    (p.new.read(p.h.app, 96), p.new.read_u32(p.field('PicoStillConnection', 'consecutive_failures')))))
    for failures, body in itertools.product([0, 254, 255],
            [b'123456789012345,2,3,4,5,6,7,8', b'1,2,3']):
        p = d.Pair(); put_connection(p, failures=failures); float_observer(p)
        initial = bytes(96);p.old.write(TELEMETRY, initial);p.new.write(p.h.app, initial)
        still_replies(p, [(1, body)])
        old = p.old.call(0x2000bf9c, [TELEMETRY], max_instructions=3000000)['r0']
        new = p.h.call('pico_still_read_status', max_instructions=3000000)
        assert old == 1 and new == 0
        assert p.new.read(p.h.app, 1) == b'\0'
        assert p.new.read_u32(p.field('PicoStillConnection', 'consecutive_failures')) == ((failures + 1) & 255)
        REPAIRS.append({'original_address': '0x2000bf9c', 'previous_failures': failures,
                        'response': body.decode(), 'original_success': old, 'rebuilt_success': new,
                        'change': 'Reject malformed telemetry instead of ignoring parser failure.'})


def choose_ap():
    scans = [[], ['Home', 'Other'], ['PicoStill_'], ['Home', 'PicoStill_aabbcc'],
             ['PicoStill_aaa', 'Elsewhere', 'PicoStill_bbb', 'PicoStill_ccc'],
             [f'PicoStill_{i:02d}' for i in range(22)]]
    for networks in scans:
        matches = [x for x in networks if x.startswith('PicoStill_')]
        for selected in ([0, len(matches) - 1] if len(matches) > 1 else [0]):
            p = d.Pair(); put_connection(p); assertions(p)
            for side, arm in enumerate((p.old, p.new)):
                def scan(f, side=side):p.events[side].append(('scan',));return len(networks)
                def ssid(f, side=side):
                    index = f.reg(1);p.events[side].append(('ssid', index))
                    ptr = f.SCRATCH + index * 64;f.write(ptr, networks[index].encode() + b'\0');return ptr
                arm.hook(0x20014c90 if not side else arm.address('fixture_scan'), scan)
                arm.hook(0x20014d00 if not side else arm.address('fixture_ssid'), ssid)
            p.old.hook(0x20004488, lambda f: 0)
            p.old.hook(d.MAP['pico_ui_draw_subheader'], lambda f: 0)
            def choose(side, f):
                count = f.reg(1 if not side else 2)
                pointer = 0x2002bf24 if not side else f.reg(1)
                ids = [f.cstring(pointer + i * 22) for i in range(count)]
                p.events[side].append(('choose', ids))
                return selected
            p.old.hook(0x2000c2c2, lambda f: choose(0, f))
            p.h.hook('choose_still', lambda f: choose(1, f))
            p.compare(0x2000c468, 'pico_still_choose_access_point',
                      {'networks': networks, 'selected': selected}, state=lambda: connection_state(p), serial=True)
    # Bounded behavior is intentionally tested without executing original
    # out-of-array writes for oversized scans or menu selections.
    for ssids, selected, reason in [([f'PicoStill_{i}' for i in range(23)], 0, '23 matching networks'),
            (['PicoStill_' + 'x' * 22], 0, 'SSID longer than 31 bytes'),
            (['PicoStill_' + 'x' * 16], 0, 'identifier longer than owned 15-byte payload'),
            (['PicoStill_abc'], -1, 'negative selection'),
            (['PicoStill_abc'], 1, 'selection beyond match count')]:
        p = d.Pair(); put_connection(p)
        p.h.hook('scan', lambda f: len(ssids))
        def ssid(f):
            ptr = f.SCRATCH + f.reg(1) * 64; f.write(ptr, ssids[f.reg(1)].encode() + b'\0'); return ptr
        p.h.hook('ssid', ssid);p.h.hook('choose_still', lambda f: d.U32(selected))
        assert p.h.call('pico_still_choose_access_point', max_instructions=3000000) == 0
        REPAIRS.append({'original_address': '0x2000c468', 'change': reason, 'rebuilt_success': False})


def connect_wifi():
    for home, connect, local, ip in itertools.product([0, 1], [0, 1], [0, 1], [bytes(4), b'\xc0\xa8\x01\x16']):
        p = d.Pair();put_connection(p, home=ip);assertions(p)
        p.service(0x20014b6c, 'fixture_disconnect', 'disconnect')
        def load(side, f):
            p.events[side].append(('load_wifi',))
            for name, address, value in [('wifi_ssid', 0x2002c2d0, b'HomeSSID\0'),
                    ('wifi_password', 0x2002c290, b'HomePassword\0'), ('wifi_security', 0x2002c2f1, b'\6')]:
                f.write(address if not side else p.field('PicoSettings', name), value)
            return 1
        p.old.hook(d.MAP['pico_storage_load_wifi_credentials'], lambda f: load(0, f))
        p.hook_new('pico_storage_load_wifi', lambda f: load(1, f))
        p.service(0x20008a8c, 'pico_network_connect_stored', 'connect_stored', connect)
        p.service(0x2000dc64, 'pico_network_wait_local_ip', 'wait_local_ip', local, (0, 1), (1, 2))
        def state():
            old, new = connection_state(p)
            return ((old, p.old.cstring(0x2002c2d0), p.old.cstring(0x2002c290), p.old.read(0x2002c2f1, 1)),
                    (new, p.new.cstring(p.field('PicoSettings', 'wifi_ssid')),
                     p.new.cstring(p.field('PicoSettings', 'wifi_password')),
                     p.new.read(p.field('PicoSettings', 'wifi_security'), 1)))
        p.compare(0x2000dcc8, 'pico_still_connect_wifi',
                  {'home': home, 'connect_result': connect, 'ip_result': local, 'home_ip': list(ip)},
                  old_args=[home], new_args=[home], state=state, serial=True)


def wait_response():
    scenarios = [
        {'name': 'immediate_success', 'connect': 1, 'replies': [(1, b'T')], 'delay': 0, 'timeout': 30},
        {'name': 'false_ping', 'connect': 1, 'replies': [(1, b'F')], 'delay': 2, 'timeout': 30},
        {'name': 'retries_then_success', 'connect': 1, 'replies': [(0, b''), (0, b''), (1, b'T')], 'delay': 1, 'timeout': 30},
        {'name': 'connect_failure', 'connect': 0, 'replies': [(0, b'')], 'delay': 2, 'timeout': 30},
        {'name': 'timeout_after_retries', 'connect': 1, 'replies': [(0, b'')], 'delay': 0, 'timeout': 2},
        {'name': 'zero_timeout', 'connect': 1, 'replies': [(1, b'T')], 'delay': 0, 'timeout': 0},
        {'name': 'absolute_deadline_wrap', 'connect': 1, 'replies': [(1, b'T')], 'delay': 0, 'timeout': 1, 'clock': 0xfffffe00}]
    for home, scenario in itertools.product([0, 1], scenarios):
        p = d.Pair(); p.tick = 100;p.clocks = [scenario.get('clock', 100)] * 2
        p.service(0x2000dcc8, 'pico_still_connect_wifi', 'connect_wifi', scenario['connect'], (0,), (1,))
        still_replies(p, scenario['replies'])
        p.compare(0x20010104, 'pico_still_wait_response', {'home': home, 'scenario': scenario['name']},
                  old_args=[home, scenario['delay'], scenario['timeout']],
                  new_args=[home, scenario['delay'], scenario['timeout']], serial=True)
        assert p.clocks[0] == p.clocks[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/application-validation.elf')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/network-direct-still-tests.json')
    options = parser.parse_args()
    d.ELF = options.elf; d.base.ELF = options.elf;d.base.RebuiltArm = d.RebuiltArm
    d.LAYOUT = d.Layout(options.elf)
    for group in (scalar_and_folded, simple_requests, parser_logging, status, choose_ap, connect_wifi, wait_response):
        one(group.__name__, group)
        print(group.__name__, 'completed', len(d.CASES), flush=True)
    report = {'passed': not FAILURES, 'total_cases': len(d.CASES),
              'repair_scenarios': len(REPAIRS), 'cases': d.CASES,
              'repaired_input_checks': REPAIRS, 'failures': FAILURES,
              'elf_sha256': hashlib.sha256(options.elf.read_bytes()).hexdigest(),
              'reference_sha256': hashlib.sha256((ROOT / 'reference/original-0.1.34.bin').read_bytes()).hexdigest(),
              'test_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'evidence_per_entry': dict(Counter(x['original_address'] for x in d.CASES)), 'scope': __doc__}
    options.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'total_cases', 'repair_scenarios', 'evidence_per_entry', 'failures')}, indent=2))
    if FAILURES:raise SystemExit(1)


if __name__ == '__main__':main()
