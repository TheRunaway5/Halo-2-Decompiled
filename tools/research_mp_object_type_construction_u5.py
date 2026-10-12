#!/usr/bin/env python3
"""Independently pair complete original object-type construction wrappers."""
import argparse,json,struct
from pathlib import Path
import research_unit_pose_requests_u5 as bodies
from oracle_mp_title_update_display import Xbe,OLD,NEW,ROOT,sha,save,encoded

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,required=True);a=p.parse_args()
 bodies.ENTRIES=(0x10ca30,0x10cbd0,0x10cc20)
 # The existing complete-body comparator is reused unchanged. Its source and
 # dependency pins remain in the generated report, alongside this wrapper pin.
 import sys
 sys.argv=[sys.argv[0],'--output-dir',str(a.output_dir/'body')];bodies.main()
 r=json.loads((a.output_dir/'body/report.json').read_text());q=next(q for q in r['owners']if q['old_entry']=='0010ca30');assert len(q['full_body_matches'])==1
 banks={int(v['u5_va'],16)for v in q['image_memory_operands']if v['old_va']=='0046fad0'};assert len(banks)==1
 packet={}
 for label,image,bank in [('old',Xbe(OLD),0x46fad0),('u5',Xbe(NEW),banks.pop())]:
  ranges={};types=[]
  def retain(p,n):
   value=image.read(p,n).hex();key=f'{p:08x}';assert key not in ranges or ranges[key]==value;ranges[key]=value
  retain(bank,52)
  for i in range(13):
   record=struct.unpack('<I',image.read(bank+i*4,4))[0];retain(record,8);slots=[]
   for j in range(100):
    slot=struct.unpack('<I',image.read(record+0x84+4*j,4))[0];slots.append(f'{slot:08x}')
    if not slot:break
    retain(slot+0x28,8)
   else:raise AssertionError('Original bank lacks bounded terminator')
   retain(record+0x84,4*len(slots));types.append(dict(type=i,address=f'{record:08x}',bank=slots))
  packet[label]=dict(image_sha256=sha(OLD if label=='old'else NEW),bank=f'{bank:08x}',types=types,ranges=ranges)
 assert len(packet['old']['types'])==len(packet['u5']['types'])==13
 for x,y in zip(packet['old']['types'],packet['u5']['types']):
  assert packet['old']['ranges'][x['address']][8:]==packet['u5']['ranges'][y['address']][8:]
  assert len(x['bank'])==len(y['bank'])
 save(a.output_dir/'metadata-packet.json',encoded(dict(schema='original-object-type-construction-metadata-v1',tool_sha256=sha(__file__),body_report_sha256=sha(a.output_dir/'body/report.json'),images=packet,scope='Literal type pointer bank, first8 record bytes, every original+84 pointer through NULL and each slot+28/+2C pair. Opaque selected-image callback addresses are not semantic or successful callback providers.')))
 print(json.dumps(dict(packet_sha256=sha(a.output_dir/'metadata-packet.json'))))
if __name__=='__main__':main()
