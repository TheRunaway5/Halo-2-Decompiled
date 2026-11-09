#!/usr/bin/env python3
"""Exercise cache-owned intro playback and MCC menus before selecting a frontend."""
import argparse
import hashlib
import json
import math
import mmap
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
INTRO_CAPTURE_PREFIX = 'Frontend intro capture: '


def intro_capture_witness(output, expected_frames):
    """Require the native capture gate's actual source-clock completion."""
    rows = [line[len(INTRO_CAPTURE_PREFIX):] for line in output.splitlines()
            if line.startswith(INTRO_CAPTURE_PREFIX)]
    if len(rows) != 1:
        raise ValueError('Missing or ambiguous frontend intro-clear capture witness')
    try:
        row = json.loads(rows[0])
    except (ValueError, TypeError) as error:
        raise ValueError('Malformed frontend intro-clear capture witness') from error
    if (not isinstance(row, dict) or row.get('schema') != 'H2_FRONTEND_INTRO_CLEAR_CAPTURE_V1' or
            row.get('clear_observed') is not True or
            type(row.get('qualified_frames')) is not int or row['qualified_frames'] != expected_frames or
            type(row.get('warmup_frames')) is not int or row['warmup_frames'] < 0 or
            type(row.get('wait_budget_ms')) is not int or row['wait_budget_ms'] != 30000):
        raise ValueError('Frontend capture did not prove the requested post-intro frames')
    for name in ('first_clear_ui_clock_ms', 'last_clear_ui_clock_ms'):
        if type(row.get(name)) is not int or not 0 <= row[name] < 2**32:
            raise ValueError('Frontend intro-clear witness lacks its source UI clock')
    waited = row.get('waited_ms')
    if (type(waited) not in (int, float) or not math.isfinite(waited) or
            not 0 <= waited < row['wait_budget_ms']):
        raise ValueError('Frontend intro-clear wait exceeded its real-time budget')
    return row


def visible_menu_capture(path):
    """Reject empty/solid captures without claiming whole-frame equivalence."""
    from PIL import Image
    path = Path(path)
    if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= 256 * 1024**2:
        raise ValueError('Frontend capture has no bounded private image owner')
    with Image.open(path) as image:
        if image.format != 'PPM' or not all(0 < size <= 8192 for size in image.size):
            raise ValueError('Frontend capture is not a bounded native PPM frame')
        image = image.convert('RGB')
        extrema = image.getextrema()
        bounds = image.getbbox()
        if bounds is None or all(low == high for low, high in extrema):
            raise ValueError('Frontend capture is black or uniform after intro clear')
        return dict(size=list(image.size), nonblack_bbox=list(bounds),
                    channel_extrema=[list(pair) for pair in extrema])


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def loading_owner(cache, runtime=None):
    """Pin the bank main.cpp actually selects, with an admission witness."""
    from native_gameplay_test import (FRONTEND_LOADING_PROFILE, FRONTEND_H2EK_LOADING_PROFILE,
                                     FRONTEND_H2EK_LOADING_FILES)
    import h2ek_loading_ambience as h2ek
    original_witness = 'Original loading: H2EK logo/sweep art and four-voice ambience admitted'
    supports_original = True
    if runtime is not None:
        # Older Windows builds implement MCC V3 only. A newer cache may also
        # hold an optional kit bank; its presence cannot change that executable.
        with runtime.open('rb') as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
            supports_original = data.find(original_witness.encode()) >= 0
    if supports_original and ((cache / h2ek.BANK).exists() or (cache / h2ek.BANK).is_symlink()):
        report = json.loads((cache / 'provenance.json').read_bytes())
        if not h2ek.verify(cache, report.get('h2ek_loading_ambience')):
            raise ValueError('Selected H2EK loading bank has no completed onboarding owner')
        return dict(profile=FRONTEND_H2EK_LOADING_PROFILE, original=True,
            files=sorted(FRONTEND_H2EK_LOADING_FILES),
            witness=original_witness)
    return dict(profile=FRONTEND_LOADING_PROFILE, original=False,
        files=['presentation/loading-v3/' + name for name in ('manifest.json', 'glow-mask.dds', 'loading.wav')],
        witness='MCC V3 loading: genuine PCM and reconstructed artwork admitted')


