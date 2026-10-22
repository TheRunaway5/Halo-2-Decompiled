#!/usr/bin/env python3
"""Exercise all four real codec libraries after relocation with their SDK hidden."""
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
    manifest = json.loads((sdk / 'manifest.json').read_text())
    if manifest.get('schema') != 'H2_MACOS_VORBIS_SDK_V1' or Path(manifest['prefix']) != sdk:
        raise RuntimeError('The test does not own the supplied SDK')
    with tempfile.TemporaryDirectory(prefix='relocated-vorbis-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
        work = Path(directory)
        relocated = work / 'Folder with spaces' / app.name
        shutil.copytree(app, relocated, symlinks=True)
        info = plistlib.loads((relocated / 'Contents/Info.plist').read_bytes())
        executable = relocated / 'Contents/MacOS' / info['CFBundleExecutable']
        for label in ('Ogg', 'Vorbis'):
            if (relocated / f'Contents/Resources/licenses/{label}.txt').read_bytes() != (sdk / f'share/licenses/{label}.txt').read_bytes():
                raise RuntimeError('The bundled license differs: ' + label)
        hidden = work / 'original-sdk'
        sdk.rename(hidden)
        try:
            subprocess.run(['codesign', '--verify', '--deep', '--strict', str(relocated)], check=True)
            environment = {k: v for k, v in os.environ.items() if not k.startswith('DYLD_')}
            for arch in sys.argv[3:]:
                subprocess.run(['/usr/bin/arch', '-' + arch, str(executable),
                                str(relocated / 'Contents/Frameworks')], env=environment, check=True)
        finally:
            hidden.rename(sdk)
    print('PASS signed relocated Ogg/Vorbis closure, exact licenses and four actual library images on both slices with original SDK absent')


if __name__ == '__main__':
    main()
