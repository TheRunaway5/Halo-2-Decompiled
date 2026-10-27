#!/usr/bin/env python3
"""Authenticate the TU1.5 right-stick scoreboard scroll input for SDL raw Y.

All65,536 SDL signed16 values pass through the existing native Y-sign/clamp
transport, then unchanged127965..1279B9 instructions. No graphics or game
results are supplied. Output is a small reference fixture, never a raw bank.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_EIP
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT)
    machine=Uc(UC_ARCH_X86,UC_MODE_32)
    machine.mem_map(0x10000,0x800000)
    for section in image.sections:
        machine.mem_write(section.va,image.section_bytes(section))
    processed=0x60000000
    machine.mem_map(processed,0x1000)
    entry,end=0x127965,0x1279b9
    probes=set(range(-32768,32768,257))|{-32768,-32767,-9002,-9001,-9000,-8999,-1,0,1,8999,9000,9001,9002,32766,32767}
    rows=[]
    digest=hashlib.sha256()
    for native in range(-32768,32768):
        raw=max(-32768,min(32767,-native))
        machine.reg_write(UC_X86_REG_ESI,raw&0xffff)
        machine.reg_write(UC_X86_REG_EDI,processed+0x20)
        machine.emu_start(entry,end,count=100)
        assert machine.reg_read(UC_X86_REG_EIP)==end
        result=struct.unpack('<h',machine.mem_read(processed+0x46,2))[0]
        expected=(raw-9000)*32767//23767 if raw>9000 else -((-raw-9000)*32768//23768) if raw<-9000 else 0
        assert result==expected,(native,raw,result,expected)
        digest.update(struct.pack('<h',result))
        if native in probes:rows.append(dict(native_right_y=native,source_right_y=raw,processed_scroll_axis=result))
    report=dict(schema='H2_SCOREBOARD_SCROLL_TU15_V1',source_xbe_sha256=NEW_PIN,
        entry=f'{entry:08x}',stop_before=f'{end:08x}',
        instruction_sha256=hashlib.sha256(image.read(entry,end-entry)).hexdigest(),
        instruction_patches=[],supplied_game_or_graphics_results=[],
        native_transport='native_tu15_input_devices.cpp vertical sign flip and signed16 clamp',
        exhaustive_cases=65536,exhaustive_result_sha256=digest.hexdigest(),probes=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(f'PASS 65536 unchanged TU1.5 right-Y transforms; {len(rows)} retained boundary/sample probes; SHA256 {digest.hexdigest()}')


if __name__=='__main__':
    main()
