#!/usr/bin/env python3
"""Authenticate the finite original/MCC human-turret response tuple.

Run through tools/run_test.py. Retain bounded metadata only; original assets
are read-only required references. This is data identity, not execution proof.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    census = json.loads(args.contracts.read_text())
    if census['schema'] != 'H2_DESTRUCTION_CONTRACTS_V1' or not census['inventory_completed']:
        raise ValueError('Require completed authored original/MCC contracts')
    name = r'objects\vehicles\h_turret_ap\h_turret_ap'
    definition = census['definitions']['vehi:' + name]
    if len(definition['xbox']) != 1 or len(definition['mcc']) != 1:
        raise ValueError('Human turret has context-dependent contracts')
    original_hash, imported_hash = next(iter(definition['xbox'])), next(iter(definition['mcc']))
    contracts = {}
    shard_pins = []
    for shard in census['contract_shards']:
        path = args.contracts.parent / shard['path']
        if sha(path) != shard['sha256']:
            raise ValueError('Contract shard changed')
        contracts.update(json.loads(path.read_text()))
        shard_pins.append(shard)
    original, imported = contracts[original_hash], contracts[imported_hash]
    def response(contract):
        if contract['model'] != name or len(contract['damage']) != 1:
            raise ValueError('Reviewed human-turret model/damage block changed')
        sections = contract['damage'][0]['sections']
        if len(sections) != 1 or len(sections[0]['responses']) != 1:
            raise ValueError('Reviewed human-turret section/response shape changed')
        return sections[0]['responses'][0]
    before, after = response(original), response(imported)
    if before['words']['04'] != '00000000' or after['words']['04'] != '02000000':
        raise ValueError('Reviewed flag words changed')
    normalized = json.loads(json.dumps(imported))
    response(normalized)['words']['04'] = '00000000'
    if normalized != original:
        raise ValueError('Model contracts differ beyond the one reviewed flag word')
    names = set(definition['xbox'][original_hash])
    if names != set(definition['mcc'][imported_hash]):
        raise ValueError('Paired model contexts differ')
    owners = []
    for owner in census['source_files']:
        if Path(owner['path']).stem not in names:
            continue
        if sha(owner['path']) != owner['sha256'] or sha(owner['mcc_path']) != owner['mcc_sha256']:
            raise ValueError('A required authored reference changed since its census')
        owners.append(owner)
    if {Path(owner['path']).stem for owner in owners} != names:
        raise ValueError('A paired model context has no authenticated map owner')
    result = dict(schema='H2_MCC_TU15_DESTRUCTION_MODEL_SCALAR_V1', model=name,
                  scope='Exact named response scalar tuple; no inferred flag meaning or runtime outcome',
                  original_response=before['words'], donor_response=after['words'],
                  shape=dict(damage_blocks=1, sections=1, responses=1),
                  authority=dict(tu15_sha256=census['tu15_sha256'], contracts_sha256=sha(args.contracts),
                                 contract_shards=shard_pins, original_contract=original_hash,
                                 donor_contract=imported_hash, paired_maps=owners),
                  tool_sha256=sha(__file__), owned_scratch=os.environ['HALO2_TEST_SCRATCH'],
                  retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(passed=True, paired_maps=len(owners), scalar_differences=1)))


if __name__ == '__main__':
    main()
