#!/usr/bin/env python3
"""Capture caller FP contexts for unchanged original biped death transitions.

Reexecutes and verifies the entire frozen moving-death call/read/write stream.
Only observation is added; no gameplay state, result or instruction is changed.
"""
import argparse,json
from pathlib import Path
from unicorn import x86_const as reg
import oracle_mp_spartan_ragdoll_constructor_1_1 as ctor
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN
p=ctor.prior
class Trace(ctor.ConstructorTrace):
 def __init__(self,*args):self.transition_context=[];super().__init__(*args)
 def hook(self,g,a,size,user):
  super().hook(g,a,size,user)
  if self.death_enabled and a in(0xe6050,0xe6dc0,0x1e8e60):
   c=self.death_calls[-1];assert int(c['entry'],16)==a
   self.transition_context.append(dict(id=c['id'],entry=c['entry'],fpcw=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}'))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();ctor.verify();a.output.parent.mkdir(parents=True,exist_ok=True)
 data=json.loads((ROOT/'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261011/with-math/raw-host-context.json').read_text())
 p.motion.CommandPoseTrace=Trace;t,_=p.motion.prepare(data)
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
 assert len(t.transition_context)==3
 a.output.write_text(json.dumps(dict(oracle_sha256=p.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,source_report=str((ctor.BASE/'report.json').relative_to(ROOT)),source_report_sha256=p.digest(ctor.BASE/'report.json'),entire_frozen_moving_death_byte_identical=True,contexts=t.transition_context,instruction_patches=[],new_supplied_gameplay_or_physics_results=[],boundaries=__doc__),indent=2)+'\n')
 print(json.dumps(dict(report_sha256=p.digest(a.output),contexts=t.transition_context)),flush=True)
if __name__=='__main__':main()
