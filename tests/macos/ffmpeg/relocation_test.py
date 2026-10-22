#!/usr/bin/env python3
"""Exercise actual production playback with every original owned SDK absent."""
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile


def main():
    app, sdks, fixtures = (Path(x).resolve(strict=True) for x in sys.argv[1:4])
    report_path, arches = Path(sys.argv[4]), sys.argv[5:]
    for name in ('ffmpeg', 'opus', 'openssl'):
        manifest = json.loads((sdks / name / 'manifest.json').read_text())
        if Path(manifest['prefix']) != sdks / name:
            raise RuntimeError('SDK ownership differs')
    record = dict(schema='H2_MACOS_FFMPEG_RELOCATION_V1', runs=[], retained_large_artifacts=[])
    try:
        with tempfile.TemporaryDirectory(prefix='relocated-ffmpeg-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
            work = Path(directory)
            relocated = work / 'Folder with spaces' / app.name
            shutil.copytree(app, relocated, symlinks=True)
            info = plistlib.loads((relocated / 'Contents/Info.plist').read_bytes())
            executable = relocated / 'Contents/MacOS' / info['CFBundleExecutable']
            for before, after in [('FFmpeg-COPYING.LGPLv2.1', 'FFmpeg-LGPL-2.1.txt'),
                ('FFmpeg-COPYING.LGPLv3', 'FFmpeg-LGPL-3.txt'), ('FFmpeg-LICENSE.md', 'FFmpeg-notices.txt'),
                ('Dav1d.txt', 'Dav1d.txt')]:
                if (sdks / 'ffmpeg/share/licenses' / before).read_bytes() != (relocated / 'Contents/Resources/licenses' / after).read_bytes():
                    raise RuntimeError('A playback source license differs')
            hidden = work / 'original-sdks'
            sdks.rename(hidden)
            try:
                subprocess.run(['codesign', '--verify', '--deep', '--strict', str(relocated)], check=True)
                environment = {k: v for k, v in os.environ.items() if not k.startswith(('DYLD_', 'OPENSSL_', 'HALO2_MOVIE_'))}
                for arch in arches:
                    command = ['/usr/bin/arch', '-' + arch, str(executable), str(fixtures), str(relocated / 'Contents/Frameworks')]
                    result = subprocess.run(command, capture_output=True, text=True, env=environment, timeout=30)
                    record['runs'].append(dict(arch=arch, command=command, exit=result.returncode,
                        stdout=result.stdout[-16384:], stderr=result.stderr[-8192:]))
                    print(result.stdout, end='')
                    if result.returncode:
                        raise RuntimeError('Relocated production playback failed: ' + result.stderr[-8192:])
                record['passed'] = True
            finally:
                hidden.rename(sdks)
    finally:
        report_path.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
