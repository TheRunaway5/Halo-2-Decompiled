#!/usr/bin/env python3
"""Measure complete original1D42D0 on actual ten-body Spartan construction.

Output packets are declared caller-owned scratch only. Body, motion, matrix,
component flags, pose, shape, contacts and world inputs are source-produced.
"""
import argparse
import json
import os
import subprocess
from pathlib import Path
from unicorn import x86_const as reg
import oracle_mp_spartan_ragdoll_constructor_1_1 as ctor
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_smg_firing_1_1 import ROOT, STOP, PIN, CACHE_PIN
from functions import _trace
p=ctor.prior
BASE=ROOT/'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010'
PIN_MANIFEST='c8a78b096ce8f8a22c487fdab55da339c4d3cf09f5c492fffe26fe22cbf8f3a7'


def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args();ctor.verify();assert p.digest(BASE/'frozen/manifest.json')==PIN_MANIFEST
 a.output.parent.mkdir(parents=True,exist_ok=True)
 err=(a.output.parent/'runtime-stderr.txt').open('w')
 proc=subprocess.Popen(['wine',str(p.player.CONTEXT_OUT/'generator.exe')],
  stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(p.player.CONTEXT_OUT/'wine-prefix')))
 data=json.loads(proc.stdout.readline())
 try:
  p.motion.CommandPoseTrace=ctor.ConstructorTrace
  t,correlation=p.motion.prepare(data)
  for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
  t.g.context_restore(t.source_fixture_cpu)
  p.motion.probe(t,'authored_heading_forward',0x3f800000,0,True)
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
  offsets={k:len(v)for k,v in (('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores),('events',t.events))}
  h=t.u(t.u(0x4e78d0)+0x44)+(t.spawned_unit&0xffff)*12;unit=t.u(h+8)
  component=t.u(t.u(0x526248)+0x44)+(t.u(unit+0xb4)&0xffff)*0xa0
  p.ENTRIES.update(t.starts);t.constructor_observe=True;t.death_executed=set()
  for name,entry,args,regs in (
   ('explicit_original_remove_dead_controller',0x1d4c50,(),((reg.UC_X86_REG_ECX,component),)),
   ('explicit_original_dispose_dead_controller',0x1d3bb0,(component,),()),
   ('explicit_original_ragdoll_group_constructor',0x1d9330,(component,0,0,1),()),
  ):assert t.phase(name,entry,args,regs)
  old=json.loads((BASE/'measured/report.json').read_text())
  for k,v in (('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores),('events',t.events)):
   assert old[k]==v[offsets[k]:],k
  assert not t.death_pending
  print(json.dumps(dict(entire_frozen_constructor_byte_identical=True)),flush=True)
  offsets={k:len(v)for k,v in (('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores),('events',t.events))}
  t.death_executed=set();cases=[]
  for index in range(t.u(component+0x74)):
   slot=t.u(component+0x70)+index*0x60;body=t.u(slot+0x40);motion=t.u(body+0x3c)
   out=PLACEMENT+0x1000+index*0x40
   row=dict(slot_index=index,component=f'{component:08x}',slot=f'{slot:08x}',body=f'{body:08x}',motion=f'{motion:08x}',output=f'{out:08x}',
    component_a0_hex=bytes(t.g.mem_read(component,0xa0)).hex(),slot60_hex=bytes(t.g.mem_read(slot,0x60)).hex(),
    bodyc0_hex=bytes(t.g.mem_read(body,0xc0)).hex(),motionc0_hex=bytes(t.g.mem_read(motion,0xc0)).hex(),
    input_transform40_hex=bytes(t.g.mem_read(motion+0x80,0x40)).hex(),before_output34_hex=bytes(t.g.mem_read(out,0x34)).hex(),
    read_begin=len(t.death_reads),store_begin=len(t.death_stores))
   complete=t.phase('explicit_original_body_matrix_slot_'+str(index),0x1d42d0,(),
    ((reg.UC_X86_REG_ECX,component),(reg.UC_X86_REG_EAX,index),(reg.UC_X86_REG_ESI,out)))
   row.update(completed=complete,after_output34_hex=bytes(t.g.mem_read(out,0x34)).hex(),read_end=len(t.death_reads),store_end=len(t.death_stores))
   cases.append(row)
   if not complete:break
  stages=t.death_stages[offsets['stages']:];calls=t.death_calls[offsets['calls']:]
  reads=t.death_reads[offsets['reads']:];stores=t.death_stores[offsets['stores']:];events=t.events[offsets['events']:]
  for row in cases:
   for key in ('read_begin','read_end'):row[key]-=offsets['reads']
   for key in ('store_begin','store_end'):row[key]-=offsets['stores']
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();err.close();assert proc.returncode==0
 p.write(a.output.parent/'raw-host-context.json',p.encoded(data));listings=[]
 entries={int(c['entry'],16)for c in calls}|{0x1d42d0,0x2e0b60}
 for entry in sorted(entries):
  routine=_trace(t.code,entry,t.starts|entries);rows=[t.code.at(at)for at in sorted(routine.code)]
  for row in rows:assert row.bytes==t.image.read(row.address,len(row.bytes))==bytes(t.g.mem_read(row.address,len(row.bytes)))
  path=a.output.parent/'instructions'/f'{entry:08x}.asm'
  p.write(path,''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows).encode())
  listings.append(dict(entry=f'{entry:08x}',file=str(path.relative_to(a.output.parent)),sha256=p.digest(path),rows=len(rows)))
 result=dict(schema=1,oracle_sha256=p.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,
  prior_manifest_sha256=PIN_MANIFEST,entire_frozen_constructor_byte_identical=True,cases=cases,stages=stages,
  calls=calls,reads=reads,stores=stores,events=events,instructions=listings,
  new_guest_inputs_or_results_supplied=[],instruction_patches=[],boundaries=[
   'Declared source1D42D0 callee roots read actual tenbody source-constructor storage; scratch output addresses are caller ABI, not input geometry.',
   'Constructor is an explicit root and natural+34B/world/contact/solver admission remains unavailable.',
   'Actual component bit6 is clear; conditional2E0B60 correction is source-listed but not executed.',
   'Inherited selected startup/command/timestep/audio/platform and emulator FP boundaries remain unchanged.'])
 ctor.verify();assert p.digest(BASE/'frozen/manifest.json')==PIN_MANIFEST
 p.write(a.output,p.encoded(result));print(json.dumps(dict(report=str(a.output),sha256=p.digest(a.output),cases=len(cases),calls=len(calls),reads=len(reads),stores=len(stores))),flush=True)
if __name__=='__main__':main()
