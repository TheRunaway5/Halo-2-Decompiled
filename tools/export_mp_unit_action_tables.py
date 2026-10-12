#!/usr/bin/env python3
"""Export independently measured original action and object-type tables."""
import argparse
import hashlib
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export(authority, output):
    expected = '657123ffd0c414bc1080a4e5bf028865eda5f2f533054fbcc72f0c5bf9e30a75'
    if sha(authority) != expected:
        raise ValueError('Action-table authority changed; re-establish original evidence')
    source = json.loads(authority.read_text())
    rows, inputs = [], []
    for index, image in enumerate(source['images'], 1):
        path = Path(image['path'])
        if sha(path) != image['sha256']:
            raise ValueError(f'Owned executable changed: {path}')
        if len(image['descriptors']) != 60:
            raise ValueError('Incomplete original action bank')
        rows.append(f'M {index} {image["action_bank"]} {image["type_bank"]}')
        for entry in image['descriptors']:
            if len(bytes.fromhex(entry['raw10_hex'])) != 16:
                raise ValueError('Incomplete original action descriptor')
            rows.append(f'A {entry["type"]} {entry["descriptor_address"]} {entry["raw10_hex"]}')
        for entry in image['supported_base_extents']:
            rows.append(f'T {entry["type"]} {entry["record_address"]} '
                        f'{entry["raw10_hex"]} {entry["signed_base_extent"]}')
        inputs.append(dict(path=str(path), sha256=image['sha256']))
    output.mkdir(parents=True, exist_ok=True)
    fixture = output/'fixtures.tsv'
    fixture.write_text('\n'.join(rows)+'\n')
    report = dict(schema='source-unit-action-tables-v1',
                  authority=dict(path=str(authority), sha256=expected), inputs=inputs,
                  tool_sha256=sha(Path(__file__)), fixture_sha256=sha(fixture),
                  supported_images=2, action_records=120, object_type_records=6,
                  runtime_unit_construction=False)
    (output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(fixture_sha256=sha(fixture), report_sha256=sha(output/'report.json'))))


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    export(args.authority,args.output)
