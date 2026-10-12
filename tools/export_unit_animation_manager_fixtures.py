#!/usr/bin/env python3
"""Export only original measured manager/channel buffers and owning callbacks."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_unit_animation_manager_1_1 import ROOT,OWNERS,BOUNDARIES,ROOTS,save,encoded,sha
def number(s):return int(s,16)
def bits(b,off,n=4):return int.from_bytes(b[off:off+n],'little')
def export(source,out):
 index=json.loads((source/'report.json').read_text());reports=[json.loads((source/c['path']).read_text())for c in index['cases']]
 for p,r in zip((source/c['path']for c in index['cases']),index['cases']):assert sha(p)==r['sha256']
 definitions={};lines=[];count=0;callbacks=0
 for r in reports:
  for k,d in r['definitions'].items():
   if k in definitions:assert definitions[k]==d
   else:definitions[k]=d
 for k,d in sorted(definitions.items()):
  lines.append('D '+k.replace(':',' ')+' '+' '.join(d[x]or'-'for x in ('raw','timing','sounds','effects','sound_refs','effect_refs')))
 for r in reports:
  for c in r['calls']:
   a=number(c['entry'])
   if a not in ROOTS:continue
   assert 'after'in c,c['id']
   s=[number(x)for x in c['stack']]
   cb,ctx=(s[0],s[1])if a==0x1CF100 else(s[1],s[2])if a==0x1CA2B0 else(s[3],s[4])if a==0x1CA090 else(0,0)
   frame=s[1]if a==0x1CA090 else s[0]if a==0x1C9F50 else 0
   node_count=s[2]if a==0x1CF100 else 0
   lines.append(f'C {r["scenario"]}:{c["id"]} {a:08x} {c["channel_offset"]} {c["dt_bits"]} {c["feature"]} {cb:08x} {ctx:08x} {frame:08x} {node_count} {int(c["return_eax"],16)&255}')
   for name in ('manager','channel'):
    lines.append('B '+name+' '+(c['before'][name]or'-'))
   nested=[q for q in r['calls'][c['id']+1:c['call_end']]if number(q['entry'])in BOUNDARIES]
   admitted=[]
   for q in nested:
    if any(k['id']<q['id']<k['call_end']for k in admitted):continue
    admitted.append(q);qs=[number(x)for x in q['stack']];qa=number(q['entry'])
    if qa==0xC04F0:row=f'K E {qs[0]:08x} {qs[1]:08x} {q["event_packet"]}'
    elif qa==0x1CEDA0:row=f'K V {qs[0]:08x} {q["selected_packed"]}'
    else:row=f'K T {qs[1]:08x} {q["registers"]["eax"]:08x}'
    lines.append(row)
    for prefix,st in (('I',q['before']),('O',q['after'])):
     for name in ('manager','channel'):lines.append(prefix+' '+name+' '+(st[name]or'-'))
   for name in ('manager','channel'):lines.append('F '+name+' '+(c['after'][name]or'-'))
   count+=1;callbacks+=len(admitted)
 body=('\n'.join(lines)+'\n').encode();save(out/'fixtures.txt',body)
 report=dict(schema='source-unit-animation-native-fixtures-v1',exporter_sha256=sha(__file__),original_index_sha256=sha(source/'report.json'),
  original_reports={c['path']:c['sha256']for c in index['cases']},definitions=len(definitions),calls=count,required_callback_deliveries=callbacks,
  fixture_sha256=sha(out/'fixtures.txt'),expected_data='Original guest entry/return buffers, original event packets and original selected packed IDs only; no native implementation-derived values.',
  source_pins=index['source_pins'],scope='Channel/manager owner comparison, inherited explicit fixture boundaries retained; no complete current collision matrices or natural live movement input asserted.')
 save(out/'report.json',encoded(report));print(json.dumps(report))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.source,a.output)
