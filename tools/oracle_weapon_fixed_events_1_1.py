#!/usr/bin/env python3
"""Unchanged TU1.1 fixed weapon-effect sequence and particle camera admission.

Literal effect metadata; declared retained effect/pool and camera/game inputs.
Part playback, particle advance, attached update and cycle are void boundaries;
retirement returns explicit success solely to observe scheduling termination.
Camera controls stop each admitted system at an explicit failed allocation.
No particle birth, effect allocation, original world or GPU proof is inferred.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Trace,ROOT,PIN,CACHE_PIN,USERS
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDI,UC_X86_REG_EIP,UC_X86_REG_ESP
BASE=0x9000000
class EventTrace(Trace):
 def __init__(self):
  self.observe=False;self.camera_probe=False;super().__init__();self.g.mem_map(BASE,0x10000)
  self.w(0x4f1f0c,BASE);self.w(BASE+0x44,BASE+0x1000);self.w(BASE+0x38,1);self.w(BASE+0x24,400)
  self.effect=BASE+0x1000;self.calls=[];self.executed=set()
 def hook(self,g,a,n,user):
  if self.observe:
   sp=g.reg_read(UC_X86_REG_ESP)
   if self.camera_probe and a in (0x156eb0,0x17a920):
    if a==0x17a920:self.admitted.append(g.reg_read(UC_X86_REG_EAX));pop=4;result=0
    else:pop=0;result=self.camera_mode
    g.reg_write(UC_X86_REG_EAX,result);g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
   if not self.camera_probe and a in (0x17bdf0,0x17aa90,0x17ba20,0x17c840,0x179080):
    row=dict(entry=f'{a:08x}',event=self.u(self.effect+0x5c,2))
    if a==0x179080:self.finished=True
    if a==0x17aa90:row.update(before=self.u(sp+8),dt=self.u(sp+12))
    self.calls.append(row);pop={0x17bdf0:4,0x17aa90:12,0x17ba20:0,0x17c840:0,0x179080:0}[a]
    g.reg_write(UC_X86_REG_EAX,1 if a==0x179080 else 0);g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
   i=self.code.at(a)
   if i is not None:
    assert i.bytes==self.image.read(a,i.size)==bytes(g.mem_read(a,i.size));self.executed.add(a)
  super().hook(g,a,n,user)
 def start(self,datum):
  self.g.mem_write(self.effect,bytes(400));self.sh(self.effect,1);self.sh(self.effect+2,0x80);self.w(self.effect+4,datum)
  definition=self.u(self.u(0x4eb114)+(datum&65535)*16+8)
  self.w(self.effect+0x64,self.u(self.u(definition+0x18)+8))
  self.sh(self.effect+0x5e,-1);self.w(self.effect+0xfc,-1);self.finished=False;self.calls=[]
  self.observe=True;self.active='declared_fixed_event_sequence'
def main():
 t=EventTrace();rows=[];fixture=[];case=0
 # Actual Needler firing, sword ready and stock SMG firing.
 for datum in (0xe3b4023f,0xe2fb0186,0xe59e0429):
  assert t.u(t.u(0x4eb114)+(datum&65535)*16+8)!=0
  for dt in (0,0x3d088889,0x3c888889,0x3dcccccd,0x3f800000,0xbf800000):
   t.camera_probe=False;t.start(datum)
   for tick in range(40):
    first=len(t.calls);delta=0 if tick==0 else dt;t.run(0x17ad20,(0x10000,delta))
    row=dict(case=case,tick=tick,datum=f'{datum:08x}',dt=delta,event=t.u(t.effect+0x5c,2),
       elapsed=t.u(t.effect+0x60),target=t.u(t.effect+0x64),active=bool(t.u(t.effect+2,1)&1),finished=t.finished,calls=t.calls[first:])
    rows.append(row);begins=[q['event']for q in row['calls']if q['entry']=='0017bdf0'];slices=[q for q in row['calls']if q['entry']=='0017aa90']
    values=[case,tick,datum,delta,row['event'],row['elapsed'],row['target'],int(row['active']),int(row['finished']),len(begins),*begins,len(slices)]
    for s in slices:values.extend((s['event'],s['before'],s['dt']))
    fixture.append('T '+' '.join(f'{v:08x}'for v in values))
    if t.finished:break
   case+=1
 cameras=[]
 for event in (0,1):
  for local_users in (1,2,4):
   for mode in (0,1,2):
    for underwater in (False,True):
     t.start(0xe3b4023f);t.camera_probe=True;t.camera_mode=mode;t.admitted=[]
     t.w(t.effect+0x58,0);t.sh(t.effect+0x5c,event);t.sh(t.effect+2,0xa0 if underwater else 0x80);t.sh(USERS+8,local_users)
     t.run(0x17aa90,(0x10000,0,0x3d088889))
     cameras.append(dict(event=event,local_users=local_users,camera_mode=mode,underwater=underwater,admitted=t.admitted))
     fixture.append('C '+' '.join(f'{v:08x}'for v in [event,local_users,mode,int(underwater),len(t.admitted),*t.admitted]))
 out=ROOT/'analysis/multiplayer/integration/weapon-fixed-events-20261012.json'
 result=dict(scope=__doc__,xbe_sha256=PIN,cache_sha256=CACHE_PIN,cases=case,timeline=rows,camera_controls=cameras,
  unchanged_executed_instructions=len(t.executed),executed_instruction_bytes_sha256=hashlib.sha256(b''.join(t.image.read(a,t.code.at(a).size)for a in sorted(t.executed))).hexdigest(),
  instruction_patches=[],new_successful_particle_allocations=[],tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
  explicit_boundaries=['17BDF0 part playback,17AA90 particle advance,17BA20 attached update,17C840 system cycle are void scheduling boundaries.',
   '179080 retirement returns declared success; no original storage deletion is inferred.',
   'Camera probe156EB0 returns each declared mode;17A920 allocation returns failure after recording actual admitted index.'])
 out.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
 (ROOT/'tests/fixtures/weapon_fixed_events_original.tsv').write_text('# Unchanged TU1.1 fixed event timeline and camera admission; explicit service boundaries in provenance.\n'+'\n'.join(fixture)+'\n')
 print(json.dumps(dict(report=str(out),cases=case,rows=len(rows),camera_controls=len(cameras),instructions=len(t.executed))),flush=True)
if __name__=='__main__':main()
