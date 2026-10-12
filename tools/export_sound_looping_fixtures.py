#!/usr/bin/env python3
"""Export literal original loop-owner inputs and ordered output stores."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
MASTER=ROOT/'analysis/multiplayer/integration/sound-driver-master-gain-native/measured/report.json'
BINDINGS=('allocator_pointer','arena_base_pointer','arena_used_pointer','arena_crc_pointer','pool_pointer','state_pointer','engine_pointer','engine_table','class_fades_pointer','class_fades_flag','driver_pointer','master_output_globals')

def word(raw):return f'{int.from_bytes(bytes.fromhex(raw),"little"):08x}'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 report=json.loads(a.report.read_text());master=json.loads(MASTER.read_text());lines=['SOUND_LOOPING_NATIVE_V1'];calls=stores=total=0
 for b in report['banks']:
  c=b['constants'];lines.append('\t'.join(('BANK',b['image'],b['name'],'1'if b['sequential']else'0',*(b['bindings'][k]for k in BINDINGS))))
  lines.append('\t'.join(('K',word(c['one_bits']),word(c['minimum_db_bits']),c['game_allocator_method'],c['pool_name']['raw_hex'],c['empty_selector']['raw_hex'],c['dialogue_selector']['raw_hex'])))
  for q in c['names']:lines.append('\t'.join(('N',q['raw_hex'])))
  m=master['images'][0 if b['image']=='old'else 1];lines.append('\t'.join(('P',*(word(raw)for raw in m['constants']))))
  for q in b['regions']:lines.append('\t'.join(('R',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
  for q in b['cases']:
   lines.append('\t'.join(('C',q['name'],q['entry'],'1'if q['returned']else'0',q['result'],q['selector_hex']or'-',*q['arguments'])))
   for s in q['stores']:lines.append('\t'.join(('S',s['instruction'],s['address'],str(s['bytes']),word(s['before']),word(s['after']))));stores+=1
   lines.append('E');calls+=1
  lines.append('ENDBANK')
 save(a.output/'fixtures.tsv',('\n'.join(lines)+'\n').encode());save(a.output/'report.json',encoded(dict(schema='sound-looping-fixtures-v1',exporter_sha256=sha(__file__),original_report_sha256=sha(a.report),master_report_sha256=sha(MASTER),fixture_sha256=sha(a.output/'fixtures.tsv'),calls=calls,stores=stores,initial_region_bytes=total,expected_from_native=False)))
 print(json.dumps(dict(calls=calls,stores=stores,initial_region_bytes=total,fixture_sha256=sha(a.output/'fixtures.tsv'))))
if __name__=='__main__':main()
