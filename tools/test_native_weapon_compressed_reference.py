#!/usr/bin/env python3
"""Decompress a bounded native weapon CPU reference in owned test scratch."""
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
    parser.add_argument('mode',nargs='?')
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='weapon-reference-',dir=os.environ.get('HALO2_TEST_SCRATCH')) as owned:
        fixture=Path(owned)/'original.json'
        fixture.write_bytes(gzip.decompress(args.fixture.read_bytes()))
        command=[str(args.probe.resolve()),str(fixture)]
        if args.mode:command.append(args.mode)
        return subprocess.run(command,check=False).returncode


if __name__=='__main__':raise SystemExit(main())
