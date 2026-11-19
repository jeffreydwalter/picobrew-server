#!/usr/bin/env python3
"""Pass the complete rebuilt update body through the reconstructed ARM receiver.

The receiver uses an in-memory flash-file model. This never writes a device.
"""
import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from package_update import encode_update,decode_update
from fast_arm import RebuiltArm
import test_network

def main():
    elf=ROOT/'build/application-validation.elf'
    binary=ROOT/'build/firmware/pico-pro-recovered.bin'
    packet=ROOT/'build/firmware/pico-pro-recovered.update.txt'
    data=binary.read_bytes();body=packet.read_bytes()
    assert decode_update(body)==data
    assert body==encode_update(data)
    test_network.ELF=elf
    test_network.RebuiltArm=RebuiltArm
    model=test_network.Harness()
    # The API wait-for-hash routine consumes this leading marker before the
    # original downloader enters its streamed receiver.
    assert body[:1]==b'#'
    model.input=body[1:]
    assert model.call('pico_update_receive',False,max_instructions=150000000)==1
    assert bytes(model.files['/sys/mcuimg2.bin'])==data
    assert model.boot==bytes.fromhex('00000000badccdab')
    report={'passed':True,'cases':1,'application_elf_sha256':hashlib.sha256(elf.read_bytes()).hexdigest(),
        'firmware_binary_sha256':hashlib.sha256(data).hexdigest(),'update_body_sha256':hashlib.sha256(body).hexdigest(),
        'firmware_bytes':len(data),'update_body_bytes':len(body),
        'receiver':'pico_update_receive','destination':'/sys/mcuimg2.bin',
        'scope':'Complete generated update body decoded by actual rebuilt ARM receiver; exact reconstructed binary and boot-record bytes observed through in-memory file/MD5/socket boundaries.',
        'hardware_executed':False}
    (ROOT/'analysis/update-package-tests.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Update package:',len(data),'firmware bytes passed through rebuilt ARM receiver.')
if __name__=='__main__':main()
