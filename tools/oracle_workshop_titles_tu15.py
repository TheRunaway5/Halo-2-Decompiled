#!/usr/bin/env python3
"""Capture whole unchanged TU1.5 delayed-title evaluator and slot writes.

Title banks, clock words, argument packets and poisoned registers are declared
inputs. Only HS argument parsing/completion are intercepted; the title effect
and its numeric conversions execute unchanged. This is not live presentation
or title-lifetime acceptance. Raw output belongs to run_test.py scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

GP=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
    R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)
STATE,CLOCK,ARGS,STACK,STOP=0x2100000,0x2102000,0x2103000,0x21fff00,0x3000000


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    opcode=561;descriptor=int.from_bytes(image.read(0x480c00+opcode*4,4),'little')
    assert descriptor==0x459654
    assert struct.unpack('<HHIIH',image.read(descriptor,14))[:3]==(4,0,0x2b3620)
    assert struct.unpack('<H',image.read(descriptor+12,2))[0]==2
    assert struct.unpack('<HH',image.read(descriptor+14,4))==(16,6)
    code=_Code(image,'.text');bodies={entry:_trace(code,entry,{entry}) for entry in (0x2b3620,0x13ecc0)}
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for first,size in ((0x10000,0x600000),(0x2100000,0x100000),(STOP,4096)):g.mem_map(first,size)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    def put(at,*words):g.mem_write(at,struct.pack('<'+'I'*len(words),*(v&0xffffffff for v in words)))
    def word(at):return int.from_bytes(g.mem_read(at,4),'little')
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    parsed=True;calls=[];trace=[];stores=[];covered=set()
    def instruction(vm,pc,size,_):
        assert bytes(vm.mem_read(pc,size))==image.read(pc,size)
        if pc==0x212010:
            sp=vm.reg_read(R.UC_X86_REG_ESP)
            packet=[word(sp+n) for n in (4,8,12,16)]
            assert packet==[0x12340001,2,descriptor+14,0x23450002]
            calls.append(dict(target=pc,arguments=packet))
            vm.reg_write(R.UC_X86_REG_EAX,ARGS if parsed else 0)
            vm.reg_write(R.UC_X86_REG_ESP,sp+20);vm.reg_write(R.UC_X86_REG_EIP,word(sp));return
        if pc==0x211da0:
            assert vm.reg_read(R.UC_X86_REG_EAX)==0 and vm.reg_read(R.UC_X86_REG_ECX)==0x12340001
            sp=vm.reg_read(R.UC_X86_REG_ESP);calls.append(dict(target=pc,result=0,thread=0x12340001))
            vm.reg_write(R.UC_X86_REG_ESP,sp+4);vm.reg_write(R.UC_X86_REG_EIP,word(sp));return
        assert any(pc in body.code for body in bodies.values()),hex(pc)
        trace.append(pc);covered.add(pc)
    def store(vm,_kind,at,size,value,_):
        if STATE<=at<STATE+36:
            stores.append(dict(pc=vm.reg_read(R.UC_X86_REG_EIP),address=at,bytes=size,
                before=bytes(vm.mem_read(at,size)).hex(),after=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
    delays=[bits(x) for x in (-2.,-1/60,-0.,0.,1/60,.05,1.,32768.,1e30)]
    delays += [0x7fc12345,0x7f800000,0xff800000]
    rows=[]
    for hs in (False,True):
        for free in range(5):
            for index,delay in enumerate(delays):
                parsed=not hs or index%3!=0
                before=bytearray(range(36))
                for slot in range(4):
                    struct.pack_into('<HH',before,8+slot*4,0xffff if slot==free else 17+slot,0xa5a0+slot)
                tick_rate=(30,60,-30,0,32767,-32768)[index%6]
                title=(0,1,0xffff,0x7fff,0x8000,0x12345678)[index%6]
                g.mem_write(STATE,bytes(before));put(0x51df50,STATE)
                # A full bank never dereferences the clock; retain a missing
                # clock pointer there as an actual read-boundary control.
                put(0x51df54,0 if free==4 else CLOCK);g.mem_write(CLOCK+2,struct.pack('<h',tick_rate));put(ARGS,title,delay)
                initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
                for register,value in zip(GP,initial):g.reg_write(register,value)
                for i in range(8):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*[0x3f000000+i*4+n for n in range(4)]),'little'))
                g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
                g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                entry=0x2b3620 if hs else 0x13ecc0
                if hs:put(STACK,STOP,opcode,0x12340001,0x23450002)
                else:put(STACK,STOP,title,delay)
                calls.clear();trace.clear();stores.clear();g.emu_start(entry,STOP,count=10000)
                assert g.reg_read(R.UC_X86_REG_EIP)==STOP
                assert g.reg_read(R.UC_X86_REG_ESP)==STACK+(16 if hs else 12)
                rows.append(dict(entry=entry,hs=hs,parsed=parsed,first_free_slot=free,tick_rate=tick_rate,title=title,delay_bits=delay,
                    before=before.hex(),after=bytes(g.mem_read(STATE,36)).hex(),initial_registers=initial,
                    registers=[g.reg_read(r) for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
                    xmm=[g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little').hex() for i in range(8)],
                    fpu_status=g.reg_read(R.UC_X86_REG_FPSW),calls=list(calls),trace=list(trace),stores=list(stores)))
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,opcode=opcode,descriptor=descriptor,arguments=[16,6],cases=rows,
        instruction_patches=[],intercepted_calls=['HS argument parsing212010','HS completion211DA0'],
        effects_intercepted=[],executed_instructions=len(covered),
        body_instructions={str(entry):[[pc,bytes(code.at(pc).bytes).hex()] for pc in sorted(body.code)] for entry,body in bodies.items()},
        source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    raw=(json.dumps(report,indent=2)+'\n').encode();assert len(raw)<=1024*1024
    a.output.write_bytes(raw)
    print(json.dumps(dict(cases=len(rows),original_instructions=len(covered),bytes=len(raw))),flush=True)


if __name__=='__main__':main()
