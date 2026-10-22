#!/usr/bin/env python3
"""Check the actual signed engine CLI and loaded dylibs after bundle relocation."""
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys


def main():
    app = Path(sys.argv[1]).resolve(strict=True)
    arch, report_path = sys.argv[2], Path(sys.argv[3])
    assert arch in ('arm64', 'x86_64')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    moved = scratch / 'Relocated engine Ω.app'
    shutil.copytree(app, moved, symlinks=True)
    contents = moved / 'Contents'
    assert not (contents / 'MacOS/data').exists(), 'Tag data was copied into the code directory'
    assert (contents / 'Resources/data/tags').is_dir()
    assert (contents / 'Resources/shaders').is_dir()
    info = plistlib.loads((contents / 'Info.plist').read_bytes())
    executable = contents / 'MacOS' / info['CFBundleExecutable']
    subprocess.run(['codesign', '--verify', '--deep', '--strict', str(moved)], check=True)
    env = {key: value for key, value in os.environ.items() if not key.startswith('DYLD_')}
    # arch is a protected system executable: set the diagnostic in its child
    # environment, after the launcher's own DYLD-variable sanitization.
    command = ['/usr/bin/arch', '-' + arch, '-e', 'DYLD_PRINT_LIBRARIES=1', str(executable), '--help']
    run = subprocess.run(command, cwd=scratch, env=env, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, text=True, timeout=30)
    assert run.returncode == 0, run.stderr[-8000:]
    assert 'halo2_native [--map PATH]' in run.stdout, 'The engine did not execute its actual CLI parser'
    loaded = []
    for line in run.stderr.splitlines():
        marker = line.find(' /')
        if line.startswith('dyld[') and marker >= 0:
            loaded.append(line[marker + 1:].strip())
    assert loaded, 'The CLI startup did not produce a dylib load trace'
    system_roots = ('/System/Library/', '/usr/lib/', '/Library/Apple/',
                    '/System/Volumes/Preboot/Cryptexes/OS/')
    for name in loaded:
        if not name.startswith(system_roots):
            assert Path(name).resolve().is_relative_to(contents), 'External runtime library loaded: ' + name
    for name in ('libMoltenVK.dylib', 'libSDL2-2.0.0.dylib'):
        assert any(Path(path).name == name and Path(path).resolve().is_relative_to(contents)
                   for path in loaded), 'Bundled runtime was not loaded: ' + name
    report = {'passed': True, 'architecture': arch, 'command': command, 'exit_code': run.returncode,
              'strict_signature_verified': True, 'relocated': True, 'loaded_images': loaded,
              'cli_help_sha256': hashlib.sha256(run.stdout.encode()).hexdigest(),
              'executable_bytes': executable.stat().st_size,
              'scope': 'Actual engine CLI startup, signed relocation and dylib loading; no renderer, gameplay, audio, input or display acceptance.'}
    report_path.write_text(json.dumps(report, indent=2)+'\n')
    print('PASS actual engine CLI, strict signature and relocated runtime closure:', arch)


if __name__ == '__main__':
    main()
