#!/usr/bin/env python3
"""Freeze renderer draw-unit bank kernel owners and validate with disposable CMake builds."""
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
    names=['tests/native_tu15_draw_units_tests.cpp','tools/validate_native_tu15_draw_units.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp','cmake/NativeTu15DrawUnits.cmake',
        'src/rendering/world/native_tu15_draw_units.cpp','include/halo2/native_tu15_draw_units.hpp',
        'src/rendering/world/native_tu15_draw_collect.cpp','include/halo2/detail/native_tu15_arithmetic_environment.hpp',
        'src/rendering/world/native_tu15_sphere_rectangle.cpp','include/halo2/native_tu15_sphere_rectangle.hpp',
        'tools/native_tu15_sphere_rectangle_cases.py',
        'src/rendering/models/native_tu15_light_parent_biped.cpp','include/halo2/native_tu15_light_parent_biped.hpp',
        'tools/native_tu15_light_parent_cases.py',
        'src/rendering/world/native_tu15_render_lighting.cpp','include/halo2/native_tu15_render_lighting.hpp',
        'tools/validate_native_tu15_render_lighting.py',
        'src/rendering/world/native_tu15_render_prepare.cpp','include/halo2/native_tu15_render_prepare.hpp',
        'src/rendering/source_geometry_resource_request.cpp','src/rendering/source_geometry_resource.cpp',
        'include/halo2/source_geometry_resource_request.hpp','include/halo2/source_geometry_resource.hpp',
        'tools/oracle_native_tu15_model_mask_order.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py',
        'tools/reference/halo2-decompiled/tools/xbe.py',
        'tools/reference/halo2-decompiled/tools/functions.py','tools/inspect_multiplayer.py',
        'tools/oracle_smg_firing_1_1.py','tools/research_mp_sandbox_update_functions.py',
        'tools/generate_physics_live_support_tu15.py','tools/validate_native_geometry_tu15.py',
        'tools/qualify_native_tu15_draw_units.py']
    pins={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names}
    for name in names:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeDrawUnits LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_DRAW_UNITS_ORACLE_SCRIPT "{source}/tools/validate_native_tu15_draw_units.py")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15DrawUnits.cmake)
