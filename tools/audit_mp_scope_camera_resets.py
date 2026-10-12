#!/usr/bin/env python3
"""Independent immutable reset extension audit; no older evidence is rewritten."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

BASE=ROOT/'analysis/multiplayer/integration/scope-camera'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
 assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN and sha(NEW_DEFAULT)==NEW_PIN
 files={};instruction_rows=0
 for measured,repeat in [('fire-measured','fire-repeat'),('reset-callback-measured','reset-callback-repeat')]:
  x,y=BASE/measured,BASE/repeat
  assert (x/'report.json').read_bytes()==(y/'report.json').read_bytes()
  files[str((x/'report.json').relative_to(ROOT))]=sha(x/'report.json')
  names=sorted(f.name for f in (x/'instructions').glob('*.asm'))
  assert names==sorted(f.name for f in (y/'instructions').glob('*.asm'))
  for name in names:
   source=x/'instructions'/name;assert source.read_bytes()==(y/'instructions'/name).read_bytes()
   image=new if '-later-' in name else old
   for line in source.read_text().splitlines():
    at,raw,*_=line.split(' ');b=bytes.fromhex(raw);assert image.read(int(at,16),len(b))==b
    instruction_rows+=1
   files[str(source.relative_to(ROOT))]=sha(source)
 r=json.loads((BASE/'fire-measured/report.json').read_text())
 assert r['local_view']==0
 first=bytes.fromhex(r['frames'][0]['stages'][0]['before']['unit_body_800_hex'])
 assert struct.unpack_from('<I',first,0x13c)[0]==0
 shots=[];reloads=[]
 for f in r['frames']:
  stages=f['stages'];assert [s['name']for s in stages]==['pressed_edges','player_actions','unit_input','blend','unit_wrapper','weapon','actions','animation','pose','postpass']
  for stage in stages:
   for call in stage['calls']:
    if call['entry']!='000c9a40':continue
    after=stage['after'];assert (after['current'],after['requested'],after['motion'])==(-1,-1,'00000000')
    row=dict(frame=f['frame'],phase=stage['name'],original_return_site=call['caller'],argument=call['stack_arguments'][0],after_current=after['current'],after_requested=after['requested'])
    if call['caller']=='001079ca':assert stage['name']=='weapon';shots.append(row)
    elif call['caller']=='000e8867':assert stage['name']=='unit_wrapper';reloads.append(row)
    else:raise AssertionError(call)
 assert [s['frame']for s in shots]==[1,3,5] and [s['argument']for s in shots]==['00000001']*3
 assert [s['frame']for s in reloads]==[15] and reloads[0]['argument']=='00000000'
 assert r['frames'][2]['stages'][4]['after']['current']==0
 assert all(f['stages'][-1]['after']['action4']=='00000001'for f in r['frames'][15:])
 comparison=json.loads((BASE/'reset-callback-measured/report.json').read_text())
 assert all(x['complete_instruction_shape_and_internal_branches_equal']for x in comparison['routines']if x['primary_groups'])
 callbacks=[]
 for image,table,record,expected in [(old,0x46ec68,0x46ea04,(0xe88b0,0xe8950,0)),(new,0x473d28,0x473ac4,(0xe97d0,0xe9870,0))]:
  assert struct.unpack('<I',image.read(table,4))[0]==record
  assert struct.unpack('<III',image.read(record,12))==expected
  callbacks.append(dict(table=f'{table:08x}',record=f'{record:08x}',record_hex=image.read(record,12).hex()))
 # Actual literal valid-unit input producer, independently already frozen in
 # the camera correspondence; this is not a full input-poll execution claim.
 owner=BASE/'update-comparison-frozen/instructions/001884e0-disc-001884e0.asm'
 for line in owner.read_text().splitlines():
  at,raw,*_=line.split(' ');b=bytes.fromhex(raw);assert old.read(int(at,16),len(b))==b
 files[str(owner.relative_to(ROOT))]=sha(owner)
 assert old.read(0x188775,5)==bytes.fromhex('e8a611f4ff')
 assert old.read(0x188805,4)==bytes.fromhex('66897d2e')
 assert old.read(0x188809,7)==bytes.fromhex('c6858900000000')
 assert old.read(0x188a74,4)==bytes.fromhex('668b552e') and old.read(0x188a78,4)==bytes.fromhex('66895624')
 out=dict(schema=1,original_xbe_sha256=PIN,later_xbe_sha256=NEW_PIN,
  independent_report_and_instruction_files_identical=True,literal_instruction_rows=instruction_rows,
  source_frames=32,source_phases=320,shots=shots,manual_reloads=reloads,actual_callback_records=callbacks,
  caller_contract={'valid_unit_admission_call':'00188775','denied_selector_store':'00188805','denied_toggle_store':'00188809','outgoing_selector_read':'00188a74','outgoing_selector_store':'00188a78',
   'early_owner_none':'001885c4 branches past zoom handling to00188931; it does not own selector removal',
   'camera':'188B60 uses retained view+2E; it does not recheck C9920 or replace it with unit CURRENT'},
  immutable_fire_report_erratum={'field':'local_view','actual_report_value':0,'actual_initial_unit_13c':'00000000',
   'correction':'Earlier tool/report prose says local view remains NONE. The retained older preallocated player/local-view context is actually view0/player0. Neither a genuine session binding nor the view-control reset buffer was captured by this extension. Unit selector/reset outputs remain independently reproduced unchanged.',
   'scope':'Preserve both report/tool bytes; use their explicit numeric outputs and this separately pinned correction.'},source_files=files)
 a.output.mkdir(parents=True,exist_ok=True);(a.output/'audit.json').write_text(json.dumps(out,indent=2)+'\n')
 print(json.dumps(dict(audit_sha256=sha(a.output/'audit.json'),rows=instruction_rows,shots=len(shots),reloads=len(reloads))))

if __name__=='__main__':main()
