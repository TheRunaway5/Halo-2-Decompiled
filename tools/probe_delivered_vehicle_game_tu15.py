#!/usr/bin/env python3
"""Validate the delivered native game using an independently frozen neutral LAN observer.

Only readable source headers and linked dependencies are borrowed. Private
copies keep the observer independent of subsequent work in that build owner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--observer-owner', type=Path, required=True)
    parser.add_argument('--vehicle-manifest', type=Path)
    parser.add_argument('--base-manifest', type=Path)
    parser.add_argument('--selected-only',action='store_true',
        help='Validate the current independently qualified selection without claiming an earlier source lineage')
    parser.add_argument('--candidate-preparation', type=Path)
    parser.add_argument('--candidate-validation', type=Path)
    parser.add_argument('--candidate-private-regression', type=Path,
        help='Allow this candidate owner\'s declared, separately running test-registration suffix')
    parser.add_argument('--vehicle-choice', choices=('light','heavy','turret'))
    parser.add_argument('--vehicle-case', choices=('ghost','banshee','warthog-mg','warthog-gauss','spectre','scorpion','wraith','h_turret_ap','c_turret_ap'))
    parser.add_argument('--map-name',default='coagulation.map')
    parser.add_argument('--require-native-flying-selector',action='store_true')
    parser.add_argument('--additional-trigger',action='store_true')
    parser.add_argument('--banshee-takeoff',action='store_true')
    parser.add_argument('--trace-native-failure',action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.vehicle_case and args.vehicle_choice:raise ValueError('Choose a fixed authored vehicle or Random family')
    if args.additional_trigger and (args.vehicle_choice or args.vehicle_case not in ('scorpion','banshee')):
        raise ValueError('Choose a fixed Scorpion or Banshee for additional-trigger acceptance')
    if args.banshee_takeoff and (args.additional_trigger or args.vehicle_choice or args.vehicle_case!='banshee'):
        raise ValueError('Takeoff requires a fixed Banshee primary-input case')
    if args.trace_native_failure and (not args.additional_trigger or args.require_native_flying_selector):
        raise ValueError('Trace the additional-trigger failure separately from native counter acceptance')
    if args.require_native_flying_selector and args.vehicle_case!='banshee':
        raise ValueError('Require the native flying selector only for the real-input Banshee case')
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    borrowed = args.observer_owner.resolve(strict=True)
    if (borrowed == owner or borrowed.parent != (ROOT / '.local/test-scratch').resolve() or
            not borrowed.name.startswith('halo2-test-')):
        raise ValueError('Use a separate active managed source/build owner')
    frozen = borrowed / 'source'; build = borrowed / 'build'
    runtime = args.runtime.resolve(strict=True); runtime_pin = sha(runtime)
    candidate=bool(args.candidate_preparation or args.candidate_validation)
    candidate_binding=None
    if candidate:
        if not(args.candidate_preparation and args.candidate_validation) or args.vehicle_manifest or args.base_manifest:
            raise ValueError('Supply both candidate receipts, or both delivered source manifests')
        prepared=json.loads(args.candidate_preparation.read_text())
        validation=json.loads(args.candidate_validation.read_text());binding=validation.get('vehicle_choice_source_binding',{})
        installed=dict(prepared['source_hashes']);changed=set();additional=set()
        private_registration=None
        if args.candidate_private_regression:
            registration=json.loads((args.candidate_private_regression/'source-binding.json').read_text())
            regression=json.loads((args.candidate_private_regression/'report.json').read_text())
            cmake=(frozen/'CMakeLists.txt').read_bytes()
            marker=b'\n# VEHICLE_CANDIDATE_REGRESSION_TU15\n'
            if (registration.get('owner')!=str(borrowed) or
                registration.get('prepared_sha256')!=sha(args.candidate_preparation) or
                registration.get('expected_cases')!=374 or registration.get('production_bodies_changed')is not False or
                registration.get('tool_sha256')!=sha(ROOT/'tools/validate_vehicle_candidate_regression_tu15.py') or
                not any(step['name']=='build' and step['exit_code']==0 for step in regression['steps']) or
                cmake.count(marker)!=1 or hashlib.sha256(cmake.split(marker)[0]).hexdigest()!=installed['CMakeLists.txt']):
                raise ValueError('Private test registrations do not match this unchanged native candidate')
            # No compiler/library mutation remains: the complete regression is
            # now only executing independent test processes. The actual game
            # binary must still match its earlier native acceptance below.
            installed['CMakeLists.txt']=sha(frozen/'CMakeLists.txt')
            private_registration=dict(regression=str(args.candidate_private_regression),
                registration_only=True,production_bodies_changed=False,
                private_cmake_sha256=installed['CMakeLists.txt'],complete_regression_pass_claimed=False)
        if(Path(prepared['owner'])!=borrowed or runtime!=build/'halo2_native' or not validation['passed'] or
            binding.get('preparation_sha256')!=sha(args.candidate_preparation) or
            binding.get('source_unchanged_after_run')is not True or binding.get('runtime_sha256')!=runtime_pin or
            not {'light','heavy','turret'}.issubset({row.get('family')for row in binding.get('cases',[])}) or
            any(not row['passed']for row in binding['cases']) or
            any(sha(frozen/name)!=pin for name,pin in installed.items())):
            raise ValueError('Candidate needs complete source-bound actual Random match acceptance')
        candidate_binding=dict(preparation_sha256=sha(args.candidate_preparation),
            validation_sha256=sha(args.candidate_validation),source_files=len(installed),launcher_installed=False,
            private_test_registration=private_registration)
    else:
        if args.selected_only and (args.vehicle_manifest or args.base_manifest):
            raise ValueError('Use current-selection qualification or explicit vehicle source lineage')
        if not args.selected_only and not(args.vehicle_manifest and args.base_manifest):raise ValueError('Supply both delivered source manifests')
        installed = json.loads((runtime.parent / 'source-hashes.json').read_text())
        qualification = json.loads((runtime.parent / 'qualification.json').read_text())
        if not qualification['passed'] or qualification['runtime_sha256']!=runtime_pin:
            raise ValueError('The delivered native runtime has no passing package qualification')
        changed=set();additional=set()
        if not args.selected_only:
            ours = json.loads(args.vehicle_manifest.read_text()); base = json.loads(args.base_manifest.read_text())
            changed = {name for name in ours.keys() | base.keys() if ours.get(name) != base.get(name)}
            additional = {name for name in ours.keys() | installed.keys() if ours.get(name) != installed.get(name)}
            if (not changed or any(installed.get(name) != ours.get(name) for name in changed) or
                    any(ours.get(name) != base.get(name) for name in additional)):
                raise ValueError('Delivered package must preserve every qualified vehicle change exactly')
    # The observer has its own C++ ABI and libraries. These shared public
    # contracts and client implementations must still match the delivered
    # native wire peer; unrelated rendering work is not borrowed as evidence.
    contracts = ('include/halo2/multiplayer.hpp', 'include/halo2/multiplayer_network.hpp',
        'include/halo2/content.hpp', 'include/halo2/native_unit_interaction.hpp',
        'include/halo2/model_markers.hpp', 'include/halo2/collision.hpp',
        'include/halo2/source_game_variant.hpp', 'include/halo2/source_model_regions.hpp',
        'include/halo2/physics_live_world_tu15.hpp', 'src/network/multiplayer_network.cpp',
        'include/halo2/multiplayer_datagrams.hpp', 'src/assets/content.cpp')
    for name in contracts:
        if sha(frozen / name) != installed.get(name):
            raise ValueError('Neutral observer contract differs from delivered source: ' + name)
    args.output = args.output.resolve(); args.output.mkdir(parents=True, exist_ok=False)
    source = owner / 'source'; source.mkdir(); headers = source / 'headers'; pins = {}
    for directory in ('include', 'src'):
        for path in (frozen / directory).rglob('*'):
            if not path.is_file() or path.suffix not in ('.hpp','.h','.def','.inc','.inl'): continue
            name = path.relative_to(frozen); target = headers / name
            target.parent.mkdir(parents=True, exist_ok=True)
            before = sha(path); shutil.copyfile(path, target)
            if sha(target) != before or sha(path) != before: raise ValueError('Observer header changed while freezing')
            pins[str(name)] = before
    entry = ROOT / 'tools/observe_vehicle_game_lan_tu15.cpp'; entry_pin = sha(entry)
    shutil.copyfile(entry, source / 'observer.cpp')
    if sha(source / 'observer.cpp') != entry_pin or sha(entry) != entry_pin:
        raise ValueError('Neutral observer changed while freezing')
    worker_entry=ROOT/'tools/probe_vehicle_goal_ordinary_game_tu15.py'
    worker_pin=sha(worker_entry);worker_text=worker_entry.read_text()
    root_line='ROOT=Path(__file__).resolve().parents[1]'
    import_line='from probe_installed_banshee_gameplay_20261022 import Inputs'
    if worker_text.count(root_line)!=1 or worker_text.count(import_line)!=1 or sha(worker_entry)!=worker_pin:
        raise ValueError('Ordinary input worker changed while freezing')
    worker_text=worker_text.replace(root_line,'ROOT=Path('+repr(str(ROOT))+')',1)
    worker_text=worker_text.replace(import_line,'sys.path.insert(0,'+repr(str(ROOT/'tools'))+')\n'+import_line,1)
    frozen_worker=source/'ordinary_worker.py';frozen_worker.write_text(worker_text)
    worker_copy_pin=sha(frozen_worker)
    (args.output/'ordinary-worker-source.txt').write_text(worker_text)
    worker_dependencies={name:sha(ROOT/'tools'/name) for name in (
        'probe_installed_banshee_gameplay_20261022.py','validate_weapon_runtime_tu15.py',
        'weapon_test_cache.py','audit_rendering.py','mcc_content.py','build_launcher_engine.py',
        'source_layout.py','native_gameplay_audio_capture.py','native_gameplay_kernel_counters_tu15.py',
        'native_gameplay_window_close.py')}
    link_target='vehicle_choice_runtime_tu15_tests'if candidate else'native_tu15_world_memory_tests'
    link = shlex.split((build / ('CMakeFiles/'+link_target+'.dir/link.txt')).read_text())
    libraries = []; copied = {}; library_pins = {}; private_libs = owner / 'libraries'; private_libs.mkdir()
    for token in link[link.index('-o') + 2:]:
        if token.startswith('-Wl,-rpath,'): continue
        if token.startswith('-Wl,@'):
            path = Path(token.removeprefix('-Wl,@')).resolve(strict=True); target = private_libs / ('response-' + str(len(copied)) + '.rsp')
            before = sha(path); shutil.copyfile(path, target)
            if sha(path) != before or sha(target) != before: raise ValueError('Observer link response changed')
            library_pins[str(path)] = before; libraries.append('-Wl,@' + str(target)); continue
        if token.startswith('-'): libraries.append(token); continue
        path = (build / token).resolve(strict=True); before = sha(path); library_pins[str(path)] = before
        if path.is_relative_to(build):
            if path not in copied:
                target = private_libs / (str(len(copied)) + '-' + path.name)
                shutil.copyfile(path, target)
                if sha(path) != before or sha(target) != before: raise ValueError('Native observer dependency changed during copy')
                copied[path] = target
                if '.so' in path.name:
                    alias = private_libs / path.name
                    if alias.exists(): raise ValueError('Duplicate shared-library filename in observer dependencies')
                    alias.symlink_to(target.name)
            libraries.append(str(copied[path]))
        else: libraries.append(str(path))
    if any(sha(frozen / name) != pin for name, pin in pins.items()):
        raise ValueError('Observer headers changed during dependency freezing')
    # Subsequent work by the original build owner may proceed independently.
    selection = json.loads((ROOT / 'gameplay-runtime-selection.json').read_text())
    maps = ROOT / selection['cache'] / 'halo2/h2_maps_win64_dx11'
    if not candidate and ROOT / selection['runtime'] != runtime:
        raise ValueError('The selected delivery changed before its real-input validation')
    if candidate and maps!=Path(prepared['maps_directory']):raise ValueError('Candidate and active MCC map owners differ')
    if Path(args.map_name).name!=args.map_name or not args.map_name.endswith('.map'):
        raise ValueError('Use one owned MCC map filename')
    map_path=maps/args.map_name;map_pin=sha(map_path)
    q = lambda value: '"' + str(value).replace('\\', '/').replace('"', '\\"') + '"'
    cmake = '''cmake_minimum_required(VERSION 3.20)
project(DeliveredNativeVehicleGame LANGUAGES CXX)
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
add_executable(vehicle_game_observer observer.cpp)
target_compile_features(vehicle_game_observer PRIVATE cxx_std_20)
target_compile_options(vehicle_game_observer PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    cmake += 'target_include_directories(vehicle_game_observer PRIVATE ${SDL2_INCLUDE_DIRS} ' + q(headers / 'include') + ' ' + q(headers / 'src') + ')\n'
    cmake += 'target_link_libraries(vehicle_game_observer PRIVATE "-Wl,--start-group" ' + ' '.join(map(q, libraries)) + ' "-Wl,--end-group")\n'
    cmake += 'add_test(NAME delivered_native_ghost COMMAND "${Python3_EXECUTABLE}" ' + q(ROOT / 'tools/run_test.py') + ' -- "${Python3_EXECUTABLE}" -B ' + q(frozen_worker) + ' --case-worker --runtime ' + q(runtime) + ' --observer $<TARGET_FILE:vehicle_game_observer> --maps ' + q(maps) + ' --output ' + q(args.output / 'gameplay') + ' --native-audio-output --native-kernel-counters --map-name '+q(args.map_name) + (' --vehicle-choice '+args.vehicle_choice if args.vehicle_choice else '') + (' --vehicle-case '+args.vehicle_case if args.vehicle_case else '') + ')\n'
    if args.require_native_flying_selector:
        needle=' --native-audio-output --native-kernel-counters --map-name '
        if cmake.count(needle)!=1:raise ValueError('Native flying proof forwarding changed')
        cmake=cmake.replace(needle,
            ' --native-audio-output --native-kernel-counters --require-native-flying-selector --map-name ')
    if args.additional_trigger:
        needle=' --native-audio-output --native-kernel-counters'
        if cmake.count(needle)!=1:raise ValueError('Additional-trigger forwarding changed')
        cmake=cmake.replace(needle,needle+' --additional-trigger',1)
    if args.banshee_takeoff:
        needle=' --native-audio-output --native-kernel-counters'
        if cmake.count(needle)!=1:raise ValueError('Takeoff input forwarding changed')
        cmake=cmake.replace(needle,needle+' --banshee-takeoff',1)
    if args.trace_native_failure:
        needle=' --native-kernel-counters'
        if cmake.count(needle)!=1:raise ValueError('Failure-diagnosis forwarding changed')
        cmake=cmake.replace(needle,' --trace-native-failure',1)
    cmake += 'set_tests_properties(delivered_native_ghost PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE)\n'
    (source / 'CMakeLists.txt').write_text(cmake)
    command = [sys.executable, '-B', str(ROOT / 'tools/run_tests.py'), '--build-worker', '--source', str(source),
        '--target', 'vehicle_game_observer', '--filter', '^delivered_native_ghost$', '--jobs', '2', '--test-jobs', '1',
        '--output', str(args.output / 'validation')]
    report = dict(passed=False, status='running', owner=str(owner), borrowed_owner=str(borrowed),
        runtime=str(runtime), runtime_sha256=runtime_pin, exact_vehicle_changes=sorted(changed),
        validation_mode='unpublished native candidate'if candidate else(
            'independently qualified current launcher selection'if args.selected_only else'qualified launcher delivery'),
        candidate_source_binding=candidate_binding,vehicle_choice=args.vehicle_choice,vehicle_case=args.vehicle_case,
        require_native_flying_selector=args.require_native_flying_selector,
        additional_trigger=args.additional_trigger,
        banshee_takeoff=args.banshee_takeoff,
        diagnostic_only=args.trace_native_failure,
        map_input=dict(path=str(map_path),sha256=map_pin),
        additional_delivered_paths=sorted(additional), independent_observer_contracts=list(contracts),
        frozen_header_hashes=pins, copied_dependency_hashes=library_pins, temporary_dependency_bytes=sum(p.stat().st_size for p in copied.values()),
        observer_source_sha256=entry_pin,worker_original_sha256=worker_pin,
        frozen_worker_sha256=worker_copy_pin,worker_dependency_hashes=worker_dependencies,
        worker_source_retention=dict(path=str(args.output/'ordinary-worker-source.txt'),
            bytes=len(worker_text.encode()),reason='Exact normal-input producer used by this recorded game'),
        command=command, full_vehicle_parity=False)
    save = lambda: (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    save(); result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode
    unchanged = (sha(runtime) == runtime_pin and sha(map_path)==map_pin and
        all(sha(headers / name) == pin for name, pin in pins.items()) and
        sha(frozen_worker)==worker_copy_pin and
        all(sha(ROOT/'tools'/name)==pin for name,pin in worker_dependencies.items()))
    report.update(passed=result == 0 and unchanged, status='completed', validation_returncode=result,
        runtime_and_private_headers_unchanged=unchanged)
    save(); print(json.dumps(dict(passed=report['passed'], report=str(args.output / 'report.json'))), flush=True)
    return 0 if report['passed'] else result or 1


if __name__ == '__main__':
    raise SystemExit(main())
