#!/usr/bin/env python3
"""Encode a binary as the original Pico getFirmware API response body.

Output is ASCII: #<decimal byte count>,<MD5>|<lowercase hex image>#.
The MD5 input is the signed decimal nibble sum, with every fifteenth nibble
(index0 included) subtracted. This preserves Pico's legacy transport checksum;
it is not a cryptographic signature or a secure update-authentication scheme.
No device, server, boot record or repository is modified by this tool.
"""
import argparse
import hashlib
from pathlib import Path
APPLICATION_CAPACITY=0x2f800

def encode_update(data,include_start_marker=True):
    if not data or len(data)>APPLICATION_CAPACITY:
        raise ValueError('Application payload must contain1..194560 bytes')
    text=data.hex()
    total=sum((-1 if index%15==0 else 1)*int(character,16)
              for index,character in enumerate(text))&0xffffffff
    signed=total if total<0x80000000 else total-0x100000000
    checksum=hashlib.md5(str(signed).encode('ascii')).hexdigest()
    envelope=f'{len(data)},{checksum}|{text}#'.encode('ascii')
    return (b'#' if include_start_marker else b'')+envelope

def decode_update(body):
    if not body.startswith(b'#') or not body.endswith(b'#'):
        raise ValueError('Missing Pico API markers')
    metadata,hexadecimal=body[1:-1].split(b'|',1)
    length,checksum=metadata.split(b',',1)
    data=bytes.fromhex(hexadecimal.decode('ascii'))
    if int(length)!=len(data) or encode_update(data)!=body:
        raise ValueError('Length, encoding or checksum mismatch')
    return data

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args();raw=args.binary.read_bytes();body=encode_update(raw)
    if decode_update(body)!=raw:raise AssertionError('Envelope round trip failed')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(body)
    print(f'{args.output}: {len(raw)} firmware bytes, {len(body)} API body bytes')
if __name__=='__main__':main()
