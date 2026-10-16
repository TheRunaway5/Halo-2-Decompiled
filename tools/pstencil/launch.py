#!/usr/bin/env python3
"""Launch the retained pStencil game runtime with current maps and TU1.5 gameplay."""
import hashlib
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
BUNDLE = ROOT / 'analysis/pstencil/live-v10'


def verify(path, expected):
    with path.open('rb') as stream:
        actual = hashlib.file_digest(stream, 'sha256').hexdigest()
    if actual != expected:
        raise SystemExit(f'pStencil input differs from the tested version: {path}')
    return actual


def main():
    manifest = json.loads((BUNDLE / 'runtime.json').read_text())
    runtime = ROOT / manifest['runtime']
    reference = Path(manifest['reference'])
    verify(runtime, manifest['runtime_sha256'])
    verify(reference, manifest['reference_sha256'])
    for name, digest in manifest['shaders'].items():
        verify(ROOT / manifest['shader_directory'] / name, digest)
    maps = ROOT / 'assets/disc/maps'
    ui = Path(manifest['ui_assets'])
    prefroot = Path(os.environ.get('XDG_DATA_HOME', str(Path.home() / '.local/share')))
    preferences = prefroot / 'Halo2Native/Halo2/menu.cfg'
    mode = 'Original'
    if preferences.is_file():
        for line in preferences.read_text().splitlines():
            words = line.split()
            if len(words) == 2 and words[0] == 'pc_graphics_mode' and words[1] in ('0', '1'):
                mode = 'pStencil' if words[1] == '1' else 'Original'
    if sys.argv[1:] == ['--launcher-info']:
        print(json.dumps(dict(runtime=str(runtime), runtime_sha256=manifest['runtime_sha256'],
            shader_directory=str(ROOT / manifest['shader_directory']), shader_files_verified=len(manifest['shaders']),
            reference_path=str(reference), reference_sha256=manifest['reference_sha256'],
            reference='OG Xbox English TU1.5 gameplay; May 2003 pStencil shadow and material adaptation',
            maps=str(maps), default_map='lockout.map', default_local_players=1,
            initial_graphics_mode=mode, graphics_modes=['Original', 'pStencil'],
            live_switch='F1 > Graphics > Graphics Mode', saved_switch='Video Settings > Graphics Mode',
            preferences=str(preferences), ui_assets=str(ui), ui_assets_role='Existing HUD and pause UI'), indent=2))
        return
    os.chdir(ROOT)
    command = [str(runtime), '--multiplayer', '--map', str(maps / 'lockout.map'),
        '--shared-maps', str(maps), '--gameplay-xbe', str(reference), '--local-players', '1',
        '--viewport', '1280', '720', '--graphics-mode', mode,
        '--hud-ui-assets', str(ui), '--pause-ui-assets', str(ui), *sys.argv[1:]]
    os.execv(str(runtime), command)


if __name__ == '__main__':
    main()
