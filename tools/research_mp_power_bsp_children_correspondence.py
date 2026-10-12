#!/usr/bin/env python3
"""Complete resident BSP child query/triangle authenticated update correspondence."""
import argparse,json,sys
from pathlib import Path
import research_mp_sandbox_update_functions as base
from oracle_mp_power_bsp_children_1_1 import ROOT,OUT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
ENTRIES=(0x1f2420,0x2e5bc0,0x3066e0,0x305e40,0x1f26b0,0x1f2db0,0x1f3320,0x1f16f0,0x2e0a60)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'final/later-correspondence/report.json');a=ap.parse_args();verify()
 base.ENTRIES=ENTRIES;sys.argv=[sys.argv[0],'--output',str(a.output)];base.main();r=json.loads(a.output.read_text())
 assert len(r['routines'])==len(ENTRIES)
 for q in r['routines']:
  assert q['normalized_instruction_and_internal_branch_topology_equal'],q
  assert q['switch_table_topology_equal'],q
 r.update(correspondence_tool_sha256=sha(__file__),source_measured_sha256=sha(OUT/'final/measured/report.json'),complete_bodies=len(ENTRIES),later_gameplay_executed=False)
 frozen_write(a.output.parent/'complete-report.json',encoded(r));print(json.dumps(dict(complete_bodies=len(ENTRIES),report=sha(a.output.parent/'complete-report.json'))))
if __name__=='__main__':main()
