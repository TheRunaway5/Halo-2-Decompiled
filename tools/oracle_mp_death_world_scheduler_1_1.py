#!/usr/bin/env python3
"""Capture unchanged1C7660 world scheduling after ordinary death.

The inherited explicit ordinary death stimulus and selected startup remain
declared. No contact, mode, pose, body, ground correction or clock value is
assigned. The original139750 clock increment fragment advances time between
complete original13A380/B7E00/1C7660/B7F50/B8040 phases. Earlier and later
game-tick systems are excluded, so this is not a whole-title scheduler.
"""
import argparse,json,os,subprocess,traceback
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_spartan_ragdoll_constructor_1_1 as ctor
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_smg_firing_1_1 import ROOT,STOP,PIN,CACHE_PIN
p=ctor.prior

class NaturalTrace(ctor.ConstructorTrace):
 def __init__(self,data):
  self.capture_rebuild=False;self.capture_entry=0x1da570;self.rebuild_return=None;self.rebuild_roots=[]
  super().__init__(data)
 def hook(self,g,address,size,user):
  start=self.capture_rebuild and address==self.capture_entry
  if start:
   assert self.rebuild_return is None
   sp=g.reg_read(reg.UC_X86_REG_ESP)
   self.rebuild_return=(self.u(sp),sp+4)
   self.death_enabled=True
  end=self.rebuild_return==(address,g.reg_read(reg.UC_X86_REG_ESP))
  if self.capture_rebuild:self.constructor_observe=self.death_enabled
  super().hook(g,address,size,user)
  if start:self.rebuild_roots.append(self.death_calls[-1]['id'])
  if end:self.rebuild_return=None;self.death_enabled=False

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
 ap.add_argument('--ticks',type=int,default=13);ap.add_argument('--capture-rebuild',action='store_true');ap.add_argument('--capture-admission',action='store_true');a=ap.parse_args();ctor.verify()
 a.output.parent.mkdir(parents=True,exist_ok=True)
 err=(a.output.parent/'runtime-stderr.txt').open('w')
 proc=subprocess.Popen(['wine',str(p.player.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,
  stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(p.player.CONTEXT_OUT/'wine-prefix')))
 data=json.loads(proc.stdout.readline());rows=[];calls=[];failure=None
 try:
  p.motion.CommandPoseTrace=NaturalTrace;t,_=p.motion.prepare(data)
  for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
  t.g.context_restore(t.source_fixture_cpu);p.motion.probe(t,'authored_heading_forward',0x3f800000,0,True)
  t.selected_physics_fixup(0xe9f2086d);t.death_enabled=True
  for name,entry,args in (
   ('original_effect_storage_constructor',0x1775d0,()),('original_effect_map_reset',0x177690,()),
   ('original_current_damage_owner',0xd7cd0,(t.spawned_unit,PLACEMENT+0x800)),
   ('explicit_original_death_request',0xd7e00,(t.spawned_unit,PLACEMENT+0x800,1,0)),
   ('explicit_original_ordinary_death_cleanup',0xcbdc0,(t.spawned_unit,0xffffffff,0,0,0)),
   ('original_after_death_object_prephysics',0xb7e00,()),('original_after_death_world_physics',0x1c7660,()),
   ('original_after_death_object_postphysics',0xb7f50,()),('original_after_death_object_newflags',0xb8040,()),
  ):assert t.phase(name,entry,args)
  old=json.loads((ctor.BASE/'report.json').read_text())
  for k,v in (('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores)):assert old[k]==v,k
  t.death_enabled=False;t.constructor_observe=False
  offsets={k:len(v)for k,v in (('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores))}
  p.ENTRIES.update(t.starts);p.ENTRIES.add(0x1c7660);t.capture_rebuild=True;t.capture_entry=0x1c7660;t.constructor_observe=True;t.death_executed=set()
  if a.capture_admission:
   t.capture_entry=0xe2520;t.constructor_observe=True;t.death_executed=set()
  unit=t.u(t.u(t.u(0x4e78d0)+0x44)+(t.spawned_unit&0xffff)*12+8)
  def observe(g,address,size,user):
   if address in {0xe2520,0xe2ce0,0x1da570,0x1d9330,0x1f0e90,0x1f08e0,0x1d7310}:
    calls.append(dict(tick=t.u(t.u(0x518444)+8),entry=f'{address:08x}',mode=t.u(unit+0x34b,1),contact=bytes(g.mem_read(unit+0x3f0,24)).hex()))
  t.g.hook_add(unicorn.UC_HOOK_CODE,observe)
  def snapshot(tick):
   component=t.u(t.u(0x526248)+0x44)+(t.u(unit+0xb4)&0xffff)*0xa0
   return dict(iteration=tick,clock=t.u(t.u(0x518444)+8),mode=t.u(unit+0x34b,1),
    state=t.u(unit+0x3dc,1),contact=bytes(t.g.mem_read(unit+0x3f0,24)).hex(),
    body_count=t.u(component+0x74),constraint_count=t.u(component+0x80),
    unit_hex=bytes(t.g.mem_read(unit,5750)).hex(),component_hex=bytes(t.g.mem_read(component,0xa0)).hex())
  rows.append(snapshot(-1))
  for tick in range(a.ticks):
   # Exact original game-tick clock owner, not a fixture clock assignment.
   assert t.image.read(0x139863,8)==bytes.fromhex('a144845100ff4008')
   t.g.emu_start(0x139863,0x13986b,count=2)
   for phase,entry in (('active_masks',0x13a380),('object_prephysics',0xb7e00),('world_physics',0x1c7660),('object_postphysics',0xb7f50),('object_newflags',0xb8040)):
    t.active=f'tick_{tick}_{phase}';t.new_invalid=None
    try:
     t.run(entry)
     # emu_start stops before STOP is hooked. Close the real direct root
     # return explicitly, as with phase(); no runtime state is changed.
     if t.rebuild_return is not None:
      assert t.rebuild_return==(t.g.reg_read(reg.UC_X86_REG_EIP),t.g.reg_read(reg.UC_X86_REG_ESP))
      t.returns_at(STOP)
      t.rebuild_return=None;t.death_enabled=False;t.constructor_observe=False
    except Exception as e:
     failure=dict(iteration=tick,phase=phase,eip=f'{t.g.reg_read(reg.UC_X86_REG_EIP):08x}',error=repr(e),traceback=traceback.format_exc(),invalid=t.new_invalid)
     print(json.dumps(failure),flush=True)
     break
   rows.append(snapshot(tick))
   print(json.dumps({k:rows[-1][k] for k in ('iteration','clock','mode','body_count','constraint_count','contact')}),flush=True)
   if failure:break
  assert failure is None
  old=json.loads((ROOT/'analysis/multiplayer/integration/biped-postphysics-phase-20261011/measured/report.json').read_text())
  assert rows==old['frames'], 'unchanged natural death world'
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();err.close()
  assert proc.returncode==0
 listings=[]
 if t.capture_entry==0x1c7660:
  from oracle_smg_firing_1_1 import _trace
  executed=set(t.death_executed);entries={int(q['entry'],16)for q in t.death_calls[offsets['calls']:]}
  for entry in sorted(entries):
   f=_trace(t.code,entry,t.starts|entries);instructions=[t.code.at(p)for p in sorted(f.code)]
   for r in instructions:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
   path=a.output.parent/'instructions'/f'{entry:08x}.asm';path.parent.mkdir(parents=True,exist_ok=True)
   path.write_text(''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in instructions))
   listings.append(dict(entry=f'{entry:08x}',file=str(path.relative_to(a.output.parent)),sha256=p.digest(path),rows=len(instructions)))
  instructions=[t.code.at(q)for q in sorted(executed)]
  for r in instructions:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
  path=a.output.parent/'instructions'/'executed-admission.asm'
  path.write_text(''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in instructions))
  listings.append(dict(kind='complete_new_execution',file=str(path.relative_to(a.output.parent)),sha256=p.digest(path),rows=len(instructions)))
 a.output.write_text(json.dumps(dict(tool_sha256=p.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,
  frozen_prior_byte_identical=True,boundaries=__doc__,object=f'{t.spawned_unit:08x}',frames=rows,calls=calls,failure=failure,
  capture_entry=f'{t.capture_entry:08x}',math=t.constructor_math,instructions=listings,
  rebuild_roots=t.rebuild_roots,rebuild_calls=t.death_calls[offsets['calls']:],
  rebuild_reads=t.death_reads[offsets['reads']:],rebuild_stores=t.death_stores[offsets['stores']:],
  all_services=t.events,instruction_patches=[],new_supplied_guest_fields_or_results=[]),indent=2)+'\n')
 print(json.dumps(dict(frames=len(rows),failure=failure,report=str(a.output))))
if __name__=='__main__':main()
