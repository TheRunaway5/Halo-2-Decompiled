#!/usr/bin/env python3
"""Build the production engine and promote an exercised local MCC runtime.

Compiler products and smoke outputs belong to run_test.py. Only a successful
runtime, its resources and small provenance reports leave that scratch owner.
Existing selections and installed runtimes survive failed builds or checks.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import uuid

from source_layout import source_file

ROOT = Path(__file__).resolve().parents[1]

# These are retained reference/component stages, with no admitted production
# caller. Their existence must not be mistaken for delivered functionality.
SEPARATE_COMPONENTS = {
    'src/app/launcher.cpp': 'Launcher entry points, separate from the engine',
    'src/app/weapon_launcher.cpp': 'Separate weapons investigation launchers',
    'src/audio/dsp/sound_gp_cpu_tu15.cpp': 'GP instruction core requires its actual platform consumer',
    'src/audio/dsp/sound_gp_cpu_tu15_ffi.c': 'GP instruction-core bridge',
    'src/audio/dsp/sound_gp_dma_tu15.cpp': 'GP DMA requires its actual platform consumer',
    'src/audio/source_sound_configuration_tu15_generated.cpp': 'Sound configuration requires original output/policy admission',
    'src/audio/native_tu15_sound_configuration_data.cpp': 'Sound configuration component data',
    'src/ui/vista/vista_lobby_leave_return.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/ui/vista/vista_network_browser.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/ui/vista/vista_network_browser_back.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/gameplay/multiplayer/multiplayer_locomotion_continuity.cpp': 'Locomotion group is not yet part of the production handoff frontier',
    'src/ui/widgets/native_vista_widget_image.cpp': 'Original Vista PE widget-image component without a production caller',
    'src/ui/widgets/vista_widget_backing.cpp': 'Vista widget heap component requires actual allocation providers',
    'src/ui/widgets/vista_widget_font_ambient.cpp': 'Vista ambient font environment component without a production caller',
    'src/rendering/world/source_sky_stencil.cpp': 'Frozen sky-stencil reference prototype requires actual production draw inputs',
}


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def source_hashes(root):
    files = [root / 'CMakeLists.txt']
    for name in ('src', 'include', 'cmake', 'shaders', 'data', 'rust'):
        files.extend(p for p in (root / name).rglob('*') if p.is_file()
                     and 'target' not in p.relative_to(root / name).parts)
    return {p.relative_to(root).as_posix(): digest(p) for p in sorted(files)}


def freeze_source(root, destination):
    """Copy code once; borrow existing asset/reference trees without changing them."""
    before = source_hashes(root)
    destination.mkdir()
    for name in ('src', 'include', 'cmake', 'shaders', 'data', 'tools', 'third_party', 'external', 'rust'):
        if (root / name).is_dir():
            shutil.copytree(root / name, destination / name,
                            ignore=shutil.ignore_patterns('__pycache__', '.git', 'target'))
    for path in root.iterdir():
        if path.is_file() and path.suffix in ('.txt', '.cmake', '.py', '.sh'):
            shutil.copy2(path, destination / path.name)
    tests = destination / 'tests'
    tests.mkdir()
    for path in (root / 'tests').iterdir():
        if path.is_file():
            shutil.copy2(path, tests / path.name)
        elif path.is_dir() and path.name != '__pycache__':
            (tests / path.name).symlink_to(path, target_is_directory=True)
    for name in ('assets', 'resources', 'multiplayer', 'analysis', '.tools'):
        if (root / name).exists():
            (destination / name).symlink_to(root / name, target_is_directory=True)
    if source_hashes(destination) != before or source_hashes(root) != before:
        raise RuntimeError('Engine source changed while freezing the build; retry the current revision')
    return before


def source_status(root, runtime):
    path = runtime.parent / 'source-hashes.json'
    if not path.is_file():
        return 'Engine source: no source inventory for the selected runtime'
    recorded = {n: h for n, h in json.loads(path.read_text()).items()
                if n == 'CMakeLists.txt' or n.startswith(('src/', 'include/', 'cmake/', 'shaders/', 'data/', 'rust/'))}
    current = source_hashes(root)
    changed = sum(recorded.get(n) != current.get(n) for n in recorded.keys() | current.keys())
    return ('Engine source: matches the selected runtime' if not changed else
            f'Engine source: {changed} files differ from the selected runtime; use --build-engine')


def wiring(graph, root):
    root = root.resolve()
    targets = {}
    for line in graph.read_text().splitlines():
        name, links, dependencies, sources = line.split('|')
        normalized = []
        for source in sources.split(';'):
            path = Path(source)
            if path.is_absolute() and path.is_relative_to(root):
                source = path.relative_to(root).as_posix()
            normalized.append(source)
            # NativeTranslationUnits.cmake compiles unchanged body partitions
            # rather than the oversized authenticated source file itself.
            partition = re.search(r'/native-translations/([^/]+)/[^/]+\.cpp$', source)
            if partition:
                try:
                    original = source_file(root, partition[1] + '.cpp')
                except FileNotFoundError:
                    pass
                else:
                    normalized.append(original.relative_to(root).as_posix())
            specialized = re.search(r'/native-operands/(.+\.cpp)$', source)
            if specialized and (root / 'src' / specialized[1]).is_file():
                normalized.append('src/' + specialized[1])
        targets[name] = (links.split(';'), dependencies.split(';'), normalized)
    closure, pending = set(), ['halo2_native']
    while pending:
        name = pending.pop()
        if name not in targets or name in closure:
            continue
        closure.add(name)
        pending.extend(p.removeprefix('$<LINK_ONLY:').removesuffix('>') for p in targets[name][0])
    linked = {s for t in closure for s in targets[t][2] if s.startswith('src/')}
    isolated = {}
    for name, (_, _, sources) in targets.items():
        for source in sources:
            if source.startswith('src/') and source.endswith(('.cpp', '.c')) and source not in linked:
                isolated.setdefault(source, []).append(name)
    registered = {s for row in targets.values() for s in row[2]}
    platform_sources = sorted(p.relative_to(root).as_posix() for p in (root / 'src/macos').rglob('*.cpp'))
    unregistered = sorted(p.relative_to(root).as_posix() for p in (root / 'src').rglob('*.cpp')
                          if p.relative_to(root).as_posix() not in platform_sources
                          if p.relative_to(root).as_posix() not in registered)
    missing = sorted((set(isolated) | set(unregistered)) - SEPARATE_COMPONENTS.keys())
    return dict(passed=not missing, engine_targets=sorted(closure), engine_sources=sorted(linked),
                separate_components=isolated,
                other_platform_sources=platform_sources,
                unregistered_sources=unregistered, unexpected_unwired_sources=missing,
                component_boundaries={s: SEPARATE_COMPONENTS[s] for s in sorted(set(isolated) | set(unregistered))
                                      if s in SEPARATE_COMPONENTS},
                scope='Transitive link graph; separate components require real callers before runtime acceptance')


def smoke(runtime, maps, name, players, scratch, timeout, root=ROOT, video_driver=''):
    log = scratch / f'{Path(name).stem}-{players}.log'
    command = [str(runtime), '--multiplayer', '--map', str(maps / name),
               '--shared-maps', str(maps), '--local-players', str(players),
               '--frames', '4', '--mute', '--viewport', '640', '480']
    # Exercise ordinary SDL/Vulkan frame submission, with no Xbox input,
    # synthetic runtime backing, or imported UI overrides.
    environment = dict(os.environ)
    for key in tuple(environment):
        if key.startswith(('HALO2_REFERENCE_', 'HALO2_CAPTURE_')) or (
                key.startswith('HALO2_TEST_') and key != 'HALO2_TEST_SCRATCH'):
            environment.pop(key)
    environment.update(HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch / 'vulkan-cache'),
                       MESA_SHADER_CACHE_DIR=str(scratch / 'mesa-cache'),
                       __GL_SHADER_DISK_CACHE_PATH=str(scratch / 'nvidia-cache'))
    if video_driver:
        environment.setdefault('SDL_VIDEODRIVER', video_driver)
    with log.open('wb') as stream:
        try:
            result = subprocess.run(command, cwd=root, stdout=stream, stderr=subprocess.STDOUT,
                                    timeout=timeout, env=environment)
            status = result.returncode
        except subprocess.TimeoutExpired:
            status = 'timeout'
    tail = log.read_bytes()[-12000:].decode(errors='replace')
    submitted = re.search(r'Multiplayer frames=(\d+) ticks=(\d+) players=(\d+) connected=(\d+) peak_players=(\d+)', tail)
    return dict(map=name, local_players=players, command=command, exit_code=status,
                passed=status == 0 and submitted is not None and int(submitted[1]) == 4 and
                    int(submitted[2]) > 0 and int(submitted[3]) == players and int(submitted[4]) == 1 and
                    int(submitted[5]) == players,
                submitted_frames=int(submitted[1]) if submitted else 0,
                simulation_ticks=int(submitted[2]) if submitted else 0,
                session_joined=bool(int(submitted[4])) if submitted else False,
                peak_players=int(submitted[5]) if submitted else 0, log_tail=tail)


def atomic_json(path, value):
    temporary = path.with_name('.' + path.name + '-' + uuid.uuid4().hex)
    try:
        temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def weapon_audio(build, source, maps, image, output, build_report):
    """Require the actual repeated local weapon scenario before promotion."""
    helper = source / 'tools/validate_weapon_audio_runtime.py'
    test = build / 'multiplayer_audio_runtime_tests'
    pins = {name: digest(source / name) for name in (
        'tools/validate_weapon_audio_runtime.py', 'tests/multiplayer_audio_runtime_tests.cpp',
        'tests/multiplayer_audio_tests.cpp', 'tests/mcc_sound_rate_tests.cpp')}
    image_pin, binary_pin = digest(image), digest(test)
    command = [sys.executable, '-B', str(helper), '--build-report', str(build_report),
               '--maps', str(maps), '--image', str(image), '--output', str(output),
               '--cycles', '3', '--timeout', '600']
    print('Checking repeated weapon shots/reloads on Coagulation (1/2/4 players) and Lockout…', flush=True)
    result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    report = json.loads((output / 'report.json').read_text())
    expected = {'coagulation1', 'coagulation2', 'coagulation4', 'lockout1'}
    cases = report.get('cases', [])
    unchanged = digest(image) == image_pin and digest(test) == binary_pin and all(
        digest(source / name) == pin for name, pin in pins.items())
    report['passed'] = bool(result.returncode == 0 and report.get('passed') and unchanged and
        len(cases) == len(expected) and {case['name'] for case in cases} == expected and
        all(case['passed'] and case['exit_code'] == 0 for case in cases) and
        report.get('binary_sha256') == binary_pin and report.get('image_sha256') == image_pin)
    report.update(test_source_sha256=pins, inputs_unchanged=unchanged)
    atomic_json(output / 'report.json', report)
    return report


def worker(args):
    from gameplay_runtime_selection import SELECTION, selected_gameplay_runtime
    from mcc_onboarding import selected
    root = args.root.resolve(strict=True)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    # Preserve the exact existing selection if another task promotes a runtime
    # while this build is in flight.
    selection = root / SELECTION
    previous = selection.read_bytes() if selection.is_file() else None
    frontend_selection = root / 'frontend-runtime-selection.json'
    previous_frontend = frontend_selection.read_bytes() if frontend_selection.is_file() else None
    prior = selected_gameplay_runtime(root)
    cache = selected(root)
    if cache is None:
        raise RuntimeError('Prepare the private MCC cache before building the launcher engine')
    maps = cache[0] / 'halo2/h2_maps_win64_dx11'
    audio_image = getattr(args, 'weapon_audio_reference_xbe', None)
    if audio_image:
        audio_image = audio_image.resolve(strict=True)
    names = sorted(prior['maps']) if prior else ['lockout.map']
    default_map = prior['default_map'] if prior else 'lockout.map'
    players = prior['local_players'] if prior else 1
    source = scratch / 'source'
    reuse_owned_build = getattr(args, 'reuse_owned_build', False)
    if reuse_owned_build:
        # A finite run_test.py coordinator already owns this frozen source
        # and build. Reuse that exact tree, including its declared refreshes;
        # never admit a checkout, installed runtime or unrelated build here.
        if not args.worker or source.is_symlink() or not source.is_dir():
            raise RuntimeError('Engine reuse requires this active scratch owner\'s private source')
        configured = scratch / 'build/CMakeCache.txt'
        expected = 'CMAKE_HOME_DIRECTORY:INTERNAL=' + str(source.resolve())
        if not configured.is_file() or expected not in configured.read_text().splitlines():
            raise RuntimeError('Engine reuse requires the same owned CMake source/build pair')
        # Component snapshots borrow authored data. Production bundlers require
        # a private regular tree, so materialize only this declared alias before
        # pinning the engine snapshot. The original data directory stays owned
        # by the checkout and is never removed or modified.
        data = source / 'data'
        if data.is_symlink():
            if data.resolve() != (root / 'data').resolve():
                raise RuntimeError('Owned engine data alias has a foreign source')
            files = [p for p in data.rglob('*') if p.is_file()]
            if any(p.is_symlink() for p in data.rglob('*')):
                raise RuntimeError('Owned engine data contains a nested symlink')
            expected_data = {p.relative_to(data).as_posix(): digest(p) for p in files}
            private = scratch / 'engine-private-data'
            shutil.copytree(data, private)
            copied = {p.relative_to(private).as_posix(): digest(p)
                      for p in private.rglob('*') if p.is_file()}
            if copied != expected_data or any(digest(data / p) != pin for p, pin in expected_data.items()):
                shutil.rmtree(private)
                raise RuntimeError('Authored data changed while freezing the owned engine bundle')
            data.unlink()
            private.rename(data)
        pins = source_hashes(source)
    else:
        pins = freeze_source(root, source)
    map_pins = {name: digest(maps / name) for name in names}
    destination = root / 'bin/engine-builds' / uuid.uuid4().hex
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    atomic_json(output / 'source-hashes.json', pins)
    report = dict(passed=False, scope='Current source build, launcher routing checks and MCC submitted-frame smoke',
                  runtime=destination.relative_to(root).as_posix() + '/halo2_native',
                  unresolved_case_failures=[], limits=['No full gameplay, Xbox framebuffer or audio parity claim'])
    report['reused_active_owned_build'] = reuse_owned_build
    promoted = False
    try:
        print('Building the complete engine and both launchers in disposable storage…', flush=True)
        command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--build-worker',
                   '--source', str(source), '--output', str(output / 'validation'),
                   '--target', 'halo2_native', '--target', 'halo2_launcher',
                   '--target', 'halo2_title_launcher', '--target', 'halo2_onboarding',
                   '--target', 'shadow_array_swap_tu15_tests',
                   '--filter', '^(launcher_first_run|title_launcher_first_run|bundled_launcher_first_run|'
                   'launcher_automatic_build|launcher_engine_promotion|native_gameplay_launcher_routing|native_frontend_launcher_routing|original_tu15_shadow_array_swap)$',
                   '--jobs', str(args.jobs), '--test-jobs', '1',
                   '--cmake-arg=-DHALO2_BUILD_LAUNCHERS_WITH_NATIVE=ON',
                   '--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF',
                   '--cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF',
                   '--cmake-arg=-DHALO2_LAUNCHER_RUNTIME_DIR=' + str(destination),
                   '--cmake-arg=-DHALO2_RUNTIME_SHADER_DIR=' + str(destination / 'shaders'),
                   '--retain-report', 'engine-targets.txt']
        if audio_image:
            command.extend(['--target', 'multiplayer_audio_runtime_tests',
                            '--target', 'multiplayer_audio_tests', '--target', 'mcc_sound_rate_tests',
                            '--retain-report', 'Testing/Temporary/LastTest.log'])
            at = command.index('--filter') + 1
            command[at] = command[at][:-2] + '|multiplayer_authored_audio|mcc_sound_codec_rates)$'
        sdk_archive = root / 'EOS-SDK-58105819-Release-v1.19.2.1.zip'
        if sdk_archive.is_file():
            command.append('--cmake-arg=-DHALO2_EOS_SDK_ARCHIVE=' + str(sdk_archive))
        client_config = getattr(args, 'eos_client_config', None)
        if client_config:
            command.append('--cmake-arg=-DHALO2_EOS_CLIENT_CONFIG=' + str(client_config.resolve(strict=True)))
        # The official runner retains its bounded configure/build/test logs.
        result = subprocess.run(command, cwd=root, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError('Engine build or launcher checks failed; see ' + str(output / 'validation'))
        if source_hashes(source) != pins:
            raise RuntimeError('Frozen engine source changed during the build')
        build = scratch / 'build'
        report['wiring'] = wiring(build / 'engine-targets.txt', source)
        if not report['wiring']['passed']:
            raise RuntimeError('Engine sources are missing from the runtime link graph: ' +
                               ', '.join(report['wiring']['unexpected_unwired_sources']))
        destination.mkdir(parents=True, exist_ok=False)
        for name in ('halo2_native', 'Halo2-Launcher', 'Halo2-Title-Launcher'):
            shutil.copy2(build / name, destination / name)
        for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env'):
            if (build / name).is_file():
                shutil.copy2(build / name, destination / name)
        if client_config and not all((destination / name).is_file() for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env')):
            raise RuntimeError('EOS runtime library or packaged game configuration was not delivered')
        for name in ('shaders', 'data', 'upscalers', 'lib'):
            if (build / name).is_dir():
                shutil.copytree(build / name, destination / name)
        runtime = destination / 'halo2_native'
        if audio_image:
            report['weapon_audio'] = weapon_audio(build, source, maps, audio_image,
                output / 'weapon-audio', output / 'validation/report.json')
            if not report['weapon_audio']['passed']:
                raise RuntimeError('Actual weapon audio checks failed; existing launcher selection retained')
        report['smoke'] = []
        cases = [(name, 1) for name in names]
        cases.extend((default_map, n) for n in sorted({players, 2, 4}) if n != 1)
        report['requested_smoke_cases'] = [dict(map=name, local_players=count) for name, count in cases]
        for name, count in cases:
            print(f'Checking {name}, {count} local player(s), four submitted frames…', flush=True)
            case = smoke(runtime, maps, name, count, scratch, args.timeout, root,
                         prior.get('video_driver', '') if prior else '')
            report['smoke'].append(case)
            if not case['passed']:
                report['unresolved_case_failures'].append(f'{name}:{count}')
        if report['unresolved_case_failures']:
            raise RuntimeError('Current engine did not complete its actual MCC frame check: ' +
                               ', '.join(report['unresolved_case_failures']))
        frontend_record = None
        if (maps / 'mainmenu.map').is_file():
            from qualify_frontend_runtime import qualify
            frontend_record = qualify(root, runtime, cache[0], output / 'frontend', scratch)
            atomic_json(destination / 'frontend-selection.json', frontend_record)
            report['frontend'] = dict(passed=True, runtime=frontend_record['runtime'])
        if source_hashes(source) != pins or any(digest(maps / name) != pin for name, pin in map_pins.items()):
            raise RuntimeError('Frozen engine source or MCC maps changed during validation; existing runtime retained')
        if (selection.read_bytes() if selection.is_file() else None) != previous:
            raise RuntimeError('Another task changed the launcher selection during validation; its selection retained')
        if frontend_record and (frontend_selection.read_bytes() if frontend_selection.is_file() else None) != previous_frontend:
            raise RuntimeError('Another task changed the frontend selection during validation; its selection retained')
        atomic_json(destination / 'source-hashes.json', pins)
        inventory = {p.relative_to(destination).as_posix(): dict(sha256=digest(p), bytes=p.stat().st_size)
                     for p in destination.rglob('*') if p.is_file()}
        atomic_json(destination / 'runtime-files.json', inventory)
        report.update(passed=True, runtime_sha256=digest(runtime),
                      retained_bytes=sum(p.stat().st_size for p in destination.rglob('*') if p.is_file()))
        atomic_json(destination / 'qualification.json', report)
        record = dict(schema='H2_LOCAL_GAMEPLAY_RUNTIME_SELECTION_V1', runtime=report['runtime'],
                      runtime_sha256=report['runtime_sha256'],
                      qualification=(destination / 'qualification.json').relative_to(root).as_posix(),
                      qualification_sha256=digest(destination / 'qualification.json'),
                      inventory=(destination / 'runtime-files.json').relative_to(root).as_posix(),
                      inventory_sha256=digest(destination / 'runtime-files.json'),
                      cache=cache[0].relative_to(root).as_posix(), maps=map_pins,
                      default_map=default_map, local_players=players)
        if prior and prior.get('video_driver'):
            record['video_driver'] = prior['video_driver']
        candidate_selection = destination / 'selection.json'
        atomic_json(candidate_selection, record)
        selected_gameplay_runtime(root, selection_name=candidate_selection.relative_to(root).as_posix())
        # Install the compiled launcher entry points too. Replace by rename so
        # an already running launcher can finish using its existing inode.
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            temporary = root / ('.' + name + '-' + uuid.uuid4().hex)
            try:
                shutil.copy2(destination / name, temporary)
                os.replace(temporary, root / name)
            finally:
                temporary.unlink(missing_ok=True)
        atomic_json(selection, record)
        promoted = True
        if frontend_record:
            atomic_json(frontend_selection, frontend_record)
        print(f'Both launchers now select {runtime}\nRetained runtime/resources: {report["retained_bytes"]} bytes', flush=True)
    except BaseException as error:
        report.update(passed=False, error=str(error))
        raise
    finally:
        if not promoted and destination.exists():
            shutil.rmtree(destination)
        atomic_json(output / 'report.json', report)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/launcher-engine-build/latest')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--status', action='store_true')
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--eos-client-config', type=Path,
                        help='Package shared untrusted game client settings and enable EOS online services')
    parser.add_argument('--weapon-audio-reference-xbe', type=Path,
                        help='Require component and repeated MCC weapon audio checks using this original reference XBE')
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--reuse-owned-build', action='store_true',
                        help='Reuse source/build inside an active run_test.py coordinator; requires --worker')
    args = parser.parse_args()
    if args.reuse_owned_build and (not args.worker or not os.environ.get('HALO2_TEST_SCRATCH')):
        parser.error('--reuse-owned-build requires --worker inside an active run_test.py coordinator')
    if args.status:
        if args.runtime and (args.root / 'CMakeLists.txt').is_file():
            print(source_status(args.root, args.runtime))
        return 0
    if args.jobs < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error('Positive build jobs and a finite positive smoke timeout are required')
    if sys.platform != 'linux':
        parser.error('Source engine promotion currently supports Linux only')
    if not args.worker:
        return subprocess.call([sys.executable, '-B', str(ROOT / 'tools/run_test.py'),
                                '--scratch-root', str(args.root / '.local/test-scratch'), '--',
                                sys.executable, '-B', str(Path(__file__).resolve()), '--worker', *sys.argv[1:]])
    signal.signal(signal.SIGTERM, lambda number, frame: sys.exit(128 + number))
    try:
        worker(args)
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print('Launcher engine build: ' + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
