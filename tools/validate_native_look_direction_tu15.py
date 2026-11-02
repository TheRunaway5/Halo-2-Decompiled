#!/usr/bin/env python3
"""Unchanged TU1.5 direction-to-look leaf, full DATA/FP and native compiler proof.

All original banks, ELF files and compiler products belong to run_test.py.
"""
import argparse, copy, hashlib, json, os, shutil, struct, subprocess, sys
from pathlib import Path
import validate_native_island_agent_phase_tu15 as physical
import validate_native_body_deactivation_listener_tu15 as metadata_parent
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def cases(image):
    rows = []
    vectors = ((1., 0., 0.), (0., 1., 0.), (-1., 0., 0.), (0., -1., 0.),
               (0., 0., 1.), (0., 0., -1.), (.3, -.4, .5), (-1., -2., 3.),
               (0., 0., 0.), (-0., -0., -0.), (1e-35, -1e-35, 1e-35),
               (float('inf'), 1., 0.),
               (0x7fc12345, 0x3f800000, 0x80000000),
               (0x7f812345, 0xbf800000, 0x3f000000),
               (0xffc12345, 0x7fc54321, 0x7f823456),
               (0x00000001, 0x80000001, 0x00000002),
               (0x7f7fffff, 0xff7fffff, 0x7f7fffff))
    for n, vector in enumerate(vectors):
        for rounding in range(4):
            index = (0, 1, 5, 0xffffffff, 0x40000000)[n % 5]
            inputs = 0x1001000
            destination = (inputs + index * 0x94 + 0x24) & 0xffffffff
            for alias in (False, True):
                direction = destination if alias else 0x1000200
                raw = bytearray(0x6000)
                for lane, value in enumerate(vector):
                    struct.pack_into('<I' if isinstance(value, int) else '<f', raw,
                                     direction - 0x1000000 + lane * 4, value)
                ordinal = len(rows)
                banks = {0x1000000: raw, 0x4f9b54: struct.pack('<I', inputs),
                         0x44923c: image.read(0x44923c, 4)}
                rows.append(dict(name=f'direction_{n}_{rounding}_{alias}', entry=0x18a2a0,
                    receiver=index, arguments=[], entry_registers={'eax': direction, 'ecx': index},
                    direction=direction, input_index=index,
                    banks=[dict(address=p, before=bytes(b).hex()) for p, b in sorted(banks.items())],
                    before_fp=[(0x27f, 0x37f, 0x7f)[ordinal % 3] | (rounding << 10),
                               ((ordinal % 8) << 11) | (0x453f if n % 3 == 0 else 0),
                               (0x1f80, 0x5fa5, 0xffc0)[ordinal % 3]],
                    stop_pc=physical.STOP, boundary=None))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    report = dict(passed=False, owned_scratch=str(scratch), retained_large_artifacts=[])
    source = ROOT/'src/gameplay/vehicles/native_tu15_look_direction.cpp'
    files = [source, ROOT/'include/halo2/native_tu15_look_direction.hpp',
             ROOT/'tests/native_tu15_look_direction_tests.cpp', Path(__file__),
             ROOT/'tools/validate_native_island_agent_phase_tu15.py',
             ROOT/'tools/validate_native_body_deactivation_listener_tu15.py']
    pins = {str(p.relative_to(ROOT)): sha(p) for p in files}
    try:
        assert sha(NEW_DEFAULT) == NEW_PIN
        image = Xbe(NEW_DEFAULT)
        namespace = dict(metadata_parent.__dict__, OWNERS=(0x18a2a0,))
        import inspect
        text = inspect.getsource(metadata_parent.metadata).replace('assert counts == [63, 46]', 'assert counts == [30]').replace('boundaries=[0x149da0, 0x1d7260]', 'boundaries=[]')
        exec(text, namespace); meta = namespace['metadata'](image)
        # Capstone labels these two authenticated FSTP m32 operands as reads.
        # Correct the observer's DATA direction; the original code is unchanged.
        code = _Code(image, '.text')
        for site in meta['sites']:
            if site['instruction'] in (0x18a2ba, 0x18a2dd):
                instruction = code.at(site['instruction'])
                assert instruction.mnemonic == 'fstp' and len(site['operands']) == 1
                operand = site['operands'][0]
                assert operand['bytes'] == 4
                operand.update(read=False, write=True)
        rows = physical.physical(image, meta, cases(image), scratch)
        prefixes = []
        for seed in (rows[0], rows[49]):
            occurrences = {}
            for event in seed['reads']:
                key = event['address'], event['bytes']
                occurrences[key] = occurrences.get(key, 0) + 1
                row = copy.deepcopy(seed)
                row.update(name=seed['name']+'_read_'+str(len(prefixes)), stop_pc=event['instruction'],
                           fault=dict(address=key[0], bytes=key[1], occurrence=occurrences[key], access='read'))
                prefixes.append(row)
            for n, event in enumerate(seed['writes']):
                row = copy.deepcopy(seed)
                row.update(name=seed['name']+'_observer_'+str(n),
                           stop_pc=event['instruction']+code.at(event['instruction']).size, observer_fault=n+1)
                prefixes.append(row)
        rows += physical.physical(image, meta, prefixes, scratch)
        fixture = scratch/'fixture.json'
        save(fixture, dict(image_sha256=NEW_PIN, backend='unchanged-tu15-native-look-direction-v1', cases=rows))
        project = scratch/'project'; project.mkdir(); shutil.copytree(ROOT/'include', project/'include')
        shutil.copyfile(files[2], project/'driver.cpp'); text = source.read_text(); (project/'native.cpp').write_text(text)
        mutants = {}
        for name, old, new in (('stride', 'index*0x94u', 'index*0x90u'),
                               ('height', 'word(m,direction+8u)', 'word(m,direction+4u)'),
                               ('period', '0x44923cu', '0x4f9b54u')):
            assert text.count(old) == 1; mutants[name] = text.replace(old, new)
            (project/(name+'.cpp')).write_text(mutants[name])
        (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "look" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint(r.stderr[-4096:])\n')
        cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(NativeLookDirection LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
        for name in ('native', *mutants):
            cmake += f'add_executable(look_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(look_{name} PRIVATE include)\ntarget_compile_features(look_{name} PRIVATE cxx_std_20)\n'
            command = '' if name == 'native' else f'"{sys.executable}" "{project}/negative.py" '
            cmake += f'add_test(NAME look_{name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command}$<TARGET_FILE:look_{name}> "{fixture}")\n'
        (project/'CMakeLists.txt').write_text(cmake)
        results = []
        for compiler in ('g++', 'clang++'):
            command = [sys.executable, str(ROOT/'tools/run_tests.py'), '--source', str(project),
                       '--scratch-root', str(scratch/'builds'), '--output', str(args.output/compiler),
                       '--jobs', '1', '--test-jobs', '1', '--filter', '^look_', '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            for name in ('native', *mutants): command += ['--target', 'look_'+name]
            result = subprocess.run(command); results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
        unchanged = all(sha(ROOT/p) == pin for p, pin in pins.items())
        report.update(passed=unchanged and all(r['exit_code'] == 0 for r in results), compiler_results=results,
            source_snapshot_unchanged=unchanged, source_hashes=pins, original_sha256=NEW_PIN,
            body_sites=meta['body_sites'], listing_sha256=meta['listing_sha256'], cases=len(rows),
            full_returns=len(rows)-len(prefixes), ordered_reads=sum(len(r['reads']) for r in rows),
            ordered_stores=sum(len(r['writes']) for r in rows), negative_controls=list(mutants),
            instruction_patches=[], supplied_callee_results=False,
            operand_metadata_corrections={'0x18a2ba': 'FSTP m32 writes, does not read',
                                          '0x18a2dd': 'FSTP m32 writes, does not read'},
            remaining=['Persistent FSTP write-fault commit semantics', 'Nonempty x87 value preservation/overflow',
                       'Wider alias/callback domains', 'Native caller and visible multiplayer integration'])
    except BaseException as error:
        report['error'] = str(error) or repr(error); raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(p.stat().st_blocks*512 for p in scratch.rglob('*') if p.is_file())
        save(args.output/'report.json', report)
    print(json.dumps({k: report[k] for k in ('passed', 'cases', 'full_returns', 'ordered_reads', 'ordered_stores')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
