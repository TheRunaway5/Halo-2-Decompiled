#!/usr/bin/env python3
"""Unchanged TU1.5 instruction-window reference for native objective rules.

Run through tools/run_test.py. Only small declared input/output fixtures leave
scratch. Xbox code executes offline in Unicorn, never in the native game.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from oracle_ctf_activation_tu15 import SOURCE
from oracle_smg_firing_1_1 import Xbe, _Code
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EFLAGS, UC_X86_REG_EIP)

REGS=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,
      UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)
OPTIONS,CLOCK,BANK=0x4000000,0x4002000,0x4004000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert scratch.is_dir(), 'Original CPU probes require tools/run_test.py ownership'
    spec=SOURCE['u5'];path=Path(spec['path'])
    assert hashlib.sha256(path.read_bytes()).hexdigest()==spec['pin']
    image=Xbe(path);code=_Code(image,'.text')
    cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(0x10000,0x600000);cpu.mem_write(code.lo,code.bytes)
    cpu.mem_map(OPTIONS,0x10000)
    writes=[]
    cpu.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,address,size,value,user:
        writes.append([uc.reg_read(UC_X86_REG_EIP),address,size,value&((1<<(8*size))-1)]))
    cases=[]
    progress_values=(-32768,-4,-1,0,1,2,3,29,30,149,150,299,300,3599,3600,32766,32767)
    windows={}
    for kind,start,end in (('arm',0x249866,0x24989a),('decay',0x24991c,0x249934)):
        windows[kind]={'start':start,'end':end,
            'sha256':hashlib.sha256(image.read(start,end-start)).hexdigest()}
        for index in (0,8):
            for progress in progress_values:
                for seconds,rate in ((seconds,rate) for seconds in (0,1,5,10,120,32767,-1)
                        for rate in (1,30,60,32767)) if kind=='arm' else ((0,30),):
                    gp=[0x12345678,0x24681357,0x13572468,0x10203040,OPTIONS+0xf000,BANK,0xffffffff,index]
                    cpu.mem_write(0x4f3218,struct.pack('<I',OPTIONS))
                    cpu.mem_write(0x51df54,struct.pack('<I',CLOCK))
                    cpu.mem_write(OPTIONS+0x244,struct.pack('<h',seconds))
                    cpu.mem_write(CLOCK+2,struct.pack('<h',rate))
                    at=BANK+0x1a0+index*2
                    cpu.mem_write(at,struct.pack('<h',progress))
                    for reg,value in zip(REGS,gp):cpu.reg_write(reg,value)
                    cpu.reg_write(UC_X86_REG_EFLAGS,0xad7);writes.clear()
                    cpu.emu_start(start,end,count=100)
                    assert cpu.reg_read(UC_X86_REG_EIP)==end
                    cases.append({'kind':kind,'progress':progress,'seconds':seconds,'rate':rate,
                        'gp':gp,'eflags':0xad7,'expected_gp':[cpu.reg_read(reg) for reg in REGS],
                        'expected_eflags':cpu.reg_read(UC_X86_REG_EFLAGS)&0x8d5,
                        'expected_progress':struct.unpack('<h',cpu.mem_read(at,2))[0],
                        'writes':list(writes)})
    assert len(cases)==986
    fixture={'schema':'H2_NATIVE_OBJECTIVE_RULE_WINDOWS_V1','original_sha256':spec['pin'],
        'windows':windows,'supplied_service_results':[],'cases':cases}
    data=json.dumps(fixture,separators=(',',':'))+'\n'
    assert len(data.encode())<=1024*1024, 'Original objective fixture exceeds its bounded reference budget'
    args.fixture.write_text(data)
    print(json.dumps({'cases':len(cases),'fixture_bytes':args.fixture.stat().st_size,
        'original_sha256':spec['pin']}))


if __name__=='__main__':main()
