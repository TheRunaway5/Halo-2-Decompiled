#!/usr/bin/env python3
"""Export only measured original format/build prefixes and complete retained bytes."""
import argparse,hashlib,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded
FIELDS=('one_bits','stereo_bits','pcm_queue','adpcm_queue','pcm_initialize_method','adpcm_initialize_method','completion_callback')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 report=json.loads(a.report.read_text());lines=[];cases=stores=whole=0
 for image in report['images']:
  tag=image['image'];constants=image['constants']
  values=[f'{int.from_bytes(bytes.fromhex(constants[k]),"little"):08x}'if k.endswith('_bits')else constants[k]for k in FIELDS]
  lines.append('\t'.join(['state',tag,image['driver'],*values]))
  for region in image['initial_regions']:lines.append('\t'.join(['region',tag,region['address'],region['raw_hex']]))
  for case in image['cases']:
   cases+=1;name=case['name'];regions={int(q['address'],16):bytearray.fromhex(q['raw_hex'])for q in image['initial_regions']}
   def apply(row):
    address=int(row['address'],16);size=row['bytes'];base=next(base for base,data in regions.items()if base<=address and address+size<=base+len(data));data=regions[base];offset=address-base
    assert data[offset:offset+size]==int(row['before'],16).to_bytes(size,'little')
    data[offset:offset+size]=int(row['after'],16).to_bytes(size,'little')
   lines.append('\t'.join(['case',name,tag,case['operation'],str(case['category']),str(case['index']),case['format'],case['descriptor'],case['queue_slot'],case['outcome']]))
   for row in case['controls']:
    apply(row);lines.append('\t'.join(['control',name,*[str(row[k])for k in('address','bytes','before','after')]]))
   for row in case['stores']:
    apply(row);stores+=1;lines.append('\t'.join(['store',name,*[str(row[k])for k in('instruction','address','bytes','before','after')]]))
   if case['request']:
    row=case['request'];lines.append('\t'.join(['request',name,*[row[k]for k in('device_identity','descriptor_address','output_address','outer_identity','descriptor24_hex','format20_hex')]]))
   for row in case['output_regions']:
    data=regions[int(row['address'],16)];assert hashlib.sha256(data).hexdigest()==row['sha256'];whole+=len(data)
 save(a.output,('\n'.join(lines)+'\n').encode())
 result=dict(schema='sound-driver-stream-original-fixtures-v1',exporter_sha256=sha(__file__),source_report_sha256=sha(a.report),fixture_sha256=sha(a.output),cases=cases,stores=stores,whole_region_bytes=whole,source_platform_returns=[],complete_outputs_checked=True,limits=report['limits'])
 save(a.output.with_suffix('.json'),encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
