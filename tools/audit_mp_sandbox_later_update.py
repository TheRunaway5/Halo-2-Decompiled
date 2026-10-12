#!/usr/bin/env python3
"""Audit the separate owned later-update sandbox correspondence/scalar freeze."""
import argparse,json,struct
from pathlib import Path
from research_mp_sandbox_update_functions import ROOT,NEW_DEFAULT,NEW_PIN,sha
from oracle_mp_lift_later_update import OLD,OLD_PIN
from oracle_smg_firing_1_1 import Xbe,PIN
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--suite',type=Path,default=ROOT/'analysis/multiplayer/sandbox-source/later-update-functions/frozen/suite.json');args=ap.parse_args();suite=json.loads(args.suite.read_text())
 assert sha(NEW_DEFAULT)==NEW_PIN and sha(OLD)==OLD_PIN
 for p,h in suite['files'].items():assert sha(ROOT/p)==h,p
 for manifest,h in suite['prior_manifests'].items():
  assert sha(ROOT/manifest)==h,manifest;m=json.loads((ROOT/manifest).read_text())
  if isinstance(m.get('files'),dict):pairs=m['files'].items()
  else:pairs=((r['path'],r['sha256'])for r in m['artifacts'])
  for p,d in pairs:assert sha(ROOT/p)==d,p
 base=ROOT/'analysis/multiplayer/sandbox-source/later-update-functions';a=json.loads((base/'report.json').read_text());b=json.loads((base/'reexecution/report.json').read_text());assert a==b
 assert a['tool_sha256']==sha(ROOT/'tools/research_mp_sandbox_update_functions.py')
 assert a['later_owned_xbe_sha256']==NEW_PIN and a['disc_1_1_sha256']==PIN
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT);rows=0;tables=0
 assert len(a['routines'])==35
 for r in a['routines']:
  assert len(r['candidates'])==1 and r['normalized_instruction_and_internal_branch_topology_equal']and r['switch_table_topology_equal']
  assert all(c['mapped_target_matches']is not False for c in r['paired_direct_calls'])
  for t in r['switch_tables']:
   assert t['mapped_table_equal'];assert old.read(int(t['old_address'],16),t['width']*t['entries']).hex()==t['old_hex'];assert new.read(int(t['later_address'],16),t['width']*t['entries']).hex()==t['later_hex'];tables+=1
  for label,image in(('disc',old),('later',new)):
   p=base/'instructions'/r[label+'_listing']['file'];assert sha(p)==r[label+'_listing']['sha256']
   for line in p.read_text().splitlines():
    s=line.split();raw=bytes.fromhex(s[1]);assert image.read(int(s[0],16),len(raw))==raw;rows+=1
  for ref in r['paired_image_memory_operands']:
   assert old.read(int(ref['old_address'],16),ref['width']).hex()==ref['old_hex'];assert new.read(int(ref['later_address'],16),ref['width']).hex()==ref['later_hex']
 c=json.loads((base/'scalar/measured/report.json').read_text());d=json.loads((base/'scalar/repeat/report.json').read_text());assert c==d
 assert c['tool_sha256']==sha(ROOT/'tools/oracle_mp_lift_later_update.py')and c['source_mapping_sha256']==sha(base/'report.json')
 assert c['owned_later_xbe_sha256']==NEW_PIN and c['disc_1_1_input_fixture_sha256']==OLD_PIN
 assert c['original_epsilon_46a180_hex']=='6f12833a'and c['original_time_pointer_slot']=='0051df54'
 source=json.loads(OLD.read_text());assert len(c['cases'])==512
 for q,prior in zip(c['cases'],source['cases']):
  assert q['input_case']==prior and q['disc_1_1_comparison_equal']
  assert q['later_original_result_xmm0_bits']==prior['result_xmm0_bits']and q['later_original_ratio_bits']==prior['ratio_bits']and q['later_original_stack_cleanup']==44
 assert c['native_gameplay_update_applied']is False and not any(c['boundaries'][k]for k in('contact_membership_and_source_activation_executed','Havok_body_mass_marker_direction_and_projected_velocity_producers_executed','complete_original_lift_or_collision_result_proven'))
 out={'suite_sha256':sha(args.suite),'files':len(suite['files']),'complete_normalized_instruction_and_branch_matches':35,'switch_tables_checked':tables,'original_instruction_rows_checked':rows,'original_later_scalar_cases':512,'all_result_ratio_cleanup_bits_equal':True,'whole_function_and_scalar_reports_independently_identical':True,'no_native_world_explosion_lift_hardware_or_latest_ever_claim':True};(args.suite.parent/'audit.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
if __name__=='__main__':main()
