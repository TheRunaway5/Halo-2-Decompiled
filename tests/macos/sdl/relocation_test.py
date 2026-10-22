#!/usr/bin/env python3
"""Run both native resource and Vulkan presentation probes with SDL's SDK absent."""
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys


def run(command, environment):
    result = subprocess.run([str(x) for x in command], text=True, capture_output=True,
                            env=environment, timeout=30)
    if result.returncode:
        raise RuntimeError(result.stdout[-8192:] + result.stderr[-8192:])
    return result


def main():
    app, graphics, sdk = [Path(x).resolve(strict=True) for x in sys.argv[1:4]]
    report_path, arches = Path(sys.argv[4]), sys.argv[5:]
    manifest = json.loads((sdk / 'manifest.json').read_text())
    if manifest['schema'] != 'H2_MACOS_SDL_SDK_V1' or Path(manifest['prefix']) != sdk:
        raise RuntimeError('SDL SDK ownership differs')
    root = Path(os.environ['HALO2_TEST_SCRATCH']) / 'Relocated apps with spaces'
    root.mkdir()
    copies = []
    for source in (app, graphics):
        target = root / source.name
        shutil.copytree(source, target, symlinks=True)
        license_bytes = (target / 'Contents/Resources/licenses/SDL2.txt').read_bytes()
        if hashlib.sha256(license_bytes).hexdigest() != manifest['license_sha256']:
            raise RuntimeError('Bundled SDL source notice differs')
        name = plistlib.loads((target / 'Contents/Info.plist').read_bytes())['CFBundleExecutable']
        copies.append((target, target / 'Contents/MacOS' / name))
    record = dict(schema='H2_MACOS_SDL_RELOCATION_V1', runs=[], passed=False)
    hidden = root / 'original-sdk-hidden'
    sdk.rename(hidden)
    try:
        environment = {k: v for k, v in os.environ.items() if not k.startswith(('DYLD_', 'SDL_'))}
        for target, _ in copies:
            run(['codesign', '--verify', '--deep', '--strict', target], environment)
        for arch in arches:
            target, executable = copies[0]
            result = run(['/usr/bin/arch', '-' + arch, executable, target / 'Contents/Frameworks'], environment)
            resources = json.loads(result.stdout)
            if not resources['passed'] or resources['architecture'] != arch:
                raise RuntimeError('SDL resource result differs')
            target, executable = copies[1]
            graphics_report = root / ('graphics-' + arch + '.json')
            run(['/usr/bin/arch', '-' + arch, executable, '--frames', '3', '--report', graphics_report], environment)
            presented = json.loads(graphics_report.read_text())
            if presented['architecture'] != arch or presented['presented_frames'] != 3:
                raise RuntimeError('Vulkan presentation did not finish')
            for key in ('loaded_sdl', 'loaded_moltenvk'):
                if Path(presented[key]).resolve().parent != (target / 'Contents/Frameworks').resolve():
                    raise RuntimeError('Vulkan probe borrowed an external image')
            record['runs'].append(dict(architecture=arch, resources=resources, graphics=presented))
        record['passed'] = True
    finally:
        hidden.rename(sdk)
        report_path.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
