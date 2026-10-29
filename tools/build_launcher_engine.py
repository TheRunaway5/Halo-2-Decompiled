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
import stat
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
    # source_campaign_screen_tu15.hpp and K12 of the native engine checklist
    # name the semantic replacement; linking both duplicates its entry points.
    'src/campaign/source_campaign_screen_tu15_generated.cpp': 'Superseded generated screen reference; production uses semantic lifecycle/timed kernels through source_campaign_screen_native_tu15.cpp',
    'src/physics/native_tu15_motion_pose.cpp': 'Standalone native pose component; actual integrate_pose retains the original production virtual owner',
    'src/gameplay/vehicles/native_tu15_seat_look.cpp': 'Standalone typed seat-look component without an admitted production camera caller',
    'src/physics/native_tu15_activation_agents.cpp': 'Standalone typed activation-agent composition without an admitted production activation caller',
    'src/physics/native_tu15_cached_motion.cpp': 'Qualified typed cached-motion component; live world activation remains a separate gate',
    'src/physics/native_tu15_compound_process.cpp': 'Standalone typed compound processing without an admitted production collision caller',
    'src/physics/native_tu15_contact_agent_dispatch.cpp': 'Offline typed contact-agent service composition; native factory and live creation coverage remain open',
    'src/physics/native_tu15_contact_agent_factory.cpp': 'Standalone typed contact-agent factory without an admitted production creation caller',
    'src/physics/native_tu15_contact_buffers.cpp': 'Standalone typed contact-buffer ownership without an admitted production contact caller',
    'src/physics/native_tu15_contact_candidate_buffers.cpp': 'Standalone typed candidate-buffer ownership without an admitted production candidate caller',
    'src/physics/native_tu15_contact_candidates.cpp': 'Typed candidate-update source review and live binding remain open',
    'src/physics/native_tu15_contact_producer.cpp': 'Standalone typed contact producer without an admitted production collision caller',
    'src/physics/native_tu15_contact_records.cpp': 'Standalone typed contact records without an admitted production producer caller',
    'src/physics/native_tu15_inverse_geometry.cpp': 'Qualified inverse-affine geometry component; native candidate-update integration remains open',
    'src/physics/native_tu15_island_activation.cpp': 'Standalone typed island activation without an admitted production world caller',
    'src/physics/native_tu15_array_allocator.cpp': 'Standalone typed array allocator requires the actual production allocator virtual owners',
    'src/physics/native_tu15_array_growth.cpp': 'Standalone typed array growth requires the actual production allocator virtual owners',
    'src/physics/native_tu15_array_reserve.cpp': 'Standalone typed array reserve requires its actual production allocator virtual owners',
    'src/physics/native_tu15_constraint_step.cpp': 'Standalone typed constraint step requires the actual production solver and commit owners',
    'src/physics/native_tu15_broad_phase.cpp': 'Standalone typed broad phase; production retains actual 0x321700 residual and its virtual service owners',
    'src/physics/native_tu15_constraint_solver.cpp': 'Standalone typed constraint solver pending positive-iteration physical qualification; production retains actual 0x326A40 residual',
    'src/physics/native_tu15_constraint_solver_rows.cpp': 'Standalone typed constraint row kernels pending their admitted solver consumer; production retains actual 0x326A40 residual',
    'src/physics/native_tu15_constraint_custom_schema.cpp': 'Standalone typed custom-schema callbacks require an admitted production solver consumer and physical qualification; production retains its existing custom-schema refusal/residual owner',
    'src/ui/vista/vista_lobby_leave_return.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/ui/vista/vista_network_browser.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/ui/vista/vista_network_browser_back.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/ui/original_matchmaking_fonts.cpp': 'Offline Xbox font reference control; excluded from the MCC runtime',
    'src/ui/original_matchmaking_emblems.cpp': 'Offline Xbox emblem reference control; excluded from the MCC runtime',
    'src/ui/original_matchmaking_audio.cpp': 'Offline Xbox matchmaking audio reference control; excluded from the MCC runtime',
    'src/ui/original_loading_assets.cpp': 'Offline Xbox loading asset reference control; excluded from the MCC runtime',
    'src/gameplay/multiplayer/multiplayer_locomotion_continuity.cpp': 'Locomotion group is not yet part of the production handoff frontier',
    'src/ui/widgets/native_vista_widget_image.cpp': 'Original Vista PE widget-image component without a production caller',
    'src/ui/widgets/vista_widget_backing.cpp': 'Vista widget heap component requires actual allocation providers',
    'src/ui/widgets/vista_widget_font_ambient.cpp': 'Vista ambient font environment component without a production caller',
    'src/rendering/world/source_sky_stencil.cpp': 'Frozen sky-stencil reference prototype requires actual production draw inputs',
    'src/rendering/world/source_decorator_decals_tu15.cpp': 'Decorator asset preparation requires an admitted production BSP scene consumer',
    'src/rendering/world/source_decorator_owners_tu15_generated.cpp': 'Decorator native owners require an admitted production BSP scene consumer',
    'src/rendering/effects/source_decorator_decal_draw.cpp': 'Decorator draw component requires the actual production BSP command bank',
    'src/rendering/effects/source_decorator_decal_material.cpp': 'Decorator material component requires the admitted production draw consumer',
    'src/rendering/loading/source_loading_program_tu15_generated.cpp': 'TU1.5 loading compositor component without a production entry/service caller',
    'src/ui/source_loading_assets.cpp': 'MCC loading bitmap adapter without a production loading-scene caller',
    'src/ui/menu_world_vertex_source.cpp': 'Offline native vertex code generation; production uses immutable kernel and metadata catalogs',
}


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


