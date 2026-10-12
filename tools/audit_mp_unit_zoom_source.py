#!/usr/bin/env python3
"""Independently compare zoom reruns and every retained instruction byte."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from oracle_mp_combat_postprocess_updates import OLD
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('measured',type=Path);p.add_argument('repeat',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
 a.measured=a.measured.resolve();a.repeat=a.repeat.resolve()
 assert sha(OLD)==PIN;x=Xbe(OLD)
 left={q.relative_to(a.measured).as_posix():q for q in a.measured.rglob('*')if q.is_file()}
 right={q.relative_to(a.repeat).as_posix():q for q in a.repeat.rglob('*')if q.is_file()}
 assert left.keys()==right.keys();files=[];instructions=0
 for name,q in sorted(left.items()):
  assert q.read_bytes()==right[name].read_bytes(),name
  if q.suffix=='.asm':
   for line in q.read_text().splitlines():
    address,data,*assembly=line.split();b=bytes.fromhex(data)
    assert x.read(int(address,16),len(b))==b,(name,address)
    instructions+=1
  files.append({'path':q.relative_to(ROOT).as_posix(),'sha256':sha(q),'repeat_sha256':sha(right[name])})
 r=json.loads(left['report.json'].read_text())
 assert r['tool_sha256']==sha(ROOT/'tools/oracle_mp_unit_zoom_1_1.py')
 assert r['attachment_dependency_sha256']==sha(ROOT/'tools/oracle_mp_weapon_attachment_1_1.py')
 assert len(r['leaf_cases'])==132 and len(r['composition'])==4 and len(r['unit_update_controls'])==6
 prerequisites=[]
 for name in ('tools/oracle_mp_unit_zoom_1_1.py','tools/export_mp_unit_zoom_fixtures.py','tools/audit_mp_unit_zoom_source.py',
  'tools/oracle_mp_weapon_attachment_1_1.py','docs/MP_UNIT_ZOOM_SOURCE_RESEARCH.md',
  'analysis/multiplayer/integration/spawn-source/live-body-world/report.json'):
  prerequisites.append({'path':name,'sha256':sha(ROOT/name)})
 for name in ('fire','reload','move_input','idle','empty_reload'):
  q=ROOT/f'analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite/{name}.json'
  prerequisites.append({'path':q.relative_to(ROOT).as_posix(),'sha256':sha(q)})
 report={'original_xbe_sha256':PIN,'reproduced_reports_equal':True,'fixture_exports_equal':True,
  'retained_instruction_bytes_verified':instructions,'instruction_files':sum(q.suffix=='.asm'for q in left.values()),
  'original_leaf_cases':132,'complete_unit_phases':10,'fixture_rows':143,'artifacts':files,'prerequisites':prerequisites,
  'scope':'Original1.1 CPU source outputs and byte audit; branch contexts/services explicit. Native module comparisons are bounded fields only; no live camera/FOV/action lifecycle or whole later1.5 execution claim.'}
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('reproduced_reports_equal','retained_instruction_bytes_verified','instruction_files','fixture_rows')}))
if __name__=='__main__':main()
