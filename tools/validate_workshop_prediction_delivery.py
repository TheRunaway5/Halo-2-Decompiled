"""Check actual E3 prediction metadata and retained encoded resource owners.

Borrow the complete immutable Cache provider cohort while feedback qualification
continues. Archive bytes, full frozen source, selected runtime and installed
inputs must stay pinned. The World case also exercises native texture allocation
and authority. Script scheduling and full mission qualification are separate.
Run through run_test.py; run_tests.py removes each private compiler tree.
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


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recovery', type=Path, required=True)
    parser.add_argument('--terminal-refresh', type=Path, action='append', default=[],
                        help='Authenticate a guarded refresh of the same frozen source, in order')
    parser.add_argument('--base-cache', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--consumer', choices=['sound', 'bitmap', 'bitmap-cold', 'bitmap-world'], default='sound')
    parser.add_argument('--bitmap-owner-source', choices=['frozen', 'root'], default='frozen')
    parser.add_argument('--bitmap-world-overlay', type=Path)
    parser.add_argument('--bitmap-world-compile-report', type=Path)
    args = parser.parse_args()
    bitmap_world = args.consumer == 'bitmap-world'
    if bitmap_world != bool(args.bitmap_world_overlay and args.bitmap_world_compile_report) or (
            not bitmap_world and (args.bitmap_world_overlay or args.bitmap_world_compile_report)):
        parser.error('bitmap-world requires its exact overlay and compile report')
    root = Path(__file__).resolve().parents[1]
    helper_pin = digest(Path(__file__))
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    recovery = json.loads(args.recovery.read_text())
    assert recovery['passed'] and recovery['complete_source_inventory_reproduced']
    source = Path(recovery['source']).resolve(strict=True)
    build = source.parent / 'build'
    assert source.name == 'source' and source.is_relative_to(root / '.local/test-scratch')
    sys.path.insert(0, str(source / 'tools'))
    import steam_workshop as workshop
    from build_launcher_engine import source_hashes
    inventory = source_hashes(source)
    source_inventory = recovery['production_inventory_sha256']
    refresh_pins = {}
    for path in args.terminal_refresh:
        refresh_pins[str(path)] = digest(path)
        refresh = json.loads(path.read_text())
        assert refresh['passed'] and refresh['status'] == 'refreshed_not_validated'
        assert Path(refresh['source']).resolve(strict=True) == source
        assert refresh['previous_inventory_sha256'] == source_inventory
        source_inventory = refresh['production_inventory_sha256']
    assert hashlib.sha256(json.dumps(inventory, sort_keys=True).encode()).hexdigest() == source_inventory
    args.output.mkdir(parents=True, exist_ok=False)
    required_interfaces = ['include/halo2/mcc_sound_dat_resources_tu15.hpp',
                           'include/halo2/source_sound_cache_read_tu15.hpp']
    missing = [name for name in required_interfaces if not (source / name).is_file()] if args.consumer == 'sound' else []
    if missing:
        report = dict(scope=__doc__, passed=False, status='requires_coherent_resource_owner_cohort',
            source=str(source), source_inventory_sha256=source_inventory,
            missing_source_interfaces=missing,
            reason='This complete consumer requires a sound resource owner absent from the frozen source; do not mix newer Cache/sound-plan headers with these archives',
            native_tests_run=False, production_admission_changed=False,
            complete_mod_support=False, owned_scratch=str(owner), retained_large_artifacts=[])
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(dict(passed=False, status=report['status'], missing=missing)))
        return 1
    selected = {name: workshop.pin(root / name) for name in
                ('Halo2-Launcher', 'Halo2-Title-Launcher', 'gameplay-runtime-selection.json', 'frontend-runtime-selection.json')}
    journal = json.loads((args.recovery.parent / 'session/session.json').read_text())
    assert journal['status'] == 'active'
    os.kill(journal['pid'], 0)
    assert Path(journal['scratch_owner']).resolve() == source.parent
    native_bitmap = args.consumer in ('bitmap-cold', 'bitmap-world')
    target = 'mcc_cpu_bitmap_lifecycle_tests' if native_bitmap else 'workshop_mod_probe'
    link = build / 'CMakeFiles' / (target + '.dir') / 'link.txt'
    link_pin = workshop.pin(link)
    words = shlex.split(link.read_text())
    words = words[words.index('-o') + 2:]
    links, libraries = [], {}
    for word in words:
        if word.endswith('.a') or ('.so' in word and not word.startswith('-')):
            path = Path(word)
            path = (path if path.is_absolute() else build / path).resolve(strict=True)
            libraries[str(path)] = workshop.pin(path)
            links.append(str(path))
        else:
            assert word.startswith('-')
            links.append(word)
    assert str(build / 'libhalo2_cache.a') in libraries
    project = owner / 'project'
    project.mkdir()
    test = root / {
        'sound': 'tests/native_workshop_prediction_delivery_tests.cpp',
        'bitmap': 'tests/workshop_prediction_bitmap_tests.cpp',
        'bitmap-cold': 'tests/workshop_prediction_bitmap_cold_tests.cpp',
        'bitmap-world': 'tests/workshop_prediction_bitmap_cold_tests.cpp',
    }[args.consumer]
    test_pin = workshop.pin(test)
    shutil.copyfile(test, project / 'prediction.cpp')
    extra_pins, extra_sources = {}, []
    if args.consumer.startswith('bitmap'):
        bitmap_source = root if args.bitmap_owner_source == 'root' else source
        for relative, target in [
            ('src/rendering/mcc_cpu_bitmap_owner.cpp', 'owner.cpp'),
            ('src/rendering/mcc_cpu_bitmap_owner.hpp', 'private/rendering/mcc_cpu_bitmap_owner.hpp'),
            ('include/halo2/source_bitmap_pending_header.hpp', 'private/halo2/source_bitmap_pending_header.hpp'),
            ('src/rendering/source_bitmap_pending_header.cpp', 'header.cpp')]:
            path = bitmap_source / relative
            extra_pins[str(path)] = workshop.pin(path)
            destination = project / target
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, destination)
        for relative, target in [
            ('src/rendering/effects/mcc_lens_bitmap_layout.cpp', 'lens.cpp')]:
            path = source / relative
            extra_pins[str(path)] = workshop.pin(path)
            shutil.copyfile(path, project / target)
        extra_sources = ['owner.cpp', 'lens.cpp', 'header.cpp']
    if native_bitmap:
        # Relink the complete current startup data owner against the frozen
        # World. The loader publishes the authentic tables before World init.
        startup = 'renderer/native_tu15_renderer_startup_data.cpp'
        for relative in (startup, 'source_bitmap_format_catalogue_tu15.hpp'):
            path = root / 'src/rendering' / relative
            extra_pins[str(path)] = workshop.pin(path)
            destination = project / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, destination)
        extra_sources.append(startup)
        io_header = root / 'src/rendering/source_texture_cache_io_tu15.hpp'
        extra_pins[str(io_header)] = workshop.pin(io_header)
        shutil.copyfile(io_header, project / 'private/rendering' / io_header.name)
    if bitmap_world:
        world = 'src/physics/physics_live_world_tu15.cpp'
        interface = 'include/halo2/physics_live_world_tu15.hpp'
        for path in (args.bitmap_world_overlay, args.bitmap_world_compile_report):
            extra_pins[str(path)] = workshop.pin(path)
        checked = json.loads(args.bitmap_world_compile_report.read_text())
        # The active cohort can preserve a newer selected renderer. The
        # patch must still produce the exact preflighted World source bytes.
        assert checked['passed']
        assert digest(source / world) == checked['frozen_world_sha256']
        for name, pin in checked['header_sha256'].items():
            assert digest(root / name) == pin
            extra_pins[str(root / name)] = workshop.pin(root / name)
        for name in (world, interface):
            destination = project / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source / name, destination)
        subprocess.run(['patch', '--batch', '--fuzz=0', '-p1', '-i', str(args.bitmap_world_overlay.resolve())],
                       cwd=project, check=True)
        assert digest(project / world) == checked['patched_world_sha256']
        shutil.copyfile(project / interface, project / 'private/halo2' / Path(interface).name)
        extra_sources.append(world)
    # Type-check the complete consumer before preparing large map/resource
    # inputs. A fixture compile failure is not a runtime/engine failure.
    syntax = [os.environ.get('CXX', 'c++'), '-std=c++20', '-fsyntax-only',
              '-I' + str(project / 'private'), '-I' + str(source / 'include'),
              '-I' + str(source / 'src'), str(project / 'prediction.cpp')]
    if bitmap_world:
        syntax.insert(1, '-DHALO2_WORKSHOP_PREDICTION_WORLD')
    with (args.output / 'consumer-syntax.log').open('w') as log:
        syntax_result = subprocess.run(syntax, stdout=log, stderr=subprocess.STDOUT)
    if syntax_result.returncode:
        report = dict(scope=__doc__, passed=False, status='consumer_fixture_compile_failure',
            helper_sha256=helper_pin, source=str(source),
            source_inventory_sha256=recovery['production_inventory_sha256'], command=syntax,
            exit_code=syntax_result.returncode, native_tests_run=False, prepared_maps_created=False,
            complete_mod_support=False, owned_scratch=str(owner), retained_large_artifacts=[])
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        return syntax_result.returncode
    catalog = workshop.scan()
    assert not catalog['errors']
    mod = next(row for row in catalog['mods'] if row['id'] == '3360515088')
    assert not mod['error']
    item = Path(mod['directory'])
    cases = []
    for name in ('earthcity_e3', 'earthcity_e3_potato'):
        path = workshop.source_owned(item, 'maps/' + name + '.map')
        pin = workshop.pin(path)
        prepared = workshop.prepare_cached(item, name, args.base_cache, owner / (name + '-prepared'))
        assert workshop.pin(path) == pin
        cases.append(dict(name=name, source=str(path), source_pin=pin, prepared=prepared))
    cmake = '''cmake_minimum_required(VERSION 3.25)
project(WorkshopPredictionDelivery LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
include(''' + json.dumps(str(source / 'cmake/TestStorage.cmake')) + ''')
add_executable(prediction_delivery prediction.cpp ''' + ' '.join(extra_sources) + ''')
target_compile_features(prediction_delivery PRIVATE cxx_std_20)
target_compile_options(prediction_delivery PRIVATE -ffunction-sections -fdata-sections)
target_link_options(prediction_delivery PRIVATE -Wl,--gc-sections)
target_include_directories(prediction_delivery PRIVATE private ''' + json.dumps(str(source / 'include')) + ' ' + json.dumps(str(source / 'src')) + ''')
target_link_libraries(prediction_delivery PRIVATE nlohmann_json::nlohmann_json SDL2::SDL2
''' + ''.join(' ' + json.dumps(word) + '\n' for word in links) + ')\n'
    stock = None
    if bitmap_world:
        cmake += 'target_compile_definitions(prediction_delivery PRIVATE HALO2_WORKSHOP_PREDICTION_WORLD)\n'
        cmake += 'set_source_files_properties(' + world + ' PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")\n'
    if args.consumer == 'bitmap-cold':
        stock = (args.base_cache / 'halo2/h2_maps_win64_dx11/lockout.map').resolve(strict=True)
        extra_pins[str(stock)] = workshop.pin(stock)
    for case in cases:
        p = case['prepared']
        cmake += 'add_test(NAME prediction_delivery_' + case['name'] + ' COMMAND prediction_delivery ' + ' '.join(
            json.dumps(str(value)) for value in (p['map'], p['shared_maps'], p['prepared_root'])) + \
            ' "${CMAKE_CURRENT_BINARY_DIR}/' + case['name'] + '.json"'
        if stock is not None:
            cmake += ' ' + json.dumps(str(stock))
        cmake += ')\n'
        cmake += 'set_tests_properties(prediction_delivery_' + case['name'] + ' PROPERTIES TIMEOUT 300)\n'
    (project / 'CMakeLists.txt').write_text(cmake)
    command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--source', str(project),
        '--scratch-root', str(owner / 'builds'), '--output', str(args.output / 'validation'),
        '--target', 'prediction_delivery', '--filter', '^prediction_delivery_', '--jobs', '2',
        '--test-jobs', '1', '--retain-report', 'Testing/Temporary/LastTest.log']
    for case in cases:
        command += ['--retain-report', case['name'] + '.json']
    result = subprocess.run(command)
    evidence = []
    for case in cases:
        path = args.output / 'validation/evidence' / (case['name'] + '.json')
        if path.is_file():
            evidence.append(json.loads(path.read_text()))
    unchanged = source_hashes(source) == inventory and workshop.pin(link) == link_pin
    unchanged &= all(digest(Path(path)) == pin for path, pin in refresh_pins.items())
    unchanged &= digest(Path(__file__)) == helper_pin
    unchanged &= workshop.pin(test) == test_pin
    unchanged &= all(workshop.pin(Path(path)) == pin for path, pin in libraries.items())
    unchanged &= all(workshop.pin(Path(path)) == pin for path, pin in extra_pins.items())
    unchanged &= all(workshop.pin(root / name) == pin for name, pin in selected.items())
    unchanged &= all(workshop.pin(Path(case['source'])) == case['source_pin'] for case in cases)
    accepted = result.returncode == 0 and unchanged and len(evidence) == 2 and all(row['passed'] for row in evidence)
    for case in cases:
        case.pop('prepared')
    report = dict(scope=__doc__, passed=accepted, exit_code=result.returncode,
        source_inventory_sha256=source_inventory, source=str(source), terminal_refresh_sha256=refresh_pins,
        archive_and_source_inputs_unchanged=unchanged, borrowed_libraries=libraries,
        link_template=link_pin, test_source=test_pin, command=command, cases=cases, evidence=evidence,
        helper_sha256=helper_pin, consumer=args.consumer, bitmap_owner_source=args.bitmap_owner_source,
        additional_sources=extra_pins,
        full_feedback_qualification_not_implied=True, native_HS_executed=False,
        native_cache_allocation_executed=native_bitmap and accepted,
        production_world_reads_executed=bitmap_world and accepted,
        mission_flow_qualified=False,
        complete_mod_support=False, owned_scratch=str(owner), retained_large_artifacts=[])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=accepted, cases=len(evidence), inputs_unchanged=unchanged)))
    return 0 if accepted else 1


if __name__ == '__main__':
    raise SystemExit(main())
