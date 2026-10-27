#!/usr/bin/env python3
"""Execute unchanged TU1.5 gameplay fade projection and render-prefix controls.

Run with tools/run_test.py. The15EBA0 consumed tail starts after unrelated
engine work; its real15B470/160570 eligibility bodies execute without hooks.
15C2A0 runs from its actual entry through its real iterator, stopping BEFORE
GPU setup or the separate scoreboard branch. No draw, full frame, loading,
network presentation or original-hardware pixel equivalence is claimed.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_ESP,
 UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR)
from oracle_game_variants import Source,STACK,STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ENGINE,GAME,TIME,LOCALS,POOL,ROWS,BITS=0x1100000,0x1102000,0x1104000,0x1105000,0x1106000,0x1107000,0x110a000

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report-output',type=Path,required=True)
 parser.add_argument('--fixture',type=Path);args=parser.parse_args()
 Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 source=Source(dict(path=str(NEW_DEFAULT),pin=NEW_PIN,globals=0x4ecc1c));u=source.g
 u.mem_map(0x1100000,0x20000)
 def word(at,value,width=4):u.mem_write(at,int(value&((1<<(8*width))-1)).to_bytes(width,'little'))
 def read(at,width=4):return int.from_bytes(u.mem_read(at,width),'little')
 def signed(at,width):return int.from_bytes(u.mem_read(at,width),'little',signed=True)
 def state():return {'counter':signed(ENGINE+0xfa,2),'opacity':[read(ENGINE+0xe4+4*i) for i in range(4)],
  'mask':read(ENGINE+0xf4,1),'frames':list(u.mem_read(ENGINE+0xf5,4))}
 def setup(q,before):
  u.mem_write(ENGINE,bytes(0x20000));u.mem_write(0x2000000,bytes(0x10000))
  for at,value in [(0x4f63b8,ENGINE),(0x4f3218,GAME),(0x51df54,TIME),(0x4f54f0,LOCALS),(0x4f54f4,POOL)]:word(at,value)
  word(ENGINE+0xc14,2);word(0x56dfa8,read(0x4822a0) if q.get('engine',True) else 0)
  word(ENGINE+0x6c,q.get('phase',1),2);word(ENGINE+0xc04,q.get('pending',1));word(ENGINE+0x70,q.get('start',0))
  word(ENGINE+0xfa,before.get('counter',0),2);word(ENGINE+0xf4,before.get('mask',0),1)
  u.mem_write(ENGINE+0xf5,bytes(before.get('frames',[0]*4)))
  for i,value in enumerate(before.get('opacity',[0]*4)):word(ENGINE+0xe4+4*i,value)
  word(GAME+0xc,q.get('mode',1),1);word(GAME+0x1b8,q.get('limit',0));word(GAME+0x1bc,7)
  word(TIME+2,q.get('rate',30),2);word(TIME+4,struct.unpack('<I',struct.pack('<f',1/q.get('rate',30)))[0]);word(TIME+8,q.get('tick',100))
  for at,value in [(POOL+0x20,16),(POOL+0x24,0x21c),(POOL+0x38,4),(POOL+0x44,ROWS),(POOL+0x48,BITS)]:word(at,value)
  mask=0
  for view in range(4):
   p=q.get('users',{}).get(str(view));player=0x10000+view if p is not None and p.get('present',True) else 0xffffffff
   word(LOCALS+0xc+view*4,player)
   if player!=0xffffffff:
    mask|=1<<view;row=ROWS+view*0x21c;word(row,1,2);word(row+2,p.get('flags',1),2)
    word(row+0xc0,p.get('team',0),1);word(row+0x2c,0x20000+view if p.get('unit',True) else 0xffffffff)
    word(row+0x170,p.get('respawn',0));word(row+0x1ac,p.get('lives',1),2)
  word(BITS,mask);u.reg_write(UC_X86_REG_FPCW,q.get('cw',0x27f));u.reg_write(UC_X86_REG_MXCSR,q.get('mxcsr',0x1f80))
 def tail(q,before):
  setup(q,before);word(STACK,STOP);u.reg_write(UC_X86_REG_ESP,STACK-44);u.reg_write(UC_X86_REG_EBP,1)
  source.executed.clear();u.emu_start(0x15ed59,STOP,count=200000)
  assert u.reg_read(UC_X86_REG_EIP)==STOP
  return {'kind':'tick','input':q,'before':before,'after':state()},set(source.executed)
 current_stop=[]
 def stop_at_draw(machine,address,size,_):
  if address in (0x36570,0x15c448):current_stop.append(address);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,stop_at_draw)
 def render(q,before):
  setup(q,before);word(0x4c66f8,q.get('user',0));word(0x4f2d78,q.get('pending_geometry',0))
  word(STACK,STOP);u.reg_write(UC_X86_REG_ESP,STACK);source.executed.clear();current_stop.clear()
  u.emu_start(0x15c2a0,STOP,count=200000);pc=u.reg_read(UC_X86_REG_EIP)
  assert pc in (STOP,0x36570,0x15c448),hex(pc)
  return {'kind':'render-prefix','input':q,'before':before,'after':state(),
   'overlay':pc==0x36570,'stop':hex(pc)},set(source.executed)
 cases=[];executed=set()
 before={'counter':0,'opacity':[0x3f800000,0x3f000000,0,0x3f800000],'mask':0,'frames':[0,1,254,255]}
 for phase,pending,mode,counter,tick in itertools.product((1,2,3),(1,2,3),(1,4),(0,119,120,121,32767),(45,46,300)):
  q={'phase':phase,'pending':pending,'mode':mode,'tick':tick,'users':{'0':{},'2':{'unit':False,'flags':1,'respawn':0,'lives':1}}}
  r,visited=tail(q,before|{'counter':counter});cases.append(r);executed|=visited
 for flags,unit,respawn,lives,limit,team,mask in itertools.product((0,1,2,0x4001),(False,True),(0,1),(-1,0,1),(0,3),(-1,0),(0,1)):
  q={'phase':1,'pending':1,'tick':300,'limit':limit,'users':{'0':{'flags':flags,'unit':unit,'respawn':respawn,'lives':lives,'team':team}}}
  r,visited=tail(q,before|{'mask':mask});cases.append(r);executed|=visited
 for user,pending_geometry,frames,alpha in itertools.product((0,2,0xffffffff),(-1,0,1),(0,89,90,255),(0,0x3e800000,0x3f800000)):
  users={} if user==0xffffffff else {str(user):{'unit':True}}
  r,visited=render({'user':user,'pending_geometry':pending_geometry,'users':users},
   {'counter':0,'opacity':[alpha]*4,'mask':15,'frames':[frames]*4});cases.append(r);executed|=visited
 # A naturally repeated source tail, not a countdown-to-alpha approximation.
 timelines={}
 for label,phase in [('opening',1),('ending',2)]:
  values=[];s={'counter':0,'opacity':[0x3f800000 if phase==1 else 0]*4,'mask':0,'frames':[0]*4}
  for tick in range(1,212):
   r,visited=tail({'phase':phase,'pending':phase,'tick':tick,'users':{'0':{}}},s);executed|=visited;s=r['after']
   if tick in (1,44,45,46,54,55,119,120,121,149,150,151,209,210,211):values.append({'tick':tick,'counter':s['counter'],'opacity_bits':s['opacity'][0]})
  timelines[label]=values
 report={'schema':'H2_TU15_GAMEPLAY_FADE_ORIGINAL_V1','source_sha256':NEW_PIN,'scratch':str(Path(os.environ['HALO2_TEST_SCRATCH']).resolve()),'cases':len(cases),
  'tick_cases':sum(r['kind']=='tick' for r in cases),'render_prefix_cases':sum(r['kind']=='render-prefix' for r in cases),
  'entries':['15EBA0 consumed tail15ED59..15EF94','15C2A0 prefix to36570 or15C448'],
  'actual_eligibility_calls':{'15B470':0x15b470 in executed,'160570':0x160570 in executed},
  'callee_stubs':False,'instruction_mutations':False,'gpu_executed':False,'scoreboard_executed':False,
  'scope':'Explicit source caller projections only; engine work before15ED59 and GPU/full scoreboard/loading are excluded. Actual original eligibility bodies execute. Four-float overlay color, not Vista ARGB packing.',
  'timelines':timelines,'fixture_sha256':None,'executed_instructions':len(executed)}
 if args.fixture:
  value=json.dumps({'schema':report['schema'],'source_sha256':NEW_PIN,'cases':cases},separators=(',',':')).encode()
  assert len(value)<=1024*1024;args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(value+b'\n');report['fixture_sha256']=hashlib.sha256(value+b'\n').hexdigest()
 args.report_output.parent.mkdir(parents=True,exist_ok=True);args.report_output.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
if __name__=='__main__':main()
