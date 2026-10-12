#!/usr/bin/env python3
"""Export original driver-prefix service stops and complete retained buffers."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded
ORDER=('driver_pointer','effects_generation','effects_context_pointer','optional_sound_flag','quiesce_flag','quiesce_clear_word','section_name','quiesce_record')
def export(report,out):
    r=json.loads(report.read_text());lines=['SOUND_DRIVER_UPDATE_STOPS_V1'];total=stores=0
    for c in r['cases']:
        lines.append('\t'.join(('CASE',c['name'],c['operation'],c['status_backing4_hex'],*(c['bindings'][k]for k in ORDER))))
        for q in c['initial_regions']:lines.append('\t'.join(('BEFORE',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
        for q in c['events']:
            if 'store'in q:
                z=q['store'];lines.append('\t'.join(('STORE',z['instruction'],z['address'],str(z['bytes']),z['before'],z['after'])));stores+=1
            else:lines.append('\t'.join(('CALL',q['service'],*q['arguments'])))
        for q in c['output_regions']:lines.append('\t'.join(('AFTER',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
        lines.append('ENDCASE')
    save(out/'fixtures.tsv',('\n'.join(lines)+'\n').encode())
    save(out/'fixtures.json',encoded(dict(schema='sound-driver-update-original-stops-fixtures-v1',exporter_sha256=sha(__file__),original_report=dict(path=str(report),sha256=sha(report)),fixture_sha256=sha(out/'fixtures.tsv'),cases=len(r['cases']),ordered_stores=stores,full_region_bytes=total,expected_from_native=False,complete_active_driver_return=False)))
    print(json.dumps(dict(cases=len(r['cases']),ordered_stores=stores,full_region_bytes=total,fixture_sha256=sha(out/'fixtures.tsv'))))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.report.resolve(),a.output.resolve())
