#!/usr/bin/env python3
"""Observe unchanged original cold biped physical packet construction.

The existing allocated/equipped player preparation executes DFAF0 naturally.
Only observation is added. No biped state, pose, contact, node, constructor
return or physics field is supplied. Inherited platform/resource delivery is
the same declared boundary as the frozen command/locomotion experiment.
"""
import argparse,json
from pathlib import Path
from unicorn import x86_const as reg
import oracle_mp_spartan_ragdoll_constructor_1_1 as ctor
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN,_trace
p=ctor.prior
ENTRIES={0xdfaf0,0xe6f60,0xe6fb0,0x1e8e60,0x1f3da0,0xbacc0}

class ColdTrace(ctor.ConstructorTrace):
 def __init__(self,data):
  self.cold_return=None;self.cold_roots=[]
  super().__init__(data)
 def hook(self,g,a,size,user):
  if a==0xdfaf0:
   assert self.cold_return is None
   sp=g.reg_read(reg.UC_X86_REG_ESP);self.cold_return=(self.u(sp),sp+8)
   self.death_enabled=True;self.constructor_observe=True
  ending=self.cold_return==(a,g.reg_read(reg.UC_X86_REG_ESP))
  super().hook(g,a,size,user)
  if a==0xdfaf0:self.cold_roots.append(self.death_calls[-1]['id'])
  if ending:self.cold_return=None;self.death_enabled=False;self.constructor_observe=False

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();ctor.verify()
 data=json.loads((ROOT/'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261011/with-math/raw-host-context.json').read_text())
 p.ENTRIES.update(ENTRIES);p.motion.CommandPoseTrace=ColdTrace;t,correlation=p.motion.prepare(data)
 assert t.cold_roots and t.cold_return is None and not t.death_pending
 entries={int(c['entry'],16)for c in t.death_calls};assert ENTRIES<=entries
 listings=[]
 for entry in sorted(entries):
  f=_trace(t.code,entry,t.starts|entries);rows=[t.code.at(at)for at in sorted(f.code)]
  for r in rows:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
  path=a.output.parent/'instructions'/f'{entry:08x}.asm';path.parent.mkdir(parents=True,exist_ok=True)
  path.write_text(''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows));listings.append(dict(entry=f'{entry:08x}',file=str(path.relative_to(a.output.parent)),sha256=p.digest(path)))
 a.output.write_text(json.dumps(dict(oracle_sha256=p.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,prior_runtime_correlation=correlation,roots=t.cold_roots,calls=t.death_calls,reads=t.death_reads,stores=t.death_stores,instructions=listings,instruction_patches=[],new_supplied_gameplay_or_physics_results=[],boundaries=__doc__),indent=2)+'\n')
 print(json.dumps(dict(report_sha256=p.digest(a.output),calls=len(t.death_calls),roots=t.cold_roots)),flush=True)
if __name__=='__main__':main()
