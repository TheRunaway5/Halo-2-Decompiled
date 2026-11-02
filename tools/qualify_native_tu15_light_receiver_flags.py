#!/usr/bin/env python3
"""Freeze31650 kernels and qualify them using disposable official CMake builds."""
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
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mutations',action='store_true');parser.add_argument('--gcc-only',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'include',source/'include')
    names=['tests/native_tu15_light_receiver_flags_tests.cpp','tools/validate_native_tu15_light_receiver_flags.py',
        'tools/native_tu15_light_receiver_flag_cases.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp',
        'cmake/NativeTu15LightReceiverFlags.cmake','src/rendering/effects/native_tu15_light_receiver_flags.cpp',
        'src/rendering/effects/effect_light_receiver_flags.cpp','include/halo2/native_tu15_light_receiver_flags.hpp',
        'include/halo2/effect_light_receiver.hpp','tools/oracle_native_tu15_model_mask_order.py',
        'tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/reference/halo2-decompiled/tools/xbe.py',
        'tools/reference/halo2-decompiled/tools/functions.py','tools/inspect_multiplayer.py','tools/oracle_smg_firing_1_1.py',
        'tools/research_mp_sandbox_update_functions.py','tools/generate_physics_live_support_tu15.py',
        'tools/validate_native_geometry_tu15.py','tools/qualify_native_tu15_light_receiver_flags.py']
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();pins={name:sha(ROOT/name)for name in names}
    for name in names:
        destination=source/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,destination)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeLightReceiverFlags LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15LightReceiverFlags.cmake)
''')
    def run(kind,compiler='g++',sanitizers=False):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(args.output/kind),'--target','native_tu15_light_receiver_flags_tests','--filter','^original_tu15_light_receiver_flags$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-light-receiver-flags.json']
        if sanitizers:command.append('--sanitizers')
        result=subprocess.run(command);receipt=json.loads((args.output/kind/'report.json').read_text())
        return dict(kind=kind,command=command,exit_code=result.returncode,steps={s['name']:s['exit_code']for s in receipt['steps']})
    results=[run('gcc')]
    if not args.gcc_only and not results[0]['exit_code']:results.append(run('clang-sanitizers','clang++',True))
    mutants=[]
    if args.mutations and all(not r['exit_code']for r in results):
        native='src/rendering/effects/native_tu15_light_receiver_flags.cpp';typed='src/rendering/effects/effect_light_receiver_flags.cpp'
        variants=[
            ('unsigned-count',native,'return std::bit_cast<std::int16_t>(std::uint16_t(read(m,queue+4u,2)));','return read(m,queue+4u,2);'),
            ('owner-none-skipped',native,'if(selected==0xffffffffu || read(m,read(m,queue+0xcu)+row_offset(index,4))==selected)','if(selected!=0xffffffffu && read(m,read(m,queue+0xcu)+row_offset(index,4))==selected)'),
            ('root-pass-lost',native,'if(root!=owner){++out.root_passes;edit(root,true);}','(void)root;'),
            ('early-definition-snapshot',native,'const auto flags=read(m,definition);','const auto flags=environment?0x13800u:read(m,definition);'),
            ('root-not-reread',native,'if(root)selected=read(m,in.stack+8u);',''),
            ('lost-stack-index',native,'store(root?0x31837u:0x317c6u,in.stack+4u,index,4);',''),
            ('single-cluster-f8',native,'|| cluster_count<=1)return;','|| cluster_count<1)return;'),
            ('constant-f8-substitution',native,'out.xmm0_word=read(m,0x46a0d0u);','out.xmm0_word=0x3f800000u;'),
            ('wrong-observer-pc',native,'store(0x31867u,record+0xf8u,','store(0x31866u,record+0xf8u,'),
            ('typed-root-none-skipped',typed,'if(owner==0xffffffffu || row.id==owner)','if(owner!=0xffffffffu && row.id==owner)'),
            ('typed-clear-both-lost',typed,'(row.flags&~0x3000u)|bits','row.flags|bits')]
        for label,name,before,after in variants:
            file=source/name;original=file.read_text();assert original.count(before)==1,(label,original.count(before))
            try:
                file.write_text(original.replace(before,after));record=run('mutant-'+label)
                record['killed']=record['steps'].get('configure')==0 and record['steps'].get('build')==0 and record['steps'].get('tests',0)!=0
                mutants.append(record)
            finally:file.write_text(original)
    unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items())
    passed=all(not r['exit_code']for r in results)and(len(results)==2 or args.gcc_only)and unchanged and(not args.mutations or len(mutants)==len(variants)and all(m['killed']for m in mutants))
    report=dict(passed=passed,gcc_only=args.gcc_only,results=results,mutations=mutants,source_sha256=pins,source_unchanged=unchanged,owned_scratch=str(scratch),retained_large_artifacts=[])
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
