#!/usr/bin/env python3
"""Export independently executed source CPU startup inputs/outputs only."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded
FIELDS=('references','arena_base','arena_used','arena_crc','allocator_identity','system',
        'sources','static_voices','controllers','controller_root','looping','effects','shared_voices')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True)
 p.add_argument('--providers',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 r=json.loads(a.report.read_text());constants=json.loads(a.providers.read_text());lines=['# Independent original CPU sound startup; platform/cache/RNG provider limits retained.']
 ambient=int.from_bytes(bytes.fromhex(constants['ambient_transition_bits']['raw4_hex']),'little')
 lines.append(f'constants\t{ambient:08x}\t{constants["voice_policy"]["raw16_hex"]}')
 values=[]
 for entry in('0032803e','00328505','00337a40'):
  q=[q for q in r['controller_calls']if q['entry']==entry and q['phase']=='original_sound_constructor_through_controller_until_device'];assert len(q)==1
  values.append(q[0]['return_eax'])
 lines.append('controller\t'+'\t'.join(values))
 for q in r['stages']:
  state=q['state'];lines.append('stage\t'+q['phase']+'\t'+'\t'.join(state['globals'][k]for k in FIELDS)+'\t'+state['arena_hex'])
 state=r['commit_fragment']['output'];lines.append('stage\tcommit_fragment\t'+'\t'.join(state['globals'][k]for k in FIELDS)+'\t'+state['arena_hex'])
 for label,rows in(('prefix',r['stores']),('commit',r['commit_fragment']['stores'])):
  for q in rows:lines.append('store\t'+label+'\t'+'\t'.join(str(q[k])for k in('instruction','address','bytes','before','after')))
 save(a.output,('\n'.join(lines)+'\n').encode())
 save(a.output.with_suffix('.json'),encoded(dict(schema='source-sound-startup-cpu-fixtures-v1',exporter_sha256=sha(__file__),source_report_sha256=sha(a.report),providers_sha256=sha(a.providers),fixture_sha256=sha(a.output),state_field_order=FIELDS,
   scope='Whole original caller stops at2205C0 without a result; separate original CPU commit fragment is not admitted original startup. Native cache fixture is a separately retained original constructor whose effects on this arena/state are empty; actual live cache and platform owners remain mandatory. Recorded original clock/CRT outputs replayed, not a chosen controller seed.')))
 print(json.dumps(dict(fixture_sha256=sha(a.output),stages=len(r['stages'])+1,stores=len(r['stores'])+len(r['commit_fragment']['stores']))))
if __name__=='__main__':main()
