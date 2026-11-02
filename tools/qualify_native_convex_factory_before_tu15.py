#!/usr/bin/env python3
"""Physically compare the bound native convex factory on actual typed BEFORE.

Requires an enclosing run_test.py owner and a prepare-only original ELF receipt.
Unchanged TU1.5 executes its complete constructor. Native scratch is independent;
every captured byte, return, FP word and ordered persistent operand is checked.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from generate_physics_live_support_tu15 import complete_body
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

OWNERS = (0x2e4eb0, 0x2f8300, 0x2e4550, 0x309060, 0x309150,
          0x2e4690, 0x2e47b0, 0x2e5da0, 0x3077c0, 0x2e7e10,
          0x306750, 0x2e5db0, 0x3077f0, 0x1f6610, 0x306790,
          0x2e7d90, 0x1cb030, 0xa9520, 0xad390, 0x31dd70,
          0xadb50, 0x2e8cf0, 0x2e8d70, 0x2e8d20, 0xad740, 0x31d520,
          0x149da0, 0x149e00, 0x233250, 0x233420)
SOURCES = ('convex_agent_factory', 'contact_agent_factory', 'inverse_geometry',
           'contact_cache', 'contact_shape', 'geometry', 'activation_services',
           'array_allocator', 'array_growth', 'array_reserve', 'agent_cleanup_children',
           'agent_cleanup')


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def metadata(image):
    code = _Code(image, '.text')
    registers = {name: n for n, name in enumerate(('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi'))}
    sites, owners, private_fs = {}, {}, []
    for owner in OWNERS:
        body = complete_body(code, owner)
        listing = []
        for pc in sorted(body.code):
            ins = code.at(pc)
            listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
            operands = []
            if ins.mnemonic != 'lea':
                for op in ins.operands:
                    if op.type != X86_OP_MEM:
                        continue
                    segment = ins.reg_name(op.mem.segment)
                    # These are the original SEH operations in the freshly
                    # declared private FS bank, never a captured persistent bank.
                    if segment == 'fs':
                        private_fs.append(pc)
                        continue
                    assert segment in (None, '', 'es'), (hex(pc), segment)
                    base, index = ins.reg_name(op.mem.base), ins.reg_name(op.mem.index)
                    operands.append(dict(base=registers[base] if base else -1,
                                         index=registers[index] if index else -1,
                                         scale=op.mem.scale, displacement=op.mem.disp & 0xffffffff,
                                         bytes=op.size, read=bool(op.access & CS_AC_READ),
                                         write=bool(op.access & CS_AC_WRITE)))
            site = dict(instruction=pc, operands=operands,
                        immutable_control_dispatch=pc in (0x3078a2, 0x3078a9))
            assert pc not in sites or sites[pc] == site
            sites[pc] = site
        owners[hex(owner)] = dict(instructions=len(body.code), listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest())
    table = dict(address=0x307988, bytes=43, hex=image.read(0x307988, 43).hex())
    return (dict(sites=list(sites.values()), immutable_dispatch_table=table),
            dict(owners=owners, declared_private_fs_sites=sorted(set(private_fs)),
                 immutable_support_dispatch_table_sha256=hashlib.sha256(bytes.fromhex(table['hex'])).hexdigest(),
                 native_control_lowering='Two original immutable compiler switch-table reads are authenticated separately; native C++ branches replace control dispatch. No persistent bank byte is excluded.'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--original-preparation', type=Path, required=True)
    parser.add_argument('--source-cohort', type=Path, required=True)
    parser.add_argument('--binding-candidate', type=Path,
                        help='Declare a single candidate binding header without changing the frozen engine')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', action='append', choices=('g++', 'clang++'))
    parser.add_argument('--poison', action='append', type=lambda value: int(value, 0))
    parser.add_argument('--mutations', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    work = scratch/('convex-before-'+args.output.name)
    work.mkdir(exist_ok=False)
    args.output.mkdir(parents=True, exist_ok=False)
    report = dict(passed=False, goal_complete=False, supplied_original_outcomes=False,
                  captured_original_gp_frame=False, instruction_patches=[], retained_large_artifacts=[])
    tracked = {}
    try:
        source = args.source_cohort.resolve(strict=True)
        manifest = args.capture.resolve(strict=True)/'capture-native-before.json'
        captured = json.loads(manifest.read_text())
        assert captured['input']['owner'] == 0x2e4eb0 and captured['input']['preserved_x87_depth'] == 0
        preparation = json.loads(args.original_preparation.read_text())
        assert preparation['prepared'] and preparation['validation_not_run'] and not preparation['passed']
        assert preparation['reference_xbe_sha256'] == NEW_PIN and sha(NEW_DEFAULT) == NEW_PIN
        assert preparation['capture_sha256'] == sha(manifest)
        original = Path(preparation['original_elf']).resolve(strict=True)
        assert original.is_relative_to(scratch) and sha(original) == preparation['original_elf_sha256']
        tracked = {p: sha(p) for p in (manifest, original, args.original_preparation.resolve(), NEW_DEFAULT,
                   Path(__file__).resolve(), ROOT/'tools/observe_native_convex_factory_before_tu15.cpp',
                   *(manifest.parent/r['before_file'] for r in captured['regions']))}
        project = work/'project'
        project.mkdir()
        shutil.copytree(source/'include', project/'include')
        for path in (source/'include').rglob('*'):
            if path.is_file():
                tracked[path] = sha(path)
                assert tracked[path] == sha(project/'include'/path.relative_to(source/'include'))
        if args.binding_candidate:
            candidate = args.binding_candidate.resolve(strict=True)
            tracked[candidate] = sha(candidate)
            target = project/'include/halo2/native_tu15_convex_agent_binding.hpp'
            shutil.copyfile(candidate, target)
            assert sha(target) == tracked[candidate]
            report['binding_candidate'] = dict(path=str(candidate), sha256=tracked[candidate],
                                              frozen_engine_unchanged=True)
        for path in (source/'src/physics').glob('*.hpp'):
            shutil.copyfile(path, project/path.name)
            tracked[path] = sha(path)
        for name in SOURCES:
            path = source/f'src/physics/native_tu15_{name}.cpp'
            shutil.copyfile(path, project/path.name)
            tracked[path] = sha(path)
        driver = ROOT/'tools/observe_native_convex_factory_before_tu15.cpp'
        shutil.copyfile(driver, project/'driver.cpp')
        operands, source_proof = metadata(Xbe(NEW_DEFAULT))
        metadata_path = work/'metadata.json'
        metadata_path.write_text(json.dumps(operands, separators=(',', ':'))+'\n')
        factory = (project/'native_tu15_convex_agent_factory.cpp').read_text()
        variants = {'native': factory}
        if args.mutations:
            for name, before, after in (
                    ('wrong_derived_vtable', '0x41c370u', '0x41c371u'),
                    ('wrong_empty_count', 'm.word(0x2f838cu,p+0x18u,0u)', 'm.word(0x2f838cu,p+0x18u,1u)'),
                    ('wrong_allocation_size_word', 'agent.identity+4u,256u,2', 'agent.identity+4u,255u,2')):
                assert factory.count(before) == 1
                variants[name] = factory.replace(before, after)
        (project/'negative.py').write_text(
            'import json,subprocess,sys\n'
            'r=subprocess.run(sys.argv[1:],capture_output=True,text=True,timeout=100)\n'
            'd=json.load(open(sys.argv[-1]))\n'
            'assert r.returncode==1 and not d["passed"] and d.get("original_completed") and d.get("native_completed") and "mismatch" in d.get("error",""),(r.returncode,d)\n'
            'print("Complete physical original rejects native constructor mutation")\n')
        cmake = '''cmake_minimum_required(VERSION 3.20)
project(NativeConvexBefore LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
add_library(common OBJECT driver.cpp '''+' '.join('native_tu15_'+name+'.cpp' for name in SOURCES if name != 'convex_agent_factory')+''')
target_include_directories(common PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_features(common PRIVATE cxx_std_20)
target_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)
'''
        retained = []
        for name, text in variants.items():
            (project/(name+'.cpp')).write_text(text)
            cmake += f'add_executable(convex_{name} $<TARGET_OBJECTS:common> {name}.cpp)\ntarget_include_directories(convex_{name} PRIVATE "${{CMAKE_SOURCE_DIR}}/include")\ntarget_compile_features(convex_{name} PRIVATE cxx_std_20)\ntarget_compile_options(convex_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
            for poison in (args.poison or (0, 165, 90)) if name == 'native' else (165,):
                relative = f'evidence/{name}_poison{poison}.json'
                retained.append(relative)
                command = f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
                if name != 'native':
                    command += f'"{sys.executable}" "{project}/negative.py" '
                command += f'$<TARGET_FILE:convex_{name}> "{original}" "{manifest}" "{metadata_path}" {poison} "${{CMAKE_BINARY_DIR}}/{relative}"'
                cmake += f'add_test(NAME convex_{name}_{poison} COMMAND {command})\nset_tests_properties(convex_{name}_{poison} PROPERTIES TIMEOUT 120)\n'
        (project/'CMakeLists.txt').write_text(cmake)
        results = []
        for compiler in args.compiler or ('g++', 'clang++'):
            command = [sys.executable, str(ROOT/'tools/run_tests.py'), '--source', str(project),
                       '--scratch-root', str(work/'builds'), '--output', str(args.output/compiler),
                       *sum((['--target', 'convex_'+name] for name in variants), []),
                       '--filter', '^convex_', '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
                       '--jobs', '2', '--test-jobs', '1',
                       *sum((['--retain-report', relative] for relative in retained), [])]
            result = subprocess.run(command)
            results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
        report.update(passed=all(result['exit_code'] == 0 for result in results), compiler_results=results,
                      reference_xbe_sha256=NEW_PIN, original_elf_sha256=sha(original),
                      capture_sha256=sha(manifest), captured_bank_bytes=captured['bank_bytes'],
                      original_operand_metadata=source_proof,
                      source_sha256={str(path.relative_to(source)): digest for path, digest in tracked.items() if path.is_relative_to(source)},
                      helper_sha256={str(path.relative_to(ROOT)): tracked[path] for path in (Path(__file__).resolve(), driver)})
    finally:
        changes = [str(path) for path, digest in tracked.items() if not path.is_file() or sha(path) != digest]
        report['inputs_unchanged'] = not changes
        report['changed_inputs'] = changes
        if changes:
            report['passed'] = False
        (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