''')
    results=[]
    for kind,compiler,sanitize in [('gcc','g++',False),('clang-sanitizers','clang++',True)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_draw_units_tests','--filter','^original_tu15_native_draw_units$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-draw-units.json']
        if sanitize:command+=['--sanitizers']
        run=subprocess.run(command)
        evidence=a.output/kind/'evidence/native-tu15-draw-units.json'
        physical=json.loads(evidence.read_text())if evidence.is_file()else {}
        original_passed=physical.get('passed')is True and physical.get('physical_cases',0)>0
        results.append(dict(kind=kind,returncode=run.returncode,command=command,original_physical_evidence_present=original_passed))
        if run.returncode or not original_passed:break
    mutants=[];expected_mutants=0
    if a.mutations and len(results)==2 and all(r['returncode']==0 and r['original_physical_evidence_present']for r in results):
        append='src/rendering/world/native_tu15_draw_units.cpp';collect='src/rendering/world/native_tu15_draw_collect.cpp'
        rectangle='src/rendering/world/native_tu15_sphere_rectangle.cpp';parent='src/rendering/models/native_tu15_light_parent_biped.cpp'
        variants=[
            ('dropped-floor-read',append,'return std::uint32_t(native_tu15_floor_16ac0(m,scaled,fp));','return std::uint32_t(_mm_cvttss_si32(scalar(scaled)));'),
            ('cleared-reused-mask',append,'store(0x2cc0cu,unit+4u,a[1]);','put(m,unit+0x14u,0);store(0x2cc0cu,unit+4u,a[1]);'),
            ('erased-refused-slot',append,'if(!geometry)return refused(false);','if(!geometry){put(m,unit,0);return refused(false);}'),
            ('arena-full-admitted',append,'if(signed_word(next)>=512)','if(signed_word(next)>512)'),
            ('pinned-instance-table',collect,'const auto flags=read(m,queue+8u),instances=read(m,bsp+0x144u),input_flags=',
                'static const auto instances=read(m,bsp+0x144u);const auto flags=read(m,queue+8u),input_flags='),
            ('lost-alpha-x87-status',collect,'opacity=alpha(bits(fade),read(m,0x46a148u));',
                'const auto saved_status=fp.status;opacity=alpha(bits(fade),read(m,0x46a148u));asm volatile("fnclex");(void)saved_status;'),
            ('skipped-zero-alpha-write',collect,'byte(m,skipped,read(m,skipped,1)|1u);','(void)skipped;'),
            ('quiet-rectangle-compare',rectangle,'asm volatile("comiss %2,%1; seta %0"','asm volatile("ucomiss %2,%1; seta %0"'),
            ('rectangle-sse-root',rectangle,
                'const auto input=bits(v);std::uint32_t output{};\n    asm volatile("flds %1; fsqrt; fstps %0":"=m"(output):"m"(input):"memory","st");return scalar(output);',
                'asm volatile("sqrtss %0,%0":"+x"(v)::"memory");return v;'),
            ('rectangle-nan-operand-order',rectangle,'u=mul(u,p00);u=add(u,p20);one=','u=mul(u,p00);u=add(p20,u);one='),
            ('unsigned-rectangle-viewport',rectangle,'return std::bit_cast<std::int16_t>(std::uint16_t(v));','return std::uint16_t(v);'),
            ('observer-held-fp-environment',rectangle,'environment.reset();observe(event);environment.emplace(fp);','observe(event);'),
            ('root-instead-of-biped',parent,'if(!read(m,body+0xaau,1))return out;','(void)read(m,body+0xaau,1);'),
            ('unsigned-queue-capacity',parent,'if(std::int32_t(count)>=std::bit_cast<std::int32_t>(capacity-1u))','if(count>=capacity-1u)'),
            ('pinned-queue-count',parent,'count=read(m,queue+4u,2);stream=read(m,queue+8u);','stream=read(m,queue+8u);'),
            ('lost-argument12-store',append,'services.argument_write(0x2cddbu,12,scaled);',''),
            ('lost-repeat-argument6-store',append,'if(services.argument_write)services.argument_write(0x2ce13u,6,scaled);',''),
            ('wrong-mask-store-pc',append,'Stores(m,observe)(0x135df5u,','Stores(m,observe)(0x135df4u,'),
            ('dropped-draw-observer',append,'memory_.write(at,event.written);observer_(event);','memory_.write(at,event.written);'),
            ('lost-return-high-word',append,'state.return_word=(mask&0xffff0000u)|(ordinal&65535u);','state.return_word=ordinal&65535u;'),
            ('wrong-resource-clock-pc','src/rendering/source_geometry_resource_request.cpp','put(m,0x1309a8u,','put(m,0x1309a9u,')]
        expected_mutants=len(variants)
        for label,name,before,after in variants:
            file=source/name;original=file.read_text();assert original.count(before)==1;file.write_text(original.replace(before,after))
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'mutants'),
                '--output',str(a.output/('mutant-'+label)),'--target','native_tu15_draw_units_tests','--filter','^original_tu15_native_draw_units$',
                '--jobs','2','--test-jobs','1']
            run=subprocess.run(command);receipt=json.loads((a.output/('mutant-'+label)/'report.json').read_text())
            steps={s['name']:s['exit_code']for s in receipt['steps']};killed=run.returncode!=0 and steps.get('configure')==0 and steps.get('build')==0 and steps.get('tests',0)!=0
            mutants.append(dict(name=label,killed=killed,command=command));file.write_text(original)
    unchanged=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==pin for n,pin in pins.items())
    passed=len(results)==2 and all(r['returncode']==0 and r['original_physical_evidence_present'] for r in results)and unchanged and(not a.mutations or len(mutants)==expected_mutants and all(m['killed']for m in mutants))
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,mutations=mutants,source_sha256=pins,source_unchanged=unchanged,
        owned_scratch=str(scratch),retained_large_artifacts=[]),indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
