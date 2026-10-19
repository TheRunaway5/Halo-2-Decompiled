#!/usr/bin/env python3
"""Play the tested pStencil opening with current campaign assets."""
import hashlib
import json
import os
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[2]
BUNDLE=ROOT/'analysis/pstencil/campaign-live-v10'

def verify(path,expected):
    with path.open('rb')as stream:actual=hashlib.file_digest(stream,'sha256').hexdigest()
    if actual!=expected:raise SystemExit(f'Campaign input differs from the tested version: {path}')

def main():
    manifest=json.loads((BUNDLE/'runtime.json').read_text())
    runtime=ROOT/manifest['runtime'];shaders=ROOT/manifest['shader_directory']
    verify(runtime,manifest['runtime_sha256'])
    reference=Path(manifest['reference']);verify(reference,manifest['reference_sha256'])
    for name,digest in manifest['shaders'].items():verify(shaders/name,digest)
    maps=ROOT/'assets/disc/maps'
    for name,digest in manifest['maps'].items():verify(maps/name,digest)
    if sys.argv[1:]==['--launcher-info']:
        print(json.dumps(dict(runtime=str(runtime),runtime_sha256=manifest['runtime_sha256'],
            shader_directory=str(shaders),shader_files_verified=len(manifest['shaders']),
            default_map='00a_introduction.map',viewport=[1920,1080],initial_graphics_mode='pStencil',
            graphics_modes=['Original','pStencil'],live_switch='F1 > Graphics > Graphics Mode',
            saved_switch='Video Settings > Graphics Mode',verification=manifest['verification'],
            full_presentation_verified=False),indent=2))
        return
    ui=Path(manifest['ui_assets'])
    command=[str(runtime),'--campaign','--map',str(maps/'00a_introduction.map'),
        '--shared-maps',str(maps),'--gameplay-xbe',str(reference),'--frames','0',
        '--viewport','1920','1080','--graphics-mode','pStencil',
        '--hud-ui-assets',str(ui),'--pause-ui-assets',str(ui),*sys.argv[1:]]
    os.chdir(ROOT);os.execv(str(runtime),command)

if __name__=='__main__':main()
