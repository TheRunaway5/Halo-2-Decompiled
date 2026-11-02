#!/usr/bin/env python3
"""Decompress authenticated numeric references only inside owned test scratch."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('probe','reference','counts','manifest','report'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    pins=json.loads(a.manifest.read_text());assert pins['original_xbe_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    def unpack(path,key):
        packed=path.read_bytes();raw=gzip.decompress(packed)
        assert hashlib.sha256(packed).hexdigest()==pins[key]['compressed_sha256']
        assert hashlib.sha256(raw).hexdigest()==pins[key]['raw_sha256']
        return raw
    reference=unpack(a.reference,'abi_reference');counts=unpack(a.counts,'count_vectors')
    with tempfile.TemporaryDirectory(prefix='native-region-reference-',dir=owner) as context:
        work=Path(context);(work/'reference.json').write_bytes(reference);(work/'counts.bin').write_bytes(counts)
        result=subprocess.run([str(a.probe.resolve()),str(work/'reference.json'),str(work/'counts.bin'),str(a.report.resolve())],timeout=120)
    assert not work.exists()
    return result.returncode


if __name__=='__main__':raise SystemExit(main())
