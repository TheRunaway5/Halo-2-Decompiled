#!/usr/bin/env python3
"""Freeze independently repeated selected original kill-fatal continuation."""
from pathlib import Path
import json
import freeze_mp_spartan_death_physical as archive
ROOT=archive.ROOT;BASE=ROOT/'analysis/multiplayer/integration/spartan-kill-trigger-sound-20261010'
def same(left,right,excluded=()):
 files=sorted(p.relative_to(left)for p in left.rglob('*')if p.is_file()and p.name not in excluded)
 for p in files:assert(left/p).read_bytes()==(right/p).read_bytes(),p
 return len(files)
def main():
 assert same(BASE/'measured',BASE/'repeat',('raw-host-context.json','runtime-stderr.txt','raw-host-allocations.json'))==555
 assert same(BASE/'correspondence',BASE/'correspondence-repeat')==19
 assert(BASE/'audit.json').read_bytes()==(BASE/'audit-repeat.json').read_bytes()
 r=json.loads((BASE/'correspondence/report.json').read_text())
 assert len(r['routines'])==9 and all(q['normalized_instruction_and_internal_branch_topology_equal']and q['switch_table_topology_equal']for q in r['routines'])
 paths=[ROOT/p for p in('tools/oracle_mp_spartan_kill_trigger_sound_1_1.py','tools/audit_mp_spartan_kill_trigger_sound_1_1.py',
  'tools/research_mp_kill_trigger_sound_update_functions.py','tools/freeze_mp_spartan_kill_trigger_sound.py','docs/MP_SPARTAN_KILL_TRIGGER_SOUND_RESEARCH.md',
  'analysis/multiplayer/integration/spartan-kill-trigger-20261010/frozen/manifest.json')]
 for folder in('probe','measured','repeat','correspondence','correspondence-repeat'):paths+=[p for p in(BASE/folder).rglob('*')if p.is_file()]
 paths+=[BASE/q for q in('audit.json','audit-repeat.json','audit-self.json')]
 paths+=list(BASE.parent.glob('spartan-kill-trigger-sound-20261010*.log'))
 archive.freeze(BASE/'frozen',paths,dict(report_sha256=archive.sha(BASE/'measured/report.json'),
  entire_prior_kill_prefix_byte_identical=True,original_report_and_555_listings_repeated_identically=True,
  original_calls=2664,original_reads=59975,original_stores=32118,paired_complete_new_routines=9,
  complete_fatal_return_with_unavailable_playback=True,source_current_velocity_retained_through_complete_return=True,
  selected_storage_and_blocked_current_ugh_prefix_not_natural_startup=True,cold_map_stop='126E22',
  new_audio_success_or_gameplay_state_supplied=False,natural_fall_landing_dead_ragdoll_admission_proven=False,
  production_runtime_headers_CMake_render_or_release_edits=False))
if __name__=='__main__':main()
