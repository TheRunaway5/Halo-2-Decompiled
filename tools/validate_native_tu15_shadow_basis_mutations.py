#!/usr/bin/env python3
"""Reject plausible shadow arithmetic substitutions with the physical oracle."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();a.output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'include',source/'include')
    kernel=ROOT/'src/rendering/shadows/native_tu15_shadow_basis_32340.cpp';text=kernel.read_text()
    paths=[kernel,Path(__file__),ROOT/'tools/validate_native_tu15_shadow_basis.py',ROOT/'tests/native_tu15_shadow_basis_tests.cpp',
        ROOT/'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp']
    pins={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest()for path in paths}
    variants={'positive':text}
    def edit(needle,replacement):
        assert text.count(needle)==1,(needle,text.count(needle));return text.replace(needle,replacement)
    variants['rounded_epsilon']=text
    math=(ROOT/'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp').read_text()
    # Spill/reload the actual computed magnitude before comparing. No witness
    # or reference output is encoded in this plausible rounding substitution.
    assert math.count('fsts %0\\n\\tfabs')==2
    rounded_math=math.replace('fsts %0\\n\\tfabs','fstps %0\\n\\tflds %0\\n\\tfabs')
    variants['basis_test_uses_y_only']=edit('out.second_basis=second_basis;',
        'out.second_basis=!below(source_abs(y),scalar(threshold));')
    variants['horizontal_uses_x']=edit('source_abs(above(ax,ay)?x:y)','source_abs(x)')
    variants['up_axis_mapping']=edit('out.up[1]=bits(subtract(multiply(bx,z),multiply(bz,x)));',
        'out.up[1]=bits(subtract(multiply(by,x),multiply(bx,y)));')
    variants['width_aspect_assumed']=edit('scalar(projection[23])','scalar(projection[22])')
    variants['unary_minus_half']=edit('subtract(scalar(0),scalar(out.half_extent[0]))','scalar(out.half_extent[0]^0x80000000u)')
    variants['eye_reconstructed']=edit('out.virtual_eye[n]=projection[11+n]','out.virtual_eye[n]=center[n]')
    variants['ambient_fp_controls']=edit('detail::NativeTu15ArithmeticEnvironment environment(fp);\n        const auto x=',
        'const auto x=')
    variants['direction_publication_omitted']=edit('address+4*n,v);','address+4*n,n==1?0u:v);')
    variants['unordered_cached_opacity_admitted']=edit('out.cached_positive=above(scalar(input.cached_opacity),scalar(zero));',
        'out.cached_positive=!(std::bit_cast<float>(input.cached_opacity)<=std::bit_cast<float>(zero));')
    variants['unordered_radius_kept']=edit('above(scalar(input.radius),scalar(minimum))?input.radius:minimum',
        'above(scalar(minimum),scalar(input.radius))?minimum:input.radius')
    shutil.copyfile(ROOT/'tests/native_tu15_shadow_basis_tests.cpp',source/'probe.cpp')
    shutil.copyfile(ROOT/'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp',source/'math.cpp')
    (source/'rounded_math.cpp').write_text(rounded_math)
    (source/'check.py').write_text(f'''import os,subprocess,sys
from pathlib import Path
command=[{str(ROOT/'.tools/python/bin/python')!r},{str(ROOT/'tools/validate_native_tu15_shadow_basis.py')!r},'--xbe',
 '/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe',
 '--native',sys.argv[1],'--report',str(Path(os.environ['HALO2_TEST_SCRATCH'])/'proof.json')]
r=subprocess.run(command,text=True,capture_output=True)
if sys.argv[2]=='positive':
 assert r.returncode==0,(r.stdout+r.stderr)[-4000:]
 print(r.stdout)
else:
 assert r.returncode==1 and 'ValueError:' in r.stderr and (' differs; native='in r.stderr or 'native stopped:'in r.stderr), (r.returncode,(r.stdout+r.stderr)[-4000:])
 print('Detected',sys.argv[2],r.stderr.splitlines()[-1][:1200])
''')
    cmake=f'''cmake_minimum_required(VERSION 3.20)
project(ShadowBasisMutations LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
add_library(math STATIC math.cpp)
add_library(probe OBJECT probe.cpp)
foreach(target IN ITEMS math probe)
 target_include_directories(${{target}} PRIVATE include)
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
'''
    for name,body in variants.items():
        (source/(name+'.cpp')).write_text(body)
        cmake+=f'''add_executable(shadow_{name} {name}.cpp $<TARGET_OBJECTS:probe>)
target_include_directories(shadow_{name} PRIVATE include)
target_compile_features(shadow_{name} PRIVATE cxx_std_20)
target_compile_options(shadow_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(shadow_{name} PRIVATE math nlohmann_json::nlohmann_json)
add_test(NAME shadow_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/check.py" $<TARGET_FILE:shadow_{name}> {name})
set_tests_properties(shadow_{name} PROPERTIES TIMEOUT 180)
'''
        if name=='rounded_epsilon':
            cmake=cmake.replace('target_link_libraries(shadow_rounded_epsilon PRIVATE math nlohmann_json::nlohmann_json)',
                'target_sources(shadow_rounded_epsilon PRIVATE rounded_math.cpp)\ntarget_link_libraries(shadow_rounded_epsilon PRIVATE nlohmann_json::nlohmann_json)')
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
        '--output',str(a.output/'cmake'),'--jobs','2','--test-jobs','1','--filter','^shadow_','--retain-report','Testing/Temporary/LastTest.log']
    for name in variants:command+=['--target','shadow_'+name]
    run=subprocess.run(command)
    unchanged=all(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==pin for name,pin in pins.items())
    (a.output/'validation.json').write_text(json.dumps(dict(passed=run.returncode==0 and unchanged,command=command,mutants=list(variants)[1:],
        source_sha256=pins,source_unchanged=unchanged,variant_sha256={name:hashlib.sha256(body.encode()).hexdigest()for name,body in variants.items()},
        owned_scratch=str(scratch),retained_large_artifacts=[]),indent=2)+'\n')
    return run.returncode


if __name__=='__main__':raise SystemExit(main())
