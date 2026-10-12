#!/usr/bin/env python3
"""Bind complete compiled entry owners for the retained death world.

These are canonical function identities, never instructions, return values or
world/pose snapshots. Source body generators authenticate their original XBE.
"""
import argparse,hashlib,json,re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT

FAMILIES={
 'startup':'death_startup_runtime_generated.cpp',
 'object_phases':'death_object_phases_generated.cpp',
 'construction':'unit_object_runtime_generated.cpp',
 'weapon':'unit_weapon_routing.cpp',
 'postphysics_phase':'biped_postphysics_phase_generated.cpp',
 'segment':'source_segment_collision_generated.cpp',
 'point':'source_point_collision_runtime_generated.cpp',
 'scheduler':'death_world_scheduler_generated.cpp',
 'death_physics':'death_world_physics_runtime_generated.cpp',
 'animation':'unit_animation_selection_runtime.cpp',
 'postphysics':'biped_postphysics_runtime_generated.cpp',
 'world':'physics_world_runtime_generated.cpp',
 'contact':'physics_contact_runtime_generated.cpp',
 'vehicle_update':'physics_vehicle_update_generated.cpp',
 'vehicle_motion':'physics_vehicle_motion_generated.cpp',
 'vehicle':'physics_vehicle_runtime_generated.cpp',
 'math':'physics_vehicle_motion_math_generated.cpp',
}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--output',type=Path,required=True)
 ap.add_argument('--manifest',type=Path,required=True);a=ap.parse_args()
 rows=['// Generated complete native function identities; no instruction/data replay.','#pragma once','#include <array>','#include <cstdint>','namespace halo2::death_world_runtime_entries {']
 owners={}
 for name,file in FAMILIES.items():
  p=ROOT/'src'/file
  entries=sorted({int(q,16)for q in re.findall(r'case 0x([a-f0-9]+)u:run_',p.read_text())})
  assert entries,(name,file)
  rows.append('inline constexpr std::array '+name+'{'+','.join('0x%xu'%q for q in entries)+'};')
  owners[name]=dict(source=str(p.relative_to(ROOT)),sha256=sha(p),complete_entries=[f'{q:08x}'for q in entries])
 rows+=['template<class T> bool contains(const T &entries,std::uint32_t entry) { for(auto p:entries)if(p==entry)return true;return false; }','} // namespace halo2::death_world_runtime_entries','']
 a.output.write_text('\n'.join(rows))
 a.manifest.write_text(json.dumps(dict(tool_sha256=sha(Path(__file__)),generated_sha256=sha(a.output),owners=owners,captured_gameplay_results_used=False),indent=2)+'\n')
 print(json.dumps({k:len(v['complete_entries'])for k,v in owners.items()}))
if __name__=='__main__':main()
