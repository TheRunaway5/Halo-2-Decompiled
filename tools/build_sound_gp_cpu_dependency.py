#!/usr/bin/env python3
"""Build the pinned GP instruction dependency as a separate serial stage.

Use the normal 1 GiB/zero-swap build container. This replaces itself with
Cargo, avoiding nested CMake/Make process overhead; it does not change any
memory limit, global toolchain, game state, device readiness or DSP program.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
REVISION = '82fb25a6917a36ac66212ae31f02aab62211f979'


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(65536), b''):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target-directory', type=Path, required=True)
    parser.add_argument('--rustup-home', type=Path)
    parser.add_argument('--cargo', default=shutil.which('cargo'))
    args = parser.parse_args()
    if not args.cargo:
        raise RuntimeError('Cargo or an explicitly supplied pinned static library is required')
    source = ROOT / 'third_party/dsp56300'
    origin = json.loads((source / 'HALO2_ORIGIN.json').read_text())
    assert origin['revision'] == REVISION and origin['license'] == 'MIT'
    for name, expected in origin['upstream_files_sha256'].items():
        path = Path(name)
        assert not path.is_absolute() and '..' not in path.parts
        assert sha(source / path) == expected, f'Pinned DSP dependency changed: {name}'
    output = args.target_directory.resolve()
    output.mkdir(parents=True, exist_ok=False)
    command = [str(args.cargo), '+1.99.0', 'build', '--manifest-path', str(source / 'Cargo.toml'),
               '--locked', '--release', '-p', 'dsp56300-emu-ffi', '--no-default-features',
               '--config', 'profile.release.package.dsp56300-core.opt-level=2',
               '--config', 'profile.release.package.dsp56300-emu.opt-level=2',
               '--config', 'profile.release.package.dsp56300-emu-ffi.opt-level=2',
               '-j', '1', '--target-dir', str(output)]
    (output / 'halo2-source-receipt.json').write_text(json.dumps(dict(
        revision=REVISION, origin_sha256=sha(source / 'HALO2_ORIGIN.json'), command=command,
        profile=dict(opt_level=0, debug=0, codegen_units=256,
                     runtime_package_opt_level=2),
        scope=__doc__), indent=2) + '\n')
    env = dict(os.environ, CARGO_PROFILE_RELEASE_OPT_LEVEL='0',
               CARGO_PROFILE_RELEASE_DEBUG='0', CARGO_PROFILE_RELEASE_CODEGEN_UNITS='256')
    if args.rustup_home:
        env['RUSTUP_HOME'] = str(args.rustup_home.resolve())
    for inherited in ('MAKEFLAGS', 'MFLAGS', 'CARGO_MAKEFLAGS'):
        env.pop(inherited, None)
    print('Authenticated pinned MIT GP instruction core; starting separate serial Cargo build', flush=True)
    os.execvpe(str(args.cargo), command, env)


if __name__ == '__main__':
    main()
