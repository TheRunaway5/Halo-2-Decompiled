#!/usr/bin/env python3
"""Unchanged TU1.5 party-A timer blocks and complete parameter writer.

Uses explicit synthetic widget/session data. Timer execution stops before the
network publication call; press execution starts after status/host/getter gates
and stops before its publication. The complete5DF30 parameter writer executes
unchanged. No instructions or callees are patched. No original Live service,
network serialization, physical controller or framebuffer is claimed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

BASE, OWNER, OBJECT, STACK, STOP = 0x1000000, 0x1009000, 0x100A000, 0x100E000, 0x1010000


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'),'Run through tools/run_test.py'
    old=Xbe(Path('assets/disc/default.xbe')); image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True
    sec=image.section('.text');raw=image.section_bytes(sec)
    def locate(entry,size):
        pattern=bytearray(old.read(entry,size));mask=[False]*size
        for i in md.disasm(bytes(pattern),entry):
            start=i.address-entry
            if (i.mnemonic=='call' or i.mnemonic.startswith('j')) and i.imm_size:
                for off in range(i.imm_offset,i.imm_offset+i.imm_size):mask[start+off]=True
            if i.disp_size and any(o.type==X86_OP_MEM and 0x10000<=o.mem.disp<0x800000 for o in i.operands):
                for off in range(i.disp_offset,i.disp_offset+i.disp_size):mask[start+off]=True
        expression=b''.join(b'.' if masked else re.escape(bytes([v])) for v,masked in zip(pattern,mask))
        matches=list(re.finditer(expression,raw,re.DOTALL))
        assert len(matches)==1,(hex(entry),len(matches))
        return sec.va+matches[0].start()
    timer=locate(0x6E360,0x30);press=locate(0x19A7CA,0x18);writer=locate(0x5DF30,0x60)
    timer_ins=list(md.disasm(image.read(timer,0xA4),timer))
    timer_calls=[i for i in timer_ins if i.mnemonic=='call']
    publication=timer_calls[-1].address
    clock_flag=next(o.mem.disp for i in timer_ins for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and o.size==1)
    clock_value=next(o.mem.disp for i in timer_ins for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and o.size==4)
    state_offset=next(o.mem.disp for i in timer_ins for o in i.operands if o.type==X86_OP_MEM and o.mem.base==UC_X86_REG_EBP and o.mem.disp>=0x7000)
    machine=Uc(UC_ARCH_X86,UC_MODE_32);machine.mem_map(0x10000,0x800000)
    for section in image.sections:machine.mem_write(section.va,image.section_bytes(section))
    machine.mem_map(BASE,0x20000)
    def put(at,value,n=4):machine.mem_write(at,int(value&((1<<(n*8))-1)).to_bytes(n,'little'))
    def word(at):return int.from_bytes(machine.mem_read(at,4),'little')
    trace=[];boundaries=set()
    def observe(uc,pc,n,_):
        if pc in boundaries:uc.emu_stop()
        else:trace.append(pc)
    machine.hook_add(UC_HOOK_CODE,observe)
    rows=[]
    for seconds,elapsed in [(s,e) for s in (10,3,1,0) for e in (0,999,1000,2500,0x80000000,0xFFFFFFFF)]:
        machine.mem_write(BASE,bytes(0x10000));put(OBJECT+8,OWNER);put(OWNER+0x30,BASE)
        put(BASE+state_offset,3);put(BASE+0x49A8,1,1);put(BASE+0x49AC,seconds)
        put(OBJECT+0x14,0);put(OBJECT+0x18,seconds)
        put(clock_flag,1,1);put(clock_value,elapsed)
        put(STACK,STOP);machine.reg_write(UC_X86_REG_ESP,STACK);machine.reg_write(UC_X86_REG_ESI,OBJECT)
        boundaries.clear();boundaries.add(publication);trace.clear()
        machine.emu_start(timer,STOP,count=1000)
        published=machine.reg_read(UC_X86_REG_EIP)==publication
        rows.append(dict(kind='timer',seconds=seconds,elapsed=elapsed,published=published,
                         value=word(OBJECT+0x18),clock=word(OBJECT+0x14),instructions=list(trace)))
    # Original state10 press block after19A8D0; status/host/getter are inputs.
    for seconds in range(0,12):
        sp=STACK;machine.mem_write(sp,struct.pack('<5I',0,0,STOP,10,3))
        machine.reg_write(UC_X86_REG_ESP,sp);machine.reg_write(UC_X86_REG_EAX,seconds)
        machine.reg_write(UC_X86_REG_EBX,0);machine.reg_write(UC_X86_REG_ESI,7)
        boundaries.clear();boundaries.update((press+(0x19A7D7-0x19A7CA),press+(0x19A7E2-0x19A7CA)))
        trace.clear();machine.emu_start(press,STOP,count=1000)
        submitted=machine.reg_read(UC_X86_REG_EIP)==press+(0x19A7D7-0x19A7CA)
        rows.append(dict(kind='press',seconds=seconds,minimum=3,published=submitted,
                         value=word(machine.reg_read(UC_X86_REG_ESP)+4) if submitted else seconds,instructions=list(trace)))
    for mode,leader,start,seconds,oldseconds,oldmode in ((0,1,1,10,0,0),(0,1,0,0,10,0),(1,2,1,10,3,0),(0,1,1,9,10,1)):
        machine.mem_write(BASE,bytes(0x8000));put(BASE+0x50,1)
        put(BASE+0x49A8,int(oldseconds>0),1);put(BASE+0x49AC,oldseconds)
        put(BASE+0x49B0,oldmode);put(BASE+0x49B4,10 if oldmode else -1)
        machine.mem_write(BASE+0x49B8,b'abcdefghijkl');machine.mem_write(OWNER,b'123456789012')
        machine.mem_write(STACK,struct.pack('<III',STOP,leader,OWNER))
        for reg,v in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_ECX,BASE),(UC_X86_REG_EAX,seconds),(UC_X86_REG_EDX,start),(UC_X86_REG_EDI,mode)):
            machine.reg_write(reg,v)
        boundaries.clear();trace.clear();machine.emu_start(writer,STOP,count=1000)
        assert machine.reg_read(UC_X86_REG_EIP)==STOP
        rows.append(dict(kind='parameter',mode=mode,member=leader,start=start,seconds=seconds,oldseconds=oldseconds,oldmode=oldmode,
          active=int.from_bytes(machine.mem_read(BASE+0x49A8,1),'little'),value=word(BASE+0x49AC),
          result_mode=word(BASE+0x49B0),requester=bytes(machine.mem_read(BASE+0x49B8,12)).hex(),instructions=list(trace)))
    report=dict(source_sha256=NEW_PIN,scope=__doc__,entries=dict(timer=timer,press=press,writer=writer),
      code_hashes={hex(v):hashlib.sha256(image.read(v,n)).hexdigest() for v,n in ((timer,0xA4),(press,0x18),(writer,0xF1))},cases=rows)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS unchanged TU1.5 party countdown: {len(rows)} controls, {sum(len(r["instructions"]) for r in rows)} instructions; '+str(report['entries']))


if __name__=='__main__':main()
