#!/usr/bin/env python3
"""Verify a signed decoder with its build SDK absent and optional owned MCC input."""
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys


def main():
    app, sdk = [Path(x).resolve(strict=True) for x in sys.argv[1:3]]
    output, map_path, arches = Path(sys.argv[3]), sys.argv[4], sys.argv[5:]
    manifest = json.loads((sdk / 'manifest.json').read_text())
    if manifest['schema'] != 'H2_MACOS_LIBDEFLATE_SDK_V1' or Path(manifest['prefix']) != sdk:
        raise RuntimeError('Decoder SDK ownership differs')
    root = Path(os.environ['HALO2_TEST_SCRATCH'])
    relocated = root / 'Folder with spaces' / app.name
    shutil.copytree(app, relocated, symlinks=True)
    license_bytes = (relocated / 'Contents/Resources/licenses/Libdeflate.txt').read_bytes()
    if hashlib.sha256(license_bytes).hexdigest() != manifest['license_sha256']:
        raise RuntimeError('Bundled libdeflate source notice differs')
    name = plistlib.loads((relocated / 'Contents/Info.plist').read_bytes())['CFBundleExecutable']
    executable = relocated / 'Contents/MacOS' / name
    record = dict(schema='H2_MACOS_LIBDEFLATE_RELOCATION_V1', runs=[], passed=False)
    hidden = root / 'original-sdk'
    sdk.rename(hidden)
    try:
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(relocated)], check=True)
        environment = {k: v for k, v in os.environ.items() if not k.startswith('DYLD_')}
        for arch in arches:
            command = ['/usr/bin/arch', '-' + arch, str(executable), str(relocated / 'Contents/Frameworks')]
            if map_path:
                command.append(map_path)
            result = subprocess.run(command, capture_output=True, text=True, timeout=60, env=environment)
            if result.returncode:
                raise RuntimeError(result.stdout[-8192:] + result.stderr[-8192:])
            decoder = json.loads(result.stdout)
            if not decoder['passed'] or decoder['architecture'] != arch:
                raise RuntimeError('Relocated decoder result differs')
            if map_path and not decoder.get('owned_map', {}).get('all_bytes_equal'):
                raise RuntimeError('The selected owned map was not compared')
            record['runs'].append(dict(architecture=arch, command=command, decoder=decoder))
        record['passed'] = True
    finally:
        hidden.rename(sdk)
        output.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
