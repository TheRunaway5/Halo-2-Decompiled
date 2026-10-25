#!/usr/bin/env python3
"""Materialize one native constructor case per pinned vehicle/variant context.

This writes bounded input manifests, not expected admission results. Native
construction, gameplay and original comparison remain separate acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py; retain only these bounded input manifests')
    index=json.loads(args.inventory.read_text())
    if index['schema']!='H2_ALL_VEHICLE_VARIANTS_TU15_V2' or not index['inventory_completed']:
        raise ValueError('Complete pinned vehicle inventory required')
    args.output.mkdir(parents=True,exist_ok=False)
    rows=[]
    for shard in index['variant_shards']:
        path=args.inventory.parent/shard['path']
        if sha(path)!=shard['sha256']:
            raise ValueError('Vehicle variant inventory shard changed')
        for case in json.loads(path.read_text()):
            # Prefer an MP context only when that same original authoring row
            # is present there. Campaign-only contracts keep campaign maps.
            maps=list(dict.fromkeys(case['maps']))
            preferred=('coagulation.map','foundation.map','headlong.map','zanzibar.map')
            selected=next((name for name in preferred if name in maps),maps[0])
            runtime_map='highplains.map' if selected=='highplains2.map' else selected
            identity=sha_bytes(json.dumps({key:case[key] for key in
                ('vehicle','variant','selection','xbox_vehicle_contract')},sort_keys=True).encode())[:20]
            filename=f'{len(rows):03d}-{identity}.json'
            data=dict(case,case_id=identity,original_context=selected,runtime_map=runtime_map,
                      inventory_sha256=sha(args.inventory),inventory_shard_sha256=shard['sha256'])
            path=args.output/filename
            path.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
            rows.append(dict(case_id=identity,path=filename,sha256=sha(path),map=runtime_map,
                             vehicle=case['vehicle'],selection=case['selection'],variant=case['variant']))
    if len(rows)!=index['variant_authoring_contexts'] or len({row['case_id'] for row in rows})!=len(rows):
        raise ValueError('Vehicle case identity is incomplete or ambiguous; requalify contract keys')
    report=dict(schema='H2_VEHICLE_VARIANT_CONSTRUCTOR_CASES_V1',prepared=True,passed=False,
                validation_not_run=True,inventory_sha256=sha(args.inventory),cases=rows,
                tool_sha256=sha(Path(__file__)),large_artifacts_retained=[])
    (args.output/'index.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(prepared=True,cases=len(rows),maps=len({row['map'] for row in rows}),
                         index=str(args.output/'index.json'))))


def sha_bytes(value):
    return hashlib.sha256(value).hexdigest()


if __name__=='__main__':
    main()
