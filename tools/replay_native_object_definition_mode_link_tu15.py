#!/usr/bin/env python3
"""Test exact CombatAction replacements against independently copied R31 archives."""
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

UNIT = 'src/gameplay/units/source_combat_action_tu15_generated.cpp'
DRIVER = 'tests/native_tu15_projectile_function_query_public_tests.cpp'
COMPARE = 'tools/validate_native_projectile_function_query_public_tu15.py'
BINDING = 'tools/native_object_definition_mode_query_replacements.py'
SPECIALIZER = 'tools/specialize_native_operands.py'
OWNER_HEADER = 'src/gameplay/objects/native_tu15_object_definition_mode_query_source_owner.hpp'
TABLE_DRIVER = 'tests/native_tu15_object_table_ratio_public_tests.cpp'
TABLE_COMPARE = 'tools/validate_native_object_table_ratio_public_tu15.py'
TABLE_BINDING = 'tools/native_object_table_ratio_replacements.py'
TABLE_HEADERS = ('include/halo2/native_tu15_object_table_ratio_kernel.hpp',
    'src/gameplay/objects/native_tu15_object_table_ratio_source_frame.hpp',
    'src/gameplay/objects/native_tu15_object_table_ratio_source_owner.hpp')
TABLE_FIXTURES = ('tools/validate_native_object_table_ratio_frame_tu15.py',
                  'tools/validate_native_object_record_ratio_frame_tu15.py')


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def inventory(path):
    return {**source_hashes(path), **test_code_hashes(path), **auxiliary_source_hashes(path)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('linked-donor', 'linked-inventory', 'installed-inventory', 'overlay-source', 'xbe', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--reverse-clip-patch',type=Path)
    parser.add_argument('--table-ratio',action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    donor = args.linked_donor.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source, build, copied = scratch/'source', donor/'build', scratch/'libraries'
    copied.mkdir()
    report = dict(passed=False, complete_build=False, stage='copying', owned_scratch=str(scratch),
                  retained_large_artifacts=[], linked_donor=str(donor), commands={})

    def save():
        (output/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')

    def run(label, command, cwd=copied, env=None, timeout=1800):
        report['stage'] = label
        report['commands'][label] = [str(arg) for arg in command]
        save()
        started = time.monotonic()
        with (output/(label+'.log')).open('w') as log:
            process = subprocess.Popen(command, cwd=cwd, env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                while True:
                    try:
                        status = process.wait(timeout=30)
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
        if status:
            raise ValueError(label+' failed: '+(output/(label+'.log')).read_text()[-6500:])

    try:
        old, installed = load(args.linked_inventory), load(args.installed_inventory)
        report['inventory_file_sha256'] = {kind:sha(path) for kind,path in
            (('donor',args.linked_inventory),('installed',args.installed_inventory))}
        overlay = args.overlay_source.resolve()
        freeze_source(donor/'source', source)
        freeze_test_code(donor/'source', source)
        if inventory(source) != old:
            raise ValueError('R31 source copy differs from its recorded inventory')
        driver_path,compare_path=(TABLE_DRIVER,TABLE_COMPARE) if args.table_ratio else (DRIVER,COMPARE)
        overlay_names=(UNIT,BINDING,SPECIALIZER,OWNER_HEADER)
        if args.table_ratio:
            overlay_names+= (TABLE_BINDING,*TABLE_HEADERS,*TABLE_FIXTURES)
        else:
            overlay_names+=(DRIVER,COMPARE)
        runtime_test_files=(driver_path,compare_path) if args.table_ratio else ()
        test_pins={name:sha(overlay/name) for name in runtime_test_files}
        for name in (*overlay_names,*runtime_test_files):
            shutil.copyfile(overlay/name, source/name)
        if sha(source/SPECIALIZER) != installed[SPECIALIZER]:
            # Clip was added after the installed 165590 snapshot. Reverse only
            # its specialization hunk, then check the older immutable hash.
            if args.table_ratio or not args.reverse_clip_patch:
                raise ValueError('Specializer differs from its exact installed source inventory')
            patch = args.reverse_clip_patch.resolve()
            report['clip_patch_sha256'] = sha(patch)
            run('reverse-clip-specializer',['git','apply','--reverse','--include='+SPECIALIZER,str(patch)],
                cwd=source,env={**os.environ,'GIT_CEILING_DIRECTORIES':str(scratch)})
        current = {**old, **{name:installed[name] for name in overlay_names}, **test_pins}
        if inventory(source) != current:
            raise ValueError('Overlay differs from the exact installed CombatAction source pins')
        report['overlay_source_sha256'] = {name:current[name] for name in (*overlay_names,*runtime_test_files)}
        print('COMBAT_ACTION_LINK_SOURCE_READY', scratch, flush=True)

        target = build/'CMakeFiles/native_projectile_function_query_public_tests.dir'
        owner = build/'CMakeFiles/halo2_physics_world_runtime.dir'
        recipe = target/'link.txt'
        link = shlex.split(recipe.read_text())
        report['link_recipe_sha256'] = sha(recipe)
        flags = {name:(path/'flags.make').read_text() for name,path in (('owner',owner),('driver',target))}
        report['flag_sha256'] = {name:sha(path/'flags.make') for name,path in (('owner',owner),('driver',target))}
        body = [line for line in (owner/'build.make').read_text().splitlines()
                if line.startswith('\t'+link[0]+' ') and f'/'+UNIT+'.o' in line and ' -c ' in line]
        if len(body) != 1 or 'native-execution/' not in body[0]:
            raise ValueError('The actual CombatAction compilation rule changed')
        report['actual_owner_compile_rule'] = body[0].strip()
        includes = shlex.split(body[0].split('$(CXX_DEFINES)',1)[1].split('$(CXX_INCLUDES)',1)[0])
        if includes != ['-I'+str(donor/'source'/Path(UNIT).parent)]:
            raise ValueError('Unexpected CombatAction per-source compiler flags')

        inputs, donor_pins, system_pins = [], {}, {}
        for arg in link[link.index('-o')+2:]:
            if arg == '-ldl':
                inputs.append(arg)
                continue
            prefix = '-Wl,@' if arg.startswith('-Wl,@') else ''
            path = build/arg.removeprefix(prefix)
            if path.is_relative_to(build):
                key = str(path.relative_to(build))
                if key not in donor_pins:
                    limit = time.monotonic()+600
                    while not path.is_file() and time.monotonic()<limit:
                        time.sleep(5)
                    if path.suffix == '.a':
                        subprocess.run(['ar','t',path],stdout=subprocess.DEVNULL,check=True,timeout=45)
                    digest = sha(path)
                    dst = copied/key
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(path,dst)
                    if sha(dst) != digest or sha(path) != digest:
                        raise ValueError('Donor link input drift: '+key)
                    donor_pins[key] = digest
                inputs.append(prefix+str(copied/key))
            elif path.is_file() and not prefix:
                system_pins[str(path)] = sha(path)
                inputs.append(arg)
            else:
                raise ValueError('Unexpected link input: '+arg)
        report.update(copied_donor_link_sha256=donor_pins,system_link_sha256=system_pins)
        print('COMBAT_ACTION_LINK_INPUTS_READY', scratch, flush=True)

        raw, prepared, instrumented = source/UNIT, scratch/'prepared.cpp', scratch/'instrumented.cpp'
        inverse=('from native_object_table_ratio_replacements import remove_object_table_ratio as table;'
                 'text=table(text);') if args.table_ratio else ''
        reverse=('import hashlib,sys;from pathlib import Path;sys.path.insert(0,sys.argv[1]);'
                 'from native_object_definition_mode_query_replacements import remove_object_definition_mode_query as r;'
                 'text=Path(sys.argv[2]).read_text();'+inverse+
                 'assert hashlib.sha256(r(text).encode()).hexdigest()==sys.argv[3]')
        python = sys.executable
        run('exact-owner-removal',[python,'-B','-c',reverse,source/'tools',raw,old[UNIT]])
        run('specialize',[python,'-B',source/SPECIALIZER,'--source',raw,'--output',prepared])
        run('instrument',[python,'-B',source/'tools/instrument_native_execution.py','--root',source,
            '--canonical',raw,'--source',prepared,'--output',instrumented,'--manifest',scratch/'instrumentation.json'])
        report['generated_inputs_sha256'] = {path.name:sha(path) for path in (prepared,instrumented,scratch/'instrumentation.json')}
        for label, cpp, custom in (('owner',instrumented,includes),('driver',source/driver_path,[])):
            parsed = [arg for line in flags[label].splitlines()
                      if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS ='))
                      for arg in shlex.split(line.partition('=')[2])]
            resolved = [arg.replace(str(donor/'source'),str(source)) for arg in (*custom,*parsed)]
            dep = scratch/(label+'.d')
            run(label,[link[0],*resolved,'-c',cpp,'-o',scratch/(label+'.o'),'-MD','-MF',dep])
            found = {}
            for arg in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                path = Path(arg).resolve()
                if path.is_relative_to(source):
                    key = str(path.relative_to(source))
                    found[key] = sha(path)
                    if found[key] != current.get(key) or found[key] != (test_pins.get(key) or installed.get(key)):
                        raise ValueError('Unexpected compiled source dependency: '+key)
            report[label+'_dependencies'] = found
        binary, link_map = scratch/'combat-action-public', scratch/'link.map'
        link_flags = [arg for arg in link[1:link.index('-o')] if not arg.endswith('.o')]
        link_flags = [arg if not arg.startswith('-Wl,--dependency-file=') else
                      '-Wl,--dependency-file='+str(scratch/'link.d') for arg in link_flags]
        run('link',[link[0],*link_flags,'-Wl,-Map='+str(link_map),scratch/'driver.o',scratch/'owner.o',
                    '-o',binary,*inputs])
        if 'libhalo2_physics_world_runtime.a(source_combat_action_tu15_generated.cpp.o)' in link_map.read_text():
            raise ValueError('The linker extracted the older CombatAction object')
        report.update(older_combat_action_member_unused=True, linked_binary_sha256=sha(binary))
        comparisons=(('table-ratio',[]),) if args.table_ratio else (('definition-mode',['--definition-mode']),('projectile',[]))
        for label, flags in comparisons:
            run(label,[python,'-B',source/compare_path,*flags,'--xbe',args.xbe.resolve(),
                       '--native',binary,'--output',output/label])
        report['source_unchanged'] = inventory(source) == current
        report['system_inputs_unchanged'] = all(sha(Path(path)) == digest for path,digest in system_pins.items())
        report.update(passed=report['source_unchanged'] and report['system_inputs_unchanged'], stage='complete')
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        save()
        check_budget(output)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
