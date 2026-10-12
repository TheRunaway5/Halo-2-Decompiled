#!/usr/bin/env python3
"""Export only executed original HUD branch inputs/outputs, with no native oracle."""
import argparse,json
from pathlib import Path
from oracle_mp_scope_presentation_1_1 import ROOT,sha

def main():
 p=argparse.ArgumentParser();p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 r=json.loads(a.report.read_text());assert len(r['cases'])==309
 lines=[]
 for case in r['cases']:
  x=case['explicit_modified_context'];modes=x['magazine_modes'];assert len(modes)==x['authored_magazine_count']
  values=[case['name'],x['current_signed_byte'],int(x['primary_present']),x['authored_zoom_count'],len(modes),*modes,
   case['source_status_current'],case['source_status_binocular'],f"{case['source_zoom_mask']:04x}"]
  lines.append(' '.join(map(str,values)))
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(lines)+'\n')
 print(json.dumps(dict(rows=len(lines),original_report_sha256=sha(a.report),fixtures_sha256=sha(a.output))))

if __name__=='__main__':main()
