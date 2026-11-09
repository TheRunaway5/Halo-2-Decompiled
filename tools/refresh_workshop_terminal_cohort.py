"""Apply the authenticated selected terminal delta after camera qualification.

The worker must have finished, its exact old source must be archived,
and the selected runtime/publication identities must still match the reviewed
reference. Change only the seven preflighted files inside this run_test.py
owner, refresh timestamps, and verify the complete predicted new inventory.
No runtime is published and no engine qualification is inferred.
"""
if not __debug__:
    raise RuntimeError('Source refresh requires enabled Python assertion guards')

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import tarfile
import uuid


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def replace(path, data):
    temporary = path.with_name(path.name + '.terminal-' + uuid.uuid4().hex)
    try:
        temporary.write_bytes(data)
        os.chmod(temporary, path.stat().st_mode & 0o777)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preflight', type=Path, required=True)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--source-archive', type=Path, required=True)
    parser.add_argument('--session-journal', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    assert not args.output.exists()
    preflight = json.loads(args.preflight.read_text())
    reference = json.loads(args.reference.read_text())
    assert preflight['passed'] and preflight['all_source_library_and_selection_inputs_unchanged']
    assert preflight['complete_selected_multiplayer_caller_typechecked'] and reference['passed']
    source = Path(preflight['active_source']).resolve(strict=True)
    if source != owner / 'source' or not source.is_relative_to(root / '.local/test-scratch'):
        raise RuntimeError('Terminal refresh requires its existing camera source/build scratch owner')
    journal = json.loads(args.session_journal.read_text())
    assert journal['status'] == 'active' and Path(journal['scratch_owner']) == owner
    workers = [row for row in journal['commands'] if any(
        Path(word).name == 'build_launcher_engine.py' for word in row.get('argv', []))]
    assert workers and workers[-1]['exit_code'] is not None and workers[-1].get('finished') is not None
    qualification = json.loads(args.qualification.read_text())
    archive_record = json.loads(args.source_archive.read_text())
    assert Path(qualification['source_cohort']['source']).resolve() == source
    assert archive_record['passed'] and archive_record['complete_stream_roundtrip_verified']
    checkpoint = archive_record.get('status') == 'tested_source_checkpoint'
    if checkpoint:
        assert workers[-1]['exit_code'] == 1 and not archive_record['engine_qualified']
        assert archive_record['failed_build_sha256'] == digest(args.qualification)
        assert qualification['status'] == 'failed' and not qualification['qualified_candidate']
        assert qualification['error'] == 'Custom assets changed during the engine build; existing runtime retained'
        assert qualification['validation']['passed'] and archive_record['CTests_passed'] == 109
        assert archive_record['validation_sha256'] == qualification['validation']['report_sha256']
        assert digest(Path(archive_record['base_archive'])) == archive_record['base_archive_sha256']
    else:
        assert workers[-1]['exit_code'] == 0
        assert qualification['passed'] and qualification['qualified_candidate'] and qualification['status'] == 'prepared'
        assert archive_record['runtime'] == qualification['runtime']
        assert archive_record['runtime_sha256'] == qualification['runtime_sha256']
    archive = Path(archive_record['path'])
    assert digest(archive) == archive_record['sha256']
    sys.path.insert(0, str(root / 'tools'))
    from build_launcher_engine import publication_identity, source_hashes
    inventory = source_hashes(source)
    inventory_sha = hashlib.sha256(json.dumps(inventory, sort_keys=True).encode()).hexdigest()
    assert inventory_sha == preflight['camera_inventory_sha256']
    # The archive's authenticated manifest must retain every exact old
    # production byte before any owned source refresh is permitted.
    manifest = None
    manifest_name = 'TESTED_SOURCE_CHECKPOINT.json' if checkpoint else 'QUALIFIED_SOURCE_ARCHIVE.json'
    with tarfile.open(archive, 'r|zst') as stream:
        for member in stream:
            if member.name == manifest_name:
                assert member.isfile() and member.size <= 8 * 1024 * 1024
                manifest = json.load(stream.extractfile(member))
                break
    assert manifest and manifest['production_files'] == len(inventory)
    if checkpoint:
        assert not manifest['engine_qualified'] and manifest['production_inventory_sha256'] == inventory_sha
        assert manifest['failed_build_sha256'] == archive_record['failed_build_sha256']
    else:
        assert manifest['runtime'] == qualification['runtime']
    assert all(manifest['files'][name]['sha256'] == value for name, value in inventory.items())
    selected = json.loads((root / 'gameplay-runtime-selection.json').read_text())
    assert selected['runtime'] == reference['runtime']
    identities = reference['identities']
    assert all(list(publication_identity(root / name)) == value for name, value in identities.items())
    retained = args.reference.parent / 'source'
    originals, replacements, rows = {}, {}, []
    expected = inventory.copy()
    for row in reference['changed_files']:
        name = row['path']
        assert not Path(name).is_absolute() and '..' not in Path(name).parts
        target, original = source / name, retained / name
        before = preflight['originals'][name]
        assert manifest['files'][name]['sha256'] == before['sha256']
        assert target.stat().st_size == before['bytes'] and digest(target) == before['sha256']
        assert digest(original) == row['sha256'] == preflight['selected_delta'][name]['sha256']
        originals[name], replacements[name] = target.read_bytes(), original.read_bytes()
        if row['kind'] == 'production':
            expected[name] = row['sha256']
        rows.append(dict(path=name, before_sha256=before['sha256'], after_sha256=row['sha256'], kind=row['kind']))
    expected_sha = hashlib.sha256(json.dumps(expected, sort_keys=True).encode()).hexdigest()
    assert expected_sha == preflight['future_inventory_sha256']
    assert len(rows) == 7 and sum(row['kind'] == 'production' for row in rows) == 3
    baseline = owner / 'workshop-feedback-16-publication-baseline.json'
    assert not baseline.exists()
    try:
        for name, data in replacements.items():
            replace(source / name, data)
        assert source_hashes(source) == expected
        assert all(digest(source / row['path']) == row['after_sha256'] for row in rows)
        assert all(list(publication_identity(root / name)) == value for name, value in identities.items())
    except BaseException:
        for name, data in originals.items():
            replace(source / name, data)
        assert source_hashes(source) == inventory
        raise
    baseline.write_text(json.dumps(identities, indent=2) + '\n')
    report = dict(scope=__doc__, passed=True, status='refreshed_not_validated',
        source=str(source), previous_inventory_sha256=inventory_sha,
        production_inventory_sha256=expected_sha, production_files=len(expected),
        source_archive=str(args.source_archive), source_archive_sha256=archive_record['sha256'],
        previous_engine_qualified=not checkpoint,
        qualification=str(args.qualification), selected_runtime=selected['runtime'],
        selected_reference=str(args.reference), changes=rows, fresh_modification_times=True,
        Root_publication_identities_unchanged=True, publication_baseline=str(baseline),
        engine_qualified=False, complete_mod_support=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status=report['status'], source=str(source), inventory_sha256=expected_sha)))


if __name__ == '__main__':
    main()
