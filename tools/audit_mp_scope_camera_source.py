#!/usr/bin/env python3
"""Independent repeat/literal-byte audit of the bounded scope camera freeze."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

BASE=ROOT/'analysis/multiplayer/integration/scope-camera'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def word(p,b):return struct.unpack_from('<I',b,p)[0]
def audit(out):
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
 assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN and sha(NEW_DEFAULT)==NEW_PIN
 files={};counts={'literal_instruction_rows':0,'source_frame_phases':0,'positive_action_states':0}
 for measured,repeat in [('measured-frozen','repeat-frozen'),('action-flags-measured','action-flags-repeat')]:
  assert (BASE/measured/'report.json').read_bytes()==(BASE/repeat/'report.json').read_bytes()
  files[str((BASE/measured/'report.json').relative_to(ROOT))]=sha(BASE/measured/'report.json')
  names=sorted(p.name for p in (BASE/measured/'instructions').glob('*.asm'))
  assert names==sorted(p.name for p in (BASE/repeat/'instructions').glob('*.asm'))
  for name in names:
   p=BASE/measured/'instructions'/name
   assert p.read_bytes()==(BASE/repeat/'instructions'/name).read_bytes()
   for line in p.read_text().splitlines():
    address,raw,*_=line.split(' ');data=bytes.fromhex(raw);assert old.read(int(address,16),len(data))==data
    counts['literal_instruction_rows']+=1
   files[str(p.relative_to(ROOT))]=sha(p)
 report=json.loads((BASE/'measured-frozen/report.json').read_text())
 for frame in report['frames']:
  for phase in ('registration','adoption','motion','render_packet'):
   state=frame[phase]['after'];raw=bytes.fromhex(state['observer_hex'])
   assert state['fov_coefficients']==[f'{word(p,raw):08x}'for p in (0x20c,0x240,0x274,0x2a8,0x2dc,0x310)]
   counts['source_frame_phases']+=1
 action=json.loads((BASE/'action-flags-measured/report.json').read_text());action_summary=[]
 for case in action['cases']:
  assert case['stages'][-1]['completion']=='complete_original_return'
  for step in case['stages']:
   state=action['states'][step['after_state']];obj=next(o for o in state['objects']if o['datum']=='e26f0000')
   body=bytes.fromhex(obj['body_hex']);component=struct.unpack_from('<h',body,0x346)[0]
   assert body[component+4:component+12]==bytes(8)
   counts['positive_action_states']+=1
  calls=action['calls'][slice(*case['ranges']['calls'])]
  final=next(o for o in action['states'][case['stages'][-1]['after_state']]['objects']if o['datum']=='e26f0000')
  body=bytes.fromhex(final['body_hex'])
  if 'jump' in case['case']:
   jumps=[c for c in calls if c['entry']=='000e5da0'];assert len(jumps)==1
   assert int(jumps[0]['return_cpu']['registers']['eax'],16)&255==1
   assert body[0x90:0x94]==bytes.fromhex('b81e4540')
  if 'crouch' in case['case']:
   assert word(0x134,body)&0x800000 and word(0x2c4,body)==0x3e2aaaab
  action_summary.append(dict(case=case['case'],parent=f'{word(0x14,body):08x}',byte_10a=body[0x10a],
   component_offset=component,action4=f'{word(component+4,body):08x}',action8=f'{word(component+8,body):08x}',
   crouch_bits=f'{word(0x2c4,body):08x}',velocity_z_bits=f'{word(0x90,body):08x}'))
 comparison=json.loads((BASE/'update-comparison-frozen/report.json').read_text());pairs={};writers={}
 for row in comparison['routines']:
  if row['primary_groups']:assert row['complete_instruction_shape_and_internal_branches_equal'],row['disc_entry']
  for ref in row.get('paired_absolute_image_operands',[]):
   if not ref['indexed']:pairs.setdefault(ref['disc_address'],ref)
  for label,image in [('disc',old),('later',new)]:
   listing=row.get(label+'_listing')
   if not listing:continue
   path=BASE/'update-comparison-frozen'/listing['file'];assert sha(path)==listing['sha256']
   for line in path.read_text().splitlines():
    address,raw,*_=line.split(' ');data=bytes.fromhex(raw);assert image.read(int(address,16),len(data))==data
    counts['literal_instruction_rows']+=1
   files[str(path.relative_to(ROOT))]=sha(path)
  if 'fov_runtime_static_initializers' in row['primary_groups']:
   refs=row['paired_absolute_image_operands'];assert len(refs)==2
   read,write=refs;assert read['bytes_equal']
   entry=int(row['later_entry'],16);needle=struct.pack('<I',entry)
   positions=[s.va+n for s in new.sections if s.name=='.data'for raw in [new.section_bytes(s)]for n in range(len(raw)-3)if raw[n:n+4]==needle]
   assert len(positions)==1
   writers[write['disc_address']]=dict(original_entry=row['disc_entry'],later_entry=row['later_entry'],
    original_input_address=read['disc_address'],later_input_address=read['later_address'],
    original_runtime_address=write['disc_address'],later_runtime_address=write['later_address'],
    bits=read['disc_hex'],later_initializer_pointer_table_address=f'{positions[0]:08x}')
 fields=[('default_fov',0x5563a4),('packet_minimum',0x565fe4),('packet_maximum',0x565fe8),
  ('render_minimum',0x5563a8),('render_maximum',0x5563ac),('magnified_minimum',0x46570c),('magnified_maximum',0x465708),
  ('one',0x465030),('packet_multiplier',0x47885c),('transition_seconds',0x4653b8),('frame_dt_multiplier',0x4701c8),
  ('half',0x46502c),('three',0x465044),('five',0x465060),('fifteen',0x465068),('six',0x46506c),
  ('ten',0x465070),('twenty',0x465078),('two',0x46507c),('four',0x465080),('twelve',0x465084),('seven',0x465218)]
 constants=[]
 for field,address in fields:
  key=f'{address:08x}'
  if key in writers:constants.append(dict(field=field,kind='executed_source_static_initializer_input',**writers[key]))
  else:
   r=pairs[key];assert r['bytes_equal'] and r['width']==4
   constants.append(dict(field=field,kind='immutable_original_operand',original_input_address=key,later_input_address=r['later_address'],bits=r['disc_hex']))
 # The source ESI immediate selects six caps. Verify the entire table separately,
 # then import the actual fourth-index (group4) word rather than a guessed cap.
 caprow=next(r for r in comparison['routines']if r['disc_entry']=='00171c70')
 cap=next(v for v in caprow['changed_nonbranch_immediates']if v['disc_value']=='0044b010')
 p,q=int(cap['disc_value'],16),int(cap['later_value'],16);assert old.read(p,24)==new.read(q,24)
 constants.append(dict(field='acceleration_cap',kind='full_immutable_six_group_table_verified',original_input_address=f'{p+16:08x}',later_input_address=f'{q+16:08x}',bits=old.read(p+16,4).hex(),whole_table_hex=old.read(p,24).hex()))
 r=pairs['00465130'];assert r['bytes_equal'] and r['width']==8
 constants.append(dict(field='reciprocal_numerator',kind='immutable_original_double_operand',original_input_address=r['disc_address'],later_input_address=r['later_address'],bits=r['disc_hex']))
 files[str((BASE/'update-comparison-frozen/report.json').relative_to(ROOT))]=sha(BASE/'update-comparison-frozen/report.json')
 out.mkdir(parents=True,exist_ok=True)
 constants_report=dict(schema=1,original_xbe_sha256=PIN,later_xbe_sha256=NEW_PIN,fields_in_native_struct_order=constants,
  source_initializer_scope='Read each immutable input, not the serialized zero runtime destination. Actual original1.1 static writers execute in the dynamic freeze; complete1.5 writers and their genuine initializer table entries are audited against the authenticated image.',
  caller_selector_copy=dict(original_owner='001884e0',read_site='00188a74',write_site='00188a78',source='retained actual local-view control record+2E',destination='outgoing command+24'),
  primary_static_equal=sum(bool(r['primary_groups'])for r in comparison['routines']))
 (out/'constants.json').write_text(json.dumps(constants_report,indent=2)+'\n')
 audit_report=dict(schema=1,counts=counts,independent_reports_byte_identical=True,literal_original_instruction_bytes_valid=True,
  actual_jump_crouch_action_words=action_summary,source_files=files,constants_sha256=sha(out/'constants.json'))
 (out/'audit.json').write_text(json.dumps(audit_report,indent=2)+'\n')
 print(json.dumps(dict(**counts,constants_sha256=sha(out/'constants.json'),audit_sha256=sha(out/'audit.json'))))
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();audit(a.output)
if __name__=='__main__':main()
