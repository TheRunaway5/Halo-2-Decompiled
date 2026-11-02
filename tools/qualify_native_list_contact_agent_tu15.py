#!/usr/bin/env python3
"""Compare native2F8030/2F7D20 with unchanged physical TU1.5 on BEFORE data.

The captured native boundary has typed inputs, not an original GP frame. This
declares a fresh ordinary cdecl caller, empty x87 stack and private FS/stack;
it never represents that caller as a captured Xbox ABI. Every service executes
its actual original body on each side's current memory. No expected postimage
or allocation/filter/class/factory result is provided to native code.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

from oracle_smg_firing_1_1 import ROOT, Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from replay_live_phantom_hardware_tu15 import ASM, reference_text_gaps


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--prepare-only', action='store_true',
                        help='Build the declared-caller original CPU reference only; no validation is claimed')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    work = scratch/('list-contact-agent-qualification-'+args.output.name)
    work.mkdir(exist_ok=False)
    args.output.mkdir(parents=True, exist_ok=False)
    capture = args.capture.resolve(strict=True)
    manifest = capture/'capture-native-before.json'
    d = json.loads(manifest.read_text())
    assert d['schema'] == 'native-tu15-contact-agent-factory-before-v1'
    owner=d['input']['owner']
    assert (owner in (0x2f8030,0x2f81f0) or
            (args.prepare_only and owner==0x2e4eb0)) and d['input']['preserved_x87_depth'] == 0
    assert not d['expected_after_consumed'] and not d['physics_result_supplied']
    assert sha(NEW_DEFAULT) == NEW_PIN
    report = dict(passed=False, reference_xbe_sha256=NEW_PIN, capture_sha256=sha(manifest),
                  input_kind='actual native typed BEFORE with explicitly declared cdecl caller',
                  captured_original_gp_frame=False, expected_after_consumed=False,
                  supplied_original_outcomes=False, instruction_patches=[], retained_large_artifacts=[])
    try:
        regions = d['regions']
        _, gaps, authentication = reference_text_gaps(Xbe(NEW_DEFAULT), capture, regions, work)
        # Fresh ABI storage is disjoint from every actual captured input bank.
        stack, fs = 0x62000000, 0x62100000
        for r in regions:
            assert not (r['address'] < fs+4096 and stack < r['address']+r['size'])
            assert sha(capture/r['before_file'])
        words = [0]*8
        words[4] = stack+0xf000-20
        env = bytearray(108)
        fp = d['input']['floating_point']
        struct.pack_into('<H', env, 0, fp['x87_control_word'])
        struct.pack_into('<H', env, 4, fp['x87_status'])
        struct.pack_into('<H', env, 8, 0xffff)
        (work/'input.bin').write_bytes(struct.pack('<10I', *words, 0x202, fp['mxcsr'])+env+bytes(128))
        arguments = [0x1ccaca, *d['input']['arguments']]
        stack_bytes = bytearray(65536)
        struct.pack_into('<5I', stack_bytes, words[4]-stack, *arguments)
        (work/'declared-stack.bin').write_bytes(stack_bytes)
        (work/'declared-fs.bin').write_bytes(struct.pack('<I', 0xffffffff)+bytes(4092))
        all_regions = [*regions, *gaps,
                       dict(address=stack, size=65536, reference_file=work/'declared-stack.bin'),
                       dict(address=fs, size=4096, reference_file=work/'declared-fs.bin')]
        sections, assembly = [], []
        for n, r in enumerate(sorted(all_regions, key=lambda r:r['address'])):
            path = r.get('reference_file', capture/r.get('before_file', ''))
            assembly.append(f'.section .bank{n},"awx"\n.incbin "{path}"\n')
            sections.append(f'.bank{n} 0x{r["address"]:x} : {{ *(.bank{n}) }}')
        asm = ASM.replace('EMIT_BANKS', '').replace('ENTRY', str(owner)).replace('FS_BASE', str(fs)).replace('INPUT_FILE', str(work/'input.bin'))
        (work/'bridge.s').write_text(asm+'\n'+'\n'.join(assembly))
        (work/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
        subprocess.run(['as', '--32', str(work/'bridge.s'), '-o', str(work/'bridge.o')], check=True, capture_output=True)
        subprocess.run(['ld', '-melf_i386', '-T', str(work/'link.ld'), str(work/'bridge.o'), '-o', str(work/'original-hardware')], check=True, capture_output=True)
        if args.prepare_only:
            report.update(prepared=True, prepared_entry=owner, validation_not_run=True, original_elf=str(work/'original-hardware'),
                          original_elf_sha256=sha(work/'original-hardware'), original_code_authentication=authentication)
            return 0
        bridge = ROOT/'tools/observe_native_contact_producer_tu15.cpp'
        prefix = bridge.read_text().split('int main(int argc,char**argv)')[0]
        prefix = prefix.replace('void start(const char*exe,unsigned ordinal){', 'void start(const char*exe,unsigned ordinal){')
        prefix = prefix.replace('for(unsigned n=0;n<=ordinal;++n){go(0x304350);if(n<ordinal)step();}', 'go('+str(owner)+'u);')
        assert 'go(0x304350)' not in prefix
        prefix = prefix.replace('mutable std::size_t reads{};std::size_t writes{};',
                                'mutable std::size_t reads{};std::size_t writes{};bool trace=true;mutable J ledger=J::array();J stores=J::array();')
        prefix = prefix.replace('++reads;}void write', '++reads;if(trace)ledger.push_back(J::array({p,b.size()}));}void write')
        prefix = prefix.replace('child.write(p,b);++writes;', 'child.write(p,b);++writes;if(trace){J bytes=J::array();for(auto v:b)bytes.push_back(std::to_integer<unsigned>(v));stores.push_back(J::array({p,bytes}));}')
        driver = work/'driver.cpp'
        driver.write_text('#include "halo2/native_tu15_list_contact_agent.hpp"\n#include "halo2/native_tu15_collection_filter.hpp"\n'+prefix+(ROOT/'tools/observe_native_list_contact_agent_tu15.cpp').read_text())
        project = work/'project'
        project.mkdir()
        source = ROOT/'src/physics/native_tu15_list_contact_agent.cpp'
        text = source.read_text()
        variants = {'native':text}
        for name, old, new in (
            ('wrong_size', 'agent.identity+4u,40u,2', 'agent.identity+4u,20u,2'),
            ('wrong_index_stride', 'children+index*8u', 'children+index*4u'),
            ('wrong_dispatch_stride', '(first_class<<5u)+second_class', '(first_class<<4u)+second_class'),
            ('wrong_vtable', 'object,0x41ccc8u', 'object,0x41cc64u')):
            assert text.count(old) == (2 if name=='wrong_size' else 1), old
            variants[name] = text.replace(old, new)
        if owner==0x2f81f0:
            old='current{[&]{return second;},[&]{return first;}'
            assert text.count(old)==1
            variants['wrong_reversal']=text.replace(old,'current{[&]{return first;},[&]{return second;}')
        (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True,timeout=90)\nd=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not d["passed"] and d.get("original_completed"),(r.returncode,d)\nprint("Completed physical original rejects list constructor mutant")\n')
        cmake = f'''cmake_minimum_required(VERSION 3.20)
project(NativeListContact LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
'''
        dependencies = ['native_tu15_contact_agent_factory.cpp', 'native_tu15_array_growth.cpp',
                        'native_tu15_array_reserve.cpp', 'native_tu15_collection_filter.cpp']
        retain = []
        for name, content in variants.items():
            (project/(name+'.cpp')).write_text(content)
            linked = ' '.join('"'+str(ROOT/'src/physics'/p)+'"' for p in dependencies)
            cmake += f'add_executable(list_{name} $<TARGET_OBJECTS:driver> {name}.cpp {linked})\ntarget_include_directories(list_{name} PRIVATE "{ROOT}/include" "{ROOT}/src/physics")\ntarget_compile_features(list_{name} PRIVATE cxx_std_20)\n'
            cases = ('actual', 'zero_count', 'negative_count', 'one_child', 'rejected', 'native_collection') if name == 'native' else ('actual',)
            for case in cases:
                output = 'evidence/'+name+'_'+case+'.json'
                retain.append(output)
                command = f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
                if name != 'native':
                    command += f'"{sys.executable}" "{project}/negative.py" '
                command += f'$<TARGET_FILE:list_{name}> "{work}/original-hardware" "{manifest}" {case} "${{CMAKE_BINARY_DIR}}/{output}"'
                cmake += f'add_test(NAME list_{name}_{case} COMMAND {command})\nset_tests_properties(list_{name}_{case} PROPERTIES TIMEOUT 110)\n'
        (project/'CMakeLists.txt').write_text(cmake)
        files = [source, ROOT/'include/halo2/native_tu15_list_contact_agent.hpp', bridge,
                 ROOT/'include/halo2/native_tu15_collection_filter.hpp',
                 ROOT/'tools/observe_native_list_contact_agent_tu15.cpp', Path(__file__),
                 *(ROOT/'src/physics'/p for p in dependencies)]
        pins = {str(p.relative_to(ROOT)):sha(p) for p in files}
        results = []
        for compiler in ('g++', 'clang++'):
            command = [sys.executable, str(ROOT/'tools/run_tests.py'), '--source', str(project),
                       '--scratch-root', str(work/'builds'), '--output', str(args.output/compiler),
                       *sum((['--target', 'list_'+name] for name in variants), []), '--filter', '^list_',
                       '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler, '--jobs', '2', '--test-jobs', '1',
                       *sum((['--retain-report', p] for p in retain), [])]
            result = subprocess.run(command)
            results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
        report.update(passed=all(r['exit_code']==0 for r in results), compiler_results=results,
                      source_hashes=pins, current_sources_unchanged=all(sha(ROOT/p)==v for p,v in pins.items()),
                      original_code_authentication=authentication)
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
