"""Decode the authenticated literal sky fixture inside the CTest scratch owner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zlib


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo-root',type=Path,required=True)
    p.add_argument('--test-executable',type=Path,required=True)
    a=p.parse_args();directory=a.repo_root/'tests/fixtures'
    metadata=json.loads((directory/'mcc-native-sky-cooked-v1.json').read_bytes())
    name=metadata['compressed_file']
    if (metadata['schema']!='H2_MCC_NATIVE_SKY_COOKED_REGRESSION_V1' or Path(name).name!=name or
            not 0<metadata['compressed_bytes']<=128*1024 or not 0<metadata['decoded_bytes']<=1024*1024):
        raise ValueError('Invalid complete sky reference owner')
    packed=(directory/name).read_bytes()
    if len(packed)!=metadata['compressed_bytes'] or hashlib.sha256(packed).hexdigest()!=metadata['compressed_sha256']:
        raise ValueError('Complete literal sky fixture changed')
    decoder=zlib.decompressobj();raw=decoder.decompress(packed,metadata['decoded_bytes']+1)
    if (len(raw)!=metadata['decoded_bytes'] or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or
            hashlib.sha256(raw).hexdigest()!=metadata['decoded_sha256']):
        raise ValueError('Decoded sky reference differs or has trailing data')
    with tempfile.TemporaryDirectory(prefix='sky-cooked-',dir=os.environ['HALO2_TEST_SCRATCH']) as folder:
        fixture=Path(folder)/'fixture.json';fixture.write_bytes(raw)
        return subprocess.run([str(a.test_executable.resolve(strict=True)),str(fixture)],check=False).returncode


if __name__=='__main__':raise SystemExit(main())