def _readmit_frontend_owner(root, runtime, cache, sidecar, receipt, capability):
    from native_gameplay_test import NativeTestOwnerChanged, _check

    pins = capability['resources']
    pinned_paths = {cache/name for name in pins}
    runtime_pin = capability['runtime']['sha256']
    receipt_pin = capability['validation']['report_sha256']
    sidecar_pin = hashlib.sha256((json.dumps(capability, indent=2)+'\n').encode('utf-8')).hexdigest()
    diagnostic = dict(passed=False, maximum_attempts=3, attempts=[],
        runtime_sha256=runtime_pin, frame_report_sha256=receipt_pin, capability_sha256=sidecar_pin,
        identity_fields=['device', 'inode', 'bytes', 'mtime_ns', 'ctime_ns'])
    diagnostic_path = receipt.parent/'frontend-owner-readmission.json'

    def record():
        diagnostic_path.write_text(json.dumps(diagnostic, indent=2)+'\n')

    for attempt in range(1, 4):
        row = dict(attempt=attempt, passed=False)
        diagnostic['attempts'].append(row)
        try:
            if digest(runtime)!=runtime_pin or any(digest(cache/name)!=value for name,value in pins.items()):
                raise RuntimeError('Frontend owner changed during validation')
            if digest(sidecar)!=sidecar_pin or digest(receipt)!=receipt_pin:
                raise RuntimeError('Frontend validation receipt or capability changed during readmission')
            _check(root, runtime, cache, frontend=True)
        except NativeTestOwnerChanged as error:
            retry = error.path in pinned_paths and error.path.is_relative_to(cache) and attempt<3
            row.update(error=type(error).__name__, path=str(error.path), before=error.before,
                after_fd=error.after_fd, after_path=error.after_path, retry_allowed=retry)
            record()
            if not retry:
                raise
        except BaseException as error:
            row.update(error=type(error).__name__, message=str(error))
            record()
            raise
        else:
            row['passed'] = True
            diagnostic['passed'] = True
            record()
            return


def _cache_declares_movie(cache):
    """Match the MCC player's private movie bank; source references are inert."""
    from mcc_movies import BANK
    bank = cache/BANK
    if bank.exists() or bank.is_symlink():
        return True
    path = cache/'provenance.json'
    if not path.exists() and not path.is_symlink():
        return False
    if path.is_symlink() or not path.is_file() or path.stat().st_size > 16*1024**2:
        raise ValueError('Invalid private cache provenance for movie startup validation')
    report = json.loads(path.read_bytes())
    if not isinstance(report, dict):
        raise ValueError('Invalid private cache provenance for movie startup validation')
    return 'port_bundled_movies' in report


