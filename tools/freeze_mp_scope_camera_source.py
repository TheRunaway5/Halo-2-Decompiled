#!/usr/bin/env python3
"""Pin this independent source/native checkpoint without rewriting earlier files."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

BASE=ROOT/'analysis/multiplayer/integration/scope-camera'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert not a.output.exists(),'Write a fresh manifest; never overwrite this checkpoint'
 files={};prior={}
 pairs=[('measured-frozen','repeat-frozen'),('action-flags-measured','action-flags-repeat'),
  ('update-comparison-frozen','update-comparison-repeat'),('fire-measured','fire-repeat'),
  ('reset-callback-measured','reset-callback-repeat'),('ordering-measured','ordering-repeat')]
 for left,right in pairs:
  assert (BASE/left/'report.json').read_bytes()==(BASE/right/'report.json').read_bytes()
  for path in sorted((BASE/left).rglob('*')):
   if not path.is_file():continue
   if path.name!='fixtures.tsv':assert path.read_bytes()==(BASE/right/path.relative_to(BASE/left)).read_bytes()
   files[str(path.relative_to(ROOT))]=sha(path)
 for folder in ['audit-frozen','reset-audit-frozen']:
  for path in sorted((BASE/folder).rglob('*')):
   if path.is_file():files[str(path.relative_to(ROOT))]=sha(path)
 toolnames=['oracle_mp_scope_camera_1_1.py','oracle_mp_scope_action_flags_1_1.py','research_mp_scope_camera_updates.py',
  'export_mp_scope_camera_fixtures.py','audit_mp_scope_camera_source.py','oracle_mp_scope_fire_1_1.py',
  'research_mp_scope_reset_updates.py','research_mp_scope_reset_callbacks.py','audit_mp_scope_camera_resets.py',
  'research_mp_scope_update_order.py','freeze_mp_scope_camera_source.py']
 for name in toolnames:files['tools/'+name]=sha(ROOT/'tools'/name)
 for name in ['docs/MP_UNIT_SCOPE_CAMERA_SOURCE_RESEARCH.md','include/halo2/unit_scope_camera.hpp',
  'src/gameplay/units/unit_scope_camera.cpp','tests/unit_scope_camera_tests.cpp']:
  files[name]=sha(ROOT/name)
 source=json.loads((BASE/'action-flags-measured/report.json').read_text())
 for name,digest in source['source_pins'].items():
  assert sha(ROOT/name)==digest,('prior_source_pin_changed',name)
  prior[name]=digest
 for name,digest in [('tools/oracle_mp_weapon_attachment_1_1.py','9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c'),
  ('analysis/multiplayer/integration/unit-zoom/measured-frozen/report.json','238c61759d2b565e3209dc72893661055b743b944f525f5fe0ad770c0975dbb2')]:
  assert sha(ROOT/name)==digest;prior[name]=digest
 old,new=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),Xbe(NEW_DEFAULT)
 assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN and sha(NEW_DEFAULT)==NEW_PIN
 # Verify owning original callback chains, not just table-pointer bytes.
 expected=[(old,0x46fad0,{0:[0,0xc7e90,0xde960],2:[0,0xf9b10,0x1021d0]}),
  (new,0x474b90,{0:[0,0xc9010,0xdf880],2:[0,0xfaa30,0x1030f0]})]
 chains=[]
 for image,table,types in expected:
  u=lambda p:struct.unpack('<I',image.read(p,4))[0]
  for typ,callbacks in types.items():
   definition=u(table+typ*4);rows=[]
   for n,callback in enumerate(callbacks):
    record=u(definition+0x84+n*4);value=u(record+0x40)
    assert value==callback,(hex(table),typ,n,hex(value),hex(callback))
    rows.append(dict(record=f'{record:08x}',callback=f'{value:08x}'))
   assert u(definition+0x84+len(callbacks)*4)==0
   chains.append(dict(table=f'{table:08x}',object_type=typ,definition=f'{definition:08x}',update_callbacks=rows))
 report=dict(schema=1,original_xbe_sha256=PIN,later_xbe_sha256=NEW_PIN,
  independently_identical_report_pairs=pairs,source_files=files,verified_prior_pins=prior,
  ordinary_attached_object_update_tables=chains,
  native_tests='Optimized and ASan/UBSan bounded scalar tests: 28 frames,112 phase states,4 authored FOV leaves; exact bits.',
  scope='Source scalar camera and required source action/request/reset checkpoint. No local-session creation, complete six-channel camera, scope HUD/first-person model visibility, full hardware matrix or world/gameplay completeness claim.',
  module_runtime_integration_by_this_agent=False)
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(manifest_sha256=sha(a.output),files=len(files),prior_pins=len(prior),tables=len(chains))))

if __name__=='__main__':main()
