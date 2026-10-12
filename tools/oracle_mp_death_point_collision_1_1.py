#!/usr/bin/env python3
"""Observe original death placement point collision, including authored instances.

Actual world/dead/body state is produced by unchanged original phases. Each
additional API query supplies only caller-owned point bytes copied verbatim
from authored instance sphere centers; no collision outcome or pose is set.
"""
import argparse,json
from pathlib import Path
from unicorn import x86_const as reg
import oracle_mp_spartan_ragdoll_constructor_1_1 as ctor
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN,_trace
p=ctor.prior

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();ctor.verify();a.output.parent.mkdir(parents=True,exist_ok=True)
 data=json.loads((ROOT/'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261011/with-math/raw-host-context.json').read_text())
 p.motion.CommandPoseTrace=ctor.ConstructorTrace;t,_=p.motion.prepare(data)
 for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
 t.g.context_restore(t.source_fixture_cpu);p.motion.probe(t,'authored_heading_forward',0x3f800000,0,True);t.selected_physics_fixup(0xe9f2086d)
 t.death_enabled=True
 for name,entry,args in(
  ('original_effect_storage_constructor',0x1775d0,()),('original_effect_map_reset',0x177690,()),
  ('original_current_damage_owner',0xd7cd0,(t.spawned_unit,PLACEMENT+0x800)),
  ('explicit_original_death_request',0xd7e00,(t.spawned_unit,PLACEMENT+0x800,1,0)),
  ('explicit_original_ordinary_death_cleanup',0xcbdc0,(t.spawned_unit,0xffffffff,0,0,0)),
  ('original_after_death_object_prephysics',0xb7e00,()),('original_after_death_world_physics',0x1c7660,()),
  ('original_after_death_object_postphysics',0xb7f50,()),('original_after_death_object_newflags',0xb8040,())):assert t.phase(name,entry,args)
 old=json.loads((ctor.BASE/'report.json').read_text())
 for k,v in(('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores)):assert old[k]==v,k
 t.death_enabled=False;t.constructor_observe=False
 for tick in range(8):
  t.g.emu_start(0x139863,0x13986b,count=2)
  for entry in(0x13a380,0xb7e00,0x1c7660,0xb7f50,0xb8040):t.run(entry)
 unit=t.u(t.u(t.u(0x4e78d0)+0x44)+(t.spawned_unit&0xffff)*12+8)
 component=t.u(t.u(0x526248)+0x44)+(t.u(unit+0xb4)&0xffff)*0xa0
 old=json.loads((ROOT/'analysis/multiplayer/integration/death-runtime-20261011/natural-admission-v2/report.json').read_text())['frames'][8]
 assert t.u(t.u(0x518444)+8)==old['clock']==8
 assert bytes(t.g.mem_read(unit,5750)).hex()==old['unit_hex']
 assert bytes(t.g.mem_read(component,0xa0)).hex()==old['component_hex']
 offsets={k:len(v)for k,v in(('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores))}
 p.ENTRIES.update(t.starts);t.death_enabled=True;t.constructor_observe=True;t.death_executed=set();queries=[]
 bsp=t.u(0x4e7918);n=t.u(bsp+0x140);bank=t.u(bsp+0x144);failure=None
 for i in range(n):
  point=bytes(t.g.mem_read(bank+i*0x58+0x3c,12));t.g.mem_write(PLACEMENT+0xa00,point)
  for flags in(0x80040d,0x800407,0x800402):
   name=f'authored_instance_center_{i}_flags_{flags:08x}'
   if not t.phase(name,0x16a9c0,(flags,PLACEMENT+0xa00,t.spawned_unit,0xffffffff)):
    failure=name;break
   queries.append(dict(instance=i,point_hex=point.hex(),flags=f'{flags:08x}',result=f'{t.g.reg_read(reg.UC_X86_REG_EAX)&255:02x}'))
  if failure:break
 cs=t.death_calls[offsets['calls']:];reads=t.death_reads[offsets['reads']:];stores=t.death_stores[offsets['stores']:];listings=[]
 entries={int(c['entry'],16)for c in cs}
 for entry in sorted(entries):
  f=_trace(t.code,entry,t.starts|entries);instructions=[t.code.at(q)for q in sorted(f.code)]
  for r in instructions:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
  path=a.output.parent/'instructions'/f'{entry:08x}.asm';path.parent.mkdir(parents=True,exist_ok=True);path.write_text(''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in instructions));listings.append(dict(entry=f'{entry:08x}',file=str(path.relative_to(a.output.parent)),sha256=p.digest(path)))
 a.output.write_text(json.dumps(dict(oracle_sha256=p.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,frozen_prior_byte_identical=True,natural_admission_snapshot_byte_identical=True,queries=queries,failure=failure,calls=cs,reads=reads,stores=stores,instructions=listings,instruction_patches=[],supplied_caller_owned_point_bytes_only=True,boundaries=__doc__),indent=2)+'\n');print(json.dumps(dict(report_sha256=p.digest(a.output),queries=len(queries),calls=len(cs),failure=failure)),flush=True)
if __name__=='__main__':main()
