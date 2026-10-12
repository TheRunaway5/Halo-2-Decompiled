#!/usr/bin/env python3
"""Freeze only independently repeated original kill-volume/death-stop evidence."""
from pathlib import Path
import json
import freeze_mp_spartan_death_physical as archive
ROOT=archive.ROOT;BASE=ROOT/'analysis/multiplayer/integration/spartan-kill-trigger-20261010'
def same(left,right):
 files=sorted(p.relative_to(left)for p in left.rglob('*')if p.is_file()and p.name not in('raw-host-context.json','runtime-stderr.txt'))
 for p in files:assert(left/p).read_bytes()==(right/p).read_bytes(),p
 return len(files)
def main():
 assert same(BASE/'probe',BASE/'repeat')==307
 assert same(BASE/'correspondence',BASE/'correspondence-repeat')==37
 assert(BASE/'audit.json').read_bytes()==(BASE/'audit-repeat.json').read_bytes()
 r=json.loads((BASE/'correspondence/report.json').read_text())
 assert len(r['routines'])==18 and all(q['normalized_instruction_and_internal_branch_topology_equal']and q['switch_table_topology_equal']for q in r['routines'])
 paths=[ROOT/p for p in('tools/oracle_mp_spartan_kill_trigger_1_1.py','tools/audit_mp_spartan_kill_trigger_1_1.py',
  'tools/research_mp_kill_trigger_update_functions.py','tools/freeze_mp_spartan_kill_trigger.py','docs/MP_SPARTAN_KILL_TRIGGER_RESEARCH.md',
  'analysis/multiplayer/integration/spartan-body-matrix-20261010/frozen/manifest.json',
  'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/frozen/manifest.json',
  'analysis/multiplayer/integration/spartan-death-physical-20261010/frozen/manifest.json')]
 for folder in('probe','repeat','correspondence','correspondence-repeat'):paths+=[p for p in(BASE/folder).rglob('*')if p.is_file()]
 paths+=[BASE/q for q in('probe.log','repeat.log','correspondence-repeat.log','audit.json','audit-repeat.json')]
 archive.freeze(BASE/'frozen',paths,dict(report_sha256=archive.sha(BASE/'probe/report.json'),
  original_report_and_listings_repeated_identically=True,original_calls=1335,original_reads=33142,original_stores=17222,
  paired_complete_routines=18,current_bounds_center_kill_admitted=True,source_diagnostic_position_delivery_complete=True,
  source_dead_bit_velocity_retention_complete=True,actual_fall_death_sound_selected=True,
  required_sound_global='526464',first_unavailable_instruction='20EFC2',
  full_fatal_cleanup_natural_fall_landing_dead_or_ragdoll_proven=False,
  production_runtime_headers_CMake_render_or_release_edits=False))
if __name__=='__main__':main()
