#!/usr/bin/env python3
"""Export only newly executed original composed FP request expectations."""
import argparse,json,struct,hashlib
from pathlib import Path
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,('Existing export changed',str(p))
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=Path);p.add_argument('output',type=Path);a=p.parse_args();r=json.loads(a.report.read_text())
 lines=['FP_RAW_SOUND_COMPOSITION_V1']
 for c in r['cases']:
  before=c['before_fp']['hands'][0];after=c['after_fp']['hands'][0]
  lines.append('CASE '+c['scenario']+' '+str(c['tick'])+' '+before['weapon']+' '+before['activity']+' '+before['sound_identifier']+' '+before['sound_activity']+' '+after['sound_identifier']+' '+after['sound_activity']+' '+str(c['view'])+' '+c['current_bsp']+' '+c['fpcw']+' '+c['event20_hex'])
  lines.append('ROOTS '+' '.join(c['roots'][k]for k in('object_pool','physics_pool','sound_system','render_bsp')))
  lines.append('SUBMIT_STATE '+' '.join(c['submit_state'][k]for k in('system','tag_table','current_matg','current_ugh','current_scenario','effect_pool','controller_pool','controller_root')))
  lines.append('DEFINITION '+c['sound']+' '+c['definition20_hex']+' '+c['class92_hex'])
  for l in c['locals']:lines.append(f"LOCAL {l['view']} {l['player']} {l['unit']} {l['mode']}")
  for o in c['associations']:lines.append('OBJECT '+o['datum']+f" {o['type']:x} "+o['parent']+f" {o['flags_12c']:x} "+o['associated_154'])
  for m in c['memory']:lines.append('MEM '+m['address']+' '+m['raw_hex'])
  lines.append('BACKING '+c['marker_before_hex']+' '+c['location_before_hex']+' '+c['source88_hex'][96:136]+' '+c['raw_before_hex'])
  lines.append('AFTER '+c['marker_after_hex']+' '+c['location_after_hex']+' '+c['request_after_hex'])
  lines.append('AT_SUBMIT '+c['request_at_submit_hex'])
  for s in c['raw_stores']:lines.append('RAW_STORE '+s['instruction']+f" {s['offset']} {s['bytes']} "+s['before']+' '+s['after'])
  for s in c['object_request']['stores']:
   lines.append('OBJECT_STORE '+s['instruction']+f" {s['offset']} {s['size']} "+f"{int.from_bytes(bytes.fromhex(s['before_hex']),'little'):08x} {int.from_bytes(bytes.fromhex(s['after_hex']),'little'):08x}")
  assert not c['submit_stores'] and c['submit_return']=='ffffffff'
  lines.append('END')
 save(a.output,('\n'.join(lines)+'\n').encode())
 meta=dict(schema='fp-raw-sound-composition-export-v1',exporter_sha256=sha(__file__),report_sha256=sha(a.report),fixture_sha256=sha(a.output),cases=len(r['cases']),raw_stores=sum(len(c['raw_stores'])for c in r['cases']),object_stores=sum(len(c['object_request']['stores'])for c in r['cases']),whole_submit_replies_replayed=0,voice_or_admission_results_supplied=0)
 save(a.output.with_suffix('.json'),(json.dumps(meta,sort_keys=True,indent=2)+'\n').encode());print(json.dumps(meta))
if __name__=='__main__':main()
