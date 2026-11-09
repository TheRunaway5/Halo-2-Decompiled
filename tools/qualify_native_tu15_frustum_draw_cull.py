#!/usr/bin/env python3
"""Freeze the frustum model cull and compare it using disposable CMake builds."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
NAMES=['tests/native_tu15_frustum_draw_cull_tests.cpp','cmake/NativeTu15FrustumDrawCull.cmake',
       'src/rendering/native_tu15_frustum_draw_cull.cpp','src/rendering/world/source_bsp_ranges.cpp',
       'src/rendering/world/source_bsp_accelerator.cpp','src/rendering/world/source_portal_visibility.cpp',
       'src/rendering/world/source_visibility.cpp','src/core/transform.cpp','src/assets/cache.cpp',
       'tools/native_tu15_frustum_draw_cull_cases.py','tools/validate_native_tu15_frustum_draw_cull.py',
       'tools/native_tu15_zanzibar_frustum_inputs.py','tools/oracle_effect_light_snapshot.py',
       'tools/qualify_native_tu15_frustum_draw_cull.py','tools/oracle_native_tu15_model_mask_order.py',
       'tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/oracle_native_campaign_screen_lifecycle_tu15.py',
       'tools/reference/halo2-decompiled/tools/xbe.py','tools/reference/halo2-decompiled/tools/functions.py',
       'tools/inspect_multiplayer.py','tools/oracle_smg_firing_1_1.py','tools/research_mp_sandbox_update_functions.py',
       'tools/generate_physics_live_support_tu15.py','tools/validate_native_geometry_tu15.py']


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--gcc-only',action='store_true');parser.add_argument('--mutations',action='store_true')
    parser.add_argument('--genuine-zanzibar',action='store_true');a=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    shutil.copytree(ROOT/'include',source/'include')
    if a.genuine_zanzibar:(source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
    for name in NAMES:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeFrustumDrawCull LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15FrustumDrawCull.cmake)
''')
    dependencies=set(NAMES)
    for compiler in ['g++']+([]if a.gcc_only else['clang++']):
        for name in NAMES:
            if not name.startswith(('tests/','src/'))or not name.endswith('.cpp'):continue
            command=[compiler,'-std=c++20','-MM','-I'+str(source/'include'),str(source/name)]
            result=subprocess.run(command,capture_output=True,text=True,check=True)
            for token in shlex.split(result.stdout.replace('\\\n',' ').split(':',1)[1]):
                path=Path(token).resolve()
                if path.is_relative_to(source):dependencies.add(path.relative_to(source).as_posix())
    pins={name:sha(source/name)for name in sorted(dependencies)};results=[]
    def run(kind,compiler='g++',sanitizers=False):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_frustum_draw_cull_tests','--filter','^original_tu15_frustum_draw_cull$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-frustum-draw-cull.json',
            '--retain-report','Testing/Temporary/LastTest.log']
        if sanitizers:command.append('--sanitizers')
        if a.genuine_zanzibar:command.append('--cmake-arg=-DHALO2_FRUSTUM_GENUINE_ZANZIBAR=ON')
        result=subprocess.run(command);receipt=json.loads((a.output/kind/'report.json').read_text())
        return dict(kind=kind,command=command,exit_code=result.returncode,steps={s['name']:s['exit_code']for s in receipt['steps']})
    for kind,compiler,sanitizers in [('gcc','g++',False)]+([]if a.gcc_only else[('clang-sanitizers','clang++',True)]):
        record=run(kind,compiler,sanitizers);results.append(record)
        if record['exit_code']:break
    mutations=[]
    if a.mutations and all(not r['exit_code']for r in results):
        path=source/'src/rendering/native_tu15_frustum_draw_cull.cpp';original=path.read_text()
        variants=[
            ('ffff-sentinel','if(index>=geometry.bounds.size())','if(index==0xffffu){unit.words[0]|=1u;word|=bit;continue;}\n        if(index>=geometry.bounds.size())'),
            ('entry-flag-retest','const auto call=[&](unsigned entry) {','const auto call=[&](unsigned entry) {if(unit.words[0]&1u)return;'),
            ('point-sum-order','((transform.basis[2].x*point.z+transform.basis[1].x*point.y)+transform.basis[0].x*point.x)+transform.position.x',
             '((transform.basis[0].x*point.x+transform.basis[1].x*point.y)+transform.basis[2].x*point.z)+transform.position.x'),
            ('signed-count','if(geometry.subpart_count<=0)return;','if(geometry.subpart_count==0)return;'),
            ('truncated-call-word','context.mode?context.main_prefix+(i-first):i','i'),
            ('lost-matrix-count-guard','if(pose.nodes.size()>255u)\n        throw FormatError("Source linear frustum node count exceeds its matrix bank");',''),
            ('late-node-composition','std::vector<Transform> nodes;nodes.reserve(pose.nodes.size());','if(geometry.subpart_count<=0)return;\n    std::vector<Transform> nodes;nodes.reserve(pose.nodes.size());')]
        for label,before,after in variants:
            if original.count(before)!=1:raise ValueError('Frustum mutation site changed: '+label)
            try:
                path.write_text(original.replace(before,after));record=run('mutant-'+label)
                record['killed']=record['steps'].get('configure')==0 and record['steps'].get('build')==0 and record['steps'].get('tests',0)!=0
                mutations.append(record)
            finally:path.write_text(original)
    unchanged=all(sha(ROOT/n)==h for n,h in pins.items())
    passed=all(not r['exit_code']for r in results)and len(results)==(1 if a.gcc_only else 2)and unchanged and(not a.mutations or len(mutations)==7 and all(m['killed']for m in mutations))
    report=dict(passed=passed,results=results,mutations=mutations,source_sha256=pins,source_unchanged=unchanged,owned_scratch=str(scratch),retained_large_artifacts=[],
        scope='Typed native frustum cull compared with whole physical original model/walk outputs; live World scheduling and rendered large-sphere acceptance remain separate.')
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
