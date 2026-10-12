#!/usr/bin/env python3
"""Export complete independently executed cache constructor records/stores."""
import argparse,json,struct
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded
FIELDS=('general_allocator_identity','static_allocator_identity','entry_pool','page_allocator','physical_buffer')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--providers',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 r=json.loads(a.report.read_text());packet=json.loads(a.providers.read_text())
 lines=['# Complete original21A6E0 CPU cache initialization; platform/raw physical input limits explicit.']
 lines.append('constants\t'+packet['callbacks']['delete_callback']['old_identity']+'\t'+packet['callbacks']['busy_callback']['old_identity'])
 for case in r['cases']:
  name=case['name'];lines.append('case\t'+name)
  for q in case['stages']:
   s=q['state'];physical=struct.unpack('<13I',bytes.fromhex(s['physical_arena_hex']))
   values=[s['globals'][k]for k in FIELDS]+[f'{physical[0]:08x}']+[f'{v:08x}'for v in physical[3:8]]+[f'{v:08x}'for v in physical[8:13]]
   lines.append('stage\t'+name+'\t'+q['phase']+'\t'+'\t'.join(values)+'\t'+s['physical_arena_hex'])
   for region in s['regions']:
    lines.append('region\t'+name+'\t'+q['phase']+'\t'+region['address']+'\t'+str(region['requested_bytes'])+'\t'+str(region['rounded_bytes'])+'\t'+region['raw_hex'])
  for q in case['services']:
   if q['service']=='raw_allocate':lines.append('allocate\t'+name+'\t'+q['source_request']['owner_identity']+'\t'+str(q['source_request']['bytes'])+'\t'+q['output_alias']+'\t'+str(q['backing_bytes']))
   else:lines.append('protect\t'+name+'\t'+'\t'.join(q['source_request']))
  for q in case['stores']:lines.append('store\t'+name+'\t'+'\t'.join(str(q[k])for k in('instruction','address','bytes','before','after')))
 save(a.output,('\n'.join(lines)+'\n').encode())
 save(a.output.with_suffix('.json'),encoded(dict(schema='source-sound-cache-startup-fixtures-v1',exporter_sha256=sha(__file__),source_report_sha256=sha(a.report),providers_sha256=sha(a.providers),fixture_sha256=sha(a.output),case_count=len(r['cases']),state_field_order=FIELDS,
   physical_array_order='index,lower[5],upper[5]; whole52-byte root retained independently',
   scope='Actual source-created normal constructor inputs plus five explicitly modified physical-limit/index branch controls. Original actual mmap/mprotect service outputs retained. Whole126B20 still stops at genuine2205C0 without a return; no device/map/voice/read completion claim.')))
 print(json.dumps(dict(fixture_sha256=sha(a.output),cases=len(r['cases']))))
if __name__=='__main__':main()
