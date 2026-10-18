#!/usr/bin/env python3
"""Keep exact GPU JSON bytes in compressed form; remove redundant raw copies.

Run via tools/run_test.py. Only event/actions/textures JSON larger than64KiB
are candidates. Preserve original raw hashes and verify gzip reconstruction.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inspection', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    folder = args.inspection.absolute()
    assert json.loads((folder / 'report.json').read_text())['passed']
    receipt = json.loads((folder / 'retention.json').read_text())
    manifest = folder / 'compression.json'; assert not manifest.exists()
    records = {}
    for path in sorted(folder.glob('*.json')):
        if not (path.name.startswith('event-') or path.name in ('actions.json', 'textures.json')):
            continue
        if path.stat().st_size <= 65536:
            continue
        raw = path.read_bytes(); digest = hashlib.sha256(raw).hexdigest()
        assert receipt['files'][path.name]['sha256'] == digest
        compressed = gzip.compress(raw, compresslevel=6, mtime=0)
        assert gzip.decompress(compressed) == raw
        destination = path.with_name(path.name + '.gz'); assert not destination.exists()
        temporary = scratch / destination.name
        temporary.write_bytes(compressed); os.replace(temporary, destination)
        assert hashlib.sha256(gzip.decompress(destination.read_bytes())).hexdigest() == digest
        records[path.name] = dict(compressed_file=destination.name, raw_bytes=len(raw), raw_sha256=digest,
            compressed_bytes=len(compressed), compressed_sha256=hashlib.sha256(compressed).hexdigest())
        # Persist the exact recovery information before retiring the raw JSON.
        manifest.write_text(json.dumps(dict(files=records, all_exact_raw_bytes_recoverable=True), indent=2) + '\n')
        path.unlink()
    shutil.copyfile(Path(__file__), folder / 'compression-tool-source.py')
    print(json.dumps(dict(files=len(records), raw_bytes=sum(r['raw_bytes'] for r in records.values()),
        compressed_bytes=sum(r['compressed_bytes'] for r in records.values()), runner_scratch=str(scratch),
        all_exact_raw_bytes_recoverable=True, folder=str(folder))), flush=True)


if __name__ == '__main__':
    main()
