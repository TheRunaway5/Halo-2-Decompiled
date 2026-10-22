#!/usr/bin/env python3
"""Run the signed decoder check after relocation with the original SDK hidden."""
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
    if manifest.get('schema') != 'H2_MACOS_OPUS_SDK_V1' or Path(manifest['prefix']) != sdk:
        raise RuntimeError('The test does not own the supplied dependency SDK')
    with tempfile.TemporaryDirectory(prefix='relocated-opus-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
        work = Path(directory)
        relocated = work / 'Folder with spaces' / app.name
        shutil.copytree(app, relocated, symlinks=True)
        info = plistlib.loads((relocated / 'Contents/Info.plist').read_bytes())
        executable = relocated / 'Contents/MacOS' / info['CFBundleExecutable']
        library = relocated / 'Contents/Frameworks/libopus.0.dylib'
        license_path = relocated / 'Contents/Resources/licenses/Opus.txt'
        if license_path.read_bytes() != (sdk / 'share/licenses/Opus.txt').read_bytes():
            raise RuntimeError('The bundled Opus license differs from its source input')
        hidden = work / 'original-sdk'
        sdk.rename(hidden)
        try:
            subprocess.run(['codesign', '--verify', '--deep', '--strict', str(relocated)], check=True)
            for arch in sys.argv[3:]:
                subprocess.run(['/usr/bin/arch', '-' + arch, str(executable), str(library)], check=True)
        finally:
            hidden.rename(sdk)
        print('PASS relocated signed MCC decoder bundle, exact Opus license, both runtime slices with original SDK absent')


if __name__ == '__main__':
    main()
