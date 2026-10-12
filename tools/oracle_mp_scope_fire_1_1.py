#!/usr/bin/env python3
"""Scoped stock SMG input through unchanged equipped constructor/tick owners.

Native incoming selector/button/clock inputs are declared. The real weapon
allocation/own manager and source unit constructor are inherited unchanged.
Successful projectile allocation is not supplied: the prior explicit failed
allocation/clear muzzle boundary is retained. Local view remains NONE.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_smg_constructor_equipped_1_1 import ConstructorTrace,ROOT,UNIT,UI,TIME,REQUEST,MANAGER_OFFSET,STOP
from oracle_spartan_equipped_animation_1_1 import CONTROL_INPUT
from oracle_smg_firing_1_1 import _trace,PLAYER_DATA

ENTRIES=(0xc9a40,0xc9920,0xc9be0,0x150f00,0xe88b0,0xe8740,0xe7e60,0x1021d0)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def h(v):return f'{v&0xffffffff:08x}'
class Trace(ConstructorTrace):
 def __init__(self):
  self.scope_ready=False;self.scope_calls=[];self.scope_stores=[]
  super().__init__();self.scope_ready=True
 def scope(self):
  component=UNIT+struct.unpack('<h',self.g.mem_read(UNIT+0x346,2))[0]
  return dict(current=struct.unpack('<b',self.g.mem_read(UNIT+0x240,1))[0],requested=struct.unpack('<b',self.g.mem_read(UNIT+0x241,1))[0],
   motion=h(self.u(UNIT+0x26c)),action4=h(self.u(component+4)),action8=h(self.u(component+8)),
   incoming_command_selector=struct.unpack('<h',self.g.mem_read(CONTROL_INPUT+0x24,2))[0],
   loaded=self.weapon_snapshot(self.initial_datum)['loaded'],magazine_mode=self.weapon_snapshot(self.initial_datum)['magazine_mode'],
   weapon_input=self.weapon_snapshot(self.initial_datum)['input_flags'],unit_body_800_hex=bytes(self.g.mem_read(UNIT,0x800)).hex(),
   source_action44_hex=bytes(self.g.mem_read(component,0x44)).hex(),weapon=self.weapon_snapshot(self.initial_datum))
 def hook(self,g,a,size,user):
  if self.scope_ready and a in ENTRIES:
   sp=g.reg_read(UC_X86_REG_ESP);self.scope_calls.append(dict(phase=self.active,entry=h(a),caller=h(self.u(sp)),registers=self.registers(),stack_arguments=[h(self.u(sp+4+n*4))for n in range(3)],before=self.scope()))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,a,n,value,user):
  if self.scope_ready and any(p<=a and a+n<=p+size for p,size in ((UNIT+0x240,2),(UNIT+0x26c,4),(CONTROL_INPUT,0x5c))):
   self.scope_stores.append(dict(phase=self.active,instruction=h(g.reg_read(UC_X86_REG_EIP)),address=h(a),size=n,before_hex=bytes(g.mem_read(a,n)).hex(),after_hex=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
  super().mem_write(g,access,a,n,value,user)
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();t=Trace()
 dependency=sha(ROOT/'tools/oracle_smg_constructor_equipped_1_1.py')
 t.services[0xb8a30]=('supplied failed projectile allocation after successful original starting loadout',-1,4)
 t.services[0x1e20c0]=('supplied clear muzzle segment for bounded allocated-SMG continuation',0,16)
 rows=[]
 for frame in range(32):
  held=1<=frame<=5;reload=frame==15;selector=0 if frame<=15 else -1
  t.tick=frame;t.w(TIME+8,frame);t.active=f'frame_{frame}_declared_input'
  t.run(0x14e780,regs=((UC_X86_REG_EDX,CONTROL_INPUT),))
  t.w(CONTROL_INPUT,(0x10000 if held else 0)|(0x40000 if frame==1 else 0))
  t.f(CONTROL_INPUT+0x14,1.0 if held else 0.0)
  t.g.mem_write(CONTROL_INPUT+0x1e,bytes(t.g.mem_read(UNIT+0x214,4)))
  t.sh(CONTROL_INPUT+0x1c,1 if reload else 0);t.sh(CONTROL_INPUT+0x24,selector)
  incoming=bytes(t.g.mem_read(CONTROL_INPUT,0x5c)).hex();stages=[]
  for name,e,args,regs in [('pressed_edges',0x150dd0,(),((UC_X86_REG_EAX,0),(unicorn.x86_const.UC_X86_REG_ECX,CONTROL_INPUT))),
   ('player_actions',0x150f00,(0,UI,CONTROL_INPUT),()),('unit_input',0x151c90,(UI,CONTROL_INPUT),((UC_X86_REG_EAX,0),)),
   ('blend',0x1d0ed0,(),((UC_X86_REG_ESI,UNIT+MANAGER_OFFSET),)),('unit_wrapper',0xc7e90,(UI,),((UC_X86_REG_EAX,0xc7e90),)),
   ('weapon',0x1021d0,(t.initial_datum,),()),('actions',0xe7e60,(UI,),()),('animation',0x112e10,(UI,REQUEST),()),('pose',0xbdf80,(UI,),()),('postpass',0x1019f0,(),())]:
   t.active=f'frame_{frame}_{name}';before=t.scope();call=len(t.scope_calls);stores=len(t.scope_stores)
   t.pose_active=name=='pose';t.run(e,args,regs);t.pose_active=False
   stages.append(dict(name=name,entry=h(e),before=before,after=t.scope(),calls=t.scope_calls[call:],ordered_stores=t.scope_stores[stores:]))
  rows.append(dict(frame=frame,declared_input=dict(held_fire=held,manual_reload=reload,selector=selector,tick=frame,raw5c_hex=incoming),stages=stages))
 a.output.mkdir(parents=True,exist_ok=True);(a.output/'instructions').mkdir(exist_ok=True)
 for e in ENTRIES:
  f=_trace(t.code,e,t.starts|set(ENTRIES));ins=[t.code.at(p)for p in sorted(f.code)]
  (a.output/'instructions'/f'{e:08x}.asm').write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
 report=dict(tool_sha256=sha(__file__),constructor_dependency_sha256=dependency,original_xbe_sha256=t.image and sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),
  frames=rows,calls=t.scope_calls,ordered_stores=t.scope_stores,inherited_services=[dict(entry=h(p),contract=v)for p,v in t.services.items()],
  local_view=struct.unpack('<h',t.g.mem_read(PLAYER_DATA+0x28,2))[0],
  boundaries=['Full original SMG allocation/equip and own-manager metadata are inherited unchanged. The unit/player are the older declared preallocated equipped context; this is separate from the fully allocated-world positive jump/crouch fixture.',
   'Incoming selector/fire/reload fields and frame tick are explicit native adapter stimuli. After the reload stimulus selectors are explicitly-1, not claimed as an executed source local-view producer. Actual local-view binding remainsNONE.',
   'Failed projectile allocation and clear muzzle segment are inherited explicit bounded services. No voice/effect/platform/local-owner success is added. No complete world or physical presentation claim.'])
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(report_sha256=sha(a.output/'report.json'))),flush=True)
import unicorn
if __name__=='__main__':main()
