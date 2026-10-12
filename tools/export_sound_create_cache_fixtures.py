#!/usr/bin/env python3
"""Export literal cache/queue/worker buffers and interleaved original events."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded

def export(report,out):
 r=json.loads(report.read_text());lines=['SOUND_CREATE_CACHE_NATIVE_V1'];stores=total=0
 for c in r['cases']:
  fields=('system','tag_table','current_matg','current_ugh','current_scenario','sources','controllers','class_fades','local_rng','environment','cache_pool','delay_scale_operand')
  lines.append('\t'.join(('CREATE',*[c['create_state'][q]for q in fields],c['raw_request_hex'],f"{c['local_association']:08x}",c['promotion_selector'])))
  lines.append('\t'.join(('CONSTANTS',*[f"{int.from_bytes(bytes.fromhex(q['raw_hex']),'little'):08x}"for q in c['create_constants']])))
  state=c['state'];lines.append('\t'.join(('CASE',c['scenario'],c['sound'],c['chunk'],state['cache_pool'],state['cache_allocator'],state['cache_base'])))
  for s in c['stages']:
   lines.append('\t'.join(('STAGE',s['name'],s['entry'],s['eax'],str(s['returned']).lower())))
   for q in s['before']:lines.append('\t'.join(('BEFORE',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
   for q in s['events']:
    if'store'in q:
     z=q['store'];lines.append('\t'.join(('STORE',z['instruction'],z['address'],str(z['bytes']),z['before'],z['after'])));stores+=1
    else:lines.append('\t'.join(('CALL',q['service'],*q['arguments'])))
   for q in s['after']:lines.append('\t'.join(('AFTER',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
   lines.append('ENDSTAGE')
  lines.append('ENDCASE')
 save(out/'fixtures.tsv',('\n'.join(lines)+'\n').encode());save(out/'fixtures.json',encoded(dict(schema='sound-create-cache-native-fixtures-v1',exporter_sha256=sha(__file__),original_report=dict(path=str(report),sha256=sha(report)),fixture_sha256=sha(out/'fixtures.tsv'),cases=len(r['cases']),stages=sum(len(c['stages'])for c in r['cases']),stores=stores,full_region_bytes=total,expected_from_native=False,native_platform_adaptation=True,admission_or_voice=False)))
 print(json.dumps(dict(stores=stores,full_region_bytes=total,fixture_sha256=sha(out/'fixtures.tsv'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.report,a.output)
