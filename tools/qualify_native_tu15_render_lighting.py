#!/usr/bin/env python3
"""Freeze renderer render lighting/section/object kernel owners and validate with disposable CMake builds."""
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
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--mutations',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();a.output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'include',source/'include')
    names=['tests/native_tu15_render_lighting_tests.cpp','tools/validate_native_tu15_render_lighting.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp','cmake/NativeTu15RenderLighting.cmake',
        'src/rendering/world/native_tu15_render_lighting.cpp','include/halo2/native_tu15_render_lighting.hpp',
        'src/rendering/world/native_tu15_render_prepare.cpp','include/halo2/native_tu15_render_prepare.hpp',
        'src/rendering/source_geometry_resource_request.cpp','src/rendering/source_geometry_resource.cpp',
        'include/halo2/source_geometry_resource_request.hpp','include/halo2/source_geometry_resource.hpp',
        'tools/oracle_native_tu15_model_mask_order.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py',
        'tools/qualify_native_tu15_render_lighting.py']
    pins={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names}
    for name in names:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeRenderLighting LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_RENDER_LIGHTING_ORACLE_SCRIPT "{ROOT}/tools/validate_native_tu15_render_lighting.py")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15RenderLighting.cmake)
''')
    results=[]
    for kind,compiler,sanitize in [('gcc','g++',False),('clang-sanitizers','clang++',True)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_render_lighting_tests','--filter','^(native_tu15_render_lighting_controls|original_tu15_native_render_lighting)$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-render-lighting.json']
        if sanitize:command+=['--sanitizers']
        run=subprocess.run(command)
        evidence=a.output/kind/'evidence/native-tu15-render-lighting.json'
        physical=json.loads(evidence.read_text())if evidence.is_file()else {}
        original_passed=physical.get('passed')is True and physical.get('physical_cases',0)>0
        results.append(dict(kind=kind,returncode=run.returncode,command=command,original_physical_evidence_present=original_passed))
        if run.returncode or not original_passed:break
    mutants=[]
    if a.mutations and len(results)==2 and all(r['returncode']==0 and r['original_physical_evidence_present'] for r in results):
        lighting='src/rendering/world/native_tu15_render_lighting.cpp'
        prepare='src/rendering/world/native_tu15_render_prepare.cpp'
        variants=[
            ('pinned-bsp',lighting,'const auto current_bsp=read(m,0x4ecc18u),instances=read(m,current_bsp+0x144u);','const auto current_bsp=bsp,instances=read(m,current_bsp+0x144u);'),
            ('model-read-order',lighting,'const auto tags=read(m,0x4f0414u),definition=read(m,tags+16u*(model&65535u)+8u);\n        const auto checksums=read(m,group+0x5cu),model_checksum=read(m,definition+8u);','const auto checksums=read(m,group+0x5cu);\n        const auto tags=read(m,0x4f0414u),definition=read(m,tags+16u*(model&65535u)+8u),model_checksum=read(m,definition+8u);'),
            ('model-checksum-fallback',lighting,'if(model_checksum!=read(m,checksums+12u*instance+8u))return 3u;','if(model_checksum!=read(m,checksums+12u*instance+8u))return fallback();'),
            ('request-fallback',lighting,'if(!services.request(header,fp))return 3u;','if(!services.request(header,fp))return fallback();'),
            ('bucket-mask',lighting,'const auto bucket=read(m,references+12u*instance+2u,2);','const auto bucket=read(m,references+12u*instance+2u,2)&32767u;'),
            ('object-read-order',lighting,'const auto stride=read(m,pool+0x24u),rows=read(m,pool+0x44u),header=rows+index*stride;','const auto rows=read(m,pool+0x44u),stride=read(m,pool+0x24u),header=rows+index*stride;'),
            ('section-read-order',prepare,'const auto definition=std::uint32_t(signed_half(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)));\n    const auto definitions=read(m,bsp+0x13cu);','const auto definitions=read(m,bsp+0x13cu);\n    const auto definition=std::uint32_t(signed_half(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)));')]
        for label,name,before,after in variants:
            kernel=source/name;original=kernel.read_text();assert original.count(before)==1;kernel.write_text(original.replace(before,after))
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'mutants'),
                '--output',str(a.output/('mutant-'+label)),'--target','native_tu15_render_lighting_tests',
                '--filter','^(native_tu15_render_lighting_controls|original_tu15_native_render_lighting)$','--jobs','2','--test-jobs','1']
            run=subprocess.run(command);receipt=json.loads((a.output/('mutant-'+label)/'report.json').read_text())
            steps={s['name']:s['exit_code']for s in receipt['steps']}
            killed=run.returncode!=0 and steps.get('configure')==0 and steps.get('build')==0 and steps.get('tests',0)!=0
            mutants.append(dict(name=label,returncode=run.returncode,killed=killed,command=command));kernel.write_text(original)
    unchanged=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==pin for n,pin in pins.items())
    passed=len(results)==2 and all(r['returncode']==0 and r['original_physical_evidence_present'] for r in results)and unchanged and (not a.mutations or len(mutants)==7 and all(m['killed']for m in mutants))
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,mutations=mutants,source_sha256=pins,source_unchanged=unchanged,
        owned_scratch=str(scratch),retained_large_artifacts=[]),indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
