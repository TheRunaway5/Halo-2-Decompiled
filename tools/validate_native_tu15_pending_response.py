#!/usr/bin/env python3
"""Run native word/ABI checks with disposable decompressed numeric references."""
import argparse,gzip,hashlib,json,os,subprocess,tempfile
from pathlib import Path


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--vectors',type=Path,required=True);p.add_argument('--reference',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    fixture=json.loads(a.reference.read_text());packed=a.vectors.read_bytes();raw=gzip.decompress(packed)
    assert hashlib.sha256(packed).hexdigest()==fixture['vector_sha256']
    assert hashlib.sha256(raw).hexdigest()==fixture['vector_raw_sha256'] and len(raw)==12+65536*6
    with tempfile.TemporaryDirectory(prefix='pending-vectors-',dir=scratch) as owned:
        path=Path(owned)/'words.bin';path.write_bytes(raw)
        result=subprocess.run([str(a.probe.resolve()),str(path),str(a.reference.resolve()),str(a.report.resolve())],timeout=60)
    assert not Path(owned).exists()
    return result.returncode


if __name__=='__main__':raise SystemExit(main())