def qualify(root, runtime, cache, output, scratch, eos_worker=None, *, progress=print,
            include_optional_movie=None, cache_scoped=False, executable_owner=None):
    from native_gameplay_test import (FRONTEND_CAPABILITY, FRONTEND_MAP, FRONTEND_SCHEMA,
                                     FRONTEND_SCOPE)
    from native_renderer_shaders import shader_capability
    root, runtime, cache = root.resolve(), runtime.resolve(strict=True), cache.resolve(strict=True)
    retained = None
    if executable_owner is not None:
        if not cache_scoped:
            raise ValueError('Retained executable ownership is only valid for cache rebuilding')
        from native_gameplay_test import _runtime_owner
        retained = _runtime_owner(root, runtime, frontend=True)
        if any(retained[key] != executable_owner[key] for key in
               ('runtime', 'runtime_sha256', 'capability_sha256', 'report')):
            raise ValueError('Selected executable owner changed before cache qualification')
    from installation_paths import resource_root
    if not runtime.is_relative_to(root) and not (retained and runtime.is_relative_to(resource_root(root))):
        raise ValueError('The frontend runtime must belong to this installation')
    sidecar = (cache/'frontend-runtime-capability.json' if cache_scoped else
               runtime.with_name(runtime.name + '.frontend-test.json'))
    if sidecar.exists() or sidecar.is_symlink():
        raise RuntimeError('Preserving existing frontend capability; select an unqualified candidate')
    maps = cache/'halo2/h2_maps_win64_dx11'
    output.mkdir(parents=True, exist_ok=True)
    pin = digest(runtime)
    if eos_worker is not None:
        from native_gameplay_test import validate_eos_worker
        validate_eos_worker(runtime, eos_worker)
    resources = [maps/name for name in ('shared.map', 'textures.dat', 'sounds_neutral.dat')]
    resources += [p for p in (cache/'halo2/h2_fonts').rglob('*') if p.is_file()]
    loading = loading_owner(cache, runtime)
    resources += [cache/name for name in loading['files']]
    from controller_prompt_assets import runtime_sources, PROFILE as prompt_profile
    owned_prompts = runtime_sources(root)[0]['files']
    for name, row in owned_prompts.items():
        path = cache/name
        if not path.is_file() or path.is_symlink() or digest(path) != row['sha256']:
            raise ValueError('Our button prompt bank is missing or changed; run launcher Repair')
        resources.append(path)
    if (cache/'frontend-map-bindings.json').exists():
        resources.append(cache/'frontend-map-bindings.json')
    # Match the frontend reader: loose reference pages are not an active pack.
    if (cache/'ui/pack.tsv').is_file():
        resources += [p for p in (cache/'ui').rglob('*') if p.is_file()]
    layout = cache/'presentation/ui/native-layout.json'
    if layout.is_file(): resources.append(layout)
    pins = {p.relative_to(cache).as_posix(): digest(p) for p in resources}
    shaders = runtime.parent/'shaders'
    shader_proof = shader_capability(shaders,runtime) if retained is None else {}
    windows = runtime.suffix.lower() == '.exe'
    if windows and (os.name != 'nt' or runtime != root/'halo2_native.exe'):
        raise ValueError('Qualify the Windows package using its native bundled Python')
    movie_declared = _cache_declares_movie(cache)
    if include_optional_movie is None:
        include_optional_movie = movie_declared
    elif not include_optional_movie and movie_declared:
        raise ValueError('Optional movie files are present or declared; their intro check remains required')
    report = dict(passed=False, runtime=str(runtime), runtime_sha256=pin, cases=[], scratch=str(scratch))
    cases = [('intro',['--menu-screen','9','--movie-frame','100']),
             ('title',['--menu-screen','9','--skip-intro']),
             ('main-menu',['--menu-screen','6','--skip-intro']),
             ('online-sign-in',['--menu-screen','20','--skip-intro'])]
    if not include_optional_movie:
        cases = cases[1:]
    for name, extra in cases:
        progress('Checking game startup: ' + name + '…')
        image = scratch/(name+'.ppm')
        command = [str(runtime),'--menu-maps',str(maps),'--shared-maps',str(maps),
                   '--hidden','--mute','--frames','4','--screenshot',str(image),*extra]
        wait_intro = name != 'intro'
        if wait_intro:
            command.append('--menu-wait-intro-clear')
        started = time.monotonic()
        failure = None
        try:
            environment = dict(os.environ,SDL_VIDEODRIVER=('windows' if windows else 'cocoa' if sys.platform=='darwin' else 'x11'),SDL_AUDIODRIVER='dummy',
                                XDG_DATA_HOME=str(scratch/'data'),XDG_CONFIG_HOME=str(scratch/'config'),
                                XDG_CACHE_HOME=str(scratch/'cache'))
            if windows:
                environment.update(APPDATA=str(scratch/'appdata'), LOCALAPPDATA=str(scratch/'localappdata'))
            result = subprocess.run(command, cwd=root, env=environment,
                                text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
            stdout, exit_code = result.stdout, result.returncode
        except subprocess.TimeoutExpired as error:
            stdout = error.stdout or ''
            if isinstance(stdout, bytes): stdout = stdout.decode('utf-8', errors='replace')
            exit_code, failure = None, 'Frontend capture exceeded its 180-second process budget'
        (output/(name+'.log')).write_text(stdout[-65536:])
        passed = exit_code == 0 and image.is_file() and 'Rendered 4 frontend frames' in stdout
        if exit_code == 0 and loading['witness'] not in stdout:
            passed, failure = False, 'Runtime did not admit the declared loading presentation'
        if name == 'intro' and 'Frontend movie:' not in stdout:
            passed, failure = False, 'The declared optional intro movie did not start'
        row = dict(name=name,passed=passed,exit_code=exit_code,seconds=time.monotonic()-started,command=command)
        if wait_intro and passed:
            try:
                row['intro_clear'] = intro_capture_witness(stdout, 4)
                row['visible_frame'] = visible_menu_capture(image)
            except (ValueError, OSError) as error:
                passed, failure = False, str(error)
        row['passed'] = passed
        if failure: row['error'] = failure
        if image.is_file(): row.update(capture_sha256=digest(image),capture_bytes=image.stat().st_size)
        report['cases'].append(row)
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        from setup_progress import ProgressUpdate
        progress(ProgressUpdate('menu-startup', 'Checking game startup', len(report['cases']), len(cases)))
        if not passed:
            raise RuntimeError('Frontend check failed: '+name+
                               (' — '+failure if failure else ' (exit '+str(exit_code)+')'))
    if digest(runtime)!=pin or any(digest(cache/name)!=value for name,value in pins.items()):
        raise RuntimeError('Frontend owner changed during validation')
    if retained is not None:
        if _runtime_owner(root, runtime, frontend=True) != retained:
            raise RuntimeError('Retained executable/shader owner changed during cache qualification')
    elif shader_capability(shaders,runtime)!=shader_proof:
        raise RuntimeError('Frontend shader package changed during validation')
    if eos_worker is not None:
        validate_eos_worker(runtime, eos_worker)
        report['eos_worker'] = eos_worker
    limits = ('Four rendered menu frames per screen after the actual source map intro clears; ' +
              ('supplied intro frame 100 also checked. ' if include_optional_movie else
               'optional intro movie is not packaged and was not checked. ') +
              'Real account sign-in and completed online gameplay remain unverified.')
    report.update(passed=True, optional_intro_movie='checked' if include_optional_movie else 'not-packaged',
                  limits=limits)
    receipt = output/'report.json'
    receipt.write_text(json.dumps(report,indent=2)+'\n')
    capability = dict(schema=FRONTEND_SCHEMA,capability=FRONTEND_CAPABILITY,
        runtime=dict(path='halo2_native.exe' if windows else str(runtime),sha256=pin),
        shaders=dict(binding='windows-executable-adjacent' if windows else 'compiled-default',
                     directory='shaders' if windows else str(shaders),**shader_proof),
        map=dict(path=FRONTEND_MAP,sha256=digest(cache/FRONTEND_MAP)),resources=pins,
        validation=dict(passed=True,scope=FRONTEND_SCOPE,report_sha256=digest(receipt),
            mcc_loading_presentation_profile=loading['profile'],
            mcc_prompt_font_profile='mcc-native-control-fonts-v1',original_loading_ambience=loading['original'],
            controller_prompt_profile=prompt_profile,
            frontend_capture_policy='source-map-intro-clear-v1',
            optional_intro_movie=report['optional_intro_movie'],
            full_game_parity=False,limits=report['limits']))
    if retained is not None:
        # Preserve portable loader/payload bindings and optional runtime banks;
        # the new proof changes only cache resources and capture validation.
        for key in ('runtime', 'shaders', 'eos_worker', 'runtime_adjacent_data', 'packaging'):
            if key in retained['report']:
                capability[key] = retained['report'][key]
    if cache_scoped:
        from custom_asset_cache import MANIFEST, SCHEMA as CUSTOM_SCHEMA, verify as verify_bundle
        verify_bundle(root, cache, json.loads((cache/'provenance.json').read_bytes()))
        capability['validation'].update(custom_asset_cache_profile=CUSTOM_SCHEMA,
            custom_asset_cache_sha256=digest(cache/MANIFEST))
    if eos_worker is not None:
        capability['eos_worker'] = eos_worker
    # The readmission pin is canonical UTF-8/LF on both platforms. Text mode
    # would translate LF to CRLF on Windows and reject our own successful proof.
    with sidecar.open('xb') as stream:
        stream.write((json.dumps(capability,indent=2)+'\n').encode('utf-8'))
    try: _readmit_frontend_owner(root,runtime,cache,sidecar,receipt,capability)
    except BaseException:
        sidecar.unlink()
        raise
    return dict(schema='H2_LOCAL_FRONTEND_RUNTIME_SELECTION_V1',runtime=(runtime.relative_to(root).as_posix()
                if runtime.is_relative_to(root) else str(runtime)),
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
