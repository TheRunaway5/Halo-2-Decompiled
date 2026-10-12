#!/usr/bin/env python3
"""Export exact selected call frames from authenticated complete original bodies."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 paired=E/'correspondence-v1/complete-report.json';external=E/'external-owners.json'
 assert sha(paired)=='616bee89417916769418d9f2a70075ab036c2bba8a1decadd46698c5d57f1933'
 assert sha(external)=='b65624f405296451711df2b9607bcb892cde3cc1a1c29e64179c3b866c5667df'
 d=json.loads(paired.read_text());x=json.loads(external.read_text())
 rows={i['old']:i for r in d['routines']for i in r['instructions']};frames=[]
 for target,body in x['external_direct_calls'].items():
  for call in body['calls']:
   row=rows[call['old_site']];frames.append((call['old_site'],body['later_target'],row['later']+len(bytes.fromhex(row['later_bytes']))))
 frames.sort();p=ROOT/'src/physics_game_arena_call_frames.hpp'
 p.write_text('// Generated from complete TU1.5 source call operands; no runtime decoder.\n#pragma once\n#include <array>\n#include <cstdint>\nnamespace halo2::physics_game_arena_frames {\nstruct Frame {std::uint32_t site,target,return_address;};\ninline constexpr std::array frames{\n'+',\n'.join(f' Frame{{0x{a:x}u,0x{b:x}u,0x{c:x}u}}'for a,b,c in frames)+'\n};\n}\n')
 report=dict(schema=1,tool_sha256=sha(__file__),correspondence_sha256=sha(paired),external_owners_sha256=sha(external),header_sha256=sha(p),frames=len(frames))
 (E/'call-frames.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
