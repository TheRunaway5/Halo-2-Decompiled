#!/usr/bin/env python3
"""Keep Python-launched child processes on the requested real Mach-O slice.

The universal executable and exact extracted slices all remain in run_tests.py's
private build. The existing launcher regression copies the thin executable;
the host Python architecture cannot accidentally select the other fat slice.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def main():
    source, destination, arch = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
    destination.parent.mkdir(parents=True, exist_ok=True)
    for expected in ('arm64', 'x86_64'):
        subprocess.run(['lipo', str(source), '-verify_arch', expected], check=True)
    subprocess.run(['lipo', str(source), '-thin', arch, '-output', str(destination)], check=True)
    subprocess.run(['codesign', '--force', '--sign', '-', str(destination)], check=True)
    subprocess.run(['codesign', '--verify', '--strict', str(destination)], check=True)
    actual = subprocess.check_output(['lipo', '-archs', str(destination)], text=True).strip()
    if actual != arch:
        raise RuntimeError('The regression would execute the wrong launcher slice')
    record = dict(universal=str(source), universal_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        slice=str(destination), slice_sha256=hashlib.sha256(destination.read_bytes()).hexdigest(), arch=actual)
    destination.with_suffix('.json').write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