TOOL_SOURCE_SUFFIXES = frozenset({'.py','.pyi','.sh','.bash','.bat','.c','.cc','.cpp','.cxx',
    '.h','.hpp','.hxx','.inl','.inc','.s','.S','.cmake','.json','.jsonl','.tsv','.csv',
    '.txt','.md','.rst','.yaml','.yml','.toml','.glsl','.vert','.frag','.geom','.comp',
    '.fx','.cs','.java','.xml','.ini','.config','.proto','.def','.idl','.rc','.tt','.ttinclude','.zlib'})


def tool_source_path(path):
    """Generator code and authored inputs, excluding caches/build/editor outputs."""
    relative = Path(path)
    if any(part in {'__pycache__','target','build','CMakeFiles','node_modules','tmp','temp'} or
           part.startswith(('.', 'cmake-build-')) for part in relative.parts[:-1]):
        return False
    return (relative.suffix in TOOL_SOURCE_SUFFIXES and not relative.name.startswith('.') and
            not re.search(r'(?:^|[.-])(?:tmp|temp|bak|orig|rej)(?:[.-]|$)', relative.name))


def source_hashes(root):
    files = [root / 'CMakeLists.txt']
    for name in ('src', 'include', 'cmake', 'shaders', 'data', 'rust'):
        files.extend(p for p in (root / name).rglob('*') if p.is_file()
                     and 'target' not in p.relative_to(root / name).parts)
    files.extend(p for p in (root / 'tools').rglob('*') if p.is_file() and
                 tool_source_path(p.relative_to(root/'tools')))
    files.extend(p for p in (root / 'multiplayer/master_server').rglob('*') if p.is_file() and
                 tool_source_path(p.relative_to(root/'multiplayer/master_server')))
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
    # Native ONLINE validation executes this Python service with the frozen
    # Rust coordinator. Borrowing its code could mix two protocol revisions.
    multiplayer = root / 'multiplayer'
    if multiplayer.is_dir():
        (destination / 'multiplayer').mkdir()
        for path in multiplayer.iterdir():
            if path.name == 'master_server' and path.is_dir():
                shutil.copytree(path, destination / 'multiplayer/master_server',
                    ignore=shutil.ignore_patterns('__pycache__', '.pytest_cache', '.git', 'target'))
            elif path.is_file():
                shutil.copy2(path, destination / 'multiplayer' / path.name)
            elif path.is_dir() and path.name != '__pycache__':
                (destination / 'multiplayer' / path.name).symlink_to(path, target_is_directory=True)
    for name in ('assets', 'resources', 'analysis', '.tools', 'Halo 2 Vista installer'):
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
                if n == 'CMakeLists.txt' or n.startswith(('src/', 'include/', 'cmake/', 'shaders/', 'data/', 'rust/')) or
                   (n.startswith('tools/') and tool_source_path(n.removeprefix('tools/')))}
    current = source_hashes(root)
    legacy = not any(n.startswith('tools/') for n in recorded) and any(n.startswith('tools/') for n in current)
    if legacy:
        # This is a diagnostic only. Keep qualified older runtimes usable,
        # while avoiding a false claim that their generator provenance matches.
        current = {n:h for n,h in current.items() if not n.startswith('tools/')}
    changed = sum(recorded.get(n) != current.get(n) for n in recorded.keys() | current.keys())
    status = ('Engine source: matches the selected runtime' if not changed else
              f'Engine source: {changed} files differ from the selected runtime; use --build-engine')
    return status+'; selected runtime lacks generator source provenance' if legacy else status


def wiring(graph, root):
    root = root.resolve()
    targets = {}
    for line in graph.read_text().splitlines():
        name, links, dependencies, sources = line.split('|')
        normalized = []
        for source in sources.split(';'):
            path = Path(source)
            if path.is_absolute():
                # CMAKE_CURRENT_LIST_DIR commonly contributes /cmake/../src.
                # Compare the actual source path with the cohort root before
                # classifying it; lexical containment can also admit ../ exits.
                path = path.resolve()
                if path.is_relative_to(root):
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
        # target_sources can archive an OBJECT library without listing it in
        # LINK_LIBRARIES. Follow only an exact ownership expression from a
        # reachable target; build-only dependencies and conditional expressions
        # do not establish that an object's code reached the engine.
        for source in targets[name][2]:
            objects = re.fullmatch(r'\$<TARGET_OBJECTS:([^<>;]+)>', source)
            if objects and objects[1] in targets:
                pending.append(objects[1])
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
                scope='Transitive library links and direct object-source ownership; separate components require real callers before runtime acceptance')


def runtime_environment(scratch):
    # Qualification exercises the executable's production defaults. Diagnostic,
    # feature and artifact-retention overrides from a developer's shell must
    # not silently change either role. Callers add only their explicit owned
    # paths and declared controls; unrelated SDK/display settings remain.
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith('HALO2_')}
    environment['HALO2_TEST_SCRATCH'] = str(scratch)
    return environment


