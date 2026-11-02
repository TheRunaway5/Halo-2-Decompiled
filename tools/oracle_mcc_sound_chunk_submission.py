#!/usr/bin/env python3
"""Observe unchanged MCC post-decode PCM pointer/length submission instructions.

This declared entry slice starts immediately after the real decode call. It
does not execute that call, manufacture a callee return, or prove waveform,
codec-state, Xbox behavior or device output. It observes actual reads of the
chunk's logical length and one-shot byte cursor before the Miles import call.
Run through tools/run_test.py; no raw memory or binary outputs are retained.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from pc_postprocess_inspect import MCC, PE
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_PROT_READ, UC_PROT_EXEC
from unicorn.x86_const import UC_X86_REG_RAX, UC_X86_REG_RBX, UC_X86_REG_RBP, UC_X86_REG_RDI, UC_X86_REG_RCX, UC_X86_REG_RDX, UC_X86_REG_R8, UC_X86_REG_R9, UC_X86_REG_EFLAGS

PIN='de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946'


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if a.output.exists():p.error('Preserve prior original observations')
    pe=PE(MCC);assert hashlib.sha256(pe.data).hexdigest()==PIN
    start,end=pe.base+0x77923e,pe.base+0x77928e
    code=pe.read(pe.base+0x779000,4096);rows=[];sites=set()
    wrapper,chunk,pcm,handle=0x200000,0x210000,0x300000,0x123456789a
    for logical in (2,62,478,480,482,960,1024,262142):
        for skip in sorted({0,1,2,logical//2,logical-1,logical,logical+1,0x7fffffff}):
            for slot in (0,1):
                u=Uc(UC_ARCH_X86,UC_MODE_64);u.mem_map(pe.base+0x779000,4096);u.mem_write(pe.base+0x779000,code)
                u.mem_protect(pe.base+0x779000,4096,UC_PROT_READ|UC_PROT_EXEC)
                for address in (wrapper,chunk,pcm):u.mem_map(address,4096)
                original=bytearray([0xa5]*4096);struct.pack_into('<I',original,0x18,skip);struct.pack_into('<Q',original,0x58,handle)
                u.mem_write(wrapper,bytes(original));u.mem_write(chunk,struct.pack('<4I',0x77531,0x20000031,logical,0x617))
                u.mem_protect(chunk,4096,UC_PROT_READ);u.mem_protect(pcm,4096,UC_PROT_READ)
                for register,value in ((UC_X86_REG_RAX,pcm),(UC_X86_REG_RBX,wrapper),(UC_X86_REG_RDI,chunk),(UC_X86_REG_RBP,slot),(UC_X86_REG_EFLAGS,0x202)):
                    u.reg_write(register,value)
                trace=[];reads=[];writes=[]
                def instruction(uc,address,size,user):
                    assert start<=address<end;trace.append(address-pe.base);sites.add(address-pe.base)
                def read(uc,access,address,size,value,user):
                    reads.append([address,size,int.from_bytes(uc.mem_read(address,size),'little')])
                def write(uc,access,address,size,value,user):writes.append([address,size,value])
                u.hook_add(UC_HOOK_CODE,instruction);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
                u.emu_start(start,end,count=64)
                submitted=min(skip,logical)
                actual={name:u.reg_read(reg) for name,reg in [('RCX',UC_X86_REG_RCX),('RDX',UC_X86_REG_RDX),('R8',UC_X86_REG_R8),('R9',UC_X86_REG_R9)]}
                assert actual==dict(RCX=handle,RDX=0xffffffff,R8=pcm+submitted,R9=logical-submitted)
                expected=bytearray(original);struct.pack_into('<Q',expected,0x20+slot*8,chunk)
                expected_writes=[[wrapper+0x20+slot*8,8,chunk]]
                if skip:struct.pack_into('<I',expected,0x18,0);expected_writes.append([wrapper+0x18,4,0])
                assert bytes(u.mem_read(wrapper,4096))==bytes(expected) and writes==expected_writes
                assert reads==[[wrapper+0x18,4,skip],[chunk+8,4,logical],[wrapper+0x58,8,handle]]
                rows.append(dict(logical_pcm_bytes=logical,byte_cursor=skip,buffer_slot=slot,submitted=actual,
                    ordered_reads=reads,ordered_writes=writes,original_trace=trace))
    result=dict(passed=True,scope=__doc__,dll=str(MCC),dll_sha256=PIN,entry_rva=hex(start-pe.base),stop_before_call_rva=hex(end-pe.base),
        code_interval_sha256=hashlib.sha256(pe.read(start,end-start)).hexdigest(),cases=rows,case_count=len(rows),
        original_instruction_sites=sorted(sites),original_instruction_count=sum(len(r['original_trace']) for r in rows),
        code_patches=[],supplied_callee_results=[],declared_post_decode_entry=True,decoded_PCM_or_device_output_verified=False,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
    assert hashlib.sha256(MCC.read_bytes()).hexdigest()==PIN
    raw=json.dumps(result,indent=2)+'\n';assert len(raw)<1024*1024
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(raw)
    print(json.dumps(dict(passed=True,cases=len(rows),original_instruction_sites=len(sites),instructions=result['original_instruction_count'])))


if __name__=='__main__':main()
