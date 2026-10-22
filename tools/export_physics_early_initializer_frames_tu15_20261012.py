#!/usr/bin/env python3
"""Export exact selected call frames from complete early/main bodies."""
import hashlib,json,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 e=ROOT/'analysis/multiplayer/sandbox-source/physics-early-initializer-tu15-20261012'
 paths=[e/'correspondence-v1/complete-report.json',ROOT/'analysis/multiplayer/sandbox-source/physics-main-heap-tu15-20261012/correspondence-v1/complete-report.json']
 assert [sha(p)for p in paths]==['0504f7dd7d9ca46abb6d129f899b5f711214768b28c7db3376850d61787dd084','579a526dba8c9fb6e090b78c79c906487935b1d74b9c4b8ef601b22d71277de4']
 early,heap=[json.loads(p.read_text())for p in paths]
 old=_Code(Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),'.text')
 selected=Xbe(NEW_DEFAULT)
 policies={0x1656a0:('Arena',0x1656a0),0x2284c0:('Pool',0x2284c0),0x1d2620:('Pool',0x1d2620),0x1482a0:('MainHeap',0x1482a0),0x1482f0:('Renderer',0x1482f0),0x33785c:('Pool',0x33785c),0x2d817e:('Required',0x2d817e)}
 rows=[row for body in early['routines']for row in body['instructions']]+heap['instructions']
 frames=[]
 for row in rows:
  i=old.at(row['old'])
  if i.mnemonic!='call':continue
  assert i.operands[0].type==2
  if i.operands[0].imm==0x1f15b0:continue
  module,entry=policies[i.operands[0].imm]
  raw=bytes.fromhex(row['later_bytes']);assert selected.read(row['later'],len(raw))==raw
  actual=_Code(selected,'.text').at(row['later']);assert actual.mnemonic=='call'
  frames.append(dict(site=i.address,target=actual.operands[0].imm,return_address=actual.address+actual.size,module=module,entry=entry))
 assert len(frames)==8
 h=ROOT/'src/physics/physics_early_initializer_call_frames.hpp'
 s='#pragma once\n#include <array>\n#include <cstdint>\nnamespace halo2::physics_early_initializer_frames {\nenum class Module { Arena, Pool, MainHeap, Renderer, Required };\nstruct Frame {std::uint32_t site,target,return_address,entry;Module module;};\ninline constexpr std::array<Frame,8> frames{{\n'
 for f in frames:s+=f'{{0x{f["site"]:x}u,0x{f["target"]:x}u,0x{f["return_address"]:x}u,0x{f["entry"]:x}u,Module::{f["module"]}}},\n'
 s+='}};\n} // namespace halo2::physics_early_initializer_frames\n';h.write_text(s)
 (e/'call-frames.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),reports=[sha(p)for p in paths],header_sha256=sha(h),frames=frames),sort_keys=True,indent=2)+'\n')
 print(sha(h))
if __name__=='__main__':main()