def smoke(runtime, maps, name, players, scratch, timeout, root=ROOT, video_driver=''):
    log = scratch / f'{Path(name).stem}-{players}.log'
    preferences = scratch / 'smoke-preferences' / uuid.uuid4().hex
    command = [str(runtime), '--multiplayer', '--graphics-mode', 'Original', '--map', str(maps / name),
               '--shared-maps', str(maps), '--local-players', str(players),
               '--frames', '4', '--mute', '--viewport', '640', '480']
    # Exercise ordinary SDL/Vulkan frame submission, with no Xbox input,
    # synthetic runtime backing, or imported UI overrides.
    environment = runtime_environment(scratch)
    environment.update(HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch / 'vulkan-cache'),
                       MESA_SHADER_CACHE_DIR=str(scratch / 'mesa-cache'),
                       __GL_SHADER_DISK_CACHE_PATH=str(scratch / 'nvidia-cache'),
                       XDG_DATA_HOME=str(preferences / 'data'),
                       XDG_CONFIG_HOME=str(preferences / 'config'),
                       XDG_CACHE_HOME=str(preferences / 'cache'))
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


RENDERER_4K_CASES = ('4:90:0:0', '4:150:10:110', '1:150:10:110', '2:150:10:110')


def renderer_reference_controls(assignments):
    controls = {}
    for assignment in assignments:
        name, separator, value = assignment.partition('=')
        if (not separator or not re.fullmatch(r'HALO2_REFERENCE_[A-Z0-9_]+', name) or
                not value or any(c in value for c in ',@\r\n\0') or name in controls):
            raise ValueError('Renderer 4K reference controls require unique HALO2_REFERENCE_NAME=VALUE assignments without comma/@/line breaks')
        controls[name] = value
    return controls


def renderer_4k_qualification(source, runtime, maps, output, scratch, controls):
    """Compare the exact packaged executable before publishing its selection.

    This is submitted-frame equality, not a performance or all-map gate. The
    frozen parity driver owns capture processes, private preferences and raw
    frames. Only its bounded report and driver-log tail leave scratch.
    """
    source, runtime, maps = source.resolve(), runtime.resolve(strict=True), maps.resolve(strict=True)
    output.mkdir(parents=True, exist_ok=True)
    helper = source / 'tools/perf_4k60/parity.py'
    map_path = maps / 'coagulation.map'
    parity_report = output / 'parity.json'
    parity_report.unlink(missing_ok=True)
    log = scratch / 'renderer-4k-parity.log'
    receipt = dict(passed=False, scope='Original/Off 3840x2160 submitted-frame equality; no 4K60 claim',
                   runtime=str(runtime), reference_controls=controls, cases=list(RENDERER_4K_CASES),
                   parity_report=str(parity_report), timeout_seconds=6600)
    try:
        pins = dict(runtime_sha256=digest(runtime), helper_sha256=digest(helper), map_sha256=digest(map_path))
        receipt.update(pins)
        assignments = ','.join(f'{name}={value}' for name, value in sorted(controls.items()))
        command = [sys.executable, '-B', str(source / 'tools/run_test.py'),
                   '--scratch-root', str(scratch / 'renderer-4k-captures'), '--',
                   sys.executable, '-B', str(helper), str(parity_report),
                   assignments + '@' + str(runtime), str(runtime), *RENDERER_4K_CASES,
                   '--maps', str(maps)]
        receipt['command'] = command
        environment = runtime_environment(scratch)
        environment.update(H2_MAP='coagulation', H2_REPEAT_CANDIDATE='1')
        print('Checking exact packaged renderer: Original/Off 4K, four parity cases and repeats…', flush=True)
        with log.open('wb') as stream:
            result = subprocess.run(command, cwd=runtime.parent, env=environment,
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=6600)
        receipt['exit_code'] = result.returncode
        if result.returncode:
            raise RuntimeError('Packaged renderer 4K parity process failed')
        if (parity_report.is_symlink() or not parity_report.is_file() or
                not 0 < parity_report.stat().st_size <= 1024 * 1024):
            raise RuntimeError('Packaged renderer 4K parity has no bounded direct report')
        data = json.loads(parity_report.read_text())
        if (data.get('passed') is not True or data.get('reference') != str(runtime) or
                data.get('candidate') != str(runtime) or data.get('reference_env') != controls or
                data.get('candidate_env') != {} or data.get('maps') != str(maps) or
                data.get('map') != 'coagulation' or data.get('map_path') != str(map_path) or
                data.get('native_size') != [3840, 2160] or
                any(data.get(name) != value for name, value in pins.items() if name != 'runtime_sha256') or
                data.get('reference_sha256') != pins['runtime_sha256'] or
                data.get('candidate_sha256') != pins['runtime_sha256']):
            raise RuntimeError('Packaged renderer 4K report does not qualify this executable/helper/map/control cohort')
        cases = data.get('cases', [])
        if (not isinstance(cases, list) or any(not isinstance(case, dict) for case in cases) or
                [case.get('case') for case in cases] != list(RENDERER_4K_CASES)):
            raise RuntimeError('Packaged renderer 4K parity did not complete the four required cases')
        expected_bytes = 3840 * 2160 * 3 + len(b'P6\n3840 2160\n255\n')
        for index, case in enumerate(cases):
            roles = ['reference', 'reference-repeat', 'candidate', 'candidate-repeat'] if index == 0 else ['reference', 'candidate']
            runs = case.get('runs', [])
            if not isinstance(runs, list) or any(not isinstance(run, dict) for run in runs):
                raise RuntimeError('Packaged renderer 4K capture records are malformed')
            hashes = [run.get('sha256') for run in runs]
            if (case.get('passed') is not True or case.get('captures_valid') is not True or
                    case.get('identical') is not True or [run.get('role') for run in runs] != roles or
                    not hashes or any(not isinstance(pin, str) or not re.fullmatch('[0-9a-f]{64}', pin) for pin in hashes) or
                    len(set(hashes)) != 1 or
                    any(run.get('valid') is not True or type(run.get('exit_code')) is not int or
                        run['exit_code'] != 0 or run.get('timed_out') or run.get('bytes') != expected_bytes
                        for run in runs) or
                    (index == 0 and (case.get('reference_deterministic') is not True or
                                     case.get('candidate_deterministic') is not True))):
                raise RuntimeError('Packaged renderer 4K parity has invalid, differing or incomplete captures/repeats')
        if pins != dict(runtime_sha256=digest(runtime), helper_sha256=digest(helper), map_sha256=digest(map_path)):
            raise RuntimeError('Packaged renderer 4K executable/helper/map changed during qualification')
        receipt.update(passed=True, parity_report_sha256=digest(parity_report))
    except (OSError, ValueError, KeyError, TypeError, RuntimeError, subprocess.TimeoutExpired) as error:
        receipt['error'] = str(error)[-1500:]
    finally:
        if (not parity_report.is_symlink() and parity_report.is_file() and
                parity_report.stat().st_size > 1024 * 1024):
            receipt['discarded_oversized_report_bytes'] = parity_report.stat().st_size
            parity_report.unlink()
        if log.is_file():
            with log.open('rb') as stream:
                stream.seek(max(0, log.stat().st_size - 16384))
                tail = stream.read()
            (output / 'driver.log').write_bytes(tail)
        atomic_json(output / 'report.json', receipt)
    return receipt


