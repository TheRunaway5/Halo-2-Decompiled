"""Stage only the nineteen required native programs in the CTest scratch owner."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zlib


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--test-executable', type=Path, required=True)
    parser.add_argument('--repo-root', type=Path, required=True)
    args = parser.parse_args()
    root = args.repo_root.resolve(strict=True)
    policy = json.loads((root/'tools/data/mcc-native-pc-sky-closure.json').read_bytes())
    fixture = {'policy': policy,
               'regions': json.loads((root/'tests/fixtures/mcc-pc-sky-closure-regions.json').read_bytes()),
               'hud': json.loads((root/'tools/data/mcc-native-hud-bootstrap.json').read_bytes())}
    report = json.loads((root/'tools/data/native-materials.json').read_bytes())
    packed = (root/'tools/data/native-materials.json.zlib').read_bytes()
    assert len(packed) == report['bytes'] and hashlib.sha256(packed).hexdigest() == report['sha256']
    decoder = zlib.decompressobj()
    raw = decoder.decompress(packed, 192 * 1024 * 1024 + 1)
    assert len(raw) == report['decoded_bytes'] and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
    bundle = json.loads(raw)
    with tempfile.TemporaryDirectory(prefix='pc-sky-closure-', dir=os.environ['HALO2_TEST_SCRATCH']) as folder:
        owned = Path(folder)
        for row in policy['maps']:
            library = bundle['maps'][row['source_map']]
            canonical = json.dumps(library, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()
            assert hashlib.sha256(canonical).hexdigest() == row['source_canonical_sha256']
            target = owned/'native-shaders'/row['source_map']
            (target/'payloads').mkdir(parents=True)
            (target/'library.json').write_bytes(canonical)
            for record in row['records']:
                payload = base64.b64decode(bundle['payloads'][record['payload'][:-4]], validate=True)
                assert hashlib.sha256(payload).hexdigest() + '.bin' == record['payload'] and len(payload) == record['bytes']
                (target/'payloads'/record['payload']).write_bytes(payload)
        del bundle, raw, packed
        fixture_path = owned/'fixture.json'
        fixture_path.write_text(json.dumps(fixture))
        return subprocess.run([str(args.test_executable.resolve(strict=True)), str(fixture_path), str(owned/'native-shaders')], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
