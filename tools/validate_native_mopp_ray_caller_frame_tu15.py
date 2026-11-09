#!/usr/bin/env python3
"""Qualify the private ray caller-frame foundation; no production binding."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys

from oracle_smg_firing_1_1 import ROOT, Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original
from native_mopp_ray_dense_original import physical_original_dense

REGIONS = ((0x61000000, 0x1000), (0x62000000, 0x8000))
FILES = (
    'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_integer_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_scale_frame.hpp',
    'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
    'include/halo2/native_tu15_mopp_ray_scale_kernel.hpp',
    'include/halo2/native_tu15_mopp_ray_traversal_kernel.hpp',
    'include/halo2/native_tu15_mopp_primitive_key_kernel.hpp',
    'include/halo2/native_tu15_mopp_ray_plane_kernel.hpp',
    'include/halo2/native_tu15_mopp_ray_branch_kernel.hpp',
    'include/halo2/native_tu15_mopp_ray_projection_kernel.hpp',
    'include/halo2/native_tu15_mopp_ray_traversal.hpp',
    'src/physics/runtime/native_tu15_mopp_aabb_traversal.hpp',
    'include/halo2/native_tu15_mopp_integer_program_kernel.hpp',
    'src/physics/runtime/physics_world_runtime_internal.hpp',
    'src/physics/runtime/physics_contact_runtime_internal.hpp',
    'tests/native_tu15_mopp_ray_caller_frame_tests.cpp',
    'tools/validate_native_mopp_ray_caller_frame_tu15.py',
    'tools/native_mopp_ray_dense_original.py',
    'tools/validate_native_tu15_prt_helpers.py',
    'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
)


def hashes():
    return {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in FILES}


def negative_controls(scratch, output, rows, io):
    """Compile actual adapter defects; never alter the original reference."""
    project=scratch/'negative-controls'
    project.mkdir()
    selected=[row for row in rows if row['name'].startswith('scale_1_basic_0')]
    assert len(selected)>1,'Negative controls need a scale return and physical retained prefixes'
    fixture=project/'fixture.json'
    fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=selected,scale_programs=True)))
    header='physics/runtime/native_tu15_mopp_ray_scale_frame.hpp'
    original=(ROOT/'src'/header).read_text()
    mutations={
        'unprotected_provider':('scope->io(std::forward<Callback>(callback));',
                                'std::forward<Callback>(callback)();'),
        'premature_spill_commit':('Base::store(pc,g[4]+offset,pending.bits,[&]{scope_.commit(pending);},true);',
                                  'scope_.commit(pending);Base::store(pc,g[4]+offset,pending.bits,[]{},true);'),
    }
    # A signal is an expected result for the deliberately unprotected callback;
    # CTest WILL_FAIL alone cannot represent expected signal termination.
    wrapper=project/'expect_rejection.py'
    wrapper.write_text('''import json,subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
print(json.dumps(dict(rejected=r.returncode!=0,exit_code=r.returncode,diagnostic=(r.stdout+r.stderr)[-3000:])))
sys.exit(0 if r.returncode!=0 else 1)
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(ray_negative LANGUAGES CXX)\ninclude(CTest)\n'
    for name,(before,after) in mutations.items():
        assert original.count(before)==1,(name,'Adapter mutation boundary drift')
        overlay=project/name
        target=overlay/header
        target.parent.mkdir(parents=True)
        target.write_text(original.replace(before,after))
        cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_mopp_ray_caller_frame_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" $<TARGET_FILE:{name}> "{fixture}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
             '--scratch-root',str(scratch/'builds'),'--output',str(output/'negative-controls'),
             '--filter','^(unprotected_provider|premature_spill_commit)$']
    for name in mutations:command+=['--target',name]
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert result.returncode==0,'Adapter negative-control build/rejection failed: '+result.stdout[-3000:]
    return dict(passed=True,compiled_mutations=list(mutations),physical_cases=len(selected),
                reference_instructions_or_outcomes_modified=False,command=command)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--integer-programs', action='store_true')
    parser.add_argument('--scale-programs', action='store_true')
    parser.add_argument('--pattern')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--negative-controls', action='store_true')
    args = parser.parse_args()
    if not args.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    report = dict(passed=False, production_binding=False, owned_scratch=str(scratch),
                  retained_large_artifacts=[], source_before=hashes())
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest() == PIN
        image = Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES = (0x30aa70,)
        original_metadata = common.metadata
        code = common._Code(image, '.text')
        def metadata(image):
            result = original_metadata(image)
            for site in result['sites']:
                mnemonic=code.at(site['instruction']).mnemonic
                if mnemonic in ('pop', 'ret'):
                    site['operands'].append(dict(base=4, index=-1, scale=1,
                                                displacement=0, bytes=4, read=True, write=False))
                # Capstone labels these pure stores as reads. Apply the same
                # authenticated mnemonic ownership as physical_original before
                # choosing failure prefixes; FST/FSTP are not RMW operations.
                if mnemonic in ('fstp','fst','fistp','fist','fisttp'):
                    for operand in site['operands']:operand.update(read=False,write=True)
            return result
        common.metadata = metadata
        sites = metadata(image)['sites']
        io = [site['instruction'] for site in sites
              if site['instruction'] not in (0x30aaa0, 0x30aaa7) and
              (site['operands'] or code.at(site['instruction']).mnemonic in ('push', 'pop', 'ret', 'call'))]
        live = ('0000000000000080ff3f', '00000000000000000080',
                '35c26821a2da0fc90040', '45230100000000c0ff7f',
                '00000000000000800040', '00000000000000c0ff3f',
                '00000000000000000000', '0000000000000080ffbf')
        floating_bits=(0,0x80000000,1,0x007fffff,0x00800000,0x3f800001,
                       0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,
                       0x7f812345,0xffc23456,0xff823456,0x3eaaaaab,
                       0x80800000,0x807fffff)
        rng = random.Random(0x30aa70)
        rows = []
        programs=[(f'empty_{n}',bytes([0]),None) for n in range(128)]
        if args.scale_programs:
            for opcode in range(1,5):
                for n in range(16):
                    value=(0,1,127,255)[n%4]
                    program=bytes([opcode,value,(value+37)&255,(255-value)&255,0])
                    programs.append((f'scale_{opcode}_basic_{n}',program,None))
                for delta in (-32,-16,0,16,32,64):
                    programs.append((f'scale_{opcode}_variable_overlap_{delta}',bytes([opcode,17,31,57,0]),delta))
                for delta in (-128,-80,-64,-48,-32,-16,0,4,16,32,48,64,80,96,128):
                    programs.append((f'scale_{opcode}_bounds_overlap_{delta}',bytes([opcode,17,31,57,0]),delta))
                programs.append((f'scale_{opcode}_bounds_variable_overlap',bytes([opcode,0,127,255,0]),None))
        if args.integer_programs:
            for opcode,count in ((5,1),(6,2),(7,3)):
                for jump in (0,1,32,128) if count==1 else (0,1,256,512):
                    program=bytes([opcode])+jump.to_bytes(count,'big')+bytes([0x8])*jump+bytes([0])
                    programs.append((f'jump_{opcode}_{jump}',program,None))
            for opcode,count in ((9,1),(10,2),(11,4)):
                for value in (0,1,(1<<(count*8))-1,0x12345678&((1<<(count*8))-1)):
                    programs.append((f'key_{opcode}_{value}',bytes([opcode])+value.to_bytes(count,'big')+bytes([0]),None))
            for opcode in range(0x60,0x6c):
                count=1 if opcode<0x64 else 2 if opcode<0x68 else 4
                for value in (0,1,(1<<(count*8))-1,0x12345678&((1<<(count*8))-1)):
                    programs.append((f'property_{opcode}_{value}',bytes([opcode])+value.to_bytes(count,'big')+bytes([0]),None))
            for opcode,count in ((9,1),(10,2),(11,4),(0x60,1),(0x61,1),(0x64,2),(0x65,2),(0x68,4),(0x69,4)):
                for delta in (-32,-16,-8,-4,0,4,8,16,24,32,64):
                    program=bytes([opcode])+bytes([0xa5]*count)+bytes([9,17,0])
                    programs.append((f'variable_overlap_{opcode}_{delta}',program,delta))
            programs.append(('mixed_fields',bytes([0x61,17,9,255,0x65,1,2,10,255,255,0x60,42,11,0x12,0x34,0x56,0x78,0]),None))
        for ordinal,(name,program,alias_delta) in enumerate(programs):
            banks = [bytearray(rng.randbytes(size)) for _, size in REGIONS]
            banks[0][0x100:0x100+len(program)] = program
            if name.startswith('scale_') and '_basic_' in name:
                # Authored inputs, never expected results: explicit zero,
                # denormal, infinity and NaN operands across PC/RC/DAZ/FTZ.
                n=int(name.rsplit('_',1)[1])
                for lane in range(4):
                    banks[0][0x800+lane*4:0x804+lane*4]=floating_bits[(n+lane)%16].to_bytes(4,'little')
                banks[0][0x814:0x818]=floating_bits[n].to_bytes(4,'little')
                for lane in range(8):
                    banks[0][0x900+lane*4:0x904+lane*4]=floating_bits[(n+3+lane)%16].to_bytes(4,'little')
            sp = 0x62006000 + ordinal % 16
            # Variables and bounds deliberately need no backing DATA: an empty
            # geometry program reads their argument words, not their contents.
            variable=rng.getrandbits(32) if name.startswith('empty_') else 0x61000800
            if alias_delta is not None and '_bounds_overlap_' not in name:
                variable=((sp-4)&0xfffffff0)-0x194-12+0x40+alias_delta
            args_words = [0x63000000, variable, 0x61000100, 0x61000900 if name.startswith('scale_') else rng.getrandbits(32)]
            if '_bounds_overlap_' in name:args_words[3]=((sp-4)&0xfffffff0)-0x194-12+0x40+alias_delta
            if name.endswith('_bounds_variable_overlap'):args_words[3]=variable
            for n, value in enumerate(args_words):
                offset = sp - REGIONS[1][0] + n*4
                banks[1][offset:offset+4] = value.to_bytes(4, 'little')
            gp = [rng.getrandbits(32) for _ in range(8)]
            gp[4] = sp
            depth = (0, 1, 4, 7 if args.scale_programs else 8)[(ordinal//16) % 4]
            rows.append(dict(id=ordinal, name=name, entry=0x30aa70, program_opcode=program[0],
                             gp=gp, eflags=0x202 | (rng.getrandbits(12) & 0xcd5),
                             xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],
                             live_x87=list(live[:depth]),
                             before_fp=[(0x7f, 0x27f, 0x37f)[ordinal % 3] | ((ordinal % 4) << 10),
                                        ((ordinal % 8) << 11) | (ordinal & 0x473f),
                                        0x1f80 | ((ordinal % 4) << 13) | (0, 0x40, 0x8000, 0x8040)[ordinal % 4]],
                             banks=[dict(address=base, before=bytes(data).hex())
                                    for (base, _), data in zip(REGIONS, banks)],
                             matrix=args_words[1], source=args_words[2], bounds=args_words[3], max_steps=1000,
                             stop_before_entries=[0x63000000]))
            if (alias_delta is not None and variable&15) or (name.startswith('scale_') and args_words[3]&15):
                rows[-1].update(alignment_fault=True,phase='alignment_fault')
        if args.pattern:
            rows=[row for row in rows if args.pattern in row['name']]
            assert rows,'No matching caller-frame input'
        run_original=physical_original_dense if args.scale_programs else physical_original
        physical, authority = run_original(image, rows, scratch, entries=(0x30aa70,),
            regions=REGIONS, trace_calls=[dict(entry=pc, gp=True, xmm=True) for pc in io],
            trace_extended=True, preserve_x87_prefix=not args.scale_programs)
        prefixes = []
        prefix_indices=list(range(min(16,len(rows))));seen=set()
        for n,row in enumerate(rows[128:],128):
            if row['program_opcode'] not in seen:
                prefix_indices.append(n);seen.add(row['program_opcode'])
        operand_sites={site['instruction']:site for site in sites}
        for index in prefix_indices:
            row,observed=rows[index],physical['cases'][index]
            ordinal = 0
            for physical_ordinal, event in enumerate(observed['reads']):
                if event['instruction'] in (0x30aaa0, 0x30aaa7):
                    continue
                prefix = copy.deepcopy(row)
                prefix.update(id=len(rows)+len(prefixes), name=row['name']+f'_read_{ordinal}',
                              phase='before_read', native_fault_ordinal=ordinal,
                              fault=dict(phase='before_read', ordinal=physical_ordinal))
                prefixes.append(prefix)
                ordinal += 1
            for ordinal, event in enumerate(observed['writes']):
                for phase in ('before_write', 'after_write'):
                    if phase=='before_write' and any(operand['read'] for operand in operand_sites[event['instruction']]['operands']):
                        continue
                    prefix = copy.deepcopy(row)
                    prefix.update(id=len(rows)+len(prefixes), name=row['name']+f'_{phase}_{ordinal}',
                                  phase=phase, native_fault_ordinal=ordinal,
                                  fault=dict(phase=phase, ordinal=ordinal))
                    prefixes.append(prefix)
        prefix_scratch = scratch/'prefixes'
        prefix_scratch.mkdir()
        prefix_physical, prefix_authority = run_original(image, prefixes, prefix_scratch,
            entries=(0x30aa70,), regions=REGIONS,
            trace_calls=[dict(entry=pc, gp=True, xmm=True) for pc in io],
            trace_extended=True, preserve_x87_prefix=not args.scale_programs)
        normal_count=len(rows)
        rows += prefixes
        physical['cases'] += prefix_physical['cases']
        executed = set()
        for row, observed in zip(rows, physical['cases'], strict=True):
            if not args.scale_programs or 'phase' not in row:assert observed['live_x87_unchanged']
            assert observed['completed'] == ('phase' not in row)
            executed.update(observed['executed_sites'])
            row.update({key: observed[key] for key in
                        ('boundary_gp', 'boundary_eflags', 'boundary_xmm', 'after_fp', 'writes')})
            row['reads'] = [event for event in observed['reads']
                            if event['instruction'] not in (0x30aaa0, 0x30aaa7)]
            row['calls'] = observed['calls']
            if args.scale_programs:row['boundary_live_x87']=observed['boundary_live_x87']
            row['expected_pc'] = observed['boundary']['instruction'] if 'phase' in row else 0x30b6b5
            if row.get('phase')=='alignment_fault':
                instruction=code.at(row['expected_pc'])
                assert instruction.mnemonic=='movaps'
                operands=operand_sites[row['expected_pc']]['operands']
                assert len(operands)==1 and operands[0]['bytes']==16 and operands[0]['read']
                operand=operands[0]
                row['alignment_fault_address']=(observed['boundary_gp'][operand['base']]+operand['displacement'])&0xffffffff
                row['execution_eflags_before_fault']=observed['calls'][-1]['eflags']
            for packet in row['calls']:
                packet.pop('arguments')
            after = {bank['address']: bank['after'] for bank in observed['banks']}
            for bank in row['banks']:
                bank['after'] = after[bank['address']]
        fixture = scratch/'fixture.json'
        fixture.write_text(json.dumps(dict(image_sha256=PIN, io_sites=io, cases=rows,scale_programs=args.scale_programs)))
        project = scratch/'project'
        project.mkdir()
        (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(ray_caller LANGUAGES CXX)
include(CTest)
add_executable(ray_caller "'''+str(ROOT/'tests/native_tu15_mopp_ray_caller_frame_tests.cpp')+'''")
target_include_directories(ray_caller PRIVATE "'''+str(ROOT/'include')+'" "'+str(ROOT/'src')+'''")
target_compile_features(ray_caller PRIVATE cxx_std_20)
target_compile_options(ray_caller PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME ray_caller COMMAND ray_caller "'''+str(fixture)+'''")
''')
        if args.sanitizers:
            with (project/'CMakeLists.txt').open('a') as stream:
                stream.write('target_compile_options(ray_caller PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\n'
                             'target_link_options(ray_caller PRIVATE -fsanitize=address,undefined)\n')
        results = []
        for compiler in ('g++', 'clang++'):
            command = [sys.executable, '-B', str(ROOT/'tools/run_tests.py'), '--source', str(project),
                       '--scratch-root', str(scratch/'builds'), '--output', str(args.output/compiler),
                       '--target', 'ray_caller', '--filter', '^ray_caller$',
                       '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            results.append(dict(compiler=compiler, exit_code=result.returncode))
            if result.returncode:
                print(result.stdout[-5000:], file=sys.stderr)
                raise AssertionError('Native ray caller-frame build/comparison failed; see retained CMake logs')
        controls=negative_controls(scratch,args.output,rows,io) if args.negative_controls else None
        report.update(passed=True, sanitizers=args.sanitizers, original_cases=len(rows), entry_cases=normal_count,
                      normal_cases=sum('phase' not in row for row in rows),
                      actual_alignment_faults=sum(row.get('phase')=='alignment_fault' for row in rows),
                      read_prefixes=sum(row.get('phase')=='before_read' for row in rows),
                      provider_store_prefixes=sum(row.get('phase')=='before_write' for row in rows),
                      completed_write_observer_prefixes=sum(row.get('phase')=='after_write' for row in rows),
                      native_comparisons_per_compiler=sum(1 if 'phase' in row else 2 for row in rows)*(2 if args.scale_programs else 1), executed_sites=sorted(executed),
                      fp_provider_modes=['ordinary','clobbered x87/CW/MXCSR including unmasked pending provider exceptions'] if args.scale_programs else ['ordinary'],
                      memory_contract='Committed ordinary banks with one16-byte vector operand; atomic provider writes and read-only observers. Device/four-scalar vector and physical page/MMIO fault policies remain separate.',
                      fault_flags_contract='Execution EFLAGS remain unchanged; native fault record carries the RF-set saved exception image, compared without masking to physical SIGSEGV context.',
                      complete_pre_io_packets=sum(len(row['calls']) for row in rows),
                      authority=authority, prefix_authority=prefix_authority, compiler_results=results,
                      negative_controls=controls,
                      scope='Private caller/integer and optional shared scale DATA controller, complete normal/prefix ABI and atomic-provider contract; no complete geometric or default parent claim')
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['source_after'] = hashes()
        if report['source_before'] != report['source_after']:
            report['passed'] = False
            report['source_drift'] = True
        (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
