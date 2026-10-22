#!/usr/bin/env python3
"""Export every original renderer storage external CALL frame."""
import hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 e=ROOT/'analysis/multiplayer/sandbox-source/physics-renderer-storage-tu15-20261012';p=e/'correspondence-v1/complete-report.json'
 assert sha(p)=='59dd620260f89a6ccc4900879f7c39281117feeec6eff042a6722f6c055f83d5'
 d=json.loads(p.read_text());old=_Code(Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),'.text');new=Xbe(NEW_DEFAULT);nc=_Code(new,'.text')
 policy={0x33785c:('Pool',0x33785c),0x2d817e:('Required',0x2d817e),0x32684b:('Heap',0x32684b)};frames=[]
 for body in d['routines']:
  for row in body['instructions']:
   i=old.at(row['old'])
   if i.mnemonic!='call':continue
   assert i.operands[0].type==2
   target=i.operands[0].imm
   if f'{target:08x}'in d['entries']:continue
   module,entry=policy[target];raw=bytes.fromhex(row['later_bytes']);assert new.read(row['later'],len(raw))==raw
   actual=nc.at(row['later']);assert actual.mnemonic=='call' and actual.operands[0].type==2
   frames.append(dict(site=i.address,target=actual.operands[0].imm,return_address=actual.address+actual.size,module=module,entry=entry))
 assert frames
 h=ROOT/'src/physics/physics_renderer_storage_call_frames.hpp'
 s='#pragma once\n#include <array>\n#include <cstdint>\nnamespace halo2::physics_renderer_storage_frames {\nenum class Module { Pool, Heap, Required };\nstruct Frame {std::uint32_t site,target,return_address,entry;Module module;};\ninline constexpr std::array<Frame,'+str(len(frames))+'> frames{{\n'
 for f in frames:s+=f'{{0x{f["site"]:x}u,0x{f["target"]:x}u,0x{f["return_address"]:x}u,0x{f["entry"]:x}u,Module::{f["module"]}}},\n'
 s+='}};\n} // namespace halo2::physics_renderer_storage_frames\n';h.write_text(s)
 (e/'call-frames.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),report_sha256=sha(p),header_sha256=sha(h),frames=frames),sort_keys=True,indent=2)+'\n');print(len(frames),sha(h))
if __name__=='__main__':main()
