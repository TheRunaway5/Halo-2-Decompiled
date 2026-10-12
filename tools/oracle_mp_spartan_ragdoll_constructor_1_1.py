#!/usr/bin/env python3
"""Separate explicit original ragdoll constructor root on genuine dead Spartan.

Reproduce the complete frozen moving-death experiment before source remove /
dispose / grouping. This is callee-ABI construction coverage, not natural +34B
admission. No field, contact, node, world, constraint or result is supplied.
"""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_spartan_death_physical_1_1 as prior
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_smg_firing_1_1 import ROOT, STOP, PIN, CACHE_PIN
from functions import _trace

BASE=ROOT/'analysis/multiplayer/integration/spartan-death-physical-20261010/moving-effect-fixed'
PINS={
 'tools/oracle_mp_spartan_death_physical_1_1.py':'15bb1fb1154b408d6b5b3a05d02bb8cd0b5473d1d410cdedf543f5c3ec266ddf',
 'tools/audit_mp_spartan_death_physical_1_1.py':'d0d00c7e4744d3653b4bbf2c8920ad6a1d2227c79c59bcc3c2fa70eb404e49d5',
 str((BASE/'report.json').relative_to(ROOT)):'a409f68196a35a4a81dd282c37da397874bac391ba557c32153e912bd0ed9526',
}


def verify():
 for p,h in PINS.items():assert prior.digest(ROOT/p)==h,p


class ConstructorTrace(prior.MovingDeathTrace):
 def __init__(self,data):
  self.constructor_observe=False
  self.constructor_math=[]
  self.constructor_math_pending=None
  super().__init__(data)
 def hook(self,g,a,size,user):
  super().hook(g,a,size,user)
  if self.constructor_observe:
   if self.constructor_math_pending is not None:
    row,destination=self.constructor_math_pending
    row['result_word']=f'{g.reg_read(getattr(reg,"UC_X86_REG_"+destination.upper()))&0xffffffff:08x}'
    self.constructor_math.append(row);self.constructor_math_pending=None
   instruction=self.code.at(a)
   if instruction.mnemonic=='rsqrtss':
    destination,operand=instruction.op_str.split(', ')
    assert operand.startswith('xmm')
    self.constructor_math_pending=(dict(order=self.death_order,instruction=f'{a:08x}',
     operand_word=f'{g.reg_read(getattr(reg,"UC_X86_REG_"+operand.upper()))&0xffffffff:08x}'),destination)
  if self.constructor_observe and self.death_calls and int(self.death_calls[-1]['entry'],16)==a:
   c=self.death_calls[-1]
   c['mxcsr']=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}'
   c['fpcw']=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}'
   c['xmm']={str(n):f'{g.reg_read(getattr(reg,f"UC_X86_REG_XMM{n}")):032x}'for n in range(8)}
  if self.constructor_observe and a==0x1d9363:
   sp=g.reg_read(reg.UC_X86_REG_ESP)
   c=next(c for _,_,c in reversed(self.death_pending)if c['entry']=='001d9330')
   c['owning_frame']=dict(pointer=f'{sp:08x}',size=0x4e94,
       before_hex=bytes(g.mem_read(sp,0x4e94)).hex(),captured_at=f'{a:08x}')
 def returns_at(self,a):
  if self.constructor_observe:
   for ret,_,c in self.death_pending:
    if ret==a and 'owning_frame' in c:
     q=c['owning_frame'];q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
  super().returns_at(a)


