#!/usr/bin/env python3
"""Literal manager entry/return states from the frozen genuinely allocated unit."""
import argparse,json
from pathlib import Path
from oracle_unit_animation_manager_1_1 import ROOT,sha,save,encoded
BASE=ROOT/'analysis/multiplayer/integration/inventory-source/unit-animation-clock'
SOURCE=ROOT/'analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json'
def manager(r,index):
 objects=r['states'][index]['objects'];unit=next(o for o in objects if o['datum']=='e26f0000')
 return unit['manager']['raw_hex']
def export(out):
 assert sha(SOURCE)=='4b9e9ed383e0081fe7d64c57636731304d66d322b3aef23259024a27061733cb'
 r=json.loads(SOURCE.read_text());lines=[s for s in(BASE/'native-fixtures/fixtures.txt').read_text().splitlines()if s.startswith('D ')];count=0;records=[]
 for c in r['calls']:
  if c['entry']not in('001cf100','001d0ed0'):continue
  unit=next(o for o in r['states'][c['before_state']]['objects']if o['datum']=='e26f0000')
  if c['entry_cpu']['registers']['esi']!=unit['manager']['pointer']:continue
  before,after=manager(r,c['before_state']),manager(r,c['after_state']);m=bytes.fromhex(before);s=c['stack24_words'];a=int(c['entry'],16)
  if a==0x1cf100:
   # Actual source branches prove only the primary is valid; therefore no
   # third completion, layer synchronization or variant/event output is hidden.
   assert m[0x20:0x28]==b'\xff'*8 and m[0x40:0x48]==b'\xff'*8
   defs={q.split()[1]+':'+q.split()[2]:q.split()for q in lines}
   key=f'{int.from_bytes(m[:4],"little"):08x}:{int.from_bytes(m[4:8],"little"):08x}';d=defs[key]
   assert d[4]==d[5]==d[6]=='-',('Actual primary would require uncaptured callbacks',key)
   points=[p['instruction']for p in r['points']if c['order']<p['order']<c['return_order']]
   assert '001ca1d6'not in points and '001cf1bd'not in points
  cb,ctx=(s[0],s[1])if a==0x1cf100 else('00000000','00000000');n=int(s[2],16)if a==0x1cf100 else 0
  result=int(c['return_cpu']['registers']['eax'],16)&255
  lines.append(f'C actual_owned:{c["id"]} {c["entry"]} {0 if a==0x1cf100 else -1} 3d088889 1 {cb} {ctx} 00000000 {n} {result}')
  for prefix,raw in(('B',before),('F',after)):
   lines.append(prefix+' manager '+raw);lines.append(prefix+' channel '+(raw[:64]if a==0x1cf100 else'-'))
  records.append(dict(call_id=c['id'],entry=c['entry'],before_state=c['before_state'],after_state=c['after_state'],node_count=n,phase=c['phase']));count+=1
 assert count==2
 save(out/'fixtures.txt',('\n'.join(lines)+'\n').encode());save(out/'report.json',encoded(dict(schema='actual-owned-unit-manager-fixtures-v1',exporter_sha256=sha(__file__),source_report_sha256=sha(SOURCE),definition_fixture_sha256=sha(BASE/'native-fixtures/fixtures.txt'),fixture_sha256=sha(out/'fixtures.txt'),cases=records,
  scope='Original complete manager states on actual allocated biped E26F0000; captured frame0→1 plus separate transition phase. Original decoded idle definition has no timing/sound/effect events. No omitted positive callback or variant result is supplied. Inherited explicit finite dt retained.')))
 print(json.dumps(dict(cases=count,fixture_sha256=sha(out/'fixtures.txt'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.output)
