#!/usr/bin/env python3
"""Play campaign in pStencil on the launcher-selected native-kernel runtime.

The default route reads gameplay-runtime-selection.json: the same qualified
MCC/native runtime and prepared cache Halo2-Launcher uses. --legacy-runtime
keeps the retained October opening bundle (TU1.5 XBE, Xbox disc maps) reachable
as an explicit reference route; it is never a fallback.
"""
import hashlib
import json
import os
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from selected_runtime import selected_runtime

ROOT=Path(__file__).resolve().parents[2]
BUNDLE=ROOT/'analysis/pstencil/campaign-live-v10'

def verify(path,expected):
    with path.open('rb')as stream:actual=hashlib.file_digest(stream,'sha256').hexdigest()
    if actual!=expected:raise SystemExit(f'Campaign input differs from the tested version: {path}')

def current(arguments):
    selection=json.loads((ROOT/'gameplay-runtime-selection.json').read_text())
    if selection.get('schema')!='H2_LOCAL_GAMEPLAY_RUNTIME_SELECTION_V1':
        raise SystemExit('Unsupported gameplay runtime selection')
    runtime=ROOT/selection['runtime'];verify(runtime,selection['runtime_sha256'])
    maps=ROOT/selection['cache']/'halo2/h2_maps_win64_dx11'
    name='00a_introduction.map'
    if '--map' in arguments:
        at=arguments.index('--map')
        if at+1>=len(arguments):raise SystemExit('--map requires a campaign cache name')
        name=arguments[at+1];arguments=arguments[:at]+arguments[at+2:]
    level=maps/Path(name).name
    if not level.is_file():raise SystemExit(f'Campaign cache is not prepared: {level}')
    if arguments==['--launcher-info']:
        print(json.dumps(dict(runtime=str(runtime),runtime_sha256=selection['runtime_sha256'],
            cache=str(ROOT/selection['cache']),default_map=name,viewport=[1920,1080],
            initial_graphics_mode='pStencil',graphics_modes=['Original','pStencil'],
            saved_switch='Video Settings > Graphics Mode',route='launcher-selected native runtime',
            legacy_route='--legacy-runtime',full_presentation_verified=False),indent=2))
        return
    defaults=[]
    for flag,values in (('--graphics-mode',['pStencil']),('--viewport',['1920','1080'])):
        if flag not in arguments:defaults.extend([flag,*values])
    command=[str(runtime),'--campaign','--map',str(level),'--shared-maps',str(maps),*defaults,*arguments]
    os.chdir(ROOT);os.execv(str(runtime),command)

def main():
    arguments=sys.argv[1:]
    if '--legacy-runtime' not in arguments:return current(arguments)
    sys.argv[1:]=[value for value in arguments if value!='--legacy-runtime']
    manifest=selected_runtime(ROOT,json.loads((BUNDLE/'runtime.json').read_text()))
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
            graphics_modes=['Original','pStencil'],live_switch=manifest.get('live_switch','F1 > Graphics > Graphics Mode'),
            saved_switch='Video Settings > Graphics Mode',fullscreen_modes=manifest.get('fullscreen_modes',[]),upscaling=manifest.get('upscaling',{}),verification=manifest['verification'],
            full_presentation_verified=False),indent=2))
        return
    ui=Path(manifest['ui_assets'])
    command=[str(runtime),'--campaign','--map',str(maps/'00a_introduction.map'),
        '--shared-maps',str(maps),'--gameplay-xbe',str(reference),'--frames','0',
        '--viewport','1920','1080','--graphics-mode','pStencil',
        '--hud-ui-assets',str(ui),'--pause-ui-assets',str(ui),*sys.argv[1:]]
    os.chdir(ROOT);os.execv(str(runtime),command)

if __name__=='__main__':main()