def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args();verify();a.output.parent.mkdir(parents=True,exist_ok=True)
 err=(a.output.parent/'runtime-stderr.txt').open('w')
 proc=subprocess.Popen(['wine',str(prior.player.CONTEXT_OUT/'generator.exe')],
  stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(prior.player.CONTEXT_OUT/'wine-prefix')))
 data=json.loads(proc.stdout.readline())
 try:
  prior.motion.CommandPoseTrace=ConstructorTrace
  t,correlation=prior.motion.prepare(data)
  for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
  t.g.context_restore(t.source_fixture_cpu)
  movement=prior.motion.probe(t,'authored_heading_forward',0x3f800000,0,True)
  t.selected_physics_fixup(0xe9f2086d)
  t.death_enabled=True
  for name,entry,args,regs in (
   ('original_effect_storage_constructor',0x1775d0,(),()),
   ('original_effect_map_reset',0x177690,(),()),
   ('original_current_damage_owner',0xd7cd0,(t.spawned_unit,PLACEMENT+0x800),()),
   ('explicit_original_death_request',0xd7e00,(t.spawned_unit,PLACEMENT+0x800,1,0),()),
   ('explicit_original_ordinary_death_cleanup',0xcbdc0,(t.spawned_unit,0xffffffff,0,0,0),()),
   ('original_after_death_object_prephysics',0xb7e00,(),()),
   ('original_after_death_world_physics',0x1c7660,(),()),
   ('original_after_death_object_postphysics',0xb7f50,(),()),
   ('original_after_death_object_newflags',0xb8040,(),()),
  ):assert t.phase(name,entry,args,regs)
  old=json.loads((BASE/'report.json').read_text())
  for k,v in (('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores)):
   assert old[k]==v,k
  assert not t.death_pending
  print(json.dumps(dict(entire_frozen_moving_death_byte_identical=True)),flush=True)
  before=t.physical_snapshot(True)
  head=t.u(t.u(0x4e78d0)+0x44)+(t.spawned_unit&0xffff)*12
  unit=t.u(head+8);datum=t.u(unit+0xb4)
  component=t.u(t.u(0x526248)+0x44)+(datum&0xffff)*0xa0
  assert datum!=0xffffffff
  selected=dict(unit=f'{t.spawned_unit:08x}',unit_pointer=f'{unit:08x}',
   component_datum=f'{datum:08x}',component_pointer=f'{component:08x}',
   source_current_mode34b=bytes(t.g.mem_read(unit+0x34b,1)).hex(),
   source_dead_state3dc=bytes(t.g.mem_read(unit+0x3dc,1)).hex(),
   direct_root_arguments_source='1DA61D stack pushes component,0,0,1; admission not taken in prior execution')
  offsets={k:len(v)for k,v in (('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores),('events',t.events))}
  prior.ENTRIES.update(t.starts)
  t.constructor_observe=True;t.death_executed=set()
  for name,entry,args,regs in (
   ('explicit_original_remove_dead_controller',0x1d4c50,(),((reg.UC_X86_REG_ECX,component),)),
   ('explicit_original_dispose_dead_controller',0x1d3bb0,(component,),()),
   ('explicit_original_ragdoll_group_constructor',0x1d9330,(component,0,0,1),()),
  ):
   if not t.phase(name,entry,args,regs):break
  stages=t.death_stages[offsets['stages']:]
  calls=t.death_calls[offsets['calls']:]
  reads=t.death_reads[offsets['reads']:]
  stores=t.death_stores[offsets['stores']:]
  events=t.events[offsets['events']:]
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60)
  proc.stdout.close();err.close();assert proc.returncode==0
 prior.write(a.output.parent/'raw-host-context.json',prior.encoded(data))
 listings=[];executed=set(t.death_executed)
 entries={int(c['entry'],16)for c in calls}|{0x1d9330,0x1da570,0xe2520,0xe2ce0,0x1f0e90,0x1f08e0}
 for entry in sorted(entries):
  f=_trace(t.code,entry,t.starts|entries)
  rows=[t.code.at(p)for p in sorted(f.code)]
  for r in rows:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
  path=a.output.parent/'instructions'/f'{entry:08x}.asm'
  prior.write(path,''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows).encode())
  listings.append(dict(entry=f'{entry:08x}',file=str(path.relative_to(a.output.parent)),
    sha256=prior.digest(path),rows=len(rows),executed_rows=sum(r.address in executed for r in rows)))
 rows=[t.code.at(p)for p in sorted(executed)]
 for r in rows:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
 path=a.output.parent/'instructions'/'executed-constructor.asm'
 prior.write(path,''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows).encode())
 listings.append(dict(kind='complete_new_execution',file=str(path.relative_to(a.output.parent)),sha256=prior.digest(path),rows=len(rows)))
 report=dict(schema=1,oracle_sha256=prior.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,
  prior_pins=PINS,entire_frozen_moving_death_byte_identical=True,
  selected_source_state=selected,before=before,stages=stages,calls=calls,reads=reads,stores=stores,
  events=events,math=t.constructor_math,instructions=listings,instruction_patches=[],new_supplied_guest_fields_or_results=[],
  boundaries=[
   'Actual original moving/dead unit, nodes, pools, physical/world state and selected phmo postprocess are reproduced from prior evidence before explicit roots.',
   '1D4C50 and 1D3BB0 are unchanged removal/disposal. Direct1D9330(component,0,0,1) uses exact positive1DA570 callee ABI but does not prove +34B/contact admission.',
   'No source physical mode, body, node, constraint, contact, world, clock or success is assigned. Any genuine unavailable dependency is retained.',
   'Inherited declared command axes/heading, selected startup/platform, audio/action boundaries and emulator FP are not live whole-title or Xbox hardware proof.'])
 verify();prior.write(a.output,prior.encoded(report))
 print(json.dumps(dict(report=str(a.output),sha256=prior.digest(a.output),calls=len(calls),reads=len(reads),stores=len(stores))),flush=True)

if __name__=='__main__':main()
