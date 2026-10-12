#!/usr/bin/env python3
"""Audit new scope presentation evidence, preserve all earlier frozen source."""
import argparse,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_mp_scope_presentation_1_1 import sha

BASE=ROOT/'analysis/multiplayer/integration/scope-presentation'
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 old,new=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),Xbe(NEW_DEFAULT)
 files={};rows=0
 for left,right in [('measured','repeat'),('controls-measured','controls-repeat'),('update-measured','update-repeat')]:
  x,y=BASE/left,BASE/right
  assert (x/'report.json').read_bytes()==(y/'report.json').read_bytes()
  files[str((x/'report.json').relative_to(ROOT))]=sha(x/'report.json')
  if not(x/'instructions').exists():continue
  names=sorted(z.name for z in(x/'instructions').glob('*.asm'));assert names==sorted(z.name for z in(y/'instructions').glob('*.asm'))
  for name in names:
   z=x/'instructions'/name;assert z.read_bytes()==(y/'instructions'/name).read_bytes();image=new if '-later-'in name else old
   for line in z.read_text().splitlines():
    address,raw,*_=line.split(' ');b=bytes.fromhex(raw);assert image.read(int(address,16),len(b))==b;rows+=1
   files[str(z.relative_to(ROOT))]=sha(z)
 r=json.loads((BASE/'measured/report.json').read_text())
 assert [f['native_incoming_selector']for f in r['frames']]==[-1,0,0,0,-1,-1]
 source_data=(ROOT/'assets/disc/maps/lockout.map').read_bytes();assert sha(ROOT/'assets/disc/maps/lockout.map')==r['map_sha256']
 assets=[]
 for tag in r['authored_hud_definitions']:
  for kind in ['bitmap','text']:
   for widget in tag[kind]:
    address=int(widget['offset'],16);masks=struct.unpack_from('<8H',source_data,address+8)
    assert list(masks[:4])==widget['yes']and list(masks[4:])==widget['no']
    assets.append(dict(definition=tag['name'],name=widget['name'],kind=kind,file_offset=widget['offset'],mask_hex=source_data[address+8:address+24].hex()))
 frames=[]
 for f in r['frames']:
  level=f['native_incoming_selector'];status=f['status_prefix']
  assert status['current_status']==level and status['binocular_status']==int(level!=-1)
  assert f['masks'][0]&0x5c0==(0x40 if level==-1 else 0x480)
  assert f['model_count']==2
  model_hex=f['original_models_hex'];assert len(model_hex)==2*0xd0c*2
  record=bytes.fromhex(model_hex)
  frames.append(dict(tick=f['tick'],current=level,binocular=status['binocular_status'],zoom_mask=f"{f['masks'][0]&0x5c0:04x}",
   source_model_count=2,source_model_render_datums=[f'{struct.unpack_from("<I",record,n*0xd0c)[0]:08x}'for n in range(2)],
   source_registered_hand_flags=[h['flags']for h in f['after_original_fp_clock']['hands']]))
 forbidden={0x13b09a,0x13b7c0,0xc9bc0,0xcd1a0,0x105120,0x167a0d,0x167996,0x169ddc,0x156eb0,0x11cf00}
 assert not forbidden.intersection(int(s['entry'],16)for s in r['inherited_services'])
 c=json.loads((BASE/'controls-measured/report.json').read_text());assert len(c['cases'])==309
 assert [v['explicit_modified_context']['current_signed_byte']for v in c['cases'][:256]]==list(range(-128,128))
 for row in c['cases']:
  x=row['explicit_modified_context'];assert row['source_status_current']==x['current_signed_byte']
  assert row['source_zoom_mask']&~0x5c0==0
 comparison=json.loads((BASE/'update-measured/report.json').read_text())
 primary=[v for v in comparison['routines']if v['primary_groups']]
 assert len(primary)==11 and all(v['complete_instruction_shape_and_internal_branches_equal']for v in primary)
 mappings=[dict(original=v['disc_entry'],later=v['later_entry'])for v in primary]
 old_manifest=ROOT/'analysis/multiplayer/integration/scope-camera/manifest.json';old_pins=json.loads(old_manifest.read_text())
 for name,digest in {**old_pins['source_files'],**old_pins['verified_prior_pins']}.items():assert sha(ROOT/name)==digest,('prior_pin_changed',name)
 copies=json.loads((BASE/'prior-native-unit-zoom/pins.json').read_text())
 for name,digest in copies.items():assert sha(BASE/'prior-native-unit-zoom'/Path(name).name)==digest
 for path in [BASE/'controls-measured/fixtures.tsv',BASE/'prior-native-unit-zoom/pins.json',ROOT/'include/halo2/unit_zoom.hpp',ROOT/'src/unit_zoom.cpp',ROOT/'tests/unit_zoom_hud_tests.cpp']:
  files[str(path.relative_to(ROOT))]=sha(path)
 report=dict(schema=1,original_xbe_sha256=PIN,later_xbe_sha256=NEW_PIN,source_instruction_rows=rows,
  independently_identical_original_reports=True,source_frames=frames,authored_hud_mask_bytes=assets,
  explicitly_modified_branch_cases=309,complete_primary_body_mappings=mappings,
  source_services_do_not_supply_hud_status_masks_or_model_counts=True,
  previous_scope_manifest_sha256=sha(old_manifest),verified_previous_scope_files=len(old_pins['source_files']),verified_previous_pins=len(old_pins['verified_prior_pins']),
  preserved_prior_native_unit_zoom=copies,source_files=files,
  scope='Raw current/binocular status and mask subset; actual source FP model requests under captured initial render-camera input. No GPU binocular mask coverage or additional hide/visibility predicate is invented.')
 assert not a.output.exists();a.output.mkdir(parents=True);(a.output/'audit.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(audit_sha256=sha(a.output/'audit.json'),rows=rows,frames=len(frames),controls=309,mappings=len(mappings))))

if __name__=='__main__':main()
