#!/usr/bin/env python3
"""Expand pinned launcher graphics dependencies inside the private CMake build."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools/macos'))
from test_port import INPUTS


def main():
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(143))
    work = Path(sys.argv[1]).resolve()
    work.mkdir(parents=True)
    for name, (url, expected) in INPUTS.items():
        target = work / name
        with urllib.request.urlopen(url, timeout=90) as response, target.open('wb') as output:
            shutil.copyfileobj(response, output)
        with target.open('rb') as stream:
            if hashlib.file_digest(stream, 'sha256').hexdigest() != expected:
                raise RuntimeError('Pinned graphics input differs: ' + name)
    sdk = work / 'sdk'
    sdk.mkdir()
    with tarfile.open(work / 'MoltenVK-macos.tar') as archive:
        archive.extractall(sdk, filter='data')
    mount = work / 'sdl-mount'
    mount.mkdir()
    try:
        subprocess.run(['hdiutil', 'attach', '-nobrowse', '-readonly', '-mountpoint', str(mount),
            str(work / 'SDL2.dmg')], check=True)
        shutil.copytree(mount / 'SDL2.framework', sdk / 'SDL2.framework', symlinks=True)
    finally:
        if os.path.ismount(mount):
            subprocess.run(['hdiutil', 'detach', str(mount)], check=True, timeout=30)
    config = sdk / 'cmake/SDL2'
    config.mkdir(parents=True)
    (config / 'SDL2Config.cmake').write_text('add_library(SDL2::SDL2 SHARED IMPORTED)\n'
        f'set_target_properties(SDL2::SDL2 PROPERTIES IMPORTED_LOCATION "{sdk}/SDL2.framework/SDL2" '
        f'INTERFACE_INCLUDE_DIRECTORIES "{sdk}/SDL2.framework/Headers")\n')
    (work / 'manifest.json').write_text(json.dumps(INPUTS, indent=2) + '\n')


if __name__ == '__main__':
    main()
