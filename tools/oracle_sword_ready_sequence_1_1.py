#!/usr/bin/env python3
"""Unchanged original Sword pickup and 1021D0 ready countdown, plus17AD20.

Pickup inherits the declared equipped resource/service boundaries. The effect
probe independently controls one literal effect record and replaces named void
part/particle services. It proves event scheduling, not their playback or birth.
No native result or copied expected value feeds original instructions.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_sword_pickup_looping_pool_1_1 import PoolTrace,ROOT,PIN,CACHE_PIN
from unicorn.x86_const import UC_X86_REG_XMM0,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from functions import _trace
class ReadyTrace(PoolTrace):
 def hook(self,g,a,n,user):
  if getattr(self,'effect_observe',False) and a in (0x17bdf0,0x17aa90,0x17ba20,0x17c840,0x179080):
   sp=g.reg_read(UC_X86_REG_ESP)
   if a==0x179080:raise RuntimeError('declared retirement boundary')
   row=dict(entry=f'{a:08x}',caller=f'{self.u(sp):08x}')
   pop={0x17bdf0:4,0x17aa90:12,0x17ba20:0,0x17c840:0}[a]
   if a==0x17aa90:row.update(before=f'{self.u(sp+8):08x}',dt=f'{self.u(sp+12):08x}')
   self.effect_calls.append(row)
   g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
  super().hook(g,a,n,user)
def main():
 t=ReadyTrace(2);t.seeded_duration(2,232);t.poll('original_pickup',{2:255},milliseconds=1)
 p=t.target_pointer;countdowns=[]
 for n in range(25):
  countdowns.append([t.u(p+0x12c),t.u(p+0x16c),t.u(p+0x184),t.u(p+0x19e,2)])
  t.active=f'original_weapon_tick_{n+1}';t.run(0x1021d0,(t.target_datum,),((UC_X86_REG_XMM0,0),))
 assert countdowns[0]==[13,0,0,21] and countdowns[20][3]==1 and countdowns[21]==[13,32,0,0]
 base=0xa000000;t.g.mem_map(base,0x10000);t.w(0x4f1f0c,base);t.w(base+0x44,base+0x1000)
 effect=base+0x1000;t.w(effect+4,0xe2fb0186);t.sh(effect+2,0x80);t.sh(effect+0x5c,0);t.f(effect+0x64,.5)
 t.effect_observe=True;t.effect_calls=[];timeline=[];t.active='declared_fixed_effect_event'
 for n in range(33):
  first=len(t.effect_calls);stopped=False
  try:t.run(0x17ad20,(0x10000,0 if n==0 else 0x3d088889))
  except RuntimeError as error:
   assert str(error)=='declared retirement boundary';stopped=True
  timeline.append(dict(tick=n,elapsed_bits=f'{t.u(effect+0x60):08x}',target_bits=f'{t.u(effect+0x64):08x}',active=bool(t.u(effect+2,1)&1),retired=stopped,calls=t.effect_calls[first:]))
  if stopped:break
 cues=[q['tick']for q in timeline if any(c['entry']=='0017bdf0'for c in q['calls'])]
 assert cues==[15],cues
 out=ROOT/'analysis/multiplayer/integration/sword-ready-sequence-20261012.json';out.parent.mkdir(parents=True,exist_ok=True)
 source={}
 for e in (0x1040f0,0x1021d0,0x17ad20):
  r=_trace(t.code,e,t.starts);raw=b''.join(t.image.read(a,t.code.at(a).size)for a in sorted(r.code));source[f'{e:08x}']=dict(instructions=len(r.code),sha256=hashlib.sha256(raw).hexdigest())
 result=dict(scope=__doc__,xbe_sha256=PIN,cache_sha256=CACHE_PIN,countdown=countdowns,effect_timeline=timeline,sound_start_tick=cues[0],instructions=source,instruction_patches=[],new_gameplay_service_results=[],void_effect_boundaries=['17BDF0 part playback','17AA90 particle advance','17BA20 attached part update','17C840 system cycle'],retirement='Stopped at original179080 call; no successful deletion supplied',tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 out.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
 fixture=ROOT/'tests/fixtures/sword_ready_sequence_original.tsv'
 fixture.write_text('# Original1021D0 countdown; original17AD20 fixed event with declared void service boundaries.\n'+''.join('R '+' '.join(f'{v:08x}'for v in row)+'\n'for row in countdowns)+''.join(f'E {q["tick"]:08x} {q["elapsed_bits"]} {q["target_bits"]} {int(q["active"]):08x} {int(q["retired"]):08x} {sum(c["entry"]=="0017bdf0"for c in q["calls"]):08x}\n'for q in timeline))
 print(json.dumps(dict(report=str(out),ready_ticks=countdowns[0][3],blade_on_tick=21,sound_start_tick=cues[0],event_stop_tick=timeline[-1]['tick'])),flush=True)
if __name__=='__main__':main()
