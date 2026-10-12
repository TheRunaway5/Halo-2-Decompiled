#!/usr/bin/env python3
"""Export literal original one-SMG selector calls; no native expected values."""
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'analysis/multiplayer/integration/inventory-source/single-weapon-selection/report.json'
SOURCE_PIN='0a0b7d14df711a4719a745b20ebf0b2171fae23030697769cc37a6fe15bb3ebc'
TOOL_PIN='f1d2a72a918e3733840501af410f196cbc706ddf2a0571bd64108a08bd066ca1'
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/single-weapon-selection/native'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,data):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==data,p
 else:p.write_bytes(data)
def packet(c,name,after=False):
 return next(p for p in c['after_packets'if after else'packets']if p['field']==name)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);a=p.parse_args()
 assert sha(SOURCE)==SOURCE_PIN
 assert sha(ROOT/'tools/oracle_mp_single_weapon_selection_1_1.py')==TOOL_PIN
 r=json.loads(SOURCE.read_text());rows=[];correlations=[]
 for case in r['cases']:
  lo,hi=case['ranges']['calls'];calls=r['calls'][lo:hi];root=calls[0];assert root['entry']=='000cf300'
  rows.append(' '.join(['C',case['case'],*case['arguments'],case['return_ax']]))
  rows.append('B '+packet(root,'actual_unit800')['raw_hex'])
  rows.append('F '+packet(root,'actual_unit800',True)['raw_hex'])
  caps=[c for c in calls if c['entry']=='000ceaf0']
  for c in caps:
   item=c['entry_cpu']['registers']['edx'];unit,mode=c['stack24_words'][:2]
   rows.append(' '.join(['K',unit,item,mode,f"{int(c['return_cpu']['registers']['eax'],16)&255:02x}",packet(c,'actual_unit800')['raw_hex'],packet(c,'actual_unit800',True)['raw_hex']]))
   rows.append('D '+item+' '+packet(c,'actual_weapon_definition360')['raw_hex'])
  correlations.append(dict(case=case['case'],root_call_id=root['id'],capability_call_ids=[c['id']for c in caps],
   original_capability_children=[c['id']for c in calls if c['entry']in('00111490','001cfde0','001cfce0')],full_source_state_identical=case['before_state']==case['after_state']))
 assert all(c['full_source_state_identical']for c in correlations)
 save(a.output_dir/'fixtures.tsv',('\n'.join(rows)+'\n').encode())
 report=dict(schema=1,tool_sha256=sha(__file__),source_report_sha256=SOURCE_PIN,source_oracle_sha256=TOOL_PIN,
  fixture_sha256=sha(a.output_dir/'fixtures.tsv'),correlations=correlations,
  authority='Only original captured complete unit packets, actual definition bytes, CEAF0 ABI/AL and original selector AX. No native-derived or recomputed expected values.',
  boundary='Native selector differential alone supplies observed CEAF0 output. Production requires the actual owning capability service. Complete original111490/1CFDE0/1CFCE0 executed in the source report; this exporter does not replace those producers in live runtime.')
 save(a.output_dir/'report.json',(json.dumps(report,sort_keys=True,indent=2)+'\n').encode())
 print(json.dumps(dict(fixture=str(a.output_dir/'fixtures.tsv'),fixture_sha256=sha(a.output_dir/'fixtures.tsv'),cases=len(correlations))))
if __name__=='__main__':main()
