"""Preserve exact tested Workshop code after an asset-only packaging refusal.

Store only the changed regular code/test files relative to the authenticated
previous source archive. Verify the complete resulting file inventory by a
stream round trip. This is a source checkpoint, never a qualified runtime.
Run through run_test.py; no build, vendor tree or asset bank is retained.
"""
if not __debug__:
    raise RuntimeError('Tested source checkpoints require enabled assertion guards')

import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import sys
import tarfile


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--failed-build', type=Path, required=True)
    parser.add_argument('--recovery', type=Path, required=True)
    parser.add_argument('--base-archive', type=Path, required=True)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.archive.resolve().is_relative_to(owner) and not args.report.resolve().is_relative_to(owner)
    assert not args.archive.exists() and not args.report.exists()
    failure = json.loads(args.failed_build.read_text())
    assert failure['status'] == 'failed' and not failure['qualified_candidate'] and not failure['runtime_installed']
    assert failure['error'] == 'Custom assets changed during the engine build; existing runtime retained'
    assert failure['validation']['passed'] and not failure['unresolved_case_failures'] and not failure['accepted_case_failures']
    validation = Path(failure['validation']['report'])
    assert digest(validation) == failure['validation']['report_sha256']
    result = json.loads(validation.read_text())
    assert result['passed'] and not result['validation_not_run']
    ctest_log = validation.with_name('tests.log')
    assert '100% tests passed out of 109' in ctest_log.read_text()
    recovery = json.loads(args.recovery.read_text())
    source = Path(recovery['source']).resolve(strict=True)
    assert source == Path(failure['source_cohort']['source']) and source.is_relative_to(root / '.local/test-scratch')
    journal = json.loads((args.recovery.parent / 'session/session.json').read_text())
    workers = [row for row in journal['commands'] if any(
        Path(word).name == 'build_launcher_engine.py' for word in row.get('argv', []))]
    assert journal['status'] == 'active' and workers and workers[-1]['exit_code'] == 1
    assert workers[-1].get('finished') is not None
    assert Path(journal['scratch_owner']) == source.parent
    assert not (root / failure['runtime']).parent.exists()
    sys.path.insert(0, str(root / 'tools'))
    from build_launcher_engine import source_hashes
    pins = source_hashes(source)
    inventory_sha = hashlib.sha256(json.dumps(pins, sort_keys=True).encode()).hexdigest()
    assert inventory_sha == recovery['production_inventory_sha256']
    assert json.loads((args.failed_build.parent / 'source-hashes.json').read_text()) == pins
    paths = {name: source / name for name in pins}
    for folder, directories, files in os.walk(source / 'tests', followlinks=False):
        directories[:] = [name for name in directories if not (Path(folder) / name).is_symlink()]
        for name in files:
            path = Path(folder) / name
            if not path.is_symlink():
                paths[path.relative_to(source).as_posix()] = path
    for path in source.iterdir():
        if path.is_file() and not path.is_symlink() and path.suffix in ('.txt', '.cmake', '.py', '.sh'):
            paths[path.name] = path
    records = {}
    for name, path in paths.items():
        assert not path.is_symlink() and path.is_file()
        records[name] = dict(bytes=path.stat().st_size, sha256=digest(path))
        if name in pins:
            assert records[name]['sha256'] == pins[name]
    # The base archive's literal member bytes are authority here, including
    # historical archives whose embedded manifest names an ancestor package.
    base_pin = digest(args.base_archive)
    preparation = root / recovery['original_preparation']
    assert digest(preparation) == recovery['original_preparation_sha256']
    assert base_pin == json.loads(preparation.read_text())['archive_sha256']
    base = {}
    with tarfile.open(args.base_archive, 'r|zst') as stream:
        for member in stream:
            assert member.isfile() and member.name not in base
            base[member.name] = dict(bytes=member.size, sha256=hashlib.file_digest(stream.extractfile(member), 'sha256').hexdigest())
    delta = {name: row for name, row in records.items() if base.get(name) != row}
    omitted = sorted(set(base) - set(records))
    manifest_name = 'TESTED_SOURCE_CHECKPOINT.json'
    assert manifest_name not in records and manifest_name not in base
    manifest = dict(schema='H2_WORKSHOP_TESTED_SOURCE_CHECKPOINT_V1', scope=__doc__,
        production_inventory_sha256=inventory_sha, production_files=len(pins), files=records,
        delta_files=sorted(delta), omitted_base_members=omitted,
        base_archive=str(args.base_archive.resolve()), base_archive_sha256=base_pin,
        failed_build_sha256=digest(args.failed_build), validation_sha256=digest(validation),
        engine_qualified=False)
    encoded = (json.dumps(manifest, sort_keys=True, indent=2) + '\n').encode()
    stage = owner / 'checkpoint.tar.zst'
    with tarfile.open(stage, 'w|zst') as stream:
        for name in sorted(delta):
            stream.add(paths[name], arcname=name, recursive=False)
        member = tarfile.TarInfo(manifest_name)
        member.size = len(encoded)
        stream.addfile(member, io.BytesIO(encoded))
    rebuilt = {name: row for name, row in base.items() if name not in omitted}
    seen = set()
    with tarfile.open(stage, 'r|zst') as stream:
        for member in stream:
            assert member.isfile() and member.name not in seen
            seen.add(member.name)
            if member.name == manifest_name:
                assert stream.extractfile(member).read() == encoded
                continue
            row = dict(bytes=member.size, sha256=hashlib.file_digest(stream.extractfile(member), 'sha256').hexdigest())
            assert row == delta[member.name]
            rebuilt[member.name] = row
    assert seen == set(delta) | {manifest_name} and rebuilt == records
    assert all(digest(path) == records[name]['sha256'] for name, path in paths.items())
    assert source_hashes(source) == pins and digest(args.base_archive) == base_pin
    assert stage.stat().st_dev == args.archive.parent.stat().st_dev
    os.link(stage, args.archive)
    report = dict(passed=True, status='tested_source_checkpoint', scope=__doc__,
        source=str(source), production_inventory_sha256=inventory_sha, production_files=len(pins),
        files=len(records), delta_files=len(delta), base_archive=str(args.base_archive.resolve()),
        base_archive_sha256=base_pin, path=str(args.archive.resolve()), bytes=args.archive.stat().st_size,
        sha256=digest(args.archive), complete_stream_roundtrip_verified=True,
        failed_build_sha256=digest(args.failed_build), validation_sha256=digest(validation),
        CTests_passed=109, engine_qualified=False, runtime_published=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, delta_files=len(delta), checkpoint_bytes=report['bytes'], inventory=inventory_sha)))


if __name__ == '__main__':
    main()
