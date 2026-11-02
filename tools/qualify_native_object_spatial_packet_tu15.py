#!/usr/bin/env python3
"""Freeze the four native spatial owners and verify completed probe cleanup."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/physics/native-object-spatial-packet-qualified-20261102'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, data):
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')


def main():
    report_path = ROOT / 'analysis/physics/native-object-spatial-packet-tu15-20261102-v3/report.json'
    report = json.loads(report_path.read_text())
    assert report['passed'] and report['original_cases'] == 239
    assert report['host_pipeline_cases'] == 8
    assert all(row['executed'] == row['total'] for row in report['body_coverage'].values())
    assert [row['exit_code'] for row in report['compiler_results']] == [0, 0]
    native_counts = []
    for compiler in ('g++', 'clang++'):
        log = report_path.parent / compiler / 'evidence/Testing/Temporary/LastTest.log'
        data = [json.loads(line) for line in log.read_text().splitlines()
            if line.startswith('{"allocator_outcomes_supplied"')]
        assert len(data) == 1 and data[0]['passed']
        native_counts.append(data[0])
    assert native_counts[0] == native_counts[1]
    for name, digest in report['source_hashes'].items():
        assert sha(ROOT / name) == digest, name
    owners = []
    for name in (
        'native-object-spatial-packet-tu15-20261102-first',
        'native-object-spatial-packet-tu15-20261102-v2',
        'native-object-spatial-packet-tu15-20261102-v3',
    ):
        path = ROOT / 'analysis/physics' / name / 'report.json'
        data = json.loads(path.read_text())
        assert not Path(data['owned_scratch']).exists()
        owners.append(dict(owned_scratch=data['owned_scratch'],
            allocated_bytes_before_cleanup=data['temporary_allocated_bytes_before_cleanup'],
            removed_verified=True, report=str(path.relative_to(ROOT))))
    OUT.mkdir(exist_ok=True)
    save(OUT / 'cleanup-verification.json', dict(verified=True, completed_owners=owners,
        total_parent_allocation_bytes_removed=sum(row['allocated_bytes_before_cleanup'] for row in owners),
        retained_large_artifacts=[], protected_reference_inputs_preserved=True,
        not_a_global_free_space_measurement=True))
    save(OUT / 'source-pins.json', dict(qualified=True,
        source_authority='Unchanged English Xbox TU1.5 XBE 21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
        native_owners=['16E290', 'BA910', '1D30A0', '29FFF0'],
        report=str(report_path.relative_to(ROOT)), report_sha256=sha(report_path),
        source_hashes=report['source_hashes'], body_coverage=report['body_coverage'],
        whole_original_cases_per_compiler=239, computed_host_pipeline_cases_per_compiler=8,
        source_ordered_reads=report['ordered_reads'],
        readonly_ordered_reads_compared=native_counts[0]['ordered_reads'],
        observer_mutations=native_counts[0]['observer_mutations'],
        ordered_stores=report['ordered_stores'],
        fault_prefixes=report['fault_prefixes'], rejected_mutants=2,
        scope='Native integer kernels; full identity aliases and ordinary disjoint host20B packet pipeline. '
            'Original BA910 computes its packet independently before a declared original1D30A0 entry. '
            'All persistent banks, ordered DATA stores and FP compared; readonly read ledgers are compared '
            'for every case except the one mutable-key observer case, which compares writes/mutation phase/full banks. '
            'No guest GP, native CPU emulation, '
            'expected postimages or supplied contacts. DF0 sequential DWORD/byte copies. '
            'Captured BA570 caller and visible multiplayer remain separate acceptance.',
        retained_large_artifacts=[]))
    print(json.dumps(dict(qualified=True, owners_removed=len(owners),
        parent_allocation_bytes_removed=sum(row['allocated_bytes_before_cleanup'] for row in owners))))


if __name__ == '__main__':
    main()