def atomic_json(path, value):
    temporary = path.with_name('.' + path.name + '-' + uuid.uuid4().hex)
    try:
        temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def publication_identity(path):
    """Read a regular publication file without adopting a concurrent rewrite."""
    try:
        before = path.lstat()
    except FileNotFoundError:
        return None
    if not stat.S_ISREG(before.st_mode):
        raise RuntimeError('Publication file is not regular: ' + path.name)
    pin = digest(path)
    after = path.lstat()
    stable = lambda value: (value.st_dev, value.st_ino, value.st_size, value.st_mtime_ns,
                            value.st_ctime_ns, value.st_mode)
    if stable(before) != stable(after):
        raise RuntimeError('Publication file changed while reading: ' + path.name)
    # Rename may change ctime. The incoming inode, contents, mtime and mode
    # remain usable even if os.replace commits and then raises/interruption.
    return (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns,
            stat.S_IMODE(after.st_mode), pin)


def publish_runtime_files(root, destination, scratch, record, frontend_record, report, expected,
                          *, cache_choice=None):
    """Rollback only this transaction's still-owned root files on any failure."""
    backup = scratch / ('engine-publication-' + uuid.uuid4().hex)
    backup.mkdir()
    state = report['publication'] = dict(complete=False, rollback_failed=False, files=[])
    changes, temporary_files = [], []
    entries = [(root / name, destination / name, None)
               for name in ('Halo2-Launcher', 'Halo2-Title-Launcher')]
    entries.append((root / 'gameplay-runtime-selection.json', None, record))
    if frontend_record:
        entries.append((root / 'frontend-runtime-selection.json', None, frontend_record))
    if cache_choice is not None:
        from mcc_onboarding import REGISTRY, SCHEMA, private_cache
        candidate = Path(cache_choice)
        cache, _ = private_cache(root, candidate if candidate.is_absolute() else root / candidate,
                                 verify=True)
        relative = cache.relative_to(root).as_posix()
        if any(c in relative for c in '\t\r\n\0') or record.get('cache') != relative:
            raise ValueError('Publication cache differs from its qualified runtime')
        # Onboarding's selection is a two-line TSV contract, not JSON.
        entries.append((root / REGISTRY, None, (SCHEMA + '\ncache\t' + relative + '\n').encode()))
    try:
        for target, source, value in entries:
            name = target.relative_to(root).as_posix()
            previous = expected[name]
            if publication_identity(target) != previous:
                raise RuntimeError('Another task changed publication file: ' + name)
            saved = backup / target.name
            if previous is not None:
                shutil.copy2(target, saved)
                if digest(saved) != previous[-1] or publication_identity(target) != previous:
                    raise RuntimeError('Publication backup changed: ' + target.name)
            row = dict(name=name, previous_sha256=previous[-1] if previous else None,
                       candidate_sha256=None, outcome='unchanged')
            state['files'].append(row)
            changes.append(dict(target=target, source=source, value=value, previous=previous,
                                saved=saved, candidate=None, row=row))
        for change in changes:
            target = change['target']
            temporary = target.with_name('.' + target.name + '-' + uuid.uuid4().hex)
            temporary_files.append(temporary)
            if change['source'] is not None:
                shutil.copy2(change['source'], temporary)
            elif isinstance(change['value'], bytes):
                temporary.write_bytes(change['value'])
            else:
                temporary.write_text(json.dumps(change['value'], indent=2, sort_keys=True) + '\n')
            change['candidate'] = publication_identity(temporary)
            change['row']['candidate_sha256'] = change['candidate'][-1]
            if publication_identity(target) != change['previous']:
                raise RuntimeError('Another task changed publication file: ' + target.name)
            # Register the exact incoming inode before rename, including the
            # failure window after rename but before this call returns.
            change['row']['outcome'] = 'attempted'
            os.replace(temporary, target)
            change['row']['outcome'] = 'published'
        if cache_choice is not None:
            from gameplay_runtime_selection import selected_gameplay_runtime
            if selected_gameplay_runtime(root) != record:
                raise RuntimeError('Published gameplay/cache selection failed final admission')
        state['complete'] = True
    except BaseException:
        # If cancellation interrupts rollback itself, conservatively keep the
        # candidate; a later finally must not delete a referenced runtime.
        state['rollback_failed'] = True
        failed = False
        for change in reversed(changes):
            if change['candidate'] is None:
                continue
            target = change['target']
            try:
                if publication_identity(target) != change['candidate']:
                    change['row']['outcome'] = 'foreign_or_missing_preserved'
                    continue
                if change['previous'] is None:
                    target.unlink()
                else:
                    temporary = target.with_name('.' + target.name + '-restore-' + uuid.uuid4().hex)
                    temporary_files.append(temporary)
                    shutil.copy2(change['saved'], temporary)
                    if digest(temporary) != change['previous'][-1]:
                        raise RuntimeError('Publication backup is no longer authentic')
                    if publication_identity(target) != change['candidate']:
                        change['row']['outcome'] = 'foreign_or_missing_preserved'
                        continue
                    os.replace(temporary, target)
                change['row']['outcome'] = 'rolled_back'
            except BaseException as error:
                failed = True
                change['row'].update(outcome='rollback_failed', error_class=type(error).__name__)
        state['rollback_failed'] = failed
        raise
    finally:
        for path in temporary_files:
            path.unlink(missing_ok=True)


