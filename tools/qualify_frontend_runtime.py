#!/usr/bin/env python3
"""Exercise the supplied intro and MCC menus before selecting a local frontend."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def qualify(root, runtime, cache, output, scratch):
    from native_gameplay_test import (FRONTEND_CAPABILITY, FRONTEND_MAP, FRONTEND_SCHEMA,
                                     FRONTEND_SCOPE, _check)
    from native_renderer_shaders import PROFILE
    root, runtime, cache = root.resolve(), runtime.resolve(strict=True), cache.resolve(strict=True)
    if not runtime.is_relative_to(root):
        raise ValueError('The frontend runtime must belong to this installation')
    sidecar = runtime.with_name(runtime.name + '.frontend-test.json')
    if sidecar.exists() or sidecar.is_symlink():
        raise RuntimeError('Preserving existing frontend capability; select an unqualified candidate')
    maps = cache/'halo2/h2_maps_win64_dx11'
    output.mkdir(parents=True, exist_ok=True)
    pin = digest(runtime)
    resources = [maps/name for name in ('shared.map', 'textures.dat', 'sounds_neutral.dat')]
    resources += [p for p in (cache/'halo2/h2_fonts').rglob('*') if p.is_file()]
    resources += [cache/'presentation/loading-v3'/name for name in ('manifest.json','glow-mask.dds','loading.wav')]
    # Match the frontend reader: loose reference pages are not an active pack.
    if (cache/'ui/pack.tsv').is_file():
        resources += [p for p in (cache/'ui').rglob('*') if p.is_file()]
    layout = cache/'presentation/ui/native-layout.json'
    if layout.is_file(): resources.append(layout)
    pins = {p.relative_to(cache).as_posix(): digest(p) for p in resources}
    shaders = runtime.parent/'shaders'
    shader_pins = {p.relative_to(shaders).as_posix(): digest(p) for p in shaders.rglob('*') if p.is_file()}
    report = dict(passed=False, runtime=str(runtime), runtime_sha256=pin, cases=[], scratch=str(scratch))
    cases = [('intro',['--menu-screen','9','--movie-frame','100']),
             ('title',['--menu-screen','9','--skip-intro']),
             ('main-menu',['--menu-screen','6','--skip-intro']),
             ('online-sign-in',['--menu-screen','20','--skip-intro'])]
    for name, extra in cases:
        image = scratch/(name+'.ppm')
        command = [str(runtime),'--menu-maps',str(maps),'--shared-maps',str(maps),
                   '--hidden','--mute','--frames','4','--screenshot',str(image),*extra]
        started = time.monotonic()
        result = subprocess.run(command, cwd=root, env=dict(os.environ,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
                                XDG_DATA_HOME=str(scratch/'data'),XDG_CONFIG_HOME=str(scratch/'config'),
                                XDG_CACHE_HOME=str(scratch/'cache')),
                                text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
        (output/(name+'.log')).write_text(result.stdout[-65536:])
        passed = result.returncode == 0 and image.is_file() and 'Rendered 4 frontend frames' in result.stdout
        if name == 'intro': passed = passed and 'Frontend movie:' in result.stdout
        row = dict(name=name,passed=passed,exit_code=result.returncode,seconds=time.monotonic()-started,command=command)
        if image.is_file(): row.update(capture_sha256=digest(image),capture_bytes=image.stat().st_size)
        report['cases'].append(row)
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        print(name, 'passed' if passed else 'failed', flush=True)
        if not passed: raise RuntimeError('Frontend check failed: '+name)
    if digest(runtime)!=pin or any(digest(cache/name)!=value for name,value in pins.items()):
        raise RuntimeError('Frontend owner changed during validation')
    report.update(passed=True,limits='Four rendered frames per screen and supplied intro frame 100; real account sign-in and completed online gameplay remain unverified.')
    receipt = output/'report.json'
    receipt.write_text(json.dumps(report,indent=2)+'\n')
    capability = dict(schema=FRONTEND_SCHEMA,capability=FRONTEND_CAPABILITY,
        runtime=dict(path=str(runtime),sha256=pin),
        shaders=dict(binding='compiled-default',profile=PROFILE,directory=str(shaders),files=shader_pins),
        map=dict(path=FRONTEND_MAP,sha256=digest(cache/FRONTEND_MAP)),resources=pins,
        validation=dict(passed=True,scope=FRONTEND_SCOPE,report_sha256=digest(receipt),
            mcc_loading_presentation_profile='mcc-logo-glow-mcc-track-oneshot-v1',
            mcc_prompt_font_profile='mcc-native-control-fonts-v1',original_loading_ambience=False,
            full_game_parity=False,limits=report['limits']))
    with sidecar.open('x') as stream:
        stream.write(json.dumps(capability,indent=2)+'\n')
    try: _check(root,runtime,cache,frontend=True)
    except BaseException:
        sidecar.unlink()
        raise
    return dict(schema='H2_LOCAL_FRONTEND_RUNTIME_SELECTION_V1',runtime=runtime.relative_to(root).as_posix(),
                runtime_sha256=pin,capability_sha256=digest(sidecar))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=ROOT)
    parser.add_argument('--runtime',type=Path,required=True)
    parser.add_argument('--cache',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--select',action='store_true')
    args=parser.parse_args()
    scratch=os.environ.get('HALO2_TEST_SCRATCH')
    if not scratch: parser.error('Run through tools/run_test.py')
    selection=args.root/'frontend-runtime-selection.json'
    before=selection.read_bytes() if selection.exists() else None
    record=qualify(args.root,args.runtime,args.cache,args.output,Path(scratch))
    if args.select:
        from gameplay_runtime_selection import selected_gameplay_runtime
        game=selected_gameplay_runtime(args.root)
        if not game or (args.root/game['runtime']).resolve()!=args.runtime.resolve():
            raise RuntimeError('Menu selection must use the currently qualified gameplay runtime')
        if (selection.read_bytes() if selection.exists() else None)!=before:
            raise RuntimeError('Another task changed the menu selection; preserving its choice')
        from build_launcher_engine import atomic_json
        atomic_json(selection,record)
    print(json.dumps(dict(passed=True,selection_installed=args.select,runtime=record['runtime'])))
    return 0


if __name__=='__main__':
    raise SystemExit(main())
