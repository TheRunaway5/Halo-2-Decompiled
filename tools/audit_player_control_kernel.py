#!/usr/bin/env python3
"""Independent raw-instruction and complete-buffer audit of command producer."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import Xbe,OLD,NEW,sha,save,encoded
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/player-control-kernel/complete'
def overlap_replay(raw,p,stores):
 v=bytearray.fromhex(raw)
 for q in stores:
  at=int(q['address'],16);data=bytes.fromhex(q['written_hex'])
  a,b=max(p,at),min(p+len(v),at+len(data))
  if a>=b:continue
  before=bytes.fromhex(q['before_hex'])
  assert v[a-p:b-p]==before[a-at:b-at],('Original owning-store before bytes differ',q['instruction'])
  v[a-p:b-p]=data[a-at:b-at]
 return v.hex()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,default=OUT);a=ap.parse_args()
 report=json.loads((a.directory/'report.json').read_text());assert len(report['cases'])==60
 for p,pin in report['source_pins'].items():assert sha(Path(p))==pin,p
 assert sha(ROOT/'tools/oracle_player_control_kernel.py')==report['oracle_sha256']
 images=[Xbe(OLD),Xbe(NEW)];listing_rows=0;intervals=0;output_packets=0;gate_counts={}
 for q in report['listings']:
  path=a.directory/q['file'];assert sha(path)==q['sha256'];index=int(path.name.split('-')[0])
  for line in path.read_text().splitlines():
   at,raw,*_=line.split();assert images[index].read(int(at,16),len(raw)//2).hex()==raw;listing_rows+=1
 for case in report['cases']:
  stores=case['stores'];body=bytes.fromhex(case['input_body_hex']);header=bytes.fromhex(case['input_header12_hex']);bp=int.from_bytes(header[8:12],'little')
  assert overlap_replay(body.hex(),bp,stores)==case['output_body_hex'];intervals+=1
  changed=[i for i,(x,y)in enumerate(zip(body,bytes.fromhex(case['output_body_hex'])))if x!=y]
  assert all(0x134<=i<0x138 for i in changed)
  target_stores=[s for s in stores if int(s['address'],16)==bp+0x134]
  assert len(target_stores)==1 and target_stores[0]['size']==4
  consumer=case['calls'][-1];assert consumer['corresponding_old_entry']=='000c8140'
  assert target_stores[0]['order']<consumer['order']
  for call in case['calls']:
   if'after_packets'not in call:continue
   between=[s for s in stores if call['order']<s['order']<call['return_order']]
   for before,after in zip(call['packets'],call['after_packets']):
    assert before['field']==after['field'] and before['pointer']==after['pointer']
    assert overlap_replay(before['raw_hex'],int(before['pointer'],16),between)==after['raw_hex'];intervals+=1
  constructor=next(c for c in case['calls']if c['corresponding_old_entry']=='000c8250')
  packet=constructor['after_packets'][0];between=[s for s in stores if constructor['return_order']<s['order']<consumer['order']]
  assert overlap_replay(packet['raw_hex'],int(packet['pointer'],16),between)==case['control7c_hex'];output_packets+=1
  gate_counts[case['admitted_al']]=gate_counts.get(case['admitted_al'],0)+1
 for old,new in zip(report['cases'][::2],report['cases'][1::2]):
  assert old['id'][4:]==new['id'][3:]
  for field in('command5c_hex','current','control7c_hex','target_flags134_before','target_flags134_after','admitted_al','output_body_hex'):
   assert old[field]==new[field],(old['id'],field)
 audit=dict(schema=1,auditor_sha256=sha(__file__),oracle_report_sha256=sha(a.directory/'report.json'),
  original_instruction_rows_verified=listing_rows,complete_buffer_intervals_replayed=intervals,
  constructor_to_consumer_complete_packets_replayed=output_packets,admission_counts=gate_counts,
  all_original_1_1_u5_defined_kernel_outputs_equal=True,native_data_used=False)
 save(a.directory/'audit.json',encoded(audit));print(json.dumps(audit))
if __name__=='__main__':main()
