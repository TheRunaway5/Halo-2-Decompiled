#!/usr/bin/env python3
"""Rebuild only the MapWorld and public-test objects against a frozen linked donor."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import time

from build_launcher_engine import freeze_source, source_hashes
from native_metadata import check_budget, load
from validate_native_replacement_snapshot import auxiliary_source_hashes, freeze_test_code, test_code_hashes

ROOT = Path(__file__).resolve().parents[1]
UNIT = 'src/runtime/source_map_world_support_tu15_generated.cpp'
DRIVER = 'tests/native_tu15_map_world_property_queries_public_tests.cpp'
COMPARISON = 'tools/validate_native_map_world_property_queries_public_tu15.py'
DEVICE_HEADERS = {
    'src/gameplay/objects/native_tu15_device_property_query_source_owner.hpp',
    'src/gameplay/objects/native_tu15_device_property_query_source_frame.hpp',
    'include/halo2/native_tu15_device_property_query_kernel.hpp',
}


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def inventory(path):
    return {**source_hashes(path), **test_code_hashes(path), **auxiliary_source_hashes(path)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('linked-donor', 'linked-inventory', 'source-donor', 'xbe', 'output'):
        p.add_argument('--'+name, type=Path, required=True)
    p.add_argument('--source-inventory', type=Path)
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    build = a.linked_donor.resolve() / 'build'
    output = a.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source, libraries = scratch/'source', scratch/'libraries'
    libraries.mkdir()
    report = dict(passed=False, complete_build=False, stage='copying', owned_scratch=str(scratch),
                  retained_large_artifacts=[], linked_donor=str(a.linked_donor.resolve()),
                  owning_source_donor=str(a.source_donor.resolve()), commands={})

    def save():
        (output/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')

    def run(label, command, timeout=1800):
        report['stage'] = label
        report['commands'][label] = [str(x) for x in command]
        save()
        started = time.monotonic()
        with (output/(label+'.log')).open('w') as log:
            process = subprocess.Popen(command, cwd=libraries, stdout=log, stderr=subprocess.STDOUT)
            try:
                while True:
                    try:
                        code = process.wait(timeout=30)
                        break
                    except subprocess.TimeoutExpired:
                        elapsed = int(time.monotonic()-started)
                        print(label, elapsed, flush=True)
                        if elapsed > timeout:
                            raise TimeoutError(label)
            finally:
                if process.poll() is None:
                    process.terminate()
                    process.wait()
        if code:
            raise ValueError(label+' failed: '+(output/(label+'.log')).read_text()[-6500:])

    save()
    try:
        test_target = 'CMakeFiles/native_map_world_property_queries_public_tests.dir'
        world_target = 'CMakeFiles/halo2_physics_world_runtime.dir'
        recipe = build/test_target/'link.txt'
        link = shlex.split(recipe.read_text())
        report['donor_link_recipe_sha256'] = sha(recipe)
        flag_paths = {label:build/target/'flags.make' for label,target in
                      (('mapworld', world_target), ('driver', test_target))}
        flag_inputs = {label:path.read_text() for label,path in flag_paths.items()}
        report['donor_flag_files_sha256'] = {label:sha(path) for label,path in flag_paths.items()}
        pins = {}

        def copy_input(path):
            key = str(path.relative_to(build))
            target = libraries/key
            if key not in pins:
                digest = sha(path)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, target)
                if sha(target) != digest or sha(path) != digest:
                    raise ValueError('Linked donor input drift: '+key)
                pins[key] = digest
            return str(target)

        link_inputs = []
        for arg in link[link.index('-o')+2:]:
            path = build/arg
            if arg.startswith('-Wl,@'+str(build)+'/'):
                link_inputs.append('-Wl,@'+copy_input(Path(arg.removeprefix('-Wl,@'))))
            elif path.is_relative_to(build) and path.is_file():
                link_inputs.append(copy_input(path))
            else:
                link_inputs.append(arg)
        report['copied_link_input_sha256'] = pins
        old = load(a.linked_inventory)
        donor = a.source_donor.resolve()
        new = load(a.source_inventory) if a.source_inventory else inventory(donor)
        report['source_inventory_sha256'] = hashlib.sha256(
            (json.dumps(new, indent=2, sort_keys=True)+'\n').encode()).hexdigest()
        overlays = {name:(ROOT/name).read_bytes() for name in (DRIVER, COMPARISON)}
        freeze_source(donor, source)
        freeze_test_code(donor, source)
        if inventory(source) != new or (a.source_inventory is None and inventory(donor) != new):
            raise ValueError('Copied owning source differs from frozen inventory')
        for name, body in overlays.items():
            (source/name).write_bytes(body)
            new[name] = sha(source/name)
        report['public_overlay_sha256'] = {name:new[name] for name in overlays}
        print('DEVICE_LINKED_INPUTS_READY', scratch, flush=True)
        python = sys.executable
        raw, prepared, instrumented = source/UNIT, scratch/'prepared.cpp', scratch/'instrumented.cpp'
        verify = (
            'import hashlib,sys;from pathlib import Path;sys.path.insert(0,sys.argv[1]);'
            'from native_device_property_query_replacements import remove_device_property_query;'
            'assert hashlib.sha256(remove_device_property_query(Path(sys.argv[2]).read_text()).encode()).hexdigest()==sys.argv[3]')
        run('exact-removal', [python,'-B','-c',verify,source/'tools',raw,old[UNIT]])
        run('specialize', [python,'-B',source/'tools/specialize_native_operands.py','--source',raw,'--output',prepared])
        run('instrument', [python,'-B',source/'tools/instrument_native_execution.py','--root',source,
            '--canonical',raw,'--source',prepared,'--output',instrumented,'--manifest',scratch/'instrumentation.json'])
        for label, cpp, extra in (('mapworld', instrumented, ['-I'+str(source/'src/runtime')]),
                                  ('driver', source/DRIVER, [])):
            flags = [arg for line in flag_inputs[label].splitlines()
                     if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS ='))
                     for arg in shlex.split(line.partition('=')[2].replace(str(build.parent/'source'),str(source)))]
            dep = scratch/(label+'.d')
            run(label, [link[0],*extra,*flags,'-c',cpp,'-o',scratch/(label+'.o'),'-MD','-MF',dep])
            dependencies = {}
            for item in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                path = Path(item).resolve()
                if not path.is_relative_to(source):
                    continue
                key = str(path.relative_to(source))
                dependencies[key] = sha(path)
                if dependencies[key] != new.get(key) or (key not in DEVICE_HEADERS|{DRIVER} and dependencies[key] != old.get(key)):
                    raise ValueError('Unexpected dependency change: '+key)
            report[label+'_dependencies'] = dependencies
        binary, link_map = scratch/'device-public', scratch/'link.map'
        run('link',[*link[:3],'-Wl,-Map='+str(link_map),scratch/'driver.o',scratch/'mapworld.o','-o',binary,*link_inputs])
        if 'libhalo2_physics_world_runtime.a(source_map_world_support_tu15_generated.cpp.o)' in link_map.read_text():
            raise ValueError('The linker took the older MapWorld archive member')
        report.update(older_mapworld_member_unused=True, linked_binary_sha256=sha(binary))
        run('compare',[python,'-B',source/COMPARISON,'--include-device','--xbe',a.xbe.resolve(),
                       '--native',binary,'--output',output/'public'],2400)
        report['source_unchanged'] = inventory(source) == new
        report.update(passed=report['source_unchanged'], stage='complete')
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        save()
        check_budget(output)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