def candidate_references(root, destination, publication):
    """Fail closed for root files that still contain a candidate path/binary."""
    expected = {row['name']: row['candidate_sha256'] for row in publication.get('files', [])}
    needles = (str(destination).encode(), destination.relative_to(root).as_posix().encode())
    references = []
    for name in ('Halo2-Launcher', 'Halo2-Title-Launcher',
                 'gameplay-runtime-selection.json', 'frontend-runtime-selection.json'):
        path = root / name
        try:
            identity = publication_identity(path)
            if identity is None:
                continue
            found = identity[-1] == expected.get(name)
            with path.open('rb') as stream:
                tail = b''
                while not found and (part := stream.read(65536)):
                    data = tail + part
                    found = any(needle in data for needle in needles)
                    tail = data[-max(map(len, needles)):]
            if publication_identity(path) != identity:
                raise RuntimeError('Publication reference changed while reading')
            if found:
                references.append(dict(name=name, status='candidate_reference'))
        except BaseException as error:
            references.append(dict(name=name, status='reference_unknown', error_class=type(error).__name__))
    return references


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
    from mcc_onboarding import REGISTRY, private_cache, selected
    root = args.root.resolve(strict=True)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    prepare_only = getattr(args, 'prepare_only', False)
    renderer_controls = renderer_reference_controls(getattr(args, 'renderer_4k_reference_control', []))
    # Preserve the exact existing selection if another task promotes a runtime
    # while this build is in flight.
    selection = root / SELECTION
    previous = selection.read_bytes() if selection.is_file() else None
    frontend_selection = root / 'frontend-runtime-selection.json'
    previous_frontend = frontend_selection.read_bytes() if frontend_selection.is_file() else None
    cache_candidate = getattr(args, 'cache_candidate', None)
    publication_names = ('Halo2-Launcher', 'Halo2-Title-Launcher', SELECTION, 'frontend-runtime-selection.json')
    if cache_candidate is not None:
        publication_names += (REGISTRY,)
    publication_baseline = {} if prepare_only else {
        name: publication_identity(root / name) for name in
        publication_names}
    baseline_selection = getattr(args, 'baseline_selection', None)
    prior = (selected_gameplay_runtime(root, selection_name=baseline_selection) if baseline_selection
             else selected_gameplay_runtime(root))
    if cache_candidate is None:
        cache = selected(root)
    else:
        candidate = Path(cache_candidate)
        cache = private_cache(root, candidate if candidate.is_absolute() else root / candidate,
                              verify=True)
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
    configuration = getattr(args, 'config', 'Release')
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
    from custom_assets import inventory as custom_asset_inventory, install as install_custom_assets
    asset_pins = custom_asset_inventory(root)
    map_pins = {name: digest(maps / name) for name in names}
    destination = root / 'bin/engine-builds' / uuid.uuid4().hex
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    atomic_json(output / 'source-hashes.json', pins)
    report = dict(passed=False,
                  scope='qualified_candidate_only' if prepare_only else
                      'Owned frozen source cohort, launcher routing checks and MCC submitted-frame smoke'
                      if reuse_owned_build else
                      'Current source build, launcher routing checks and MCC submitted-frame smoke',
                  status='building', qualified_candidate=False, runtime_installed=False,
                  runtime=destination.relative_to(root).as_posix() + '/halo2_native',
                  unresolved_case_failures=[], accepted_case_failures=[],
                  limits=['No full gameplay, Xbox framebuffer or audio parity claim'])
    report['reused_active_owned_build'] = reuse_owned_build
    report['build_configuration'] = configuration
    # A reused qualification belongs to this declared source/build pair, which
    # can intentionally differ from a concurrently edited working tree. Record
    # its exact inventory rather than calling that tree the current sources.
    report['source_cohort'] = dict(
        kind='active_owned_frozen' if reuse_owned_build else 'working_tree_snapshot',
        source=str(source.resolve()), build=str((scratch / 'build').resolve()),
        inventory_sha256=digest(output / 'source-hashes.json'))
    if cache_candidate is not None:
        report['cache_candidate'] = cache[0].relative_to(root).as_posix()
    promoted = False
    prepared = False
    candidate_qualified = False
    try:
        print('Building the complete engine and both launchers in disposable storage…', flush=True)
        command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--build-worker',
                   '--source', str(source), '--output', str(output / 'validation'),
                   '--target', 'halo2_native', '--target', 'halo2_launcher',
                   '--target', 'halo2_title_launcher', '--target', 'halo2_onboarding',
                   '--target', 'shadow_array_swap_tu15_tests',
                   '--target', 'multiplayer_tests', '--target', 'native_request_protocol_tests',
                   '--target', 'frontend_custom_variant_settings_tests',
                   '--target', 'frontend_custom_match_settings_tests',
                   '--target', 'frontend_match_customization_tests',
                   '--filter', '^(launcher_first_run|title_launcher_first_run|bundled_launcher_first_run|'
                   'launcher_automatic_build|launcher_engine_promotion|native_gameplay_launcher_routing|native_frontend_launcher_routing|original_tu15_shadow_array_swap|native_mcc_split_screen_menu_startup|'
                   'native_request_protocol|native_session_configuration|native_session_configuration_loading|native_custom_variant_settings|'
                   'original_frontend_custom_match_settings|native_mcc_frontend_custom_match_settings|original_frontend_match_customization|native_mcc_frontend_match_customization)$',
                   '--jobs', str(args.jobs), '--test-jobs', '1',
                   '--config', configuration,
                   '--cmake-arg=-DHALO2_BUILD_LAUNCHERS_WITH_NATIVE=ON',
                   '--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF',
                   '--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=ON',
                   '--cmake-arg=-DHALO2_CUSTOM_ASSET_ROOT=' + str(root),
                   '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT=' + str(root),
                   '--cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP=' + str(maps / default_map),
                   '--cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF',
                   '--cmake-arg=-DHALO2_LAUNCHER_RUNTIME_DIR=' + str(destination),
                   '--cmake-arg=-DHALO2_RUNTIME_SHADER_DIR=' + str(destination / 'shaders'),
                   '--retain-report', 'engine-targets.txt',
                   '--retain-report', 'multiplayer-menu-startup.json']
        # A present kit bank changes the actual loading path. Require its
        # submitted-frame handoffs and the fresh/reuse onboarding regressions
        # before a candidate can replace either selected launcher runtime.
        if (cache[0] / 'presentation/original-loading-v2/manifest.json').is_file():
            command.extend(['--target', 'original_loading_gpu_tests',
                '--target', 'native_loading_presentation_tests',
                '--target', 'bundled_loading_ambience_tests', '--target', 'vista_loading_tests',
                '--target', 'vista_loading_thread_tests',
                '--cmake-arg=-DHALO2_ORIGINAL_LOADING_CACHE=' + str(cache[0])])
            at = command.index('--filter') + 1
            command[at] = command[at][:-2] + ('|original_loading_gpu|original_loading_gpu_4_3|native_vista_loading_adapter|h2ek_loading_onboarding|'
                'bundled_loading_ambience|original_vista_loading|original_vista_loading_thread)$')
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
        # The executables/tests bind their compiled default to the canonical
        # candidate shader bank. Build first without claiming validation, then
        # make that bank available before any required submitted-frame tests.
        build_command=command.copy()
        build_output=output/'build-only'
        build_command[build_command.index('--output')+1]=str(build_output)
        build_command.append('--build-only')
        result = subprocess.run(build_command, cwd=root, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError('Engine build failed; see ' + str(build_output))
        build_report=json.loads((build_output/'report.json').read_text())
        if (build_report.get('status')!='built' or build_report.get('passed')is not False or
                build_report.get('validation_not_run')is not True):
            raise RuntimeError('Engine build-only receipt must not claim test acceptance')
        if source_hashes(source) != pins:
            raise RuntimeError('Frozen engine source changed during the build')
        build = scratch / 'build'
        destination.mkdir(parents=True,exist_ok=False)
        from stage_native_renderer_shaders import stage_production_shaders
        from native_renderer_shaders import validate_native_package
        shader_package=stage_production_shaders(build,destination/'shaders')
        staged_shader_proof=validate_native_package(destination/'shaders',executable=build/'halo2_native')
        executable_pin=digest(build/'halo2_native')
        report['native_renderer_shader_package']={
            'passed':shader_package['passed'],
            'module_count':shader_package['module_count'],
            'inventory_sha256':shader_package['inventory_sha256'],
            'report':str((destination/'shaders/production-shaders.json').relative_to(root)),
            'report_sha256':staged_shader_proof['package']['sha256']}
        print('Checking the complete qualification suite against the staged production shader bank…',flush=True)
        result=subprocess.run(command,cwd=root,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError('Engine launcher or loading checks failed; see '+str(output/'validation'))
        validation_report=json.loads((output/'validation/report.json').read_text())
        if (validation_report.get('passed')is not True or validation_report.get('status')!='passed' or
                validation_report.get('validation_not_run')is not False or
                not any(step.get('name')=='tests' and step.get('exit_code')==0
                        for step in validation_report.get('steps',[]))):
            raise RuntimeError('Engine qualification requires completed passing CTest checks')
        if source_hashes(source)!=pins:
            raise RuntimeError('Frozen engine source changed during qualification')
        if digest(build/'halo2_native')!=executable_pin:
            raise RuntimeError('Engine executable changed after its shader bank was staged')
        if validate_native_package(destination/'shaders',
                expected_sha256=staged_shader_proof['package']['sha256'],
                executable=build/'halo2_native')!=staged_shader_proof:
            raise RuntimeError('Staged production shader bank changed during qualification')
        report['validation']=dict(passed=True,report=str(output/'validation/report.json'),
            report_sha256=digest(output/'validation/report.json'),
            build_only_report=str(build_output/'report.json'),
            build_only_report_sha256=digest(build_output/'report.json'))
        menu_startup_path=output/'validation/evidence/multiplayer-menu-startup.json'
        menu_startup=json.loads(menu_startup_path.read_text())
        if menu_startup.get('passed') is not True or menu_startup.get('frames',0)<1:
            raise RuntimeError('Engine qualification requires the real non-reference split-screen menu Start')
        report['split_screen_menu_startup']=dict(passed=True,report=str(menu_startup_path),
            report_sha256=digest(menu_startup_path),frames=menu_startup['frames'])
        # An active frozen cohort can predate CMake's custom-source target.
        # Asset delivery has its own current, pinned owner, so stage it through
        # this coordinator even when that older build did not create a bank.
        staged_assets = install_custom_assets(root, build / 'custom-assets')
        if staged_assets['files'] != asset_pins:
            raise RuntimeError('Custom assets changed during the engine build; existing runtime retained')
        report['wiring'] = wiring(build / 'engine-targets.txt', source)
        if not report['wiring']['passed']:
            raise RuntimeError('Engine sources are missing from the runtime link graph: ' +
                               ', '.join(report['wiring']['unexpected_unwired_sources']))
        for name in ('halo2_native', 'Halo2-Launcher', 'Halo2-Title-Launcher', 'eos_sdk_worker'):
            shutil.copy2(build / name, destination / name)
        for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env'):
            if (build / name).is_file():
                shutil.copy2(build / name, destination / name)
        if client_config and not all((destination / name).is_file() for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env')):
            raise RuntimeError('EOS runtime library or packaged game configuration was not delivered')
        for name in ('data', 'upscalers', 'lib'):
            if (build / name).is_dir():
                shutil.copytree(build / name, destination / name)
        custom_assets = install_custom_assets(build / 'custom-assets', destination / 'custom-assets')
        if custom_assets['files'] != asset_pins:
            raise RuntimeError('Custom assets changed during the engine build; existing runtime retained')
        report['custom_assets'] = dict(
            path='custom-assets/custom-assets.json',
            sha256=digest(destination / 'custom-assets/custom-assets.json'),
            files=len(asset_pins), bytes=custom_assets['bytes'],
            scope='Authenticated custom asset sources; gameplay admission remains separate')
        runtime = destination / 'halo2_native'
        from native_gameplay_test import eos_worker_descriptor, validate_eos_worker
        report['eos_worker'] = eos_worker_descriptor(runtime)
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
                key = f'{name}:{count}'
                # An explicitly accepted case stays recorded as failed in the
                # qualification; it no longer blocks this one publication.
                if key in (getattr(args, 'accept_smoke_failure', None) or []):
                    print(f'Accepted failing smoke case {key}; recorded in the qualification', flush=True)
                    report['accepted_case_failures'].append(key)
                else:
                    report['unresolved_case_failures'].append(key)
        if report['unresolved_case_failures']:
            raise RuntimeError('Current engine did not complete its actual MCC frame check: ' +
                               ', '.join(report['unresolved_case_failures']))
        frontend_record = None
        if (maps / 'mainmenu.map').is_file():
            from qualify_frontend_runtime import qualify
            frontend_record = qualify(root, runtime, cache[0], output / 'frontend', scratch,
                                      report['eos_worker'])
            atomic_json(destination / 'frontend-selection.json', frontend_record)
            report['frontend'] = dict(passed=True, runtime=frontend_record['runtime'])
        if renderer_controls:
            report['renderer_4k'] = dict(passed=False, report=str(output / 'renderer-4k/report.json'))
            report['renderer_4k'].update(renderer_4k_qualification(
                source, runtime, maps, output / 'renderer-4k', scratch, renderer_controls))
            if report['renderer_4k'].get('passed') is not True:
                raise RuntimeError('Packaged renderer 4K frame qualification failed; existing launcher selection retained')
            if digest(runtime) != executable_pin:
                raise RuntimeError('Packaged renderer executable changed after its compiled shader qualification')
        if (source_hashes(source) != pins or custom_asset_inventory(root) != asset_pins or
                any(digest(maps / name) != pin for name, pin in map_pins.items())):
            raise RuntimeError('Frozen engine source or MCC maps changed during validation; existing runtime retained')
        if not prepare_only and (selection.read_bytes() if selection.is_file() else None) != previous:
            raise RuntimeError('Another task changed the launcher selection during validation; its selection retained')
        if not prepare_only and frontend_record and (frontend_selection.read_bytes() if frontend_selection.is_file() else None) != previous_frontend:
            raise RuntimeError('Another task changed the frontend selection during validation; its selection retained')
        atomic_json(destination / 'source-hashes.json', pins)
        validate_eos_worker(runtime, report['eos_worker'])
        if digest(runtime) != executable_pin:
            raise RuntimeError('Packaged renderer executable changed before final qualification sealing')
        inventory = {p.relative_to(destination).as_posix(): dict(sha256=digest(p), bytes=p.stat().st_size)
                     for p in destination.rglob('*') if p.is_file()}
        atomic_json(destination / 'runtime-files.json', inventory)
        report.update(passed=True, status='prepared', qualified_candidate=True, runtime_sha256=executable_pin,
                      retained_bytes=sum(p.stat().st_size for p in destination.rglob('*') if p.is_file()))
        atomic_json(destination / 'qualification.json', report)
        record = dict(schema='H2_LOCAL_GAMEPLAY_RUNTIME_SELECTION_V1', runtime=report['runtime'],
                      runtime_sha256=report['runtime_sha256'],
                      qualification=(destination / 'qualification.json').relative_to(root).as_posix(),
                      qualification_sha256=digest(destination / 'qualification.json'),
                      inventory=(destination / 'runtime-files.json').relative_to(root).as_posix(),
                      inventory_sha256=digest(destination / 'runtime-files.json'),
                      eos_worker=report['eos_worker'],
                      cache=cache[0].relative_to(root).as_posix(), maps=map_pins,
                      default_map=default_map, local_players=players)
        if prior and prior.get('video_driver'):
            record['video_driver'] = prior['video_driver']
        candidate_selection = destination / 'selection.json'
        atomic_json(candidate_selection, record)
        admission = dict(selection_name=candidate_selection.relative_to(root).as_posix())
        if cache_candidate is not None:
            admission['cache_candidate'] = cache[0]
        selected_gameplay_runtime(root, **admission)
        candidate_qualified = True
        if prepare_only:
            # Qualification and the actual admission reader have passed.
            # Keep this explicit candidate without adopting any ROOT selection.
            prepared = True
            print(f'Qualified candidate prepared at {runtime}\nROOT launchers/selections were not published; '
                  f'retained runtime/resources: {report["retained_bytes"]} bytes', flush=True)
        else:
            # Existing launchers can finish using their inode; the publication
            # transaction also preserves them if a later root write fails.
            publication = dict(cache_choice=cache[0]) if cache_candidate is not None else {}
            publish_runtime_files(root, destination, scratch, record, frontend_record, report,
                                  publication_baseline, **publication)
            promoted = True
            report.update(status='published', runtime_installed=True)
            print(f'Both launchers now select {runtime}\nRetained runtime/resources: {report["retained_bytes"]} bytes', flush=True)
    except BaseException as error:
        report.update(passed=False, status='failed', qualified_candidate=candidate_qualified, error=str(error))
        raise
    finally:
        if not (promoted or prepared) and destination.exists():
            publication = report.setdefault('publication', {})
            references = candidate_references(root, destination, publication)
            publication['candidate_references'] = references
            publication['candidate_retained'] = bool(references or publication.get('rollback_failed'))
            if publication['candidate_retained']:
                report['retained_bytes'] = sum(p.stat().st_size for p in destination.rglob('*') if p.is_file())
            else:
                shutil.rmtree(destination)
        atomic_json(output / 'report.json', report)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/launcher-engine-build/latest')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--config', choices=('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'),
                        default='Release', help='CMake build configuration (default: Release)')
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
    parser.add_argument('--accept-smoke-failure', action='append', default=[], metavar='MAP:PLAYERS',
                        help='Publish even if this smoke case fails (for example elongation.map:1); '
                             'the failure is recorded as accepted in the qualification')
    parser.add_argument('--prepare-only', action='store_true',
                        help='Qualify and retain a candidate without publishing ROOT launchers or selections')
    parser.add_argument('--baseline-selection',
                        help='Validate this project-relative qualified selection as the preparation baseline; requires --prepare-only')
    parser.add_argument('--cache-candidate', type=Path,
                        help='Qualify this verified private MCC cache without selecting it during preparation')
    parser.add_argument('--renderer-4k-reference-control', action='append', default=[], metavar='NAME=VALUE',
                        help='Require four Original/Off 4K parity cases on this exact packaged runtime versus these reference environment controls before publication (repeatable)')
    args = parser.parse_args()
    if args.reuse_owned_build and (not args.worker or not os.environ.get('HALO2_TEST_SCRATCH')):
        parser.error('--reuse-owned-build requires --worker inside an active run_test.py coordinator')
    if args.baseline_selection and not args.prepare_only:
        parser.error('--baseline-selection requires --prepare-only')
    if args.status:
        if args.runtime and (args.root / 'CMakeLists.txt').is_file():
            print(source_status(args.root, args.runtime))
        return 0
    if args.jobs < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error('Positive build jobs and a finite positive smoke timeout are required')
    try:
        renderer_reference_controls(args.renderer_4k_reference_control)
    except ValueError as error:
        parser.error(str(error))
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
