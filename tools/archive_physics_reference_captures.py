#!/usr/bin/env python3
"""Losslessly archive explicitly selected, completed physics reference captures.

Run through tools/run_test.py. Every file is hashed and unpacked under owned
scratch before an unpacked input tree can be retired. Assets, source snapshots,
reports, selected runtimes and unselected reference inputs are never swept.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import shutil
import subprocess
import sys


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def files_at(path):
    result = []
    for entry in sorted(path.rglob('*')):
        if entry.is_symlink():
            raise ValueError(f'Reference input contains a link: {entry}')
        if entry.is_file():
            result.append(entry)
    return result


def allocated(paths):
    return sum(p.stat().st_blocks * 512 for p in paths)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort', type=Path, required=True)
    parser.add_argument('--capture', action='append', required=True,
                        help='Explicit cohort-relative required-failure-inputs directory')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--retire-unpacked', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    cohort = args.cohort.resolve(strict=True)
    destination = args.output.resolve()
    if destination.exists():
        raise ValueError('Archive output must be a new directory')
    if destination.is_relative_to(scratch):
        raise ValueError('Explicitly retained archive must survive owned scratch cleanup')
    captures = []
    for selected in args.capture:
        relative = Path(selected)
        path = cohort / relative
        if relative.is_absolute() or '..' in relative.parts or not path.is_dir():
            raise ValueError('Capture must be an existing cohort-relative directory')
        if path.is_symlink() or path.name != 'required-failure-inputs' or path.parent.name != 'original':
            raise ValueError('Only an explicit completed failure-input tree may be selected')
        if not path.resolve().is_relative_to(cohort):
            raise ValueError('Capture escaped cohort')
        captures.append(path)
    if len(set(captures)) != len(captures):
        raise ValueError('Duplicate capture selection')
    # Keep archival below interactive/compiler priority without starving it.
    if hasattr(os, 'setpriority'):
        os.setpriority(os.PRIO_PROCESS, 0, max(os.getpriority(os.PRIO_PROCESS, 0), 6))
    all_files = {}
    rows = []
    for capture in captures:
        manifest = {}
        selected = files_at(capture)
        for path in selected:
            key = str(path.relative_to(cohort))
            record = dict(bytes=path.stat().st_size, sha256=sha(path))
            manifest[key] = record
            all_files[key] = record
        rows.append(dict(relative_path=str(capture.relative_to(cohort)), files=len(manifest),
                         logical_bytes=sum(x['bytes'] for x in manifest.values()),
                         allocated_bytes=allocated(selected), manifest=manifest,
                         metadata_sha256=sha(capture / 'capture.json')))
    listing = scratch / 'archive-files.nul'
    listing.write_bytes(b''.join(key.encode() + b'\0' for key in all_files))
    temporary_archive = scratch / 'actual-failed-captures.tar.zst'
    command = ['tar', '-I', 'zstd -T1 -6', '-cf', str(temporary_archive), '-C', str(cohort),
               '--null', '--verbatim-files-from', '-T', str(listing)]
    subprocess.run(command, check=True)
    restored = scratch / 'roundtrip'
    restored.mkdir()
    subprocess.run(['tar', '--zstd', '-xf', str(temporary_archive), '-C', str(restored)], check=True)
    restored_files = {str(p.relative_to(restored)) for p in files_at(restored)}
    if restored_files != set(all_files):
        raise ValueError('Archive contains a missing or unexpected file')
    for key, expected in all_files.items():
        for path in (restored / key, cohort / key):
            if path.stat().st_size != expected['bytes'] or sha(path) != expected['sha256']:
                raise ValueError(f'Archive/source verification changed: {key}')
    destination.mkdir(parents=True)
    archive = destination / temporary_archive.name
    partial = destination / (temporary_archive.name + '.partial')
    try:
        shutil.copyfile(temporary_archive, partial)
        if sha(partial) != sha(temporary_archive):
            raise ValueError('Retained archive changed while copying')
        partial.rename(archive)
    finally:
        partial.unlink(missing_ok=True)
    archive_record = dict(path=str(archive), bytes=archive.stat().st_size,
                          allocated_bytes=archive.stat().st_blocks * 512, sha256=sha(archive))
    replacement_files = []
    for index, row in enumerate(rows):
        manifest_path = destination / f'capture-{index:02d}-all-file-hashes.json.xz'
        original = json.dumps(row.pop('manifest'), sort_keys=True).encode()
        packed = lzma.compress(original)
        if lzma.decompress(packed) != original:
            raise ValueError('Hash manifest compression changed data')
        manifest_path.write_bytes(packed)
        row['manifest'] = dict(path=str(manifest_path), bytes=len(packed), sha256=sha(manifest_path))
        capture = cohort / row['relative_path']
        metadata = capture.parent / 'archived-capture-metadata.json.xz'
        if metadata.exists():
            raise ValueError('Existing metadata must not be overwritten')
        raw = (capture / 'capture.json').read_bytes()
        packed = lzma.compress(raw)
        if lzma.decompress(packed) != raw:
            raise ValueError('Capture metadata compression changed data')
        metadata.write_bytes(packed)
        redirect = capture.parent / 'required-input-archive.json'
        if redirect.exists():
            raise ValueError('Existing redirect must not be overwritten')
        receipt = dict(passed=True, archive=archive_record, archive_prefix=row['relative_path'],
                       all_file_hashes=row['manifest'], original_metadata_sha256=row['metadata_sha256'],
                       compressed_metadata=str(metadata), all_files_roundtrip_hash_verified=True,
                       unpack_command=['tar', '--zstd', '-xf', str(archive), '-C', 'OWNED_TEST_SCRATCH'],
                       captured_after_role='Recorded failing native candidate; independent original execution computes physics outcomes.')
        redirect.write_text(json.dumps(receipt, indent=2) + '\n')
        row.update(redirect=str(redirect), compressed_metadata=str(metadata))
        replacement_files.extend([metadata, redirect])
    if args.retire_unpacked:
        # Full content was checked again immediately before the retained archive
        # was copied. Only these explicitly selected owned trees are removed.
        for capture in captures:
            shutil.rmtree(capture)
        if any(p.exists() for p in captures):
            raise ValueError('An explicitly retired capture still exists')
    report = dict(passed=True, archive=archive_record, captures=rows, regular_files=len(all_files),
                  logical_bytes=sum(v['bytes'] for v in all_files.values()),
                  unpacked_allocated_bytes=sum(r['allocated_bytes'] for r in rows),
                  source_and_full_unpacked_hashes_verified=True,
                  unpacked_sources_retired=args.retire_unpacked, command=command,
                  owned_scratch=str(scratch), helper_sha256=sha(Path(__file__)),
                  retained_large_artifacts=[archive_record], source_reports_changed=False)
    path = destination / 'cleanup-verification.json'
    for _ in range(4):
        path.write_text(json.dumps(report, indent=2) + '\n')
        retained = allocated(files_at(destination) + replacement_files)
        report.update(replacement_allocated_bytes=retained,
                      scoped_net_allocated_reduction=(report['unpacked_allocated_bytes'] - retained)
                      if args.retire_unpacked else None)
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ['passed', 'regular_files', 'logical_bytes',
          'unpacked_allocated_bytes', 'replacement_allocated_bytes', 'scoped_net_allocated_reduction',
          'owned_scratch']}, indent=2))


if __name__ == '__main__':
    main()
