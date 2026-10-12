#!/usr/bin/env python3
"""Export literal original 151C90 input/consumer-entry packets; no native outputs."""
import argparse,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured'
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/player-control-native'
PINS={
 BASE/'report.json':'12b7411de3bfa03f089de889d860011fb57cf20f9128cadaa4a1e272b1b65dd0',
 BASE/'audit.json':'5a3c8dc9382f021e79882edb8bf5e98bd710654bb63803b22b1cd50104c1452f',
 ROOT/'tools/oracle_mp_command_locomotion_pose_1_1.py':'807e303f942380bc809816e00b28053cc1c78e8542a1ac92516ab26cc25645ad',
 ROOT/'analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/report.json':'104075c71753a770ba82863a63aa793248b5bad418e397a76eb363c619b82fa8'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,p
 else:p.write_bytes(b)
def enc(r):return (json.dumps(r,sort_keys=True,indent=2)+'\n').encode()
def u(raw,p,n=4):return int.from_bytes(raw[p:p+n],'little')
def packet(call,name):return bytes.fromhex(next(p['raw_hex']for p in call['packets']if p['field']==name))
def unit(state,datum):return next(o for o in state['objects']if int(o['datum'],16)==datum)
def source_rows(report):
 rows=[];correlations=[]
 for case in report['cases']:
  name=case['case'];calls=[c for c in report['calls']if c['probe']==name]
  owner=next(c for c in calls if c['entry']=='00151c90')
  consumer=next(c for c in calls if c['entry']=='000c8140'and c['order']>owner['order'])
  admission=next(c for c in calls if c['entry']=='00151a20'and c['order']>owner['order'])
  restriction=next(c for c in calls if c['entry']=='000e5680'and c['order']>admission['order'])
  before=report['states'][owner['before_state']];after=report['states'][consumer['before_state']]
  player=bytes.fromhex(before['player']['raw21c_hex']);datum=u(player,0x2c)
  obj=unit(before,datum);body=bytes.fromhex(obj['body_hex']);header=bytes.fromhex(obj['header_hex'])
  target=int(owner['stack24_words'][0],16)
  target_before=bytes.fromhex(unit(before,target)['body_hex']);target_after=bytes.fromhex(unit(after,target)['body_hex'])
  command=packet(owner,'stack1_command5c');control=packet(consumer,'ecx_control7c')
  assert len(command)==0x5c and len(control)==0x7c
  assert u(body,0x13c)==0xffffffff or u(body,0x1fc,2)==0xffff
  restricted=int(restriction['return_cpu']['registers']['eax'],16)&255
  admitted=int(admission['return_cpu']['registers']['eax'],16)&255
  rows.append(['P','owned_'+name,command.hex(),f'{datum:08x}',f'{header[3]:02x}',f'{u(body,0x14):08x}',
   f'{body[0x3dc]:02x}',restricted,f'{u(target_before,0x134):08x}',body[0x70:0x7c].hex(),
   f'{u(body,0x13c):08x}',f'{u(body,0x1fc,2):04x}',f'{u(target_after,0x134):08x}',control.hex(),admitted])
  correlations.append(dict(id='owned_'+name,owner_call=owner['id'],consumer_call=consumer['id'],admission_call=admission['id'],
   restriction_call=restriction['id'],before_state=owner['before_state'],after_flag_store_state=consumer['before_state'],
   all_inputs_and_expected_outputs='Literal original call packets and actual owning state; no recomputed expected geometry'))
 return rows,correlations
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,default=OUT);ap.add_argument('--kernel-report',type=Path);a=ap.parse_args()
 for p,h in PINS.items():assert sha(p)==h,p
 r=json.loads((BASE/'report.json').read_text());rows,correlations=source_rows(r)
 kernel_sha=None
 if a.kernel_report:
  k=json.loads(a.kernel_report.read_text());kernel_sha=sha(a.kernel_report)
  assert sha(ROOT/'tools/oracle_player_control_kernel.py')==k['oracle_sha256']
  for path,pin in k['source_pins'].items():assert sha(Path(path))==pin,path
  assert len(k['cases'])==60 and not k['instruction_patches'] and not k['supplied_service_results']
  for listing in k['listings']:assert sha(a.kernel_report.parent/listing['file'])==listing['sha256']
  for q in k['cases']:
   c=q['current'];rows.append(['P',q['id'],q['command5c_hex'],c['datum'],c['object_type'],c['parent'],c['physical_state'],
    c['restricted_e5680'],q['target_flags134_before'],c['forward12_hex'],c['related_object_13c'],c['seat'],
    q['target_flags134_after'],q['control7c_hex'],q['admitted_al']])
   correlations.append(dict(id=q['id'],kernel_case=q['id'],expected_outputs='Actual original instruction execution, stopped at original C8140 entry'))
 fixture=a.directory/'fixtures.tsv';save(fixture,('N '+str(len(rows))+'\n'+'\n'.join(' '.join(map(str,q))for q in rows)+'\n').encode())
 report=dict(schema=1,exporter_sha256=sha(__file__),source_pins={str(p.relative_to(ROOT)):h for p,h in PINS.items()},
  original_kernel_report_sha256=kernel_sha,fixture_sha256=sha(fixture),rows=len(rows),correlations=correlations,
  row_schema='P id command5c_hex current_datum type parent physical E5680_AL target_flags_before forward12_hex related13C seat16 target_flags_after control7c_hex 151A20_AL',
  native_expected_data_used=False)
 save(a.directory/'export.json',enc(report));print(json.dumps(dict(fixture=str(fixture),rows=len(rows),sha256=sha(fixture))))
if __name__=='__main__':main()
