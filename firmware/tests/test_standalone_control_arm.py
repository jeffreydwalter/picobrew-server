#!/usr/bin/env python3
"""Execute the real ARM IPSR/PRIMASK guard and ISR filter interlock in Unicorn."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_SP, UC_ARM_REG_LR,
                               UC_ARM_REG_XPSR, UC_ARM_REG_PRIMASK)


def main():
    root=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=str(root/'.deps/toolchain/bin/arm-none-eabi-gcc'))
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='pico-arm-control-') as directory:
        elf_path=Path(directory)/'guard.elf'
        keep=['arm_prepare','arm_filter_probe','arm_signals_enabled',
              'pico_local_service','pico_local_control_stopped','pico_local_control_foreground']
        command=[args.cc,'-mcpu=cortex-m4','-mthumb','-mfloat-abi=soft','-Os',
                 '-ffunction-sections','-fdata-sections','-DPICO_STANDALONE=1',
                 '-Wall','-Wextra','-Werror','-I'+str(root/'include'),'-nostartfiles',
                 '--specs=nano.specs','--specs=nosys.specs','-T'+str(root/'tests/validation.ld'),
                 '-Wl,--gc-sections,--entry=arm_prepare']
        command += ['-Wl,--undefined='+name for name in keep]
        command += [str(root/name) for name in ['src/local_control.c','src/actuators.c',
                                               'tests/standalone_control_arm.c']]
        command += ['-o',str(elf_path)]
        subprocess.run(command,check=True)
        uc=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
        uc.mem_map(0x20000000,0x100000)
        with elf_path.open('rb') as handle:
            elf=ELFFile(handle)
            for segment in elf.iter_segments():
                if segment['p_type']=='PT_LOAD': uc.mem_write(segment['p_vaddr'],segment.data())
            symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
        sentinel=0x200ff000
        uc.mem_write(sentinel,b'\x00\xbe')
        returned=False
        def stop(machine,address,size,context):
            nonlocal returned
            returned=True; machine.emu_stop()
        uc.hook_add(UC_HOOK_CODE,stop,begin=sentinel,end=sentinel)
        def call(name,exception=0,masked=0):
            nonlocal returned
            returned=False
            uc.reg_write(UC_ARM_REG_SP,0x200f0000)
            uc.reg_write(UC_ARM_REG_LR,sentinel|1)
            uc.reg_write(UC_ARM_REG_XPSR,(1<<24)|exception)
            uc.reg_write(UC_ARM_REG_PRIMASK,masked)
            uc.emu_start(symbols[name]|1,0,count=200000)
            assert returned, name+' did not return'
            return uc.reg_read(UC_ARM_REG_R0)
        def value(name): return struct.unpack('<I',uc.mem_read(symbols[name],4))[0]
        call('arm_prepare')
        assert call('pico_local_control_foreground')==1
        call('pico_local_service'); assert value('arm_poll_count')==1
        assert value('arm_history_polls')==1
        assert value('arm_telemetry_polls')==1
        for exception,masked in [(0,1),(16,0),(16,1)]:
            assert call('pico_local_control_foreground',exception,masked)==0
            call('pico_local_service',exception,masked)
            assert value('arm_poll_count')==1
            assert value('arm_history_polls')==1
            assert value('arm_telemetry_polls')==1
        for exception,masked in [(0,1),(16,0),(16,1)]:
            call('arm_prepare')
            call('arm_filter_probe',exception,masked)
            assert call('pico_local_control_stopped')==1
            assert call('arm_signals_enabled')==0
            assert value('arm_outputs')==0 and value('arm_high_writes')==0
            assert value('arm_pause_count')==0 and value('arm_poll_count')==0
            assert value('arm_history_polls')==0
            assert value('arm_telemetry_polls')==0
        print('PASS: ARM foreground polling; no polling with IPSR/PRIMASK; ISR filter trip disables all outputs without UI.')


if __name__=='__main__':
    main()
