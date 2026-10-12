#!/usr/bin/env python3
"""Export complete original master-output writes and whole source regions."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    r=json.loads(a.report.read_text());lines=['SOUND_DRIVER_MASTER_GAIN_V1'];constant_lines=[];stores=total=0
    for index,image in enumerate(r['images'],1):
        words=[f"{int.from_bytes(bytes.fromhex(value),'little'):08x}"for value in image['constants']]
        lines.append('\t'.join(('IMAGE',image['driver'],image['output_globals'],*words)))
        constant_lines.append(' '.join(('C',str(index),*words)))
        for case in image['cases']:
            lines.append('\t'.join(('CASE',case['name'],case['input_address'],case['x87_cw'],case['mxcsr'])))
            for region in case['before']:
                lines.append('\t'.join(('BEFORE',region['address'],region['raw_hex'])));total+=len(bytes.fromhex(region['raw_hex']))
            for event in case['stores']:
                lines.append('\t'.join(('STORE',event['instruction'],event['address'],str(event['bytes']),event['before'],event['after'])));stores+=1
            for region in case['after']:
                lines.append('\t'.join(('AFTER',region['address'],region['raw_hex'])));total+=len(bytes.fromhex(region['raw_hex']))
            lines.append('ENDCASE')
    save(a.output/'fixtures.tsv',('\n'.join(lines)+'\n').encode());save(a.output/'constants.tsv',('\n'.join(constant_lines)+'\n').encode())
    save(a.output/'report.json',encoded(dict(schema='sound-driver-master-gain-fixtures-v1',exporter_sha256=sha(__file__),original_report_sha256=sha(a.report),
        fixture_sha256=sha(a.output/'fixtures.tsv'),constants_sha256=sha(a.output/'constants.tsv'),cases=sum(len(q['cases'])for q in r['images']),stores=stores,full_before_after_bytes=total,expected_from_native=False)))
    print(json.dumps(dict(fixture_sha256=sha(a.output/'fixtures.tsv'),cases=sum(len(q['cases'])for q in r['images']),stores=stores,full_before_after_bytes=total)))
if __name__=='__main__':main()
