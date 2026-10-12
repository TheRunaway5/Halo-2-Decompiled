#!/usr/bin/env python3
"""Export literal complete creation-prefix inputs/ordered original outputs."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded

def export(report_path,providers_path,out):
 r=json.loads(report_path.read_text());p=json.loads(providers_path.read_text());words=[int.from_bytes(bytes.fromhex(q['raw_hex']),'little')for q in p['words']]
 lines=['SOUND_CREATE_NATIVE_V1','K\t'+'\t'.join(f'{x:08x}'for x in words)];calls=stores=regions=0
 for m in r['maps']:
  state=m['state'];order=('system','tag_table','current_matg','current_ugh','current_scenario','sources','controllers','class_fades','local_rng','environment','cache_pool','delay_scale_operand')
  lines.append('\t'.join(['M',m['image'],m['map']['path']]+[state[k]for k in order]))
  for q in m['regions']:lines.append('\t'.join(('R',q['address'],q['raw_hex'])));regions+=len(bytes.fromhex(q['raw_hex']))
  for q in m['cases']:
   lines.append('\t'.join(('C',q['scenario'],q['sound'],f'{q["local_association"]:08x}',q['promotion_selector'],q['raw_request_hex'],q['cache_request']['sound'],q['cache_request']['chunk'],q['cache_request']['flags'])))
   mutable=[(int(z['address'],16),bytearray.fromhex(z['raw_hex']))for z in m['regions']]
   req=next(b for a,b in mutable if a==0xa400000);req[:]=bytes.fromhex(q['raw_request_hex'])
   for z in q['stores']:
    at=int(z['address'],16);n=z['bytes'];a,b=next((a,b)for a,b in mutable if a<=at and at+n<=a+len(b));assert int.from_bytes(b[at-a:at-a+n],'little')==int(z['before'],16)
    b[at-a:at-a+n]=int(z['after'],16).to_bytes(n,'little');lines.append('\t'.join(('S',z['instruction'],z['address'],str(n),z['before'],z['after'])))
   assert next(b.hex()for a,b in mutable if a==int(state['current_ugh'],16))==q['final_ugh_hex']
   assert next(b.hex()for a,b in mutable if a==0xa000000)==q['final_arena_hex'];assert req.hex()==q['final_request_hex']
   lines.append('E');calls+=1;stores+=len(q['stores'])
  lines.append('ENDMAP')
 save(out/'fixtures.tsv',('\n'.join(lines)+'\n').encode());save(out/'fixtures.json',encoded(dict(schema='original-sound-create-prefix-fixtures-v1',exporter_sha256=sha(__file__),report=dict(path=str(report_path),sha256=sha(report_path)),providers=dict(path=str(providers_path),sha256=sha(providers_path)),fixture_sha256=sha(out/'fixtures.tsv'),calls=calls,stores=stores,initial_region_bytes=regions,expected_values_from_native=False,cache_results_supplied=False,admitted_or_playback=False)))
 print(json.dumps(dict(calls=calls,stores=stores,initial_region_bytes=regions,fixture_sha256=sha(out/'fixtures.tsv'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--providers',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.report,a.providers,a.output)
