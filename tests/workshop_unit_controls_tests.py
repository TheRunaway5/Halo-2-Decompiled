"""Expand the bounded original reference in the current test's owned scratch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zlib


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--test-executable',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    packed=a.fixture.read_bytes();descriptor=json.loads(a.fixture.with_suffix('').read_text())
    assert len(packed)==descriptor['packed_bytes'] and hashlib.sha256(packed).hexdigest()==descriptor['packed_sha256']
    decoder=zlib.decompressobj();raw=decoder.decompress(packed,4*1024*1024+1)
    assert len(raw)<=4*1024*1024 and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
    assert len(raw)==descriptor['decoded_bytes'] and hashlib.sha256(raw).hexdigest()==descriptor['decoded_sha256']
    reference=scratch/'unit-controls-original.json';reference.write_bytes(raw)
    return subprocess.run([str(a.test_executable),str(reference)]).returncode


if __name__=='__main__':raise SystemExit(main())
