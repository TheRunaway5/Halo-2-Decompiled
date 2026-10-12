#!/usr/bin/env python3
"""Observe missing pair-retirement entries in unchanged ordinary moving death.

This adds function-entry/return metadata only, outside the frozen original
call/read/store sequence. No world, body, pair, contact, node or result is set.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import x86_const as reg
import oracle_mp_biped_controller_1_1 as base
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN
ctor=base.ctor;p=base.p
class Trace(base.Trace):
 def __init__(self,*args):self.membership_calls=[];self.membership_pending=[];super().__init__(*args)
 def hook(self,g,a,size,user):
  if self.death_enabled:
   sp=g.reg_read(reg.UC_X86_REG_ESP)
   for ret,old_sp,row in self.membership_pending[:]:
    if a==ret and sp>=old_sp+4 and sp<=old_sp+0x44:
     row.update(return_order=self.death_order,return_esp=f'{sp:08x}',returned_eax=f'{g.reg_read(reg.UC_X86_REG_EAX):08x}');self.membership_pending.remove((ret,old_sp,row))
   if a in self.starts or a in {0x31c6d0,0x31f6b0}:
    row=dict(order=self.death_order,entry=f'{a:08x}',entry_esp=f'{sp:08x}',caller=f'{self.u(sp):08x}',stack16=[f'{v:08x}'for v in struct.unpack('<16I',g.mem_read(sp+4,64))],registers={n:f'{g.reg_read(getattr(reg,"UC_X86_REG_"+n.upper())):08x}'for n in('eax','ecx','edx','ebx','esi','edi')},fpcw=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}')
    self.membership_calls.append(row);self.membership_pending.append((self.u(sp),sp,row))
  super().hook(g,a,size,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();ctor.verify();a.output.parent.mkdir(parents=True,exist_ok=True)
 data=json.loads((ROOT/'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261011/with-math/raw-host-context.json').read_text())
 p.motion.CommandPoseTrace=Trace;t,_=p.motion.prepare(data)
 for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
 t.g.context_restore(t.source_fixture_cpu);p.motion.probe(t,'authored_heading_forward',0x3f800000,0,True);t.selected_physics_fixup(0xe9f2086d)
 t.death_enabled=True
 for name,entry,args in(
  ('original_effect_storage_constructor',0x1775d0,()),('original_effect_map_reset',0x177690,()),
  ('original_current_damage_owner',0xd7cd0,(t.spawned_unit,base.transition.PLACEMENT+0x800)),
  ('explicit_original_death_request',0xd7e00,(t.spawned_unit,base.transition.PLACEMENT+0x800,1,0)),
  ('explicit_original_ordinary_death_cleanup',0xcbdc0,(t.spawned_unit,0xffffffff,0,0,0)),
  ('original_after_death_object_prephysics',0xb7e00,()),('original_after_death_world_physics',0x1c7660,()),
  ('original_after_death_object_postphysics',0xb7f50,()),('original_after_death_object_newflags',0xb8040,())):assert t.phase(name,entry,args)
 old=json.loads((ctor.BASE/'report.json').read_text())
 for k,v in(('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores)):assert old[k]==v,k
 report=dict(oracle_sha256=p.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,source_report=str((ctor.BASE/'report.json').relative_to(ROOT)),source_report_sha256=p.digest(ctor.BASE/'report.json'),entire_frozen_moving_death_byte_identical=True,contexts=t.transition_context,controller_calls=t.controller_calls,membership_calls=t.membership_calls,math=t.controller_math,instruction_patches=[],new_supplied_gameplay_or_physics_results=[],boundaries=__doc__)
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(report_sha256=p.digest(a.output),entries=len(t.membership_calls))),flush=True)
if __name__=='__main__':main()
