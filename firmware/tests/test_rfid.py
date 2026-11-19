#!/usr/bin/env python3
"""Compare the real original and reconstructed RFID code at SPI/GPIO boundaries."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from original_arm import ArmFirmware, IMAGE_SHA256
from rebuilt_arm import RebuiltArm


def crc_a(data):
    crc = 0x6363
    for byte in data:
        byte ^= crc & 255
        byte ^= (byte << 4) & 255
        crc = ((crc >> 8) ^ (byte << 8) ^ (byte << 3) ^ (byte >> 4)) & 65535
    return bytes((crc & 255, crc >> 8))


class ReaderModel:
    def __init__(self, scenario):
        self.scenario = scenario
        self.registers = {i: (i * 7 + 5) & 255 for i in range(0, 128, 2)}
        self.fifo = []
        self.selected = False
        self.address = None
        self.reading = False
        self.events = []
        self.transactions = 0

    def event(self, name, *args):
        self.events.append((name, *args))

    def gpio(self, pin, value):
        self.event('write', pin, value)
        if pin == 4:
            self.selected = value == 0
            self.address = None

    def transceive(self):
        sent = bytes(self.fifo)
        self.transactions += 1
        self.registers[8], self.registers[12], self.registers[24] = 0x30, 0, 0
        mode = self.scenario.get('mode', 'card')
        if mode == 'timeout':
            self.registers[8] = 1
            self.fifo = []
            return
        if mode == 'irq_exhaustion':
            self.registers[8] = 0
            self.fifo = []
            return
        if mode == 'error':
            self.registers[12] = self.scenario.get('error', 1)
        if mode == 'collision':
            self.registers[12] = 8
            self.registers[28] = self.scenario.get('collision', 0)
        if 'response' in self.scenario:
            self.fifo = list(self.scenario['response'])
            self.registers[24] = self.scenario.get('valid_bits', 0)
            return
        uid = self.scenario.get('uid', bytes.fromhex('04112233'))
        if sent and sent[0] == 0x26:
            self.fifo = [4, 0]
        elif sent and sent[0] == 0x50:
            self.registers[8] = 1
            self.fifo = []
        elif len(sent) >= 2 and sent[0] in (0x93, 0x95, 0x97):
            level = (sent[0] - 0x93) // 2
            index = level * 3
            cascade = len(uid) > index + 4
            if sent[1] == 0x70:
                sak = 4 if cascade else 8
                self.fifo = list(bytes([sak]) + crc_a([sak]))
                if self.scenario.get('bad_sak_crc'):
                    self.fifo[-1] ^= 1
            else:
                fragment = bytes([0x88]) + uid[index:index + 3] if cascade else uid[index:index + 4]
                checksum = 0
                for byte in fragment:
                    checksum ^= byte
                self.fifo = list(fragment + bytes([checksum]))
        else:
            self.fifo = [0x12, 0x34, 0x56]

    def transfer(self, value):
        self.event('spi', value)
        if not self.selected:
            raise AssertionError('SPI operation without RFID selection')
        if self.address is None:
            self.address = value & 0x7e
            self.reading = bool(value & 0x80)
            return 0
        reg = self.address
        if self.reading:
            if reg == 0x12:
                return self.fifo.pop(0) if self.fifo else 0
            if reg == 0x14:
                return len(self.fifo)
            return self.registers.get(reg, 0)
        if reg == 0x12:
            self.fifo.append(value)
        elif reg == 0x14 and value & 0x80:
            self.fifo.clear()
        elif reg in (8, 10):
            self.registers[reg] &= ~value
        else:
            self.registers[reg] = value
        if reg == 2 and value == 3:
            checksum = crc_a(self.fifo)
            self.registers[0x44], self.registers[0x42] = checksum
            if not self.scenario.get('crc_timeout'):
                self.registers[10] |= 4
        if reg == 0x1a and value & 0x80:
            self.transceive()
        return 0


ENTRIES = {
 'construct': (0x20018494, 'pico_rfid_construct'),
 'configure': (0x200184a0, 'pico_rfid_configure_spi'),
 'write': (0x200184bc, 'pico_rfid_write_register'),
 'write_buffer': (0x20018500, 'pico_rfid_write_register_buffer'),
 'read': (0x20018550, 'pico_rfid_read_register'),
 'read_buffer': (0x20018598, 'pico_rfid_read_register_buffer'),
 'set': (0x200185ac, 'pico_rfid_set_register_bits'),
 'clear': (0x200185ca, 'pico_rfid_clear_register_bits'),
 'crc': (0x200185e8, 'pico_rfid_calculate_crc'),
 'reset': (0x2001865e, 'pico_rfid_reset'),
 'initialize': (0x20018698, 'pico_rfid_initialize_reader'),
 'antenna_off': (0x20018724, 'pico_rfid_antenna_off'),
 'communicate': (0x2001872c, 'pico_rfid_communicate'),
 'transceive': (0x20018888, 'pico_rfid_transceive'),
 'request': (0x20018916, 'pico_rfid_request'),
 'select': (0x20018928, 'pico_rfid_select'),
 'down': (0x20018b9a, 'pico_rfid_power_down'),
 'up': (0x20018bbc, 'pico_rfid_power_up'),
 'halt': (0x20018bd8, 'pico_rfid_halt'),
 'new': (0x20018c22, 'pico_rfid_is_new_card_present'),
 'serial': (0x20018c44, 'pico_rfid_read_card_serial'),
 'probe': (0x20006040, 'pico_rfid_probe'),
}
VALUE_RETURNS = {'read','crc','communicate','transceive','request','select','halt','new','serial','probe'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/application-validation.elf')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/rfid-tests.json')
    args = parser.parse_args()
    addresses = {x['name']: int(x['address'], 16) for x in json.loads((ROOT / 'reference/function_map.json').read_text())}
    records = []
    rng = random.Random(0x18494)

    def execute(operation, parameters=(), scenario=None, initial=None):
        outputs = []
        for rebuilt in (False, True):
            machine = RebuiltArm(args.elf) if rebuilt else ArmFirmware()
            if rebuilt:
                machine.call_symbol('rfid_fixture_bind')
                reader = machine.address('rfid_fixture_reader')
                data = machine.address('rfid_fixture_data')
                result = machine.address('rfid_fixture_result')
                uid = machine.address('rfid_fixture_uid')
            else:
                reader, data, result, uid = (0x2002d570 if operation == 'probe' else 0x20081000), 0x20082000, 0x20083000, 0x20084000
            model = ReaderModel(scenario or {})
            reader_bytes = bytes.fromhex('000102030405060708090a00041200')
            machine.write(reader, reader_bytes)
            machine.write(data, bytes(range(128)))
            machine.write(result, bytes([0xa5]) * 128)
            machine.write(uid, initial or bytes.fromhex('040411223344556677889908'))
            hooks = {
                'pinMode': ('rfid_fixture_pin_mode', lambda m: model.event('mode', m.reg(1 if rebuilt else 0), m.reg(2 if rebuilt else 1))),
                'digitalWrite': ('rfid_fixture_write', lambda m: model.gpio(m.reg(1 if rebuilt else 0), m.reg(2 if rebuilt else 1))),
                'SPI_transfer_byte': ('rfid_fixture_transfer', lambda m: model.transfer(m.reg(1))),
                'SPI_set_bit_order': ('rfid_fixture_order', lambda m: model.event('order', m.reg(1))),
                'SPI_set_data_mode': ('rfid_fixture_mode', lambda m: model.event('spi_mode', m.reg(1))),
                'delay': ('rfid_fixture_delay', lambda m: model.event('delay', m.reg(1 if rebuilt else 0))),
            }
            for original_name, (symbol, callback) in hooks.items():
                if rebuilt:
                    machine.hook_symbol(symbol, callback)
                else:
                    machine.hook(addresses[original_name], callback)
            call_args = [reader]
            if operation == 'construct': call_args += list(parameters or (4,18,0))
            elif operation in ('write','set','clear'): call_args += list(parameters)
            elif operation == 'read': call_args += list(parameters)
            elif operation == 'write_buffer': call_args += [0x12, parameters[0], data]
            elif operation == 'read_buffer':
                model.fifo = list(range(20,84))
                call_args += [0x12, parameters[0], result, parameters[1]]
            elif operation == 'crc': call_args += [data, parameters[0], result]
            elif operation == 'request':
                machine.write(result + 32, bytes([parameters[0]]))
                call_args += [result, result + 32]
            elif operation == 'select': call_args += [uid, parameters[0]]
            elif operation in ('transceive','communicate'):
                machine.write(result + 96, bytes([parameters[0], parameters[1]]))
                call_args += [data, 3, result, result + 96, result + 97, parameters[2], parameters[3]]
                if operation == 'communicate': call_args[1:1] = [12, 0x30]
            original, symbol = ENTRIES[operation]
            outcome = machine.call(machine.address(symbol) if rebuilt else original,
                                   args=call_args[:4], stack_args=call_args[4:], max_instructions=4000000)
            outputs.append({'return': outcome['r0'] if operation in VALUE_RETURNS else None,
                            'reader': machine.read(reader,15), 'result':machine.read(result,128),
                            'uid':machine.read(uid,12),'events':model.events,
                            'instructions':outcome['instructions']})
        before, after = outputs
        for key in ('return','reader','result','uid','events'):
            if before[key] != after[key]:
                diagnostic = {'operation':operation,'parameters':parameters,'scenario':scenario,
                              'field':key,'original':repr(before[key])[:16000],'rebuilt':repr(after[key])[:16000]}
                (ROOT/'analysis/rfid-failure.json').write_text(json.dumps(diagnostic,indent=2)+'\n')
                raise AssertionError(diagnostic)
        records.append({'operation':operation,'parameters':parameters,'events':len(before['events']),
                        'original_instructions':before['instructions']})

    for operation in ('construct','configure','reset','initialize','antenna_off','down','up','probe'):
        execute(operation)
    for _ in range(40):
        reg, value = rng.randrange(64)*2, rng.randrange(256)
        for operation in ('write','set','clear'): execute(operation,(reg,value))
        execute('read',(reg,))
    for count in (0,1,2,3,16,63,64):
        execute('write_buffer',(count,))
        execute('crc',(count,))
        for alignment in (0,1,3,7,8): execute('read_buffer',(count,alignment))
    execute('crc',(3,),{'crc_timeout':True})
    for operation in ('transceive','communicate'):
        for scenario in ({'mode':'timeout'}, {'mode':'irq_exhaustion'}, {'mode':'error','error':1},
                         {'mode':'error','error':0x10}, {'mode':'collision'},
                         {'response':[0x12,0x34]}, {'response':[0x12,0x34,0x56]},
                         {'response':[0xa],'valid_bits':4}, {'response':[0xa]},
                         {'response':list(bytes([0x12,0x34])+crc_a([0x12,0x34]))}):
            for capacity in (0,2,8):
                for check in (0,1): execute(operation,(capacity,0,0,check),scenario)
    for capacity in (0,1,2,8):
        for mode in ('card','timeout','collision','error'): execute('request',(capacity,),{'mode':mode})
    for operation in ('select','serial','new','halt'):
        for uid in ('04112233','04112233445566','04112233445566778899'):
            execute(operation,(0,),{'uid':bytes.fromhex(uid)})
        for scenario in ({'mode':'timeout'}, {'mode':'collision','collision':0x20},
                         {'mode':'collision','collision':0}, {'mode':'error'}, {'bad_sak_crc':True}):
            execute(operation,(0,),scenario)
    for known in (1,2,3,7,8,24,32,56,80,81,256):
        execute('select',(known,))
    report={'passed':len(records),'failed':0,'firmware_sha256':IMAGE_SHA256,
            'elf_sha256':hashlib.sha256(args.elf.read_bytes()).hexdigest(),
            'source_sha256':hashlib.sha256((ROOT/'src/rfid.c').read_bytes()).hexdigest(),
            'original_instructions':sum(x['original_instructions'] for x in records),
            'scope':'Original and rebuilt RFID register, FIFO, CRC, request, anti-collision/select, power and probe instructions execute. Hardware SPI/GPIO/register behavior is a deterministic model; state, returns and exact I/O ordering compared.',
            'limits':'No physical RFID or interrupt concurrency; FIFO counts <=64 and UID output buffers follow documented valid capacities. UID application workflows are separately tested.',
            'cases':records}
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print('RFID:',len(records),'exact original/rebuilt ARM scenarios passed.')


if __name__=='__main__':main()
