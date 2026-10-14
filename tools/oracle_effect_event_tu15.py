#!/usr/bin/env python3
"""Replay the recovered clock cases against unchanged English TU1.5 bodies.

Parts, particle updates and retirement are explicit observed fixture services;
the complete select/next/advance and random-range instructions execute.
"""
import hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
import argparse
a=argparse.ArgumentParser();a.add_argument('--xbe',type=Path,required=True);a.add_argument('--fixture',type=Path,required=True);args=a.parse_args()
ROOT=Path.cwd()
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
x=Xbe(args.xbe);expected='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd';assert hashlib.sha256(x.data).hexdigest()==expected
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
for sec in x.sections:u.mem_write(sec.va,x.section_bytes(sec))
u.mem_map(0x2100000,0x300000);u.mem_map(0x3000000,4096)
body,header,table,definition,events,streams,stack,stop=0x2101000,0x2100000,0x2200000,0x2210000,0x2220000,0x2230000,0x23fff00,0x3000000
handle=0x10000
w=lambda a,v:u.mem_write(a,struct.pack('<I',v&0xffffffff))
h=lambda a,v:u.mem_write(a,struct.pack('<H',v&65535))
rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def state():return [struct.unpack('<H',u.mem_read(body+2,2))[0],*struct.unpack('<hh',u.mem_read(body+0x5c,4)),*struct.unpack('<III',u.mem_read(body+0x60,12))]
current=None;trace=[]
def service(uc,pc,size,data):
 if pc not in [0x17dc60,0x17d210,0x17beb0,0x17ce40,0x17a4a0]:return
 sp=uc.reg_read(UC_X86_REG_ESP);args=[];pop=0;kind={0x17dc60:'sync',0x17d210:'begin',0x17beb0:'particles',0x17ce40:'after',0x17a4a0:'retire'}[pc]
 if pc==0x17d210:assert rd(sp+4)==body;pop=4
 if pc==0x17beb0:assert rd(sp+4)==handle;args=[rd(sp+8),rd(sp+12)];pop=12
 if pc==0x17dc60:assert uc.reg_read(UC_X86_REG_EDX)==body
 trace.append(dict(kind=kind,state=state(),args=args))
 if pc==0x17a4a0:
  assert uc.reg_read(UC_X86_REG_EAX)==handle
  h(body+2,state()[0]|current['retire_flags']);uc.reg_write(UC_X86_REG_EAX,int(current['retire_result']))
 uc.reg_write(UC_X86_REG_EIP,rd(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
u.hook_add(UC_HOOK_CODE,service)
def execute(c):
 global current,trace
 current=c;trace=[]
 u.mem_write(header,bytes(0x100));w(header+0x24,400);w(header+0x38,1);w(header+0x44,body);w(0x4f720c,header)
 w(0x4f0414,table);w(table+8,definition);w(definition,c['effect_flags']);h(definition+4,c['loop']);w(definition+0x14,len(c['events']));w(definition+0x18,events)
 u.mem_write(body,bytes(400));h(body,1);w(body+4,0);s=c['state'];h(body+2,s[0]);h(body+0x5c,s[1]);h(body+0x5e,s[2]);u.mem_write(body+0x60,struct.pack('<III',*s[3:]))
 for n,d in enumerate(c['events']):u.mem_write(events+n*0x38,bytes(0x38));u.mem_write(events+n*0x38+4,struct.pack('<5I',*d))
 w(0x4f3cd8,streams);w(streams,c['random'] if c['effect_flags']&4 else 0x76543210);w(streams+4,0x76543210 if c['effect_flags']&4 else c['random'])
 for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
 u.reg_write(UC_X86_REG_FPCW,0x37f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
 if c['action']=='select':pc=0x17b130;args=[];u.reg_write(UC_X86_REG_EDX,handle);u.reg_write(UC_X86_REG_EDI,c['index']&0xffffffff)
 elif c['action']=='next':pc=0x17c020;args=[body]
 else:pc=0x17c140;args=[handle,c['seconds']]
 u.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args));u.reg_write(UC_X86_REG_ESP,stack);u.emu_start(pc,stop,count=50000);assert u.reg_read(UC_X86_REG_EIP)==stop
 result=False;next=-999
 if c['action']=='select':result=0<=c['index']<len(c['events'])
 elif c['action']=='next':next=struct.unpack('<i',struct.pack('<I',u.reg_read(UC_X86_REG_EAX)))[0]
 else:result=bool(u.reg_read(UC_X86_REG_EAX)&255)
 unused=rd(streams+4 if c['effect_flags']&4 else streams);assert unused==0x76543210
 return dict(state=state(),random=rd(streams if c['effect_flags']&4 else streams+4),result=result,next=next,trace=trace)
cases=[json.loads(line) for line in (ROOT/'analysis/map-world-features/effect-events-20261012/cases.jsonl').read_text().splitlines()]
result=[]
for n,case in enumerate(cases):
 original=execute(case['input'])
 if original!=case['original']:
  raise RuntimeError('TU1.5 differs from retained kernel at case '+str(n))
 result.append(dict(input=case['input'],original=original))
args.fixture.parent.mkdir(parents=True,exist_ok=True)
args.fixture.write_text(''.join(json.dumps(case)+'\n' for case in result))
print('PASS',len(result),'whole-original TU1.5 event states, RNG words and ordered downstream boundaries')
