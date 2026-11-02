#!/usr/bin/env python3
"""Unchanged physical TU1.5 17C020 event selection, including its actual RNG.

No instruction patch, supplied callee or game-service result. Exact stores,
read-fault prefixes, GP/XMM and live masked x87 state are independent expected
inputs for the native owner. This is not Xbox graphics/audio output proof.
"""
import argparse
import ctypes
import gzip
import hashlib
import json
import os
from pathlib import Path
import random
import signal
import struct
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK, STOP, NONE, pack
from oracle_weapon_effect_state_tu15 import ALIASES, REGS, DEBUG_FLAGS, CWS, CSRS, EXTENDED, BODY, TAGS, DEFINITION, EVENTS, BAD
from oracle_weapon_effect_advance_tu15 import getfp, putfp, state
from weapon_reference_versions import images

ENTRY=0x17c020
RANDOM=0x101c000
FLOATS=[0,0x80000000,1,0x80000001,0x3e800000,0x3f000000,0x3f800000,0xbf000000,0x7f800000,0xff800000,0x7fc12345,0x7f812345]


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 _,image,pin=next(x for x in images()if x[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;rng=random.Random(ENTRY);rows=[];instructions={};steps=0
 kinds=['fault-tags','fault-body','fault-definition','fault-current','fault-events','fault-random','alias-rng-events-pointer','alias-rng-probability-snan','alias-rng-probability-qnan','alias-rng-counter','alias-counter-current','zero-count','negative-count','last-loop','loop-none','primary-random','auxiliary-random']
 recipes=[('base',n)for n in range(144)]+[('rng-control',n)for n in range(24)]+[(kind,n)for n,kind in enumerate(kinds)]
 with tempfile.TemporaryDirectory(prefix='native-next-event-',dir=scratch)as temporary,OriginalAudioCpu(image,Path(temporary)/'original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000)as host:
  host.trace(6,host.debug_offset+7*8,0)
  for kind,n in recipes:
   gp=[rng.getrandbits(32)for _ in ALIASES];gp[4]=STACK;body=BODY;count=3;current=[-1,0,1,2,3,4][n%6];flags=[0,2,0x200,0x202,0x100,0x8000][n%6];loop=[-1,0,1,2][n%4];definition_flags=[0,4,0x18,0xffffffff][n%4];random_address=RANDOM;probabilities=[FLOATS[(n+2)%len(FLOATS)],FLOATS[(n+5)%len(FLOATS)],0]
   fault_expected=kind.startswith('fault-')or kind=='alias-rng-events-pointer'
   if kind!='base':current=-1;flags=0;loop=-1;definition_flags=4;probabilities=[0x3f000000,0,0]
   if kind=='alias-counter-current':body=STACK-0x64
   if kind=='fault-current':body=0x2100000-0x58
   if kind=='zero-count':count=0
   if kind=='negative-count':count=0x80000000
   if kind=='last-loop':current=2;flags=2;loop=1
   if kind=='loop-none':current=2;flags=0x200;loop=-1
   if kind=='primary-random':definition_flags=4
   if kind=='auxiliary-random':definition_flags=0
   if kind=='alias-rng-events-pointer':random_address=DEFINITION+0x18
   elif kind=='alias-rng-counter':random_address=STACK-8
   elif kind.startswith('alias-rng-probability-'):
    random_address=EVENTS+4;start=0x7f800001 if kind.endswith('snan')else 0x7fc00001
    inverse=pow(1664525,-1,1<<32)
    for desired in range(start,start+10000):
     seed=((desired-1013904223)*inverse)&NONE
     if 0<seed<0x7f800000:break
    else:raise ValueError('No declared positive probability/RNG alias')
    probabilities[0]=seed
   memory=[]
   def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
   word(0x4f0414,TAGS);word(body+4,0x80010007);word(body+2,flags,2)
   if kind!='fault-current':word(body+0x5c,current,2)
   word(TAGS+7*16+8,DEFINITION);word(DEFINITION,definition_flags);word(DEFINITION+4,loop,2);word(DEFINITION+0x14,count);word(DEFINITION+0x18,EVENTS)
   for event,probability in enumerate(probabilities):word(EVENTS+event*0x38+4,probability)
   word(0x4f3cd8,random_address)
   if not kind.startswith('alias-rng-'):
    word(RANDOM,rng.getrandbits(32));word(RANDOM+4,rng.getrandbits(32))
   for at in [0x46a0c8,0x46a160]:word(at,int.from_bytes(image.read(at,4),'little'))
   argument=body
   if kind=='fault-tags':word(0x4f0414,BAD)
   elif kind=='fault-body':argument=BAD
   elif kind=='fault-definition':word(TAGS+7*16+8,BAD)
   elif kind=='fault-events':word(DEFINITION+0x18,BAD)
   elif kind=='fault-random':word(0x4f3cd8,BAD)
   fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|((n*0x100)&0x4700)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=[rng.randbytes(16).hex()for _ in range(8)])
   host.write(STACK-64,rng.randbytes(128))
   for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
   host.write(STACK,pack([STOP,argument]));initial_stack=host.read(STACK-64,128).hex();host.prepare(ENTRY,STACK,dict(zip(ALIASES,gp)));putfp(host,fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags;trace=[];stores=[];fault=[];depth=len(fp['stack']);defined=NONE&~DEBUG_FLAGS
   for _ in range(4000):
    pc=regs.rip
    if pc==STOP:break
    assert ENTRY<=pc<=0x17c12f,hex(pc)
    ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw;instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
    def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
    if ins.mnemonic=='push':pending.append(((regs.rsp-4)&NONE,4))
    assert ins.mnemonic!='call','Unexpected external owner in original next-event selector'
    for operand in ins.operands:
     if operand.type!=3:continue
     at=(reg(ins.reg_name(operand.mem.base))+reg(ins.reg_name(operand.mem.index))*operand.mem.scale+operand.mem.disp)&NONE;operands.append((at,operand.size))
     if operand.access&2:pending.append((at,operand.size))
    host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
    if os.WIFSTOPPED(status)and os.WSTOPSIG(status)==signal.SIGSEGV:
     assert fault_expected and operands,(kind,hex(pc),status);fault=[dict(instruction=pc,address=operands[0][0],size=operands[0][1])];break
    assert os.WIFSTOPPED(status)and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
    for at,width in pending:
     assert STACK-64<=at<STACK+128 or random_address<=at<random_address+8,(kind,hex(pc),hex(at));stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
    if ins.mnemonic in ['fild','fld']:depth+=1
    elif ins.mnemonic in ['fcomip','fcompi','fstp']:depth-=1
    if ins.mnemonic in ['cmp','comiss','ucomiss','add','sub','fcomip','fcompi']:defined=NONE&~DEBUG_FLAGS
    elif ins.mnemonic in ['test','xor','and','or']:defined=(NONE&~DEBUG_FLAGS)&~0x10
    elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
    elif ins.mnemonic in ['shr','shl']:defined=(NONE&~DEBUG_FLAGS)&~(0x10 if ins.operands[-1].imm==1 else 0x810)
    elif ins.mnemonic in ['inc','dec']:defined=(defined&1)|((NONE&~DEBUG_FLAGS)&~1)
   else:raise ValueError('Original next-event selection exceeded declared instruction limit')
   assert bool(fault)==fault_expected,(kind,hex(regs.rip))
   if not fault:assert regs.rip==STOP and regs.rsp==STACK+8
   physical=ctypes.create_string_buffer(512);host.trace(14,0,ctypes.addressof(physical));assert physical.raw[4].bit_count()==depth,(kind,'physical x87 tag/depth mismatch',physical.raw[4],depth)
   rows.append(dict(entry=ENTRY,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,initial_stack=initial_stack,calls=[],stores=stores,fault=fault,trace_offsets=b''.join(struct.pack('<H',pc-ENTRY)for pc in trace).hex(),trace_base=ENTRY,expected=state(host,regs,depth),flags_mask=defined))
 data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_NEXT_EVENT_REFERENCE_V1',original_sha256=pin,scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode();compressed=gzip.compress(data,mtime=0);assert len(data)<=8*1024*1024 and len(compressed)<=1024*1024;args.fixture.write_bytes(compressed);report=dict(passed=True,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),instruction_map=instructions,read_stops=sum(bool(r['fault'])for r in rows),alias_cases=sum(r['kind'].startswith('alias-')for r in rows),actual_rng_updates=sum(s[0]==0x17c0fa for r in rows for s in r['stores']),controlled_lower_calls=0,physical_tag_depth_checked=True,supplied_game_services=[],instruction_patches=[],backend='Physical Linux i386 PTRACE_SINGLESTEP',original_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),raw_bytes=len(data),raw_sha256=hashlib.sha256(data).hexdigest(),fixture_bytes=len(compressed),fixture_sha256=hashlib.sha256(compressed).hexdigest(),scratch_owner=str(scratch),physical_xbox_proof=False,full_user_goal_complete=False);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='instruction_map'}))

if __name__=='__main__':main()
