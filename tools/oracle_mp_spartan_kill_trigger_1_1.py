#!/usr/bin/env python3
"""Original kill-volume admission on genuine allocated moving Spartan storage.

A separately declared diagnostic point is derived from actual authored volume
coordinates and delivered only through the complete original position setter.
This is not natural falling/time progression. No unit root, velocity, death,
contact, animation, world/PVS, clock or result field is assigned by the observer.
"""
import argparse,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_spartan_death_physical_1_1 as prior
import oracle_mp_spartan_ragdoll_constructor_1_1 as ctor
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_smg_firing_1_1 import ROOT,STOP,PIN,CACHE_PIN
from functions import _trace
ENTRIES={0xdefc0,0x1efef0,0x11db80,0x11da40,0x11d950,0xd7c60,0xd7cd0,
 0xd9180,0xd7e00,0xdac40,0xcb410,0xcbdc0,0xc5190,0xeff50,0xb8490,0xb8320,
 0xe6050,0xe6dc0,0x1110f0,0x110bf0,0xcac00,0x1c70d0,0xe2520,0x1d9330}

class KillTrace(prior.MovingDeathTrace):
 def physical_snapshot(self,backing=False):
  r=super().physical_snapshot(backing)
  s=self.u(0x4e7920);n=self.u(s+0x230);v=self.u(s+0x234);b=self.u(0x526258)
  r['kill_storage']=dict(scenario=f'{s:08x}',count=n,rows=f'{v:08x}',rows_hex=bytes(self.g.mem_read(v,n*2)).hex(),
   disabled_bits_pointer=f'{b:08x}',disabled_bits_hex=bytes(self.g.mem_read(b,((n+31)//32)*4)).hex())
  if hasattr(self,'spawned_unit'):
   h=self.u(self.u(0x4e78d0)+0x44)+(self.spawned_unit&0xffff)*12;p=self.u(h+8)
   r['unit_literal']=dict(header=f'{h:08x}',header0c_hex=bytes(self.g.mem_read(h,12)).hex(),pointer=f'{p:08x}',raw800_hex=bytes(self.g.mem_read(p,0x800)).hex(),
    root_hex=bytes(self.g.mem_read(p+0x30,12)).hex(),velocity_hex=bytes(self.g.mem_read(p+0x88,12)).hex(),
    requested_current4_hex=bytes(self.g.mem_read(p+0x214,4)).hex(),physical_mode=f'{self.u(p+0x34b,1):02x}',death_state=f'{self.u(p+0x3dc,1):02x}')
  return r
 def hook(self,g,a,size,user):
  super().hook(g,a,size,user)
  if self.death_enabled and self.death_calls and int(self.death_calls[-1]['entry'],16)==a:
   c=self.death_calls[-1];sp=g.reg_read(reg.UC_X86_REG_ESP);packet=None;n=None
   if a in(0xd9180,0xcb410,0xcbdc0,0xc5190,0xeff50):
    packet=self.u(sp+4 if a==0xd9180 else sp+8);n=0x88 if a==0xd9180 else 0x54 if a==0xcb410 else 0x58 if a==0xeff50 else None
   if a==0xdac40:packet=g.reg_read(reg.UC_X86_REG_EDI);n=0x54
   if a==0xd7c60:packet=g.reg_read(reg.UC_X86_REG_EDX);n=0x88
   if a==0x11da40:packet=g.reg_read(reg.UC_X86_REG_EDI);n=12
   if packet and n:c['packet']=dict(pointer=f'{packet:08x}',size=n,entry_hex=bytes(g.mem_read(packet,n)).hex())
   if a in ENTRIES:c['owner_entry']=self.physical_snapshot(False)
 def returns_at(self,a):
  if self.death_enabled:
   for ret,_,c in self.death_pending:
    if ret==a:
     if 'packet'in c:
      p=c['packet'];p['return_hex']=bytes(self.g.mem_read(int(p['pointer'],16),p['size'])).hex()
     if int(c['entry'],16)in ENTRIES:c['owner_return']=self.physical_snapshot(False)
  super().returns_at(a)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 ctor.verify();a.output.parent.mkdir(parents=True,exist_ok=True)
 err=(a.output.parent/'runtime-stderr.txt').open('w');proc=subprocess.Popen(['wine',str(prior.player.CONTEXT_OUT/'generator.exe')],
  stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(prior.player.CONTEXT_OUT/'wine-prefix')))
 data=json.loads(proc.stdout.readline())
 try:
  prior.motion.CommandPoseTrace=KillTrace;t,correlation=prior.motion.prepare(data)
  for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
  t.g.context_restore(t.source_fixture_cpu)
  movement=prior.motion.probe(t,'authored_heading_forward',0x3f800000,0,True)
  old=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/report.json').read_text())
  want=next(c for c in old['cases']if c['case']=='authored_heading_forward')
  for left,right in zip(want['stages'],movement['stages']):
   for k in('before_state','after_state'):
    assert {k:v for k,v in old['states'][left[k]].items()if k!='id'}=={k:v for k,v in t.states[right[k]].items()if k!='id'}
  print(json.dumps(dict(prior_movement_byte_identical=True)),flush=True)
  t.selected_physics_fixup(0xe9f2086d);t.death_enabled=True
  prior.ENTRIES.update(t.starts|ENTRIES)
  for name,e in(('original_effect_storage_constructor',0x1775d0),('original_effect_map_reset',0x177690)):
   assert t.phase(name,e)
  before=t.physical_snapshot(True);s=t.u(0x4e7920);assert t.u(s+0x230)==1
  index=t.u(t.u(s+0x234),2);v=t.u(s+0x10c)+index*0x44
  raw=bytes(t.g.mem_read(v,0x44));assert int.from_bytes(raw[4:6],'little')==0xffff
  origin=struct.unpack_from('<3f',raw,0x24);extent=struct.unpack_from('<3f',raw,0x30)
  forward=struct.unpack_from('<3f',raw,0xc);up=struct.unpack_from('<3f',raw,0x18)
  right=(up[1]*forward[2]-up[2]*forward[1],up[2]*forward[0]-up[0]*forward[2],up[0]*forward[1]-up[1]*forward[0])
  center=tuple(origin[i]+0.5*(forward[i]*extent[0]+right[i]*extent[1]+up[i]*extent[2])for i in range(3))
  point=PLACEMENT+0x1000;diagnostic=struct.pack('<3f',*center);oldpoint=bytes(t.g.mem_read(point,12));t.g.mem_write(point,diagnostic)
  declared=dict(kind='external_diagnostic_authored_volume_interior_point_not_natural_fall',volume_index=index,volume_address=f'{v:08x}',volume44_hex=raw.hex(),
   formula='origin + 0.5*(forward*extent.x + cross(up,forward)*extent.y + up*extent.z)',caller_scratch=f'{point:08x}',before_hex=oldpoint.hex(),supplied_point_hex=diagnostic.hex(),point_values=center)
  stages=[('actual_moving_root_kill_check',0x1efef0,(),((reg.UC_X86_REG_EDI,t.spawned_unit),)),
   ('explicit_authored_interior_point_check',0x11da40,(),((reg.UC_X86_REG_EAX,index),(reg.UC_X86_REG_EDI,point))),
   ('explicit_original_diagnostic_position_delivery',0xb8490,(t.spawned_unit,0,0),((reg.UC_X86_REG_EAX,point),(reg.UC_X86_REG_ECX,0),(reg.UC_X86_REG_EDX,0))),
   ('actual_retained_root_kill_check_after_position_delivery',0x1efef0,(),((reg.UC_X86_REG_EDI,t.spawned_unit),)),
   ('explicit_original_current_biped_motion_owner',0xdefc0,(t.spawned_unit,),())]
  for name,e,args,regs in stages:
   if not t.phase(name,e,args,regs):break
   if name=='explicit_original_diagnostic_position_delivery':
    h=t.u(t.u(0x4e78d0)+0x44)+(t.spawned_unit&0xffff)*12
    if not(t.u(h+2,1)&0x40):break
  final=t.physical_snapshot(True)
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();err.close();assert proc.returncode==0
 prior.write(a.output.parent/'raw-host-context.json',prior.encoded(data));listings=[]
 roots={int(c['entry'],16)for c in t.death_calls}|ENTRIES
 for e in sorted(roots):
  f=_trace(t.code,e,t.starts|roots);rows=[t.code.at(p)for p in sorted(f.code)]
  for r in rows:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
  path=a.output.parent/'instructions'/f'{e:08x}.asm';prior.write(path,''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows).encode())
  listings.append(dict(entry=f'{e:08x}',file=str(path.relative_to(a.output.parent)),sha256=prior.digest(path),rows=len(rows),executed_rows=sum(r.address in t.death_executed for r in rows)))
 rows=[t.code.at(p)for p in sorted(t.death_executed)]
 for r in rows:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
 path=a.output.parent/'instructions'/'executed-kill-trigger.asm';prior.write(path,''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows).encode())
 listings.append(dict(kind='all_new_execution',file=str(path.relative_to(a.output.parent)),sha256=prior.digest(path),rows=len(rows)))
 report=dict(schema=1,oracle_sha256=prior.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,
  prior_pins=ctor.PINS,prior_runtime_correlation=correlation,prior_movement_byte_identical=True,
  initial=before,declared_caller_point=declared,final=final,stages=t.death_stages,calls=t.death_calls,reads=t.death_reads,stores=t.death_stores,events=t.events,
  instructions=listings,instruction_patches=[],new_supplied_guest_owning_fields_or_success=[],boundaries=[
   'One declared diagnostic input point derives from actual authored kill-volume coordinates. Full original B8490 consumes it; unit/physical/world fields are never written by observer.',
   'This is not natural falling or repeated frame/time progression. Inherited tick0/dt and command axes1/0/authored heading remain explicit.',
   'Original owning allocation/physics/control/animation/current node/PVS setup is inherited; supplied services are recorded, never silently successful.',
   'TU1.1 dynamically executes; authenticated1.5 correspondence is separately measured, not full1.5 runtime/Xbox hardware proof.'])
 ctor.verify();prior.write(a.output,prior.encoded(report));print(json.dumps(dict(report=str(a.output),sha256=prior.digest(a.output),calls=len(t.death_calls),stages=len(t.death_stages))),flush=True)
if __name__=='__main__':main()
