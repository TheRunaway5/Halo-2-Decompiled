#!/usr/bin/env python3
"""Verify real crypto/TLS with the original SDK, module and config paths absent."""
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile


def main():
    app, sdk = (Path(value).resolve(strict=True) for value in sys.argv[1:3])
    report_path, arches = Path(sys.argv[3]), sys.argv[4:]
    manifest = json.loads((sdk / 'manifest.json').read_text())
    if manifest.get('schema') != 'H2_MACOS_OPENSSL_SDK_V1' or Path(manifest['prefix']) != sdk:
        raise RuntimeError('The test does not own the supplied SDK')
    record = dict(schema='H2_MACOS_OPENSSL_RELOCATION_V1', architectures=arches, runs=[], retained_large_artifacts=[])
    try:
        with tempfile.TemporaryDirectory(prefix='relocated-openssl-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
            work = Path(directory)
            relocated = work / 'Folder with spaces' / app.name
            shutil.copytree(app, relocated, symlinks=True)
            info = plistlib.loads((relocated / 'Contents/Info.plist').read_bytes())
            executable = relocated / 'Contents/MacOS' / info['CFBundleExecutable']
            license_path = relocated / 'Contents/Resources/licenses/OpenSSL.txt'
            if license_path.read_bytes() != (sdk / 'share/licenses/OpenSSL.txt').read_bytes():
                raise RuntimeError('The bundled OpenSSL source license differs')
            record['license_sha256'] = hashlib.sha256(license_path.read_bytes()).hexdigest()
            hidden = work / 'original-sdk'
            sdk.rename(hidden)
            try:
                subprocess.run(['codesign', '--verify', '--deep', '--strict', str(relocated)], check=True)
                environment = {k: v for k, v in os.environ.items() if not k.startswith(('DYLD_', 'OPENSSL_'))}
                for arch in arches:
                    # These deliberately absent owned paths prevent host
                    # configuration/provider state from satisfying this check.
                    command = ['/usr/bin/arch', '-' + arch, '/usr/bin/env',
                        'OPENSSL_CONF=' + str(work / 'absent-config'), 'OPENSSL_MODULES=' + str(work / 'absent-modules'),
                        str(executable), str(relocated / 'Contents/Frameworks')]
                    result = subprocess.run(command, env=environment, capture_output=True, text=True)
                    record['runs'].append(dict(arch=arch, command=command, exit=result.returncode,
                        stdout=result.stdout[-8192:], stderr=result.stderr[-4096:]))
                    print(result.stdout, end='')
                    if result.returncode:
                        raise RuntimeError('Relocated crypto/TLS failed: ' + result.stderr[-4096:])
                record['passed'] = True
            finally:
                hidden.rename(sdk)
        print('PASS signed relocated OpenSSL bundle, exact source license, real crypto/TLS on both slices with SDK/config/modules absent')
    finally:
        report_path.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
