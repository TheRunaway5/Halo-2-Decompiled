#!/usr/bin/env python3
"""Untouched original teleporter control with declared dependency inputs.

Both images execute1A0580/0680/0790 and30BD0/11DDD0. The occupied pool
iterator, root reader, destination-query result and external mutations are
explicit test services. This is not original world physics or live traversal.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_mp_sandbox_update_functions import full_tokens
from oracle_smg_firing_1_1 import _Code,_trace

BASE=0x1000000;ENGINE=BASE;POOL=BASE+0x2000;PLAYERS=BASE+0x3000
OBJECTPOOL=BASE+0x6000;HEADERS=BASE+0x7000;OBJECTS=BASE+0x8000
SCENARIO=BASE+0x10000;FLAGS=BASE+0x11000;GLOBALS=BASE+0x12000
TAGS=BASE+0x13000;TAG=BASE+0x14000;RUNTIME=BASE+0x15000;CONSTANTS=BASE+0x16000
TIME=BASE+0x17000;STACK=BASE+0x90000;STOP=BASE+0xa0000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def vec(v):return [bits(x) for x in v]
def controls():
 rng=random.Random(0x1a0580)
 # Raw inputs deliberately vary boundaries, ties, unavailable/attached units,
 # local views, timer branches and signed/nearest-even conversion. They are
 # explicit control cases, not claims about authored physical world states.
 rows=[]
 for n in range(768):
  flags=[dict(position=vec((0,0,0)),facing=bits(rng.uniform(-6,6)),type=7,channel=0),
         dict(position=vec((8,2,.15)),facing=bits(rng.uniform(-6,6)),type=8,channel=0),
         dict(position=vec((8,2,.15)),facing=bits(rng.uniform(-6,6)),type=7,channel=1),
         dict(position=vec((0,0,0)),facing=bits(rng.uniform(-6,6)),type=8,channel=1)]
  players=[]
  for i in range(4):
   position=rng.choice(((0,0,0),(.5,0,0),(.49999997,0,0),(.25,.25,.25),(-.1,.1,.1),(8,2,.15),(50,0,0)))
   players.append(dict(datum=0xec700000+i,unit=0xe2600000+i if rng.randrange(8) else 0xffffffff,
       flags=rng.choice((1,2,0x81,0x101,0x181,0x185)),local=rng.choice((-1,i)),
       parent=rng.choice((0xffffffff,0xffffffff,0xffffffff,0xe270000a)),
       root=vec(position),forward=vec((rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-1,1)))))
  pairs=[[0,1,rng.choice((0,0,0,1,2,-1)),rng.choice((0,1,120,-1))],
         [2,3,rng.choice((0,0,0,1,2,-1)),rng.choice((0,1,120,-1))]]
  rows.append(dict(flags=flags,players=players,pairs=pairs,count=2,busy=rng.randrange(4),
      rate=rng.choice((30,60)),seconds=bits(rng.choice((0,.05,.15,.25,.5,1))),steps=rng.choice((1,2,4))))
 return rows
def initializer_inputs():
 rng=random.Random(0x1a0340);rows=[]
 for n in range(96):
  flags=[]
  for i in range(n):
   flags.append(dict(position=vec((i,0,0)),facing=0,type=rng.choice((0,7,7,8,8,9)),
       channel=rng.randrange(-1,18),selector12=rng.choice((-1,0,1))))
  if n==95:
   flags=[dict(position=vec((i,0,0)),facing=0,type=t,channel=i,selector12=1)for i in range(16)for t in(7,8,7,8)]
  rows.append(flags)
 return rows
def run_image(path,pin,version,inputs,initializers):
 assert sha(path)==pin;x=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
 g.mem_map(BASE,0x100000)
 mapping=json.loads((ROOT/'analysis/multiplayer/integration/teleporters-lifts-20261010/paired/report.json').read_text())
 mapped={int(r['disc_1_1_entry'],16):int(r['later_entry'],16) for r in mapping['routines'] if 'later_entry'in r}
 mapped.update({0x1a2c60:0x1a8f10,0x11ddd0:0x11ef30,0x1a0340:0x1a65f0})
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 for entry in (0x1a0340,0x11ddd0):
  oc=_Code(old,'.text');nc=_Code(x,'.text')
  oi=[oc.at(p)for p in sorted(_trace(oc,entry,{entry}).code)]
  target=entry if version=='1.1'else mapped[entry]
  ni=[nc.at(p)for p in sorted(_trace(nc,target,{target}).code)]
  assert full_tokens(oi,old)==full_tokens(ni,x)
 def address(old):return old if version=='1.1'else mapped[old]
 def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
 def u(p):return struct.unpack('<I',g.mem_read(p,4))[0]
 def words(p,n):return list(struct.unpack('<'+'I'*n,g.mem_read(p,4*n)))
 def vwrite(p,v):g.mem_write(p,struct.pack('<3I',*v))
 def reg(r):return g.reg_read(r)
 def ret(clean=0,value=None):
  sp=reg(R.UC_X86_REG_ESP);target=u(sp);g.reg_write(R.UC_X86_REG_ESP,sp+4+clean)
  if value is not None:g.reg_write(R.UC_X86_REG_EAX,value)
  g.reg_write(R.UC_X86_REG_EIP,target)
 pool_slot,obj_slot,engine_slot,scenario_slot,globals_slot,tags_slot,time_slot=(
   (0x4f01f4,0x4e78d0,0x4f10b8,0x4e7920,0x4e791c,0x4eb114,0x518444)if version=='1.1'else
   (0x4f54f4,0x4ecbd0,0x4f63b8,0x4ecc20,0x4ecc1c,0x4f0414,0x51df54))
 for p,value in ((pool_slot,POOL),(obj_slot,OBJECTPOOL),(engine_slot,ENGINE),(scenario_slot,SCENARIO),
     (globals_slot,GLOBALS),(tags_slot,TAGS),(time_slot,TIME),(POOL+0x44,PLAYERS),(OBJECTPOOL+0x44,HEADERS),
     (SCENARIO+0x11c,FLAGS),(GLOBALS+0x16c,0),(TAGS+8,TAG),(TAG+0xc,RUNTIME),(RUNTIME+0x534,CONSTANTS)):w(p,value)
 state={};executed=set()
 services={address(a):a for a in (0x1a2c60,0xbacc0,0x1a0400,0x1a24f0,0xba940,0xb8490,0x188ee0,0x1a0280,0xa9880)}
 def hook(_g,a,_n,_user):
  executed.add(a)
  if a not in services:return
  original=services[a];sp=reg(R.UC_X86_REG_ESP)
  if original==0x1a2c60:
   iterator=reg(R.UC_X86_REG_ESI);index=u(iterator+12)+1
   if index>0x7fffffff:index=0
   while index<4 and state['input']['players'][index]['unit']==0xffffffff:index+=1
   w(iterator,PLAYERS+index*0x21c if index<4 else 0);w(iterator+8,0xec700000+index if index<4 else 0xffffffff);w(iterator+12,index)
   ret(value=int(index<4));return
  if original==0xbacc0:
   i=reg(R.UC_X86_REG_EAX)&0xffff;g.mem_write(reg(R.UC_X86_REG_ESI),bytes(g.mem_read(OBJECTS+i*0x800+0x64,12)));ret();return
  if original==0x1a0400:
   pair=(reg(R.UC_X86_REG_EDX)-(ENGINE+0xafc))//16;player=u(sp+4)
   state['events'].append(['query',pair,player]);ret(4,int(bool(state['input']['busy']&(1<<pair))));return
  if original==0x1a24f0:
   packet=reg(R.UC_X86_REG_EAX);assert u(packet+4)==22 and u(packet+8)==u(packet+12)
   state['events'].append(['event',u(packet+8)]);ret();return
  if original==0xba940:
   # The whole original body returns immediately for parentNONE. The
   #controller admits only such units; this is not a forward-vector writer.
   unit=reg(R.UC_X86_REG_EAX);assert u(OBJECTS+(unit&0xffff)*0x800+0x14)==0xffffffff
   state['events'].append(['detach',unit]);ret();return
  if original==0xb8490:
   unit=u(sp+4);position=words(reg(R.UC_X86_REG_EAX),3);heading=words(reg(R.UC_X86_REG_ECX),3);up=words(reg(R.UC_X86_REG_EDX),3)
   assert u(sp+8)==u(sp+12)==0
   state['events'].append(['position',unit,*position,*heading,*up]);vwrite(OBJECTS+(unit&0xffff)*0x800+0x64,position);vwrite(OBJECTS+(unit&0xffff)*0x800+0x70,heading);ret(12);return
  if original==0x188ee0:state['events'].append(['view',reg(R.UC_X86_REG_ECX),*words(reg(R.UC_X86_REG_EAX),3)])
  elif original==0x1a0280:state['events'].append(['blocked-local',reg(R.UC_X86_REG_EAX)])
  elif original==0xa9880:state['events'].append(['blocked-unit',reg(R.UC_X86_REG_ECX)])
  ret()
 g.hook_add(UC_HOOK_CODE,hook);outputs=[];initialized=[]
 def reset_cpu():
  w(STACK,STOP)
  for r,value in ((R.UC_X86_REG_ESP,STACK),(R.UC_X86_REG_FPCW,0x027f),(R.UC_X86_REG_MXCSR,0x1f80),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff)):g.reg_write(r,value)
 for flags in initializers:
  for i,f in enumerate(flags):
   p=FLAGS+32*i;vwrite(p,f['position']);w(p+12,f['facing']);g.mem_write(p+16,struct.pack('<hhh',f['type'],f['selector12'],f['channel']))
  w(SCENARIO+0x118,len(flags));g.mem_write(ENGINE+0xafc,b'\xcd'*0x104)
  reset_cpu();g.emu_start(address(0x1a0340),STOP,count=1000000);assert reg(R.UC_X86_REG_EIP)==STOP
  initialized.append(dict(count=u(ENGINE+0xbfc),pairs=[list(struct.unpack('<4i',g.mem_read(ENGINE+0xafc+i*16,16)))for i in range(16)]))
 for case in inputs:
  state.update(input=case,events=[])
  for i,f in enumerate(case['flags']):
   p=FLAGS+32*i;vwrite(p,f['position']);w(p+12,f['facing']);g.mem_write(p+16,struct.pack('<hhh',f['type'],-1,f['channel']))
  w(SCENARIO+0x118,len(case['flags']));w(ENGINE+0xbfc,case['count'])
  for i,pair in enumerate(case['pairs']):g.mem_write(ENGINE+0xafc+i*16,struct.pack('<4i',*pair))
  for i,p in enumerate(case['players']):
   player=PLAYERS+i*0x21c;obj=OBJECTS+i*0x800
   g.mem_write(player+2,struct.pack('<H',p['flags']));g.mem_write(player+0x28,struct.pack('<h',p['local']));w(player+0x2c,p['unit'])
   w(HEADERS+i*12+8,obj);w(obj+0x14,p['parent']);vwrite(obj+0x64,p['root']);vwrite(obj+0x70,p['forward'])
  g.mem_write(TIME+2,struct.pack('<h',case['rate']));w(CONSTANTS+4,case['seconds'])
  for _ in range(case['steps']):
   reset_cpu()
   g.emu_start(address(0x1a0580),STOP,count=100000);assert reg(R.UC_X86_REG_EIP)==STOP
  outputs.append(dict(pairs=[list(struct.unpack('<4i',g.mem_read(ENGINE+0xafc+i*16,16)))for i in range(case['count'])],
      flags=[struct.unpack('<H',g.mem_read(PLAYERS+i*0x21c+2,2))[0]for i in range(4)],
      roots=[words(OBJECTS+i*0x800+0x64,3)for i in range(4)],forwards=[words(OBJECTS+i*0x800+0x70,3)for i in range(4)],events=state['events']))
 default_up=words(u(0x46fc50 if version=='1.1'else 0x474d10),3)
 return dict(version=version,image_sha256=pin,outputs=outputs,initialized=initialized,default_up=default_up,executed=[f'{a:08x}'for a in sorted(executed)])
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 inputs=controls();initializers=initializer_inputs()
 banks=[run_image(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN,'1.1',inputs,initializers),run_image(NEW_DEFAULT,NEW_PIN,'u5',inputs,initializers)]
 assert banks[0]['outputs']==banks[1]['outputs']
 assert banks[0]['initialized']==banks[1]['initialized']
 assert banks[0]['default_up']==banks[1]['default_up']
 fixture=a.output/'fixture.json';fixture.write_text(json.dumps(dict(schema=1,inputs=inputs,outputs=banks[1]['outputs'],initializer_inputs=initializers,initialized=banks[1]['initialized'],default_up=banks[1]['default_up']),sort_keys=True,separators=(',',':'))+'\n')
 report=dict(tool_sha256=sha(__file__),fixture_sha256=sha(fixture),banks=[{k:v for k,v in b.items()if k not in('outputs','initialized')}for b in banks],cases=len(inputs),initializers=len(initializers),
     boundary=__doc__,native_live_integration=False)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(cases=len(inputs)*2,fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
