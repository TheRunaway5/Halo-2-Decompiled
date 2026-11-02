"""Retain an exact code-only reference for a qualified frozen engine candidate.

Borrowed assets, vendor trees, builds and captures are excluded. Only the
explicit source archive and bounded receipt survive run_test.py ownership.
"""
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
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.archive.resolve().is_relative_to(scratch)
    assert not args.report.resolve().is_relative_to(scratch)
    qualification = json.loads(args.qualification.read_text())
    assert qualification['passed'] and qualification['status'] == 'prepared' and qualification['qualified_candidate']
    source = Path(qualification['source_cohort']['source']).resolve(strict=True)
    assert source.is_relative_to(root / '.local/test-scratch') and source.name == 'source'
    package = (root / qualification['runtime']).parent
    assert digest(package / 'halo2_native') == qualification['runtime_sha256']
    pins = json.loads((package / 'source-hashes.json').read_text())
    from build_launcher_engine import source_hashes
    assert source_hashes(source) == pins
    paths = {name: source / name for name in pins}
    production_count = len(paths)
    # Follow neither borrowed test directories nor individual borrowed files.
    for folder, directories, files in os.walk(source / 'tests', followlinks=False):
        directories[:] = [name for name in directories if not (Path(folder) / name).is_symlink()]
        for name in files:
            path = Path(folder) / name
            if not path.is_symlink():
                paths[path.relative_to(source).as_posix()] = path
    test_count = len(paths) - production_count
    for path in source.iterdir():
        if path.is_file() and not path.is_symlink() and path.suffix in ('.txt', '.cmake', '.py', '.sh'):
            paths[path.name] = path
    # The current sweep/archive coordinators are external to the sealed
    # production cohort. Preserve them under distinct names, without replacing
    # the cohort's own tool versions or changing its production provenance.
    for name in ('test_workshop_candidate_sweep.py', 'summarize_workshop_candidate_sweep.py',
                 'archive_qualified_engine_source.py'):
        paths['qualification-external/' + name] = root / 'tools' / name
    records = {}
    for name, path in sorted(paths.items()):
        assert not Path(name).is_absolute() and '..' not in Path(name).parts
        assert path.is_file() and not path.is_symlink(), name
        records[name] = dict(bytes=path.stat().st_size, sha256=digest(path))
        if name in pins:
            assert records[name]['sha256'] == pins[name], name
    manifest = (json.dumps(dict(schema='H2_QUALIFIED_CODE_SOURCE_REFERENCE_V1',
        scope=__doc__, runtime=qualification['runtime'], runtime_sha256=qualification['runtime_sha256'],
        production_files=production_count, copied_test_files=test_count, files=records),
        sort_keys=True, indent=2) + '\n').encode()
    stage = scratch / 'source.tar.zst'
    manifest_name = 'QUALIFIED_SOURCE_ARCHIVE.json'
    assert manifest_name not in records
    with tarfile.open(stage, 'w|zst') as archive:
        for name, path in sorted(paths.items()):
            archive.add(path, arcname=name, recursive=False)
        member = tarfile.TarInfo(manifest_name)
        member.size = len(manifest)
        archive.addfile(member, io.BytesIO(manifest))
    expected = dict(records)
    expected[manifest_name] = dict(bytes=len(manifest), sha256=hashlib.sha256(manifest).hexdigest())
    seen, logical_bytes = set(), 0
    with tarfile.open(stage, 'r|zst') as archive:
        for member in archive:
            assert member.isfile() and member.name in expected and member.name not in seen
            row = expected[member.name]
            assert member.size == row['bytes']
            stream = archive.extractfile(member)
            assert stream is not None and hashlib.file_digest(stream, 'sha256').hexdigest() == row['sha256']
            seen.add(member.name)
            logical_bytes += member.size
    assert seen == set(expected)
    assert all(digest(path) == records[name]['sha256'] for name, path in paths.items())
    assert source_hashes(source) == pins
    assert digest(package / 'halo2_native') == qualification['runtime_sha256']
    args.archive.parent.mkdir(parents=True, exist_ok=True)
    assert not args.archive.exists() and not args.archive.is_symlink()
    # Both locations belong to this checkout's filesystem. New-only atomic
    # publication leaves no partial archive outside scratch on interruption.
    assert stage.stat().st_dev == args.archive.parent.stat().st_dev
    os.link(stage, args.archive)
    report = dict(passed=True, scope=__doc__, runtime=qualification['runtime'],
        runtime_sha256=qualification['runtime_sha256'], files=len(expected),
        production_files=production_count, copied_test_files=test_count,
        archive_logical_bytes=logical_bytes, path=str(args.archive.resolve()),
        bytes=args.archive.stat().st_size, sha256=digest(args.archive),
        complete_stream_roundtrip_verified=True, source_inputs_unchanged=True,
        owned_scratch=str(scratch), retained_large_artifacts=[dict(path=str(args.archive.resolve()),
            bytes=args.archive.stat().st_size,
            reason='Required exact frozen source reference for the requested qualified feedback build')])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('files', 'archive_logical_bytes', 'bytes', 'sha256')}))


if __name__ == '__main__':
    main()
