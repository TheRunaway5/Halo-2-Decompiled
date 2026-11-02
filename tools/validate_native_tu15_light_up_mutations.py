#!/usr/bin/env python3
"""Owned precision/branch/NaN mutation controls for the native light-up kernel."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import validate_native_tu15_light_up_vector as reference

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=reference.PIN:raise ValueError('Wrong original executable')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    rows=list(reference.cases());gold=scratch/'golden';gold.mkdir()
    proof,receipt=reference.physical(Xbe(args.xbe),rows,gold);expected=[]
    for original in proof['cases']:
        bank=next(b for b in original['banks'] if b['address']==reference.DATA)
        expected.append(dict(up=list(struct.unpack_from('<3I',bytes.fromhex(bank['after']),reference.UP-reference.DATA)),
            magnitude=original['result_bits'],fp=original['after_fp'],
            stores=[[s['instruction'],(s['address']-reference.UP)//4,struct.unpack('<I',bytes.fromhex(s['written']))[0]]
                    for s in original['writes'] if reference.UP<=s['address']<reference.UP+12],
            constant_reads=[a['address'] for a in original['reads'] if a['address'] in (0x46a0d0,0x46a0ec)],
            host_fp_and_stack_restored=True))
    fixture=scratch/'physical-expected.json';fixture.write_text(json.dumps(dict(rows=rows,expected=expected),separators=(',',':')))
    project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    shutil.copyfile(ROOT/'tests/native_tu15_light_up_vector_tests.cpp',project/'probe.cpp')
    core=(ROOT/'src/rendering/effects/native_tu15_light_up_vector.cpp').read_text()
    tail=(ROOT/'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp').read_text()
    variants={'positive':(core,tail)}
    def edit(text,needle,replacement):
        if text.count(needle)!=1:raise ValueError('Mutation source changed: '+needle)
        return text.replace(needle,replacement)
    variants['rounded_cz']=(edit(core,r'fmuls %0\n\tflds %7',r'fstp %%st(0)\n\tflds %0\n\tfmuls %0\n\tflds %7'),tail)
    variants['inclusive_epsilon']=(edit(core,'seta %2','setae %2'),tail)
    variants['nan_skip']=(edit(core,'if(!too_small) {','if(!too_small && (magnitude&0x7fffffffu)<=0x7f800000u) {'),tail)
    variants['commute_multiply']=(edit(core,'Scalar multiply(Scalar a,Scalar b){asm volatile',
        'Scalar multiply(Scalar a,Scalar b){const auto exchange=a;a=b;b=exchange;asm volatile'),tail)
    variants['reverse_tail_sum']=(core,edit(tail,'load(input(2));load(input(1));load(input(0));','load(input(0));load(input(1));load(input(2));'))
    variants['compare_spill']=(edit(core,r'\n\tfsqrt\n\tfsts %1\n\tfabs',r'\n\tfsqrt\n\tfstps %1\n\tflds %1\n\tfabs'),tail)
    narrow=edit(core,'detail::NativeTu15ArithmeticEnvironment environment(fp);',
        'detail::NativeTu15ArithmeticEnvironment environment(fp);const std::uint16_t intended=fp.control_word,narrow=intended&~0x300u;asm volatile("fldcw %0"::"m"(narrow):"memory");')
    narrow=edit(narrow,'        up={bits(x),bits(y),bits(z)};',
        '        up={bits(x),bits(y),bits(z)};asm volatile("fldcw %0"::"m"(intended):"memory");')
    variants['single_precision_cross']=(narrow,tail)
    start=core.index('        // Preserve the unspilled')
    end=core.index('        auto nx=',start)
    double_phase='''        std::array<double,3> a{},b{};
        for(unsigned n=0;n<3;++n){a[n]=std::bit_cast<float>(up[n]);b[n]=std::bit_cast<float>(forward[n]);}
        const auto cx=a[1]*b[2]-b[1]*a[2],cy=b[0]*a[2]-a[0]*b[2],cz=b[1]*a[0]-a[1]*b[0];
        cross={std::bit_cast<std::uint32_t>(float(cx)),std::bit_cast<std::uint32_t>(float(cy)),std::bit_cast<std::uint32_t>(float(cz))};
        const auto length=std::sqrt((cz*double(std::bit_cast<float>(cross[2]))+double(std::bit_cast<float>(cross[1]))*std::bit_cast<float>(cross[1]))+double(std::bit_cast<float>(cross[0]))*std::bit_cast<float>(cross[0]));
        magnitude=std::bit_cast<std::uint32_t>(float(length));
        const bool too_small=std::bit_cast<float>(epsilon)>std::fabs(length);
'''
    variants['double_sse_cross']=((core[:start]+double_phase+core[end:]).replace('#include <bit>','#include <bit>\n#include <cmath>'),tail)
    (project/'check.py').write_text(r'''import json,subprocess,sys
from pathlib import Path
j=json.loads(Path(sys.argv[1]).read_text())
p=subprocess.run([sys.argv[2]],input=''.join(json.dumps(r)+'\n' for r in j['rows']),text=True,capture_output=True)
assert p.returncode==0,(p.returncode,p.stderr[-3000:])
actual=[json.loads(x) for x in p.stdout.splitlines()]
assert len(actual)==len(j['expected'])
failures=[(r['name'],key) for r,a,e in zip(j['rows'],actual,j['expected']) for key in e if a.get(key)!=e[key]]
if sys.argv[3]=='positive':assert not failures,failures[:3]
else:assert failures,'Mutation escaped complete physical original qualification'
print('Physical original control',sys.argv[3],len(actual),'cases;',len(failures),'differences;',failures[:2])
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(RendererLightVectorMutations LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\nfind_package(Python3 COMPONENTS Interpreter REQUIRED)\n'
    for name,(a,b) in variants.items():
        (project/(name+'.cpp')).write_text(a);(project/(name+'-tail.cpp')).write_text(b)
        cmake+=f'''add_executable(light_{name} probe.cpp {name}.cpp {name}-tail.cpp)
target_include_directories(light_{name} PRIVATE include)
target_compile_features(light_{name} PRIVATE cxx_std_20)
target_compile_options(light_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(light_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME light_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/check.py" "{fixture}" "$<TARGET_FILE:light_{name}>" "{name}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
        '--output',str(args.output/'cmake'),'--filter','^light_','--jobs','2','--test-jobs','2','--retain-report','Testing/Temporary/LastTest.log']
    for name in variants:command+=['--target','light_'+name]
    result=subprocess.run(command)
    report=dict(passed=result.returncode==0,command=command,scratch=str(scratch),physical_original=receipt,
        variants=list(variants),retained_large_artifacts=[])
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    return result.returncode


if __name__=='__main__':raise SystemExit(main())
