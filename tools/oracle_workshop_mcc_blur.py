#!/usr/bin/env python3
"""Execute the complete unchanged MCC-only rasterizer_blur wrapper/effect.

The parser/completion boundaries receive declared ABI inputs. Argument loads,
deferred return, empty effect and completion dispatch execute original code.
This is offline contract evidence, not an admitted engine adapter or a claim
that MCC's empty legacy effect implements prototype/Xbox cinematic blur.
Run through run_test.py, with raw output inside HALO2_TEST_SCRATCH.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from audit_workshop_compatibility import Pe64, MCC_PIN

ENTRY,EFFECT,PARSE,COMPLETE=0x18078a9d0,0x180030cc0,0x1806f9e60,0x1806f9f50
ARGS,STACK,STOP=0x210000000,0x21fff0008,0x220000000
GP=(R.UC_X86_REG_RAX,R.UC_X86_REG_RCX,R.UC_X86_REG_RDX,R.UC_X86_REG_RBX,
    R.UC_X86_REG_RSP,R.UC_X86_REG_RBP,R.UC_X86_REG_RSI,R.UC_X86_REG_RDI,
    R.UC_X86_REG_R8,R.UC_X86_REG_R9,R.UC_X86_REG_R10,R.UC_X86_REG_R11,
    R.UC_X86_REG_R12,R.UC_X86_REG_R13,R.UC_X86_REG_R14,R.UC_X86_REG_R15)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(scratch)
    image=Pe64(Path.home()/'.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    declaration=image.builtin(691)
    assert (declaration['name'],declaration['return_type'],declaration['arguments'],declaration['mcc_flags'],
        declaration['mcc_evaluator'])==('rasterizer_blur',4,[7],0,hex(ENTRY))
    header=struct.unpack_from('<I',image.data,0x3c)[0];optional=header+24
    rva,size=struct.unpack_from('<II',image.data,optional+112+3*8)
    bounds=[struct.unpack_from('<III',image.read(image.base+rva+n,12)) for n in range(0,size,12)]
    selected=[row for row in bounds if row[0]+image.base==ENTRY]
    assert len(selected)==1 and selected[0][1]+image.base==ENTRY+46
    decoder=Cs(CS_ARCH_X86,CS_MODE_64)
    instructions=list(decoder.disasm(image.read(ENTRY,46),ENTRY))
    assert sum(i.size for i in instructions)==46 and len(instructions)==16
    effect=image.read(EFFECT,3);assert effect==bytes.fromhex('c20000')
    allowed={i.address:bytes(i.bytes) for i in instructions};allowed[EFFECT]=effect
    boundaries={}
    for entry in (PARSE,COMPLETE):
        # Translation may decode beyond an intercepted entry before invoking
        # the code hook. Retain complete original pages, never zero-filled
        # instruction tails. No instruction inside these boundaries executes.
        page=entry&~4095
        boundaries[page]=image.read(page,8192)
    g=Uc(UC_ARCH_X86,UC_MODE_64)
    mapped=set()
    for at,length in ((ENTRY,46),(EFFECT,3),*[(at,len(raw)) for at,raw in boundaries.items()],
            (ARGS,4096),(STACK-4096,8192),(STOP,1)):
        first=at&~4095;last=(at+length+4095)&~4095
        for page in range(first,last,4096):
            if page not in mapped:g.mem_map(page,4096);mapped.add(page)
    g.mem_write(ENTRY,image.read(ENTRY,46));g.mem_write(EFFECT,effect)
    for at,raw in boundaries.items():g.mem_write(at,raw)
    def qword(at):return int.from_bytes(g.mem_read(at,8),'little')
    def put(at,value):g.mem_write(at,struct.pack('<Q',value&0xffffffffffffffff))
    calls=[];trace=[];reads=[];writes=[];covered=set();parsed=False
    def instruction(vm,pc,size,_):
        if pc in (PARSE,COMPLETE):
            assert bytes(vm.mem_read(pc,size))==image.read(pc,size)
            packet=[vm.reg_read(r) for r in (R.UC_X86_REG_RCX,R.UC_X86_REG_RDX,R.UC_X86_REG_R8,R.UC_X86_REG_R9)]
            calls.append(dict(target=pc,register_arguments=packet))
            if pc==PARSE:vm.reg_write(R.UC_X86_REG_RAX,ARGS if parsed else 0)
            sp=vm.reg_read(R.UC_X86_REG_RSP);vm.reg_write(R.UC_X86_REG_RIP,qword(sp));vm.reg_write(R.UC_X86_REG_RSP,sp+8);return
        assert pc in allowed and bytes(vm.mem_read(pc,size))==allowed[pc],hex(pc)
        trace.append(pc);covered.add(pc)
    def read(vm,_kind,at,size,value,_):
        if ARGS<=at<ARGS+4096:reads.append(dict(pc=vm.reg_read(R.UC_X86_REG_RIP),address=at,bytes=size))
    def write(vm,_kind,at,size,value,_):
        writes.append(dict(pc=vm.reg_read(R.UC_X86_REG_RIP),address=at,bytes=size,value=value&((1<<(size*8))-1)))
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
    cases=[]
    values=(0,1,32767,32768,65535,0x12345678,0xffff0000,0xa55a8001)
    for parsed_ in (False,True):
        for initial_flags in (0x202,0x247):
            for thread in (0x12340001,0xf2340001):
                for value in values:
                    parsed=parsed_;g.mem_write(STACK-256,b'\xa5'*512);g.mem_write(ARGS,struct.pack('<I',value)+b'\xb6'*12)
                    initial=[0x1122334455667788+n*0x0101010101010101 for n in range(16)]
                    initial[1]=691;initial[2]=thread;initial[4]=STACK
                    for reg,word in zip(GP,initial):g.reg_write(reg,word)
                    for n in range(16):g.reg_write(R.UC_X86_REG_XMM0+n,int.from_bytes(bytes((n+j)&255 for j in range(16)),'little'))
                    g.reg_write(R.UC_X86_REG_EFLAGS,initial_flags);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
                    g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                    put(STACK,STOP);calls.clear();trace.clear();reads.clear();writes.clear()
                    g.emu_start(ENTRY,STOP,count=1000)
                    assert g.reg_read(R.UC_X86_REG_RIP)==STOP and g.reg_read(R.UC_X86_REG_RSP)==STACK+8
                    assert len(calls)==(2 if parsed else 1)
                    assert calls[0]['target']==PARSE and calls[0]['register_arguments']==[691,thread,initial[8],initial[9]]
                    if parsed:
                        assert reads==[dict(pc=ENTRY+18,address=ARGS,bytes=2)]
                        assert calls[1]['target']==COMPLETE and calls[1]['register_arguments']==[thread,0,initial[8],initial[9]]
                        assert EFFECT in trace
                    else:assert not reads and EFFECT not in trace
                    assert bytes(g.mem_read(ARGS,16))==struct.pack('<I',value)+b'\xb6'*12
                    assert all(STACK-48<=r['address']<STACK for r in writes)
                    cases.append(dict(parsed=parsed,argument_bits=value,thread=thread,initial_flags=initial_flags,
                        initial_registers=initial,registers=[g.reg_read(r) for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
                        xmm=[g.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16,'little').hex() for n in range(16)],
                        calls=list(calls),argument_reads=list(reads),stack_writes=list(writes),trace=list(trace)))
    assert covered==set(allowed)
    report=dict(scope=__doc__,mcc_sha256=MCC_PIN,declaration=declaration,cases=cases,
        complete_wrapper_bounds=selected[0],body_instructions=[[pc,raw.hex()]for pc,raw in sorted(allowed.items())],
        instruction_patches=[],effect_interceptions=[],intercepted_calls=[hex(PARSE),hex(COMPLETE)],
        executed_instructions=len(covered),native_adapter_enabled=False,source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    raw=(json.dumps(report,indent=2)+'\n').encode();assert len(raw)<=1024*1024
    a.output.write_bytes(raw);print(json.dumps(dict(cases=len(cases),instructions=len(covered),bytes=len(raw))))


if __name__=='__main__':main()
