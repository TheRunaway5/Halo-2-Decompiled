#!/usr/bin/env python3
"""Skip an owned-data comparison only when its external fixture is absent."""
import stat
import subprocess
import sys
from pathlib import Path


def main():
    if len(sys.argv) < 3:
        print('usage: run_owned_fixture_test.py fixture executable [arguments...]', file=sys.stderr)
        return 2
    fixture = Path(sys.argv[1])
    try:
        mode = fixture.stat().st_mode
    except FileNotFoundError:
        print('Owned comparison fixture is not installed: '+str(fixture))
        return 77
    if not stat.S_ISREG(mode):
        print('Owned comparison fixture is not a regular file: '+str(fixture), file=sys.stderr)
        return 1
    return subprocess.run([sys.argv[2], str(fixture), *sys.argv[3:]], check=False).returncode


if __name__ == '__main__':
    sys.exit(main())
