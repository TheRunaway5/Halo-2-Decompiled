#!/usr/bin/env python3
"""Export exact original admission/query results, ordered writes and full input banks."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded,ROOT
from oracle_sound_admission_native import PAIR

def export(report_path,out):
 r=json.loads(report_path.read_text());shared=ROOT/'analysis/multiplayer/integration/sound-create-native/measured/providers.json';driver=ROOT/'analysis/multiplayer/integration/sound-platform-voice-contract/paired/providers.json'
 c=json.loads(shared.read_text());d=json.loads(driver.read_text());p=json.loads((PAIR/'providers.json').read_text())
 words=[int.from_bytes(bytes.fromhex(q['raw_hex']),'little')for q in c['words']]
 extra=next(q for q in p['words']if q['old_va']=='004651cc');words.append(int.from_bytes(bytes.fromhex(extra['raw_hex']),'little'))
 parameters=[int.from_bytes(bytes.fromhex(q['raw4_hex']),'little')for q in d['constants']]
 lines=['SOUND_ADMISSION_NATIVE_V1','K\t'+'\t'.join(f'{v:08x}'for v in words),'P\t'+'\t'.join(f'{v:08x}'for v in parameters)];calls=stores=total=0
 order=('system','tag_table','current_matg','current_ugh','class_fades','local_rng','alternate_codec_flag','first_person_globals','current_scene')
 for b in r['banks']:
  lines.append('\t'.join(['M',b['image'],b['map']['path']]+[b['state'][k]for k in order]))
  for q in b['regions']:lines.append('\t'.join(('R',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
  for q in b['cases']:
   lines.append('\t'.join(('C',q['kind'],q['entry'],q['sound'],q['argument'],str(int(q['reason_output'])),q['result'],q['raw_request_hex'])))
   for s in q['stores']:lines.append('\t'.join(('S',s['instruction'],s['address'],str(s['bytes']),s['before'],s['after'])));stores+=1
   lines.append('E');calls+=1
  lines.append('ENDMAP')
 save(out/'fixtures.tsv',('\n'.join(lines)+'\n').encode())
 save(out/'fixtures.json',encoded(dict(schema='sound-admission-fixtures-v1',report=dict(path=str(report_path),sha256=sha(report_path)),exporter_sha256=sha(__file__),providers=[dict(path=str(p),sha256=sha(p))for p in(shared,driver,PAIR/'providers.json')],fixture_sha256=sha(out/'fixtures.tsv'),calls=calls,stores=stores,initial_bytes=total,expected_values_from_native=False,successful_full_admission=False)))
 print(json.dumps(dict(calls=calls,stores=stores,initial_bytes=total,fixture_sha256=sha(out/'fixtures.tsv'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.report,a.output)
