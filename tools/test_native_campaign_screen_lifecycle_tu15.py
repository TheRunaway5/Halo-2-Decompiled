#!/usr/bin/env python3
"""Run semantic screen lifecycle comparisons against a retained original fixture."""
import argparse
import gzip
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='screen-lifecycle-',dir=os.environ.get('HALO2_TEST_SCRATCH')) as owned:
        fixture=Path(owned)/'original.json'
        fixture.write_bytes(gzip.decompress(args.fixture.read_bytes()))
        return subprocess.run([str(args.probe.resolve()),str(fixture)],check=False).returncode


if __name__=='__main__':
    raise SystemExit(main())
