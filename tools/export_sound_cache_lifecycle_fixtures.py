#!/usr/bin/env python3
"""Export literal original lifecycle buffers, ordered stores and platform calls."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded
def export(report,out):
    r=json.loads(report.read_text());lines=['SOUND_CACHE_LIFECYCLE_V1'];stores=total=stages=0
    order=('entry_pool_pointer','page_allocator_pointer','physical_buffer_pointer','work_mutex_slot','work_head','requests_added','sound_system_pointer')
    for c in r['cases']:
        lines.append('\t'.join(('CASE',c['scenario'],c['control'],c['chunk'],*(c['bindings'][k]for k in order))))
        for s in c['stages']:
            stages+=1;lines.append('\t'.join(('STAGE',s['name'],s['entry'],str(s['repeat']),str(s['returned']).lower(),','.join(s['returns']))))
            for q in s['before']:lines.append('\t'.join(('BEFORE',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
            for q in s['events']:
                if 'store'in q:
                    v=q['store'];lines.append('\t'.join(('STORE',v['instruction'],v['address'],str(v['bytes']),v['before'],v['after'])));stores+=1
                else:lines.append('\t'.join(('CALL',q['service'],*q['arguments'])))
            for q in s['after']:lines.append('\t'.join(('AFTER',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
            lines.append('ENDSTAGE')
        lines.append('ENDCASE')
    save(out/'fixtures.tsv',('\n'.join(lines)+'\n').encode())
    save(out/'fixtures.json',encoded(dict(schema='sound-cache-lifecycle-fixtures-v1',exporter_sha256=sha(__file__),original_report=dict(path=str(report),sha256=sha(report)),fixture_sha256=sha(out/'fixtures.tsv'),cases=len(r['cases']),stages=stages,stores=stores,full_region_bytes=total,expected_from_native=False,ordinary_active_audio_loop=False)))
    print(json.dumps(dict(cases=len(r['cases']),stages=stages,stores=stores,full_region_bytes=total,fixture_sha256=sha(out/'fixtures.tsv'))))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.report.resolve(),a.output.resolve())
