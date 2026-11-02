#!/usr/bin/env python3
"""Build and exercise a complete coherent current-source campaign client.

Copy current source and selected original fixtures into owned disposable
storage. Use exact committed dependency archives, not dirty dependency trees.
The production CMake graph builds the actual native executable. Added CTests
only invoke the existing MCC-only runtime probe; no engine code, input state,
callbacks or results are substituted. No installed runtime is changed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIRECTORIES = ('src', 'include', 'cmake', 'shaders', 'tools', 'third_party', 'external', 'multiplayer', 'tests', 'rust', 'data')
SQUADS = ('1st_hum_floor_mar', '1st_hum_johnson', '1st_cov_wv1_elt', '1st_cov_wv1_grt')
# Required observations are test inputs from the previous actual failure
# boundaries, not engine decisions or substitutes for authored script behavior.
PRIOR_NATIVE_BOUNDARIES = (('threat', 'area'), ('reset', 'behavior_reset'),
                          ('reset', 'clump_retirement'), ('reset', 'area'))


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def new_game_tests(source, maps, mcc_root, output, ticks, timeout,acceptance='cold_startup'):
    if acceptance not in ('cold_startup','opening_handoff'):
        raise ValueError('Unknown explicit New Game acceptance criterion')
    result = output / 'new-game-startup.json'
    values = [str(source / 'tools/run_test.py'), str(maps),
              str(source / 'tools/test_campaign_new_game_runtime.py'),
              str(mcc_root), str(result)]
    if any(']]' in value for value in values):
        raise ValueError('Unsupported CMake path delimiter')
    additions = f'''\n# Disposable bounded New Game cases.\nadd_test(NAME current_campaign_new_game_frontend COMMAND "${{Python3_EXECUTABLE}}"
  [[{values[0]}]] -- $<TARGET_FILE:frontend_tests> [[{values[1]}]] --campaign-only)
set_tests_properties(current_campaign_new_game_frontend PROPERTIES TIMEOUT 120)
add_test(NAME current_campaign_new_game_startup COMMAND "${{Python3_EXECUTABLE}}"
  [[{values[0]}]] -- "${{Python3_EXECUTABLE}}" [[{values[2]}]]
  --runtime $<TARGET_FILE:halo2_native> --mcc-root [[{values[3]}]]
  --ticks {ticks} --timeout {timeout} --acceptance {acceptance} --report-output [[{values[4]}]])
set_tests_properties(current_campaign_new_game_startup PROPERTIES TIMEOUT {timeout + 60})
add_test(NAME current_campaign_opening_release COMMAND "${{Python3_EXECUTABLE}}"
  [[{values[0]}]] -- $<TARGET_FILE:campaign_tests> --opening-release-timing)
set_tests_properties(current_campaign_opening_release PROPERTIES TIMEOUT 60)
add_test(NAME current_campaign_session_timers_world COMMAND $<TARGET_FILE:native_campaign_session_timers_world_tests>
  [[{maps / '00a_introduction.map'}]])
set_tests_properties(current_campaign_session_timers_world PROPERTIES TIMEOUT 180)
'''
    return additions, [dict(name='current_campaign_new_game_startup', report=str(result))]


def focused_campaign_tests(args,source,maps,output):
    if not getattr(args,'armory',False):
        return new_game_tests(source,maps,args.mcc_root,output,args.ticks,args.timeout,getattr(args,'acceptance','cold_startup'))
    result=output/'armory-startup.json'
    values=[str(source/'tools/run_test.py'),str(source/'tools/test_campaign_armory_runtime.py'),
            str(args.mcc_root),str(result),str(maps/'00a_introduction.map')]
    if any(']]'in value for value in values):
        raise ValueError('Unsupported CMake path delimiter')
    additions=f'''\n# Disposable natural authored Armory and actual World timer acceptance.\nadd_test(NAME current_campaign_armory_startup COMMAND "${{Python3_EXECUTABLE}}"
  [[{values[0]}]] -- "${{Python3_EXECUTABLE}}" [[{values[1]}]]
  --runtime $<TARGET_FILE:halo2_native> --mcc-root [[{values[2]}]]
  --ticks {args.ticks} --timeout {args.timeout} --report-output [[{values[3]}]])
set_tests_properties(current_campaign_armory_startup PROPERTIES TIMEOUT {args.timeout+60})
add_test(NAME current_campaign_session_timers_world COMMAND $<TARGET_FILE:native_campaign_session_timers_world_tests>
  [[{values[4]}]])
set_tests_properties(current_campaign_session_timers_world PROPERTIES TIMEOUT 180)
'''
    return additions,[dict(name='current_campaign_armory_startup',report=str(result))]


def install_graphics_batch_hook(args,source,output):
    from install_campaign_new_game_graphics_hook import hook_text
    owner=source.parent;build=owner/'build';build.mkdir(exist_ok=True)
    batch=source/'tools/test_campaign_new_game_graphics_batch.py'
    graphics_report=output/'graphics-batch.json'
    gpu_command=[sys.executable,str(source/'tools/run_test.py'),
        '--scratch-root',str(owner/'final-gpu-scratch'),'--',sys.executable,'-B',str(batch),
        '--runtime',str(build/'halo2_native'),'--mcc-root',str(args.mcc_root),
        '--frames',str(args.graphics_frames),'--timeout',str(args.graphics_timeout),
        '--cold-data-check','--report-output',str(graphics_report)]
    hook=build/'CTestCustom.cmake'
    if hook.exists()and'# Explicit final New Game graphical acceptance.'not in hook.read_text():
        raise ValueError('Refuse to overwrite another CTestCustom owner')
    hook.write_text(hook_text(gpu_command))
    (output/'graphics-CTestCustom.cmake').write_bytes(hook.read_bytes())
    return dict(command=gpu_command,hook_sha256=sha(hook),helper_sha256=sha(batch),
        report=str(graphics_report),acceptance_order=['cold_data_build_only','cold_data_ownership','main_view_data_contract',
        'bsp_resource_admission','frame_policy_native_root','transparent_resident_owner',
        'campaign_machine_resources','machine_scene_existing_regressions',
        'renderer_framebuffer_aliases','graphics_startup','opening_handoff'],frames=args.graphics_frames,
        timeout_seconds=args.graphics_timeout,passed=False)


def refresh_input_path(name, selected_fixtures):
    path = Path(name)
    if (path.is_absolute() or not path.parts or '..' in path.parts
            or any(part in ('.git', '__pycache__') for part in path.parts)):
        raise ValueError('Unsafe refresh source path: ' + name)
    if len(path.parts) == 1:
        if name not in ('CMakeLists.txt', 'AGENTS.md', '.gitignore'):
            raise ValueError('Unsupported refresh root file: ' + name)
    elif (path.parts[0] not in SOURCE_DIRECTORIES
          or path.parts[:2] == ('tools', 'reference')
          or path.parts[:2] == ('rust', 'target')
          or (path.parts[:2] == ('tests', 'fixtures') and
              (len(path.parts) < 3 or path.parts[2] not in selected_fixtures))):
        raise ValueError('Refresh path is outside captured source owners: ' + name)
    return path


def current_input_manifest(selected_fixtures):
    """Read current captured owners for an explicitly coordinated refresh."""
    actual = {'CMakeLists.txt', 'AGENTS.md', '.gitignore'}
    for directory in SOURCE_DIRECTORIES:
        for parent, directories, files in os.walk(ROOT / directory):
            relative = Path(parent).relative_to(ROOT)
            directories[:] = [name for name in directories
                              if name not in ('.git', '__pycache__')]
            if relative == Path('tools'):
                directories[:] = [name for name in directories if name != 'reference']
            elif relative == Path('rust'):
                directories[:] = [name for name in directories if name != 'target']
            elif relative == Path('tests/fixtures'):
                directories[:] = [name for name in directories if name in selected_fixtures]
            for filename in files:
                path = Path(parent) / filename
                if not path.is_file():
                    continue
                name = str(path.relative_to(ROOT))
                try:
                    refresh_input_path(name, selected_fixtures)
                except ValueError:
                    continue
                actual.add(name)
    return {name:sha(ROOT/name) for name in sorted(actual)}


class RefreshCaptureRejected(ValueError):
    """Pre-publication capture failed; the owned source/build is untouched."""


MAX_REFRESH_STAGED_BYTES = 2 * 1024 * 1024 * 1024
MAX_REFRESH_CAPTURE_ATTEMPTS = 3


def stage_refresh_file(original, destination, remaining):
    """Copy a finite declared-size input, rejecting a concurrent grow/truncate."""
    with original.open('rb') as incoming:
        size = os.fstat(incoming.fileno()).st_size
        if size > remaining:
            raise RefreshCaptureRejected('Refresh staging exceeds bounded byte budget')
        with destination.open('wb') as outgoing:
            left = size
            while left:
                block = incoming.read(min(left, 1024 * 1024))
                if not block:
                    raise RefreshCaptureRejected('Checkout truncated during refresh staging: ' + str(original))
                outgoing.write(block)
                left -= len(block)
            if incoming.read(1):
                raise RefreshCaptureRejected('Checkout grew during refresh staging: ' + str(original))
    return size


def declared_refresh(source, current_inputs, manifest_path, manifest_sha, selected_fixtures):
    """Stage/authenticate before publication; checkout races cannot poison a build."""
    source = Path(source)
    manifest_path = Path(manifest_path)
    try:
        with manifest_path.open('rb') as stream:
            raw = stream.read(8 * 1024 * 1024 + 1)
    except OSError as error:
        raise RefreshCaptureRejected('Refresh manifest unavailable: ' + str(error)) from error
    if len(raw) > 8 * 1024 * 1024 or hashlib.sha256(raw).hexdigest() != manifest_sha:
        raise RefreshCaptureRejected('Refresh manifest size or SHA-256 differs')
    requested = json.loads(raw)
    if not isinstance(requested, dict) or not 1 <= len(requested) <= 65536:
        raise ValueError('Refresh manifest requires a bounded path/hash object')
    for name, digest in requested.items():
        refresh_input_path(name, selected_fixtures)
        if not isinstance(digest, str) or not re.fullmatch('[0-9a-f]{64}', digest):
            raise ValueError('Invalid refresh source SHA-256')
    if not {'CMakeLists.txt', 'AGENTS.md', '.gitignore'} <= requested.keys():
        raise ValueError('Refresh manifest omitted captured root owners')
    # A refresh must describe the complete captured input set, including new
    # source files. Other concurrent checkout edits cannot silently enter it.
    def require_checkout():
        try:
            if current_input_manifest(selected_fixtures) != requested:
                raise RefreshCaptureRejected('Refresh manifest is not the complete captured source set')
        except OSError as error:
            raise RefreshCaptureRejected('Checkout changed during refresh inventory: ' + str(error)) from error
    require_checkout()
    changed = sorted(name for name, digest in requested.items()
                     if current_inputs.get(name) != digest)
    removed = sorted(current_inputs.keys() - requested.keys())
    # The root CMake is staged even if its declared bytes did not change:
    # the previous iteration's owned test append is not part of that input.
    publication = sorted(set(changed) | {'CMakeLists.txt'})
    for name in sorted(set(publication) | set(removed)):
        refresh_input_path(name, selected_fixtures)
        destination = source / name
        if any(parent.is_symlink() for parent in destination.parents if parent != source and source in parent.parents):
            raise ValueError('Refresh destination has a borrowed directory owner')
    for name, digest in current_inputs.items():
        if name != 'CMakeLists.txt' and (not (source/name).is_file() or sha(source/name) != digest):
            raise ValueError('Owned source changed before refresh: ' + name)
    with tempfile.TemporaryDirectory(prefix='refresh-stage-', dir=source.parent) as temporary:
        stage = Path(temporary) / 'inputs'
        backup = Path(temporary) / 'previous'
        size = 0
        try:
            for name in publication:
                destination = stage/name
                destination.parent.mkdir(parents=True, exist_ok=True)
                size += stage_refresh_file(ROOT/name, destination, MAX_REFRESH_STAGED_BYTES-size)
                if sha(destination) != requested[name]:
                    raise RefreshCaptureRejected('Checkout changed during refresh staging: ' + name)
                os.utime(destination, None)  # Never reuse an older object after changed bytes.
            require_checkout()
        except OSError as error:
            raise RefreshCaptureRejected('Checkout unavailable during refresh staging: ' + str(error)) from error
        # Preserve old entries before the first publication. No worker runs
        # during this transaction; a filesystem failure restores exact inodes.
        previous = set()
        touched = []
        created_directories = []
        for name in sorted(set(publication) | set(removed)):
            destination = source/name
            if destination.exists() or destination.is_symlink():
                old = backup/name
                old.parent.mkdir(parents=True, exist_ok=True)
                os.link(destination, old, follow_symlinks=False)
                previous.add(name)
        try:
            for name in publication:
                destination = source/name
                missing = []
                parent = destination.parent
                while not parent.exists():
                    missing.append(parent)
                    parent = parent.parent
                destination.parent.mkdir(parents=True, exist_ok=True)
                created_directories.extend(reversed(missing))
                os.replace(stage/name, destination)
                touched.append(name)
            for name in removed:
                (source/name).unlink()
                touched.append(name)
            for name, digest in requested.items():
                if not (source/name).is_file() or sha(source/name) != digest:
                    raise ValueError('Owned source changed during refresh publication: ' + name)
        except Exception:
            for name in reversed(touched):
                if name in previous:
                    os.replace(backup/name, source/name)
                else:
                    (source/name).unlink()
            for directory in reversed(created_directories):
                directory.rmdir()
            raise
    return requested, dict(changed={name:requested[name] for name in changed},
                           removed=removed, manifest_sha256=manifest_sha,
                           staged_bytes=size, transactional_publication=True)


def owned_new_game_iterations(args, source, maps, inputs, selected_fixtures,
                              command, base_cmake, report, save):
    """Finite active investigation, owned by the outer run_test supervisor."""
    control = args.refresh_control.resolve()
    if control.exists():
        raise ValueError('Use a fresh absent refresh-control path; stale controls are rejected')
    report['owned_build_worker'] = True
    report['refresh_control'] = str(control)
    report['maximum_source_refreshes'] = args.owned_refreshes
    report['refresh_wait_seconds'] = args.refresh_wait_seconds
    report['maximum_capture_rejections_per_refresh'] = MAX_REFRESH_CAPTURE_ATTEMPTS
    report['refresh_capture_rejections'] = []
    report['iterations'] = []
    current = dict(inputs)
    refresh_receipt = None
    for ordinal in range(args.owned_refreshes + 1):
        output = args.output / f'iteration-{ordinal}'
        output.mkdir()
        acceptance=getattr(args,'acceptance','cold_startup')
        additions, cases = focused_campaign_tests(args,source,maps,output)
        original_cmake = (source / 'CMakeLists.txt').read_text() if ordinal else None
        # Every iteration starts from its authenticated root CMake bytes, not
        # from a prior appended test registration or a later checkout edit.
        cmake = source / 'CMakeLists.txt'
        if ordinal:
            if hashlib.sha256(original_cmake.encode()).hexdigest() != current['CMakeLists.txt']:
                raise ValueError('Frozen CMake changed before iteration registration')
            cmake.write_text(original_cmake + additions)
        else:
            original_cmake = base_cmake
            cmake.write_text(original_cmake + additions)
        staged = dict(current)
        staged['CMakeLists.txt'] = sha(cmake)
        manifest = output / 'source-inputs.json'
        manifest.write_text(json.dumps(current, indent=2, sort_keys=True) + '\n')
        iteration_command = list(command)
        if '--build-worker'not in iteration_command:iteration_command.insert(2, '--build-worker')
        iteration_command[iteration_command.index('--output')+1] = str(output / 'validation')
        record = dict(ordinal=ordinal, source_inputs_sha256=sha(manifest),
                      staged_cmake_sha256=staged['CMakeLists.txt'],
                      test_registration_delta=additions, declared_refresh=refresh_receipt,
                      command=iteration_command, runtime_cases=cases, passed=False,
                      acceptance=acceptance,validation_budget=dict(ticks=args.ticks,timeout=args.timeout))
        report['iterations'].append(record)
        if getattr(args,'graphics_batch',False):
            record['graphics_batch']=install_graphics_batch_hook(args,source,output)
        report['runtime_cases'] = cases
        report['staged_cmake_sha256'] = staged['CMakeLists.txt']
        report['test_registration_delta'] = additions
        report['command'] = iteration_command
        report['phase'] = f'building and validating declared iteration{ordinal}'
        save()
        record['returncode'] = subprocess.call(iteration_command, cwd=source)
        record['snapshot_changed'] = [name for name, digest in staged.items()
                                      if not (source/name).is_file() or sha(source/name) != digest]
        test_log = output / 'validation/tests.log'
        text = test_log.read_text(errors='replace') if test_log.exists() else ''
        record['frontend_menu_handoff_passed'] = bool(re.search(
            r'current_campaign_new_game_frontend\s+\.+\s+Passed', text))
        record['natural_startup_passed'] = bool(re.search(
            r'current_campaign_new_game_startup\s+\.+\s+Passed', text))
        record['opening_release_passed'] = bool(re.search(
            r'current_campaign_opening_release\s+\.+\s+Passed', text))
        record['session_timers_world_passed'] = bool(re.search(
            r'current_campaign_session_timers_world\s+\.+\s+Passed', text))
        if getattr(args,'armory',False):
            record['armory_startup_passed']=bool(re.search(r'current_campaign_armory_startup\s+\.+\s+Passed',text))
            record['shared_area_queries_passed']=all(re.search(name+r'\s+\.+\s+Passed',text)
                for name in ('native_campaign_ai_area_tu15','native_campaign_ai_area_count_tu15'))
            record['area_choice_component_and_binding_passed']=all(re.search(name+r'\s+\.+\s+Passed',text)
                for name in ('native_campaign_ai_area_choice_tu15','native_campaign_ai_area_choice_tu15_binding'))
        for case in cases:
            path = Path(case['report'])
            case['executed'] = path.is_file()
            if path.is_file():
                case['passed'] = json.loads(path.read_text())['passed']
                case['report_sha256'] = sha(path)
        record['passed'] = (record['returncode'] == 0 and not record['snapshot_changed']
                            and record['frontend_menu_handoff_passed']
                            and record['natural_startup_passed'] and record['opening_release_passed']
                            and record['session_timers_world_passed']
                            and all(case.get('passed') for case in cases))
        if getattr(args,'armory',False):
            record['passed']=(record['returncode']==0 and not record['snapshot_changed']
                              and record['armory_startup_passed'] and record['shared_area_queries_passed']
                              and record['area_choice_component_and_binding_passed']
                              and record['session_timers_world_passed']
                              and all(case.get('passed')for case in cases))
        if getattr(args,'graphics_batch',False):
            graphics_report=Path(record['graphics_batch']['report'])
            record['graphics_batch']['executed']=graphics_report.is_file()
            if graphics_report.is_file():
                record['graphics_batch']['passed']=json.loads(graphics_report.read_text())['passed']
                record['graphics_batch']['report_sha256']=sha(graphics_report)
            record['passed']=record['passed']and record['graphics_batch']['passed']
        report['passed'] = record['passed']
        report['returncode'] = record['returncode']
        report['phase'] = f'completed declared iteration{ordinal}'
        save()
        if ordinal == args.owned_refreshes or (getattr(args,'graphics_batch',False)and record['passed']):
            break
        report['phase'] = f'waiting at most{args.refresh_wait_seconds}s for control sequence{ordinal+1}'
        save()
        deadline = time.monotonic() + args.refresh_wait_seconds
        instruction = None
        attempted_manifests = set()
        rejections = 0
        while time.monotonic() < deadline:
            if not control.is_file():
                time.sleep(.5)
                continue
            with control.open('rb') as stream:
                raw_control = stream.read(16385)
            if len(raw_control) > 16384:
                raise ValueError('Refresh control exceeds bounded schema')
            candidate = json.loads(raw_control)
            if not isinstance(candidate, dict):
                raise ValueError('Refresh control is not an object')
            sequence = candidate.get('sequence')
            if type(sequence) is not int or sequence not in (ordinal, ordinal + 1):
                raise ValueError('Unexpected refresh control sequence')
            if sequence == ordinal:
                time.sleep(.5)
                continue
            action = candidate.get('action')
            if action == 'finish' and candidate.keys() == {'sequence', 'action'}:
                instruction = candidate
                break
            required = {'sequence', 'action', 'manifest', 'manifest_sha256'}
            if action != 'refresh' or not required <= candidate.keys() or candidate.keys() - required - {'ticks','timeout','acceptance'}:
                raise ValueError('Unsupported refresh control action/schema')
            ticks, timeout = candidate.get('ticks',args.ticks),candidate.get('timeout',args.timeout)
            if type(ticks) is not int or not 1 <= ticks <= 18000 or type(timeout) is not int or not 1 <= timeout <= 1800:
                raise ValueError('Invalid explicitly requested New Game validation budget')
            acceptance=candidate.get('acceptance',getattr(args,'acceptance','cold_startup'))
            if acceptance not in ('cold_startup','opening_handoff'):
                raise ValueError('Invalid explicitly requested New Game acceptance criterion')
            if getattr(args,'armory',False)and(acceptance!='cold_startup' or ticks>1800 or timeout>600):
                raise ValueError('Armory requires its explicit cold-start12..1800tick/600second profile')
            if getattr(args,'armory',False)and ticks<12:
                raise ValueError('Armory requires at least12ticks to reach the recorded callback boundary')
            if acceptance=='opening_handoff' and not any(row['passed'] and row['acceptance']=='cold_startup'
                                                        for row in report['iterations']):
                raise ValueError('Opening handoff requires a passed cold-start iteration first')
            pin = candidate['manifest_sha256']
            if not isinstance(pin,str) or not re.fullmatch('[0-9a-f]{64}',pin):
                raise ValueError('Invalid refresh manifest SHA-256')
            if pin in attempted_manifests:
                time.sleep(.5)
                continue
            attempted_manifests.add(pin)
            try:
                accepted, receipt = declared_refresh(source, current,
                    candidate['manifest'], pin, selected_fixtures)
            except RefreshCaptureRejected as error:
                rejections += 1
                report['refresh_capture_rejections'].append(dict(sequence=sequence,
                    manifest=str(candidate['manifest']),manifest_sha256=pin,reason=str(error),
                    source_published=False,owned_build_preserved=True))
                report['phase'] = (f'waiting for replacement manifest at control sequence{sequence}; '
                                   f'capture rejected ({rejections}/{MAX_REFRESH_CAPTURE_ATTEMPTS})')
                if rejections >= MAX_REFRESH_CAPTURE_ATTEMPTS:
                    report.update(passed=False,phase='refresh capture rejection limit reached')
                    save()
                    return 1
                save()
                continue
            current, refresh_receipt = accepted, receipt
            args.ticks,args.timeout=ticks,timeout
            args.acceptance=acceptance
            refresh_receipt['validation_budget'] = dict(ticks=ticks,timeout=timeout,acceptance=acceptance)
            instruction = candidate
            report['phase'] = f'authenticated refresh for declared iteration{ordinal+1}'
            save()
            break
        if instruction is None:
            report.update(passed=False, phase='refresh control expired', refresh_control_expired=True)
            save()
            return 1
        if instruction['action'] == 'finish':
            break
    report['checkout_drift'] = [name for name, digest in current.items()
                                if not (ROOT/name).is_file() or sha(ROOT/name) != digest]
    report['phase'] = 'completed finite owned investigation'
    report['full_campaign_or_coop_qualified'] = False
    report['native_only_engine_qualified'] = False
    report['menu_to_world_continuous_runtime_qualified'] = False
    save()
    return 0 if report['passed'] else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--dependency-receipt', type=Path, required=True)
    ap.add_argument('--compiler', default='clang++')
    ap.add_argument('--jobs', type=int, default=2)
    ap.add_argument('--ticks', type=int, default=300)
    ap.add_argument('--timeout', type=int, default=300)
    ap.add_argument('--actor-components', action='store_true',
                    help='Also freeze/run original actor components and complete cold-world ownership checks')
    ap.add_argument('--ownership-only', action='store_true',
                    help='Build the real production graph and run only complete cold-world ownership; no client/encounter run')
    ap.add_argument('--new-game', action='store_true',
                    help='Build native client/frontend and check menu handoff plus natural MCC introduction startup')
    ap.add_argument('--armory',action='store_true',
                    help='Build native client, shared area components and real World timers; observe natural authored Armory startup')
    ap.add_argument('--acceptance',choices=('cold_startup','opening_handoff'),default='cold_startup',
                    help='New Game only: explicit acceptance purpose, independent of tick budget')
    ap.add_argument('--graphics-batch',action='store_true',
                    help='New Game only: initial owned PRE_TEST ColdData, short GPU gate and natural rendered handoff')
    ap.add_argument('--graphics-frames',type=int,default=30000)
    ap.add_argument('--graphics-timeout',type=int,default=900)
    ap.add_argument('--owned-refreshes', type=int, default=0,
                    help='New Game only: permit1..3 exact declared source refreshes in one owned active build')
    ap.add_argument('--refresh-control', type=Path,
                    help='Fresh absent JSON control path; atomically publish refresh/finish instructions here')
    ap.add_argument('--refresh-wait-seconds', type=int, default=600,
                    help='Bounded wait after each iteration,1..1200seconds; default600')
    ap.add_argument('--require-native-ai-queries', action='store_true',
                    help='Require the native entries that previously blocked each authored encounter')
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    max_ticks,max_timeout = (18000,1800) if args.new_game else (1800,600)
    if args.jobs < 1 or not 1 <= args.ticks <= max_ticks or not 1 <= args.timeout <= max_timeout:
        ap.error(f'Require positive jobs,1..{max_ticks}ticks and1..{max_timeout}seconds per runtime')
    if args.new_game and (args.ownership_only or args.actor_components or args.require_native_ai_queries):
        ap.error('--new-game is a focused mode; encounter/ownership options cannot be combined')
    if args.armory and(args.new_game or args.ownership_only or args.actor_components or args.require_native_ai_queries or args.ticks<12):
        ap.error('--armory is a focused mode requiring at least12ticks')
    if args.acceptance!='cold_startup' and not args.new_game:
        ap.error('Opening-handoff acceptance requires --new-game')
    if args.graphics_batch and (not args.new_game or args.acceptance!='cold_startup'
            or not 6<=args.graphics_frames<=30000 or not 1<=args.graphics_timeout<=900):
        ap.error('Graphics batch requires focused New Game/cold CLI check,6..30000frames and1..900seconds')
    if (not 0 <= args.owned_refreshes <= 3 or not 1 <= args.refresh_wait_seconds <= 1200
            or (args.owned_refreshes and (not(args.new_game or args.armory) or args.refresh_control is None))
            or (args.refresh_control is not None and not args.owned_refreshes)):
        ap.error('Owned refreshes require focused New Game/Armory,1..3refreshes, a fresh control path and1..1200seconds wait')
    compiler = shutil.which(args.compiler)
    if compiler is None:
        ap.error('Compiler unavailable')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source = scratch / 'current-source'
    source.mkdir()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    args.mcc_root = args.mcc_root.resolve(strict=True)
    report = dict(scope=__doc__, tool_sha256=sha(Path(__file__)), scratch=str(scratch),
                  phase='copying current sources', passed=False, installed_runtime_changed=False,
                  retained_large_artifacts=[], engine_source_substitutions=[], runtime_results_supplied=False)
    report['ownership_only'] = args.ownership_only
    report['new_game'] = args.new_game
    report['armory']=args.armory
    if args.ownership_only:
        report['scope'] = 'Frozen production CMake graph and real standalone cold-world constructor/data ownership; no client or encounter execution'
    elif args.new_game:
        report['scope'] = 'Frozen native client and linked real-MCC frontend handoff, followed by bounded ordinary campaign CLI introduction startup; no scripted completion, placed squad or supplied result'
    elif args.armory:
        report['scope']='Frozen native client/shared area/timer World integration and natural authored Armory CLI startup; no injected script, actor, squad, input or gameplay result; no full rendered training/native-only/launcher acceptance'
    report_path = args.output / 'current-source-cohort.json'

    def save():
        report_path.write_text(json.dumps(report, indent=2) + '\n')

    save()
    for name in ('CMakeLists.txt', 'AGENTS.md', '.gitignore'):
        shutil.copy2(ROOT / name, source / name)
    for name in SOURCE_DIRECTORIES:
        excluded = {'__pycache__', '.git'}
        if name == 'tools':
            excluded.add('reference')
        if name == 'tests':
            excluded.add('fixtures')
        if name == 'rust':
            excluded.add('target')
        shutil.copytree(ROOT / name, source / name, symlinks=True,
                        ignore=shutil.ignore_patterns(*excluded))
    for name in ('assets', 'resources', 'docs', '.tools'):
        (source / name).symlink_to(ROOT / name, target_is_directory=True)
    (source / 'tools/reference').symlink_to(ROOT / 'tools/reference', target_is_directory=True)
    fixture_root = source / 'tests/fixtures'
    fixture_root.mkdir()
    selected = 'campaign_activation_tu15'
    selected_fixtures = {selected, 'campaign_session_seed_order_tu15.json'}
    if args.new_game:
        selected_fixtures.update(('frontend_ui_bc3_original.tsv',
                                 'frontend_ui_dialog1_original.bc3',
                                 'frontend_ui_dialog1_original.rgba'))
    if args.armory:
        selected_fixtures.update(('native_campaign_ai_area_tu15.json.gz','native_campaign_ai_area_count_tu15.json.gz',
                                 'native_campaign_ai_area_choice_tu15_v2.json.gz',
                                 'native_campaign_ai_area_choice_transport_tu15.json.gz'))
    if args.actor_components or args.ownership_only:
        selected_fixtures.update(re.findall(r'\$\{CMAKE_CURRENT_SOURCE_DIR\}/tests/fixtures/([^"/]+)',
                                           (source / 'cmake/CampaignAiObjectDecisions.cmake').read_text()))
        selected_fixtures.update(re.findall(r'#include "fixtures/([^"]+)"',
                                           (source / 'tests/native_tu15_world_memory_tests.cpp').read_text()))
    report['captured_fixture_owners'] = sorted(selected_fixtures)
    for fixture in (ROOT / 'tests/fixtures').iterdir():
        if fixture.name == selected:
            shutil.copytree(fixture, fixture_root / selected)
        elif fixture.name in selected_fixtures:
            shutil.copy2(fixture, fixture_root / fixture.name)
        else:
            (fixture_root / fixture.name).symlink_to(fixture, target_is_directory=fixture.is_dir())
    if not (fixture_root / selected).is_dir():
        raise ValueError('Required original activation fixtures are absent')
    inputs = {}
    for name in ('CMakeLists.txt', 'AGENTS.md', '.gitignore'):
        inputs[name] = sha(source / name)
    for directory in SOURCE_DIRECTORIES:
        for path in (source / directory).rglob('*'):
            relative = path.relative_to(source)
            if relative.parts[:2] == ('tools', 'reference'):
                continue
            if relative.parts[:2] == ('tests', 'fixtures') and (len(relative.parts) < 3 or relative.parts[2] not in selected_fixtures):
                continue
            if path.is_file():
                inputs[str(relative)] = sha(path)
    changed = [name for name, digest in inputs.items() if sha(ROOT / name) != digest]
    if changed:
        report['phase'] = 'capture rejected before build'
        report['capture_changed'] = changed
        save()
        raise ValueError('Current source changed while capturing: ' + ','.join(changed[:10]))
    (args.output / 'source-inputs.json').write_text(json.dumps(inputs, indent=2, sort_keys=True) + '\n')
    report['source_inputs_sha256'] = sha(args.output / 'source-inputs.json')
    report['captured_source_files'] = len(inputs)
    report['nonselected_fixtures_are_shared_and_not_qualified'] = True
    report['phase'] = 'preparing pinned dependency archives'
    save()
    dependency = json.loads(args.dependency_receipt.read_text())
    datachannel = scratch / 'datachannel'
    datachannel.mkdir()
    for ordinal, row in enumerate(dependency['inputs']):
        archive = scratch / f'dependency-{ordinal}.tar'
        with archive.open('wb') as stream:
            subprocess.run(['git', '-C', row['repository'], 'archive', row['commit']], stdout=stream, check=True)
        if sha(archive) != row['archive_sha256']:
            raise ValueError('Committed dependency archive differs')
        destination = datachannel if ordinal == 0 else datachannel / 'deps' / Path(row['repository']).name
        destination.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as stream:
            stream.extractall(destination, filter='data')
    report['dependency_inputs'] = dependency['inputs']
    report['dependency_working_trees_modified'] = False
    additions = '\n# Disposable current-source campaign acceptance cases.\n'
    report['runtime_cases'] = []
    if args.new_game or args.armory:
        from physics_test_cache import runtime_cache
        maps, mcc_inputs = runtime_cache(args.mcc_root)
        report['mcc_inputs'] = mcc_inputs
        additions, report['runtime_cases'] = focused_campaign_tests(args,source,maps,args.output)
    for ordinal, squad in enumerate(() if args.ownership_only or args.new_game or args.armory else SQUADS, 1):
        result = args.output / f'runtime-{ordinal}.json'
        name = f'current_campaign_ai_{ordinal}'
        # Paths are CMake bracket arguments; values containing closing brackets
        # cannot be silently reinterpreted as extra CMake commands.
        values = [str(source / 'tools/test_campaign_ai_runtime.py'), str(args.mcc_root), squad, str(result)]
        if any(']]' in value for value in values):
            raise ValueError('Unsupported CMake path delimiter')
        native_ai_option = (''.join(' --require-native-ai-query '+entry for entry in PRIOR_NATIVE_BOUNDARIES[ordinal-1])
                            if args.require_native_ai_queries else '')
        additions += f'''add_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
  [[{values[0]}]] --runtime $<TARGET_FILE:halo2_native>
  --mcc-root [[{values[1]}]] --squad [[{squad}]] --ticks {args.ticks}
  --timeout {args.timeout} --report-output [[{result}]]{native_ai_option})
set_tests_properties({name} PROPERTIES TIMEOUT {args.timeout + 60})
'''
        report['runtime_cases'].append(dict(name=name, squad=squad, report=str(result)))
    cmake = source / 'CMakeLists.txt'
    original_cmake = cmake.read_text()
    cmake.write_text(original_cmake + additions)
    if cmake.read_text().removesuffix(additions) != original_cmake:
        raise ValueError('Acceptance registration is not an exact append')
    staged_inputs = dict(inputs)
    staged_inputs['CMakeLists.txt'] = sha(cmake)
    report['test_registration_delta'] = additions
    report['staged_cmake_sha256'] = sha(cmake)
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(source),
               '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / 'validation'),
               '--target', 'halo2_native', '--target', 'campaign_completion_tu15_tests',
               '--target', 'campaign_session_seed_order_tu15_tests',
               '--filter', r'^original_campaign_activation_tu15$|^current_campaign_ai_[1-4]$|^native_campaign_session_seed_order_',
               '--jobs', str(args.jobs), '--test-jobs', '1', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler,
               '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL=' + str(datachannel),
               '--cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF', '--cmake-arg=-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld',
               '--cmake-arg=-DHALO2_CAMPAIGN_SEED_ORDER_MCC_MAP=' + str(args.mcc_root / 'halo2/h2_maps_win64_dx11/01b_spacestation.map')]
    if args.ownership_only:
        start = command.index('--target')
        stop = command.index('--jobs')
        command[start:stop] = ['--target', 'native_tu15_cold_data_tests',
                              '--filter', '^native_tu15_cold_data$']
    elif args.new_game:
        start = command.index('--target')
        stop = command.index('--jobs')
        command[start:stop] = ['--target', 'halo2_native', '--target', 'frontend_tests',
                              '--target', 'campaign_tests', '--target', 'native_campaign_session_timers_world_tests',
                              '--filter', '^current_campaign_(new_game_(frontend|startup)|opening_release|session_timers_world)$']
    elif args.armory:
        start,stop=command.index('--target'),command.index('--jobs')
        command[start:stop]=['--target','halo2_native','--target','native_campaign_ai_area_tu15_tests',
                             '--target','native_campaign_ai_area_count_tu15_tests',
                             '--target','native_campaign_ai_area_choice_tu15_tests',
                             '--target','native_campaign_ai_area_choice_tu15_binding_tests',
                             '--target','native_campaign_session_timers_world_tests',
                             '--filter','^(current_campaign_(armory_startup|session_timers_world)|native_campaign_ai_area(_count|_choice)?_tu15(_binding)?)$']
    elif args.actor_components:
        command += ['--target', 'campaign_ai_object_decisions_tu15_tests']
        command += ['--target', 'native_tu15_world_memory_tests']
        filter_index = command.index('--filter') + 1
        command[filter_index] += '|^original_campaign_ai_object_decisions_tu15$|^native_tu15_world_memory_ownership$'
    for seed in (() if args.ownership_only or args.new_game or args.armory else (305419896, 2271560481)):
        command += ['--retain-report', f'campaign-session-seed-{seed}.json']
    report['command'] = command
    report['phase'] = ('building production cold-world owner test' if args.ownership_only
                       else 'building and validating current native client')
    save()
    if args.graphics_batch and not args.owned_refreshes:
        command.insert(2,'--build-worker')
        report['graphics_batch']=install_graphics_batch_hook(args,source,args.output)
        report['owned_build_worker']=True
        save()
    if args.owned_refreshes:
        try:
            return owned_new_game_iterations(args, source, maps, inputs, selected_fixtures,
                                               command, original_cmake, report, save)
        except Exception as error:
            report.update(passed=False, phase='finite owned investigation failed', error=str(error))
            save()
            raise
    report['returncode'] = subprocess.call(command, cwd=source)
    report['snapshot_changed'] = [name for name, digest in staged_inputs.items() if sha(source / name) != digest]
    report['checkout_drift'] = [name for name, digest in inputs.items() if not (ROOT / name).is_file() or sha(ROOT / name) != digest]
    runtime_passed = []
    for case in report['runtime_cases']:
        path = Path(case['report'])
        case['executed'] = path.is_file()
        if path.is_file():
            result = json.loads(path.read_text())
            case['passed'] = result['passed']
            case['report_sha256'] = sha(path)
        runtime_passed.append(case.get('passed', False))
    test_log = args.output / 'validation/tests.log'
    text = test_log.read_text(errors='replace') if test_log.exists() else ''
    report['original_activation_passed'] = bool(re.search(r'original_campaign_activation_tu15\s+\.+\s+Passed', text))
    report['session_seed_tests_passed'] = all(re.search(r'native_campaign_session_seed_order_' + str(seed) + r'\s+\.+\s+Passed', text)
        for seed in (305419896, 2271560481))
    report['actor_components_requested'] = args.actor_components
    report['actor_components_passed'] = (bool(re.search(r'original_campaign_ai_object_decisions_tu15\s+\.+\s+Passed', text))
                                         if args.actor_components else None)
    report['cold_world_ownership_passed'] = (bool(re.search(r'native_tu15_world_memory_ownership\s+\.+\s+Passed', text))
                                           if args.actor_components or args.ownership_only else None)
    if args.new_game:
        report['original_activation_passed'] = None
        report['session_seed_tests_passed'] = None
        report['frontend_menu_handoff_passed'] = bool(re.search(
            r'current_campaign_new_game_frontend\s+\.+\s+Passed', text))
        report['natural_startup_passed'] = bool(re.search(
            r'current_campaign_new_game_startup\s+\.+\s+Passed', text))
        report['opening_release_passed'] = bool(re.search(
            r'current_campaign_opening_release\s+\.+\s+Passed', text))
        report['menu_to_world_continuous_runtime_qualified'] = False
        report['passed'] = (report['returncode'] == 0 and not report['snapshot_changed']
                            and report['frontend_menu_handoff_passed']
                            and report['natural_startup_passed'] and report['opening_release_passed']
                            and all(runtime_passed))
    elif args.armory:
        report['original_activation_passed']=None
        report['session_seed_tests_passed']=None
        report['armory_startup_passed']=bool(re.search(r'current_campaign_armory_startup\s+\.+\s+Passed',text))
        report['session_timers_world_passed']=bool(re.search(r'current_campaign_session_timers_world\s+\.+\s+Passed',text))
        report['shared_area_queries_passed']=all(re.search(name+r'\s+\.+\s+Passed',text)
            for name in ('native_campaign_ai_area_tu15','native_campaign_ai_area_count_tu15'))
        report['area_choice_component_and_binding_passed']=all(re.search(name+r'\s+\.+\s+Passed',text)
            for name in ('native_campaign_ai_area_choice_tu15','native_campaign_ai_area_choice_tu15_binding'))
        report['passed']=(report['returncode']==0 and not report['snapshot_changed'] and all(runtime_passed)
                          and report['armory_startup_passed'] and report['session_timers_world_passed']
                          and report['area_choice_component_and_binding_passed']
                          and report['shared_area_queries_passed'])
    elif args.ownership_only:
        report['original_activation_passed'] = None
        report['session_seed_tests_passed'] = None
        report['cold_world_ownership_passed'] = None
        report['cold_data_ownership_passed'] = bool(re.search(r'native_tu15_cold_data\s+\.+\s+Passed', text))
        report['complete_world_memory_test_requested'] = False
        report['passed'] = (report['returncode'] == 0 and not report['snapshot_changed']
                            and report['cold_data_ownership_passed'])
    else:
        report['passed'] = (report['returncode'] == 0 and not report['snapshot_changed'] and all(runtime_passed)
                            and report['original_activation_passed'] and report['session_seed_tests_passed']
                            and (not args.actor_components or
                                 (report['actor_components_passed'] and report['cold_world_ownership_passed'])))
    report['phase'] = 'completed'
    report['full_campaign_or_coop_qualified'] = False
    report['native_only_engine_qualified'] = False
    report['native_ai_queries_required'] = args.require_native_ai_queries
    if args.graphics_batch:
        graphics_report=Path(report['graphics_batch']['report'])
        report['graphics_batch']['executed']=graphics_report.is_file()
        if graphics_report.is_file():
            report['graphics_batch']['passed']=json.loads(graphics_report.read_text())['passed']
            report['graphics_batch']['report_sha256']=sha(graphics_report)
        report['passed']=report['passed']and report['graphics_batch']['passed']
    save()
    print(json.dumps(dict(passed=report['passed'], returncode=report['returncode'], report=str(report_path))))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
