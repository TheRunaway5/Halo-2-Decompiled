#!/usr/bin/env python3
"""Resolve the real universal image through a relative Unicode symlink."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    executable = Path(sys.argv[1]).resolve(strict=True)
    report_path, arches = Path(sys.argv[2]), sys.argv[3:]
    record = dict(schema='H2_MACOS_EXECUTABLE_PATH_V1', runs=[])
    try:
        with tempfile.TemporaryDirectory(prefix='launcher-path-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
            work = Path(directory)
            destination = work / 'Moved app with spaces Δ' / 'Actual executable Ω'
            destination.parent.mkdir()
            shutil.copy2(executable, destination)
            link = work / 'Launch link λ'
            link.symlink_to(destination.relative_to(work))
            other = work / 'unrelated working directory'
            other.mkdir()
            for arch in arches:
                command = ['/usr/bin/arch', '-' + arch, './' + link.name, str(destination), str(other)]
                result = subprocess.run(command, cwd=work, capture_output=True, text=True, timeout=15)
                record['runs'].append(dict(arch=arch, command=command, exit=result.returncode,
                    stdout=result.stdout[-4096:], stderr=result.stderr[-4096:]))
                if result.returncode:
                    raise RuntimeError('Actual executable lookup failed: ' + result.stderr)
                print(result.stdout, end='')
            record['passed'] = True
    finally:
        report_path.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
