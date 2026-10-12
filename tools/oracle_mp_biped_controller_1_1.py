#!/usr/bin/env python3
"""Observe controller caller scratch and FP without changing frozen death execution.

Every retained input, allocation and effect is produced by the original image.
Additional function-entry and RSQRTSS observations are separate from the pinned
call/read/store stream, which must remain byte identical in its entirety.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import x86_const as reg
import oracle_mp_biped_death_transition_1_1 as transition
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN
ctor=transition.ctor;p=ctor.prior
ENTRIES={0x1da6f0,0x1daac0,0x317890,0x1dc750,0x2e4dd0,0x184510,0x184800,0x2e25a0,0x3213e0,0x1d2b10,0x1da570}
class Trace(transition.Trace):
 def __init__(self,*args):
  self.controller_calls=[];self.controller_math=[];self.controller_pending=None
  super().__init__(*args)
 def hook(self,g,a,size,user):
  super().hook(g,a,size,user)
  if not self.death_enabled:return
  if self.controller_pending is not None:
   row,dst=self.controller_pending;row['result_word']=f'{g.reg_read(getattr(reg,"UC_X86_REG_"+dst.upper()))&0xffffffff:08x}'
   self.controller_math.append(row);self.controller_pending=None
  instruction=self.code.at(a)
  if instruction.mnemonic=='rsqrtss':
   dst,operand=instruction.op_str.split(', ');assert operand.startswith('xmm')
   self.controller_pending=(dict(order=self.death_order,instruction=f'{a:08x}',operand_word=f'{g.reg_read(getattr(reg,"UC_X86_REG_"+operand.upper()))&0xffffffff:08x}'),dst)
  if a in ENTRIES:
   sp=g.reg_read(reg.UC_X86_REG_ESP)
   self.controller_calls.append(dict(order=self.death_order,entry=f'{a:08x}',entry_esp=f'{sp:08x}',caller=f'{self.u(sp):08x}',stack16=[f'{v:08x}'for v in struct.unpack('<16I',g.mem_read(sp+4,64))],registers={n:f'{g.reg_read(getattr(reg,"UC_X86_REG_"+n.upper())):08x}'for n in('eax','ecx','edx','ebx','esi','edi')},fpcw=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}'))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();ctor.verify();a.output.parent.mkdir(parents=True,exist_ok=True)
 data=json.loads((ROOT/'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261011/with-math/raw-host-context.json').read_text())
 p.motion.CommandPoseTrace=Trace;t,_=p.motion.prepare(data)
 for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
 t.g.context_restore(t.source_fixture_cpu);p.motion.probe(t,'authored_heading_forward',0x3f800000,0,True);t.selected_physics_fixup(0xe9f2086d)
 t.death_enabled=True
 for name,entry,args in(
  ('original_effect_storage_constructor',0x1775d0,()),('original_effect_map_reset',0x177690,()),
  ('original_current_damage_owner',0xd7cd0,(t.spawned_unit,transition.PLACEMENT+0x800)),
  ('explicit_original_death_request',0xd7e00,(t.spawned_unit,transition.PLACEMENT+0x800,1,0)),
  ('explicit_original_ordinary_death_cleanup',0xcbdc0,(t.spawned_unit,0xffffffff,0,0,0)),
  ('original_after_death_object_prephysics',0xb7e00,()),('original_after_death_world_physics',0x1c7660,()),
  ('original_after_death_object_postphysics',0xb7f50,()),('original_after_death_object_newflags',0xb8040,())):assert t.phase(name,entry,args)
 old=json.loads((ctor.BASE/'report.json').read_text())
 for k,v in(('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores)):assert old[k]==v,k
 assert len(t.transition_context)==3
 report=dict(oracle_sha256=p.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,source_report=str((ctor.BASE/'report.json').relative_to(ROOT)),source_report_sha256=p.digest(ctor.BASE/'report.json'),entire_frozen_moving_death_byte_identical=True,contexts=t.transition_context,controller_calls=t.controller_calls,math=t.controller_math,instruction_patches=[],new_supplied_gameplay_or_physics_results=[],boundaries=__doc__)
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(report_sha256=p.digest(a.output),controller_entries=len(t.controller_calls),estimates=len(t.controller_math))),flush=True)
if __name__=='__main__':main()
