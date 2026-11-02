"""Freeze required literal metadata; remove its raw owner on exit."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import zlib


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared-root', type=Path, required=True)
    parser.add_argument('--authority', type=Path, required=True)
    parser.add_argument('--output-prefix', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    raw_path = owner / 'tiling-fixture.json'
    subprocess.run([sys.executable, '-B', str(root / 'tools/export_mcc_native_tiling_specular_fixture.py'),
        '--prepared-root', str(args.prepared_root), '--authority', str(args.authority), '--output', str(raw_path)], check=True)
    raw = raw_path.read_bytes()
    packed = zlib.compress(raw, 9)
    assert len(raw) <= 32 * 1024 * 1024 and len(packed) <= 256 * 1024 and zlib.decompress(packed) == raw
    target = args.output_prefix.with_suffix('.json.zlib')
    metadata = dict(schema='H2_MCC_NATIVE_TILING_SPECULAR_REGRESSION_V1', scope=__doc__,
        compressed_file=target.name, compressed_bytes=len(packed), compressed_sha256=sha(packed),
        decoded_bytes=len(raw), decoded_sha256=sha(raw), authority_sha256=sha(args.authority.read_bytes()),
        profile_sha256=sha((root / 'tools/data/mcc-native-tiling-specular-profiles.json').read_bytes()),
        exporter_sha256=sha((root / 'tools/export_mcc_native_tiling_specular_fixture.py').read_bytes()),
        authored_rows=14, paired_original_rows=2, native_template_variants=2, original_packing_cases=3689,
        retained_reference_reason='Required authored metadata, independent original banks and original packing results for portable regression')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(packed)
    args.output_prefix.with_suffix('.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps(dict(required_metadata_fixture=str(target), compressed_bytes=len(packed), decoded_bytes=len(raw),
                         raw_owned_scratch=str(owner), retained_large_artifacts=[])))


if __name__ == '__main__':
    main()
