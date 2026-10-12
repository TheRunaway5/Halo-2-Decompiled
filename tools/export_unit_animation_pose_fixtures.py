#!/usr/bin/env python3
"""Original measured local-pose sampler deliveries; no expected geometry math."""
import argparse,json
from pathlib import Path
from oracle_unit_animation_manager_1_1 import sha,save,encoded
def export(source,out):
 r=json.loads(source.read_text());defs={};lines=[];total=0;decodes=0
 for c in r['calls']:
  if 'definition_hex'not in c:continue
  key=c['graph']+' '+c['packed'];data=c['definition_hex']+' '+c['graph_masks_hex']
  if key in defs:assert defs[key]==data
  else:defs[key]=data
 for key,v in sorted(defs.items()):lines.append('D '+key+' '+v)
 for c in r['calls']:
  if c['entry']not in('001ca950','001d0f80'):continue
  s=c['stack'];kind='M'if c['entry']=='001d0f80'else'C';weight=s[2]if kind=='M'else s[0]
  ctx=[s[1],s[5],s[6]]if kind=='M'else['00000000']*3
  lines.append(f'C {c["id"]} {kind} {c["frequency"]} {c["feature_dc"]} {c["feature_dd"]} {weight} {c["node_count"]} '+' '.join(ctx))
  for label,field in(('B','manager_hex'if kind=='M'else'channel_hex'),('N','mask_hex'),('L','before_output'),('F','after_output')):lines.append(label+' '+(c[field]or'-'))
  samples=[q for q in r['calls'][c['id']+1:c['call_end']]if q['entry']=='00280030'];assert len(samples)==1
  q=samples[0];qs=q['stack'];parent=next(z for z in reversed(r['calls'][c['id']:q['id']])if z['entry']=='001ca950');root=parent['root_graph']
  mask_kind='None'if not q['mask']else'Caller'if q['mask']==c['mask']else'Graph5C'if q['mask']==root+0x5c else'Graph7C'if q['mask']==root+0x7c else'Intersection'
  if mask_kind=='Intersection':assert q['mask']==0x566144
  lines.append(f'K {parent["graph"]} {qs[0]} {q["node_count"]} {qs[2]} {qs[3]} {qs[4]} {int(qs[7],16)&255} {mask_kind}')
  for label,field in(('I','before_output'),('O','after_output'),('A','mask_hex')):lines.append(label+' '+(q[field]or'-'))
  total+=1;decodes+=len(samples)
 save(out/'fixtures.txt',('\n'.join(lines)+'\n').encode());report=dict(schema='source-unit-local-pose-fixtures-v1',exporter_sha256=sha(__file__),original_report_sha256=sha(source),fixture_sha256=sha(out/'fixtures.txt'),definitions=len(defs),owners=total,decoder_deliveries=decodes,
  expected_data='Original raw manager/channel/mask/local entry and return snapshots, original280030 arguments/outputs. Only undefined upper bits of its interpolation stack word are excluded; actual defined AL byte is retained.',
  unexecuted='No positive1d18f0 blend,1d1b00 root override, graph-mask intersection or inherited-map owner occurred in this genuine neutral composition; these remain required external producers or source-listed-only branches.')
 save(out/'report.json',encoded(report));print(json.dumps(report))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.source,a.output)
