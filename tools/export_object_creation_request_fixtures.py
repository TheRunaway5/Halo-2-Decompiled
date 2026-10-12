#!/usr/bin/env python3
"""Export only original object-request input/output bytes and write events."""
import argparse
import json
from pathlib import Path
from oracle_mp_title_update_display import sha, save, encoded


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); report=json.loads(args.report.read_text())
    lines=['OBJECT_CREATION_REQUEST_V1']; writes=total=0
    for image in report['images']:
        constants=image['constants']
        lines.append('\t'.join(('IMAGE',image['image_sha256'],report['pool'],report['descriptor'],
            constants['one_bits'],constants['basis_forward'],constants['basis_up'])))
        for case in image['cases']:
            lines.append('\t'.join(('CASE',case['name'],case['operation'],case['definition'],case['owner'],
                case['location'],case['mask'],case['returned_eax'])))
            for region in report['regions']:
                lines.append('\t'.join(('BEFORE',region['address'],region['raw_hex'])))
                total+=len(bytes.fromhex(region['raw_hex']))
            for store in case['stores']:
                lines.append('\t'.join(('STORE',store['instruction'],store['address'],str(store['bytes']),store['before'],store['after'])))
                writes+=1
            for region in case['after']:
                lines.append('\t'.join(('AFTER',region['address'],region['raw_hex'])))
                total+=len(bytes.fromhex(region['raw_hex']))
            lines.append('ENDCASE')
    save(args.output/'fixtures.tsv',('\n'.join(lines)+'\n').encode())
    save(args.output/'report.json',encoded(dict(schema='object-creation-request-fixtures-v1',
        exporter_sha256=sha(__file__),original_report_sha256=sha(args.report),
        fixture_sha256=sha(args.output/'fixtures.tsv'),cases=sum(len(q['cases']) for q in report['images']),
        stores=writes,full_before_after_bytes=total,expected_from_native=False)))
    print(json.dumps(dict(fixture_sha256=sha(args.output/'fixtures.tsv'),stores=writes,full_before_after_bytes=total)))


if __name__=='__main__':
    main()
