"""Decode the pinned finite vehicle source fixture inside an owned test lifecycle."""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, required=True)
    parser.add_argument('--test-executable', type=Path, required=True)
    args = parser.parse_args()
    root = args.repo_root.resolve(strict=True)
    metadata_path = root / 'tests/fixtures/mcc-native-vehicle-restoration-v1.json'
    metadata = json.loads(metadata_path.read_bytes())
    if (set(metadata) != {'schema', 'compressed_file', 'compressed_bytes', 'compressed_sha256',
                          'decoded_bytes', 'decoded_sha256', 'scope'} or
            metadata['schema'] != 'HALO2_MCC_NATIVE_VEHICLE_FIXTURE_OWNER_V1' or
            metadata['compressed_file'] != 'mcc-native-vehicle-restoration-v1.json.xz' or
            not 0 < metadata['compressed_bytes'] <= 1024 * 1024 or
            not 0 < metadata['decoded_bytes'] <= 32 * 1024 * 1024):
        raise ValueError('Invalid finite vehicle source fixture owner')
    packed = (metadata_path.parent / metadata['compressed_file']).read_bytes()
    if len(packed) != metadata['compressed_bytes'] or hashlib.sha256(packed).hexdigest() != metadata['compressed_sha256']:
        raise ValueError('Complete compressed vehicle source fixture changed')
    decoder = lzma.LZMADecompressor()
    raw = decoder.decompress(packed, max_length=metadata['decoded_bytes'] + 1)
    if (not decoder.eof or decoder.unused_data or len(raw) != metadata['decoded_bytes'] or
            hashlib.sha256(raw).hexdigest() != metadata['decoded_sha256']):
        raise ValueError('Complete literal vehicle fixture changed or has unowned trailing bytes')
    with tempfile.TemporaryDirectory(prefix='vehicle-restoration-', dir=os.environ['HALO2_TEST_SCRATCH']) as folder:
        fixture = Path(folder) / 'fixture.json'
        fixture.write_bytes(raw)
        return subprocess.run([str(args.test_executable.resolve(strict=True)), str(fixture),
            str(root / 'tools/data/mcc-native-vehicle-restoration.json')], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
