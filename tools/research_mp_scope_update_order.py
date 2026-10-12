#!/usr/bin/env python3
"""Narrow original attached-object ordering correspondence; no harness order claim."""
from pathlib import Path
import json
import research_mp_live_combat_updates as comparison
from oracle_smg_firing_1_1 import ROOT
from oracle_mp_combat_postprocess_updates import sha

def main():
 prior=ROOT/'analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/report.json'
 assert sha(prior)=='104075c71753a770ba82863a63aa793248b5bad418e397a76eb363c619b82fa8'
 row=next(r for r in json.loads(prior.read_text())['owners']if r['old_entry']=='000de960')
 assert row['u5_entry']=='000df880' and row['complete_instruction_and_internal_branch_topology_equal']
 comparison.main(groups={'ordinary_attached_reload_action_order':(0xbd360,0x10cdf0,0xde960,0x1021d0)},
  seed_pairs={0xde960:(0xdf880,dict(kind='previous_complete_literal_source_body_topology',report=str(prior.relative_to(ROOT)),sha256=sha(prior)))},
  source_files=(),configuration_tool={'path':str(Path(__file__).relative_to(ROOT)),'sha256':sha(__file__)})

if __name__=='__main__':main()
