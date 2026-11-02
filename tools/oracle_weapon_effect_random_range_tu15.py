#!/usr/bin/env python3
"""Physical unchanged TU1.5 259B0: actual LCG and extended random range.

No supplied callee or game-service result. Caller data includes deliberate
aliases and read/readonly-write faults. Original instruction bytes are never
patched. Live x87 depth is measured from the physical tag file at every step.
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
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE,pack
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,CSRS,EXTENDED,BAD
from oracle_weapon_effect_advance_tu15 import getfp,putfp,state
from weapon_reference_versions import images
ENTRY=0x259b0
SEED=0x1004000
READONLY=0x4100000
PAIRS=[(0,0x3f800000),(0x3f800000,0),(0x80000000,0),(0,0x80000000),
 (0x3e800000,0x3f000000),(0xbf800000,0x3f800000),(0x7f800000,0x7f800000),
 (0xff800000,0x7f800000),(0x7fc12345,0x3f800000),(0x3f800000,0xffc12345),
 (0x7f812345,0x3f800000),(0x3f800000,0xff812345),(1,0x7fffff),
 (0x80000001,0x807fffff),(0x7f7fffff,0xff7fffff),(0xff7fffff,0x7f7fffff),
 (0x00800000,0x00800001),(0x3f800001,0x3f800002),(0x3f800000,0x3f800000),
 (0x7fc12345,0xffc54321),(0x7f812345,0xff854321),(0x3f000001,0x40000001),
 (0x80800001,0x00800001),(0x4b000001,0x4b000002)]

def depth(host):
 raw=ctypes.create_string_buffer(512);host.trace(14,0,ctypes.addressof(raw));return raw.raw[4].bit_count()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);_,image,pin=next(x for x in images()if x[0]=='TU1.5');rng=random.Random(ENTRY);decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;rows=[];instructions={};steps=0
 kinds=['alias-push','alias-min','alias-max','alias-min-snan','alias-max-snan','alias-min-qnan','alias-max-qnan','alias-cross-arguments','fault-read','fault-write','fault-read-edge','fault-write-edge'];recipes=[('base',n)for n in range(288)]+[(kind,n)for n,kind in enumerate(kinds)]
 with tempfile.TemporaryDirectory(prefix='native-random-range-',dir=scratch)as temporary,OriginalAudioCpu(image,Path(temporary)/'original.elf',0x27f,0x1f80,extra_declared_regions=[(READONLY-4096,4096)],read_only_declared_regions=[(READONLY,4096)],source_bank_bytes=0x1100000)as host:
  host.trace(6,host.debug_offset+7*8,0)
  for kind,n in recipes:
   minimum,maximum=PAIRS[(n//12)%len(PAIRS)];seed_address=SEED;seed=rng.getrandbits(32);memory=[]
   if kind=='alias-push':seed_address=STACK-4
   elif kind.startswith('alias-min'):seed_address=STACK+4
   elif kind.startswith('alias-max'):seed_address=STACK+8
   elif kind=='alias-cross-arguments':seed_address=STACK+6
   if kind.endswith('snan')or kind.endswith('qnan'):
    target=0x7f812345 if kind.endswith('snan')else 0x7fc12345;value=((target-1013904223)*pow(1664525,-1,1<<32))&NONE
    if kind.startswith('alias-min'):minimum=value
    else:maximum=value
   if kind=='fault-read':seed_address=BAD
   elif kind=='fault-write':seed_address=READONLY
   elif kind=='fault-read-edge':seed_address=0x2100000-2
   elif kind=='fault-write-edge':seed_address=READONLY-2
   def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
   if kind!='fault-read':word(seed_address,seed,2 if kind=='fault-read-edge'else 4)
   for at in [0x46a0c8,0x46a160]:word(at,int.from_bytes(image.read(at,4),'little'))
   gp=[rng.getrandbits(32)for _ in ALIASES];gp[1]=seed_address;gp[4]=STACK;count=(n//12)%7
   fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|((n*0x100)&0x4700)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],stack=[EXTENDED[(n+i)%len(EXTENDED)]for i in range(count)],xmm=[rng.randbytes(16).hex()for _ in range(8)])
   host.write(STACK-64,rng.randbytes(128))
   for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
   host.write(STACK,pack([STOP,minimum,maximum]));initial_stack=host.read(STACK-64,128).hex();host.prepare(ENTRY,STACK,dict(zip(ALIASES,gp)));putfp(host,fp);assert depth(host)==count;regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags;trace=[];stores=[];fault=[];defined=NONE&~DEBUG_FLAGS
   for _ in range(100):
    pc=regs.rip
    if pc==STOP:break
    assert ENTRY<=pc<=0x259e8,hex(pc);ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw;instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
    def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
    if ins.mnemonic=='push':pending.append(((regs.rsp-4)&NONE,4))
    assert ins.mnemonic!='call','Unexpected external owner in random-range leaf'
    for operand in ins.operands:
     if operand.type!=3:continue
     at=(reg(ins.reg_name(operand.mem.base))+reg(ins.reg_name(operand.mem.index))*operand.mem.scale+operand.mem.disp)&NONE;operands.append((at,operand.size,bool(operand.access&2)))
     if operand.access&2:pending.append((at,operand.size))
    host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
    if os.WIFSTOPPED(status)and os.WSTOPSIG(status)==signal.SIGSEGV:
     assert kind.startswith('fault-')and operands,(kind,hex(pc),status);info=ctypes.create_string_buffer(128);host.trace(0x4202,0,ctypes.addressof(info));address=struct.unpack_from('<Q',info.raw,16)[0];fault=[dict(instruction=pc,address=address,operand_address=operands[0][0],size=operands[0][1],kind='write'if operands[0][2]else'read')];break
    assert os.WIFSTOPPED(status)and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
    for at,width in pending:
     assert STACK-64<=at<STACK+128 or seed_address<=at<seed_address+4,(kind,hex(pc),hex(at));stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
    if ins.mnemonic in ['add','sub','cmp']:defined=NONE&~DEBUG_FLAGS
    elif ins.mnemonic in ['test','and','xor','or']:defined=(NONE&~DEBUG_FLAGS)&~0x10
    elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
    elif ins.mnemonic in ['shr','shl']:defined=(NONE&~DEBUG_FLAGS)&~0x810
    assert depth(host)<=8
   else:raise ValueError('Original random-range instruction limit')
   assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
   if not fault:assert regs.rip==STOP and regs.rsp==STACK+12 and depth(host)==count+1
   final_memory=[[at,host.read(at,width).hex()]for at,_,width in memory]
   rows.append(dict(entry=ENTRY,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,initial_stack=initial_stack,calls=[],stores=stores,fault=fault,final_memory=final_memory,readonly=[READONLY,4096],trace_base=ENTRY,trace_offsets=b''.join(struct.pack('<H',pc-ENTRY)for pc in trace).hex(),expected=state(host,regs,depth(host)),flags_mask=defined))
 data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_RANDOM_RANGE_REFERENCE_V1',original_sha256=pin,scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode();compressed=gzip.compress(data,mtime=0);assert len(data)<=8*1024*1024 and len(compressed)<=1024*1024;args.fixture.write_bytes(compressed);report=dict(passed=True,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),instruction_map=instructions,read_stops=sum(bool(r['fault'])and r['fault'][0]['kind']=='read'for r in rows),write_stops=sum(bool(r['fault'])and r['fault'][0]['kind']=='write'for r in rows),alias_cases=sum(r['kind'].startswith('alias-')for r in rows),maximum_input_x87_depth=max(len(r['fp']['stack'])for r in rows),maximum_output_x87_depth=max(len(r['expected']['fp']['stack'])for r in rows),x87_depth_from_physical_tags=True,controlled_lower_calls=0,supplied_game_services=[],instruction_patches=[],backend='Physical Linux i386 PTRACE_SINGLESTEP',original_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cpu_owner_sha256=hashlib.sha256((Path(__file__).parent/'original_audio_cpu.py').read_bytes()).hexdigest(),raw_bytes=len(data),raw_sha256=hashlib.sha256(data).hexdigest(),fixture_bytes=len(compressed),fixture_sha256=hashlib.sha256(compressed).hexdigest(),scratch_owner=str(scratch),physical_xbox_proof=False,full_user_goal_complete=False);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='instruction_map'}))
if __name__=='__main__':main()
