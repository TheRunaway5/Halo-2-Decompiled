#!/usr/bin/env python3
"""Export original raw state/ordered writes without providing voice replies."""
import argparse,hashlib,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.report.read_text());lines=['# Complete source raw-region inputs; no voice/platform return.'];cases=stores=0
 for image in r['images']:
  tag=image['image'];lines.append('state\t'+tag+'\t'+'\t'.join(image[k]for k in('driver_address','cache_address','queue_address')))
  for region in image['initial_regions']:lines.append('region\t'+tag+'\t'+region['address']+'\t'+region['raw_hex'])
  for q in image['cases']:
   if q['operation']=='cleanup_witness':continue
   cases+=1;name=q['name'];regions={int(b['address'],16):bytearray.fromhex(b['raw_hex'])for b in image['initial_regions']}
   def apply(w):
    at=int(w['address'],16);n=w['bytes'];begin=next(b for b,raw in regions.items()if b<=at and at+n<=b+len(raw));raw=regions[begin];index=at-begin
    assert raw[index:index+n]==int(w['before'],16).to_bytes(n,'little')
    raw[index:index+n]=int(w['after'],16).to_bytes(n,'little')
   lines.append('case\t'+name+'\t'+tag+'\t'+q['operation']+'\t'+str(q['index'])+'\t'+q['outcome'])
   for w in q['controls']:
    apply(w);lines.append('control\t'+name+'\t'+'\t'.join(str(w[k])for k in('address','bytes','before','after')))
   for w in q['stores']:
    apply(w);stores+=1;lines.append('store\t'+name+'\t'+'\t'.join(str(w[k])for k in('instruction','address','bytes','before','after')))
   for result in q['output_regions']:assert hashlib.sha256(regions[int(result['address'],16)]).hexdigest()==result['sha256']
   for bad in q['invalid']:lines.append('invalid\t'+name+'\t'+bad['address']+'\t'+str(bad['bytes']))
   if q['platform_request']:
    b=q['platform_request'];lines.append('request\t'+name+'\t'+'\t'.join(b[k]for k in('device_identity','output_address','outer_identity','descriptor24_hex')))
 save(a.output,('\n'.join(lines)+'\n').encode());save(a.output.with_suffix('.json'),encoded(dict(schema='source-sound-driver-owners-fixtures-v1',exporter_sha256=sha(__file__),source_report_sha256=sha(a.report),fixture_sha256=sha(a.output),source_cases=1140,native_cases=cases,represented_stores=stores,complete_regions_reconstructed_from_original_stores_and_sha256=True,whole_cleanup_witnesses_retained_separately=True,limits=r['limits'])))
 print(json.dumps(dict(cases=cases,stores=stores,fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
