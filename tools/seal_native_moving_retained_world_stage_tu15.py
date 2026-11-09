#!/usr/bin/env python3
"""Seal exact final1480 retained moving stage after completed owned proofs."""
from pathlib import Path
import datetime
import hashlib
import json
import shutil

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'analysis/physics/native-moving-shape-retained-world-stage-v2-20261102'
PHYSICS = ROOT/'analysis/physics'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    manifest_path = OUT/'source-pins.json'
    r = json.loads(manifest_path.read_text())
    assert not r['qualified']
    actual_folder = PHYSICS/'native-moving-retained-union-actual1479-20261102-first'
    actual = json.loads((actual_folder/'report.json').read_text())
    assert actual['passed'] is True and not actual.get('error')
    assert len(actual['mutants']) == 6
    for cc in ('g++', 'clang++'):
        profile = json.loads((actual_folder/(cc+'-actual.json')).read_text())
        assert profile['passed'] is True and not profile.get('error')
        assert len(profile['profiles']) == 4 and profile['production_guard_models'] == list(range(9))
    proof_paths = [
        PHYSICS/'native-moving-retained-final-source-20261102-first/report.json',
        PHYSICS/'native-moving-retained-final-world-20261102-first/report.json',
        PHYSICS/'native-moving-retained-full-context-20261102-second/report.json',
        PHYSICS/'native-moving-retained-final-world-20261102-first/counters/report.json',
        PHYSICS/'native-moving-retained-final-world-20261102-first/models/report.json',
        PHYSICS/'native-moving-retained-final-world-20261102-first/models/g++/report.json',
        PHYSICS/'native-moving-retained-final-world-20261102-first/models/clang++/report.json',
        PHYSICS/'native-moving-retained-host-transaction-20261102-first/report.json',
        actual_folder/'report.json',
    ]
    owners = {}
    for p in proof_paths:
        report = json.loads(p.read_text())
        assert report['passed'] is True and not report.get('error'), str(p)
        owner = report.get('owned_scratch')
        amount = report.get('temporary_allocated_bytes_before_cleanup')
        if owner:
            assert not Path(owner).exists(), owner
            if isinstance(amount, int):
                row = owners.setdefault(owner, dict(owned_scratch=owner,
                    allocated_bytes_before_cleanup=amount, removed_verified=True, reports=[]))
                row['allocated_bytes_before_cleanup'] = max(row['allocated_bytes_before_cleanup'], amount)
                row['reports'].append(dict(path=str(p.relative_to(ROOT)), sha256=sha(p), measured_bytes=amount))
    old_cleanup = json.loads((OUT/'cleanup-verification.json').read_text())
    for row in old_cleanup['completed_owners']:
        prior = owners.get(row['owned_scratch'])
        if prior:
            prior['allocated_bytes_before_cleanup'] = max(
                prior['allocated_bytes_before_cleanup'], row['allocated_bytes_before_cleanup'])
        else:
            owners[row['owned_scratch']] = row
    cleanup = dict(verified=True, completed_owners=list(owners.values()),
        owner_count=len(owners), allocated_bytes_removed=sum(row['allocated_bytes_before_cleanup'] for row in owners.values()),
        scope='Disjoint completed moving/World/Host owners; same-owner observations deduplicated at maximum measured allocation. No global free-space claim.',
        retained_large_artifacts=[], protected_required_archives_unchanged=True)
    (OUT/'cleanup-verification.json').write_text(json.dumps(cleanup, indent=2)+'\n')
    base = json.loads((ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-broad-phase-edges-original-v1480-20261102/source-manifest.json').read_text())['source_files_sha256']
    for name, pin in r['source_sha256'].items():
        assert sha(OUT/'staged'/name) == pin, name
        assert r['base_hashes'][name] == base.get(name), name
    for name, pin in actual['compiled_source_sha256'].items():
        if name.startswith(('src/', 'include/')):
            assert r['source_sha256'].get(name, base.get(name)) == pin, name
    saved = OUT/'pre-qualification-source-pins.json'
    assert not saved.exists()
    shutil.copyfile(manifest_path, saved)
    r['pre_qualification_manifest_sha256'] = sha(saved)
    r['baseline_dependency_source_sha256'] = r.pop('dependency_source_sha256')
    r['dependency_source_sha256'] = actual['compiled_source_sha256']
    r.update(qualified=True, root_reviewed=False, live_qualified=False,
        qualified_at=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        proofs={str(p.relative_to(ROOT)): sha(p) for p in proof_paths},
        actual_case_reports={str(p.relative_to(ROOT)): sha(p) for p in actual_folder.glob('*-actual.json')},
        source_documentation_sha256=sha(OUT/'README.md'), cleanup_sha256=sha(OUT/'cleanup-verification.json'),
        validation_tools_sha256={str(p.relative_to(ROOT)): sha(p) for p in (
            ROOT/'tools/validate_native_moving_retained_union_actual_tu15.py',
            ROOT/'tools/qualify_native_moving_retained_world_stage_tu15.py',
            ROOT/'tools/validate_native_moving_retained_host_transaction_tu15.py',
            Path(__file__).resolve())},
        scope='Exact final1480 retained-native2E route, transactional explicit Host registration and exact-invocation witness; selected whole-native actual retained-DATA/FP/SourcePC proof, linked World and exact Host/counter contexts. No arbitrary raw/private observer or wider solver admission.',
        remaining=['Independent final World/Host source/proof peer',
                   'Fresh finite1481 imported-MCC full450/1200/1200 with actual vehicle camera, all prior gates and strict Original comparison',
                   'Wider solver/count/shape/gameplay/profiling/exception domains remain explicit native refusals'])
    manifest_path.write_text(json.dumps(r, indent=2)+'\n')
    print(sha(manifest_path))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
