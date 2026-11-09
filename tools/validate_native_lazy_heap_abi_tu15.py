#!/usr/bin/env python3
"""Qualify lazy-heap ABI prefixes with unchanged original hardware clock inputs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if (seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
import validate_native_array_growth_tu15 as reference
import native_lazy_heap_abi_inputs as inputs
from specialize_native_operands import digest,leaf_body

BODY_PIN='f01d35461465d3460d347dfcb6a5cc3c3af7a438957fcbecf99856763183192a'


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=False,scratch_directory=str(scratch),retained_large_artifacts=[]),indent=2)+'\n')
    source=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';canonical=leaf_body(source.read_text(),'149930')
    assert digest(canonical[0].replace(canonical[1]+' &s','STATE &s'))==BODY_PIN
    body=canonical[0];start=body.index('{\n')+2
    body=body[:start]+('    if(native_lazy_heap_detail::run(s,[](auto &v,std::uint32_t owner,std::uint32_t site,bool tail){dispatch(v,owner,site,tail);}))return;\n'
        '    ++translated_fallbacks;\n')+body[start:]
    (scratch/'lazy_heap_body.inc').write_text(body+'\n')
    assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT)
    banks,provenance=inputs.load(reference,image);rows=inputs.cases(reference);reference.OWNERS=(0x149930,)
    directory,authority=reference.original(image,banks,rows,scratch)
    fixture=directory/'lazy-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
    binary=scratch/'lazy-heap-abi';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_lazy_heap_abi_tests.cpp'),
        str(ROOT/'src/physics/native_tu15_lazy_heap.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True);modes={}
    for mode in ('native','reference'):
        env=dict(os.environ)
        for key in ('HALO2_REFERENCE_LAZY_HEAP','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
        if mode=='reference':env['HALO2_REFERENCE_LAZY_HEAP']='1'
        result=subprocess.run([str(binary),str(fixture)],env=env,text=True,capture_output=True)
        if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
        modes[mode]=json.loads(result.stdout)
    files=[source,ROOT/'include/halo2/native_tu15_lazy_heap.hpp',ROOT/'src/physics/native_tu15_lazy_heap.cpp',
        ROOT/'src/physics/runtime/native_tu15_lazy_heap_abi.hpp',ROOT/'tests/native_tu15_lazy_heap_abi_tests.cpp',
        Path(__file__),Path(inputs.__file__),Path(reference.__file__),ROOT/'tools/observe_native_array_growth_tu15.cpp']
    executed={pc for row in rows for pc in row['executed_sites']};body_sites=reference.complete_body(reference._Code(image,'.text'),0x149930).code
    report=dict(passed=True,modes=modes,input_provenance=provenance,source_authority=authority,
        original_instructions_unchanged=True,clock_input_replay=True,clock_input_source='read-only observations after unchanged original RDTSC',
        clock_input_checks=['authenticated instruction PC','event order','sample count'],
        independently_computed_clock_values=False,native_outputs_used_by_original=False,allocator_outcomes_supplied=False,
        cohorts={name:sum(row['cohort']==name for row in rows) for name in sorted({row['cohort']for row in rows})},
        actual_required_boundaries={hex(owner):sum(row['required_owner']and row['required_entry']==owner for row in rows)
            for owner in sorted({row['required_entry']for row in rows if row['required_owner']})},
        body_coverage=dict(total=len(body_sites),executed=len(executed&set(body_sites)),unexecuted=sorted(set(body_sites)-executed)),
        genuine_initialized_default_binding_zero=True,declared_initialized_cases_are_genuine_slow_closure=False,
        slow_normal_return_qualified=False,allocation_cleanup_trim_yield_owner_closure=False,
        elapsed_fp_path_qualified=False,exceptional_partial_abi_qualification=False,whole_platform_clock_closure=False,
        address_undefined_sanitizers=args.sanitizers,
        source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()for p in files},
        scratch_directory=str(scratch),retained_large_artifacts=[])
    args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return 0


if __name__=='__main__':sys.exit(main())
