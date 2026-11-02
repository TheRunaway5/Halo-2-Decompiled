#!/usr/bin/env python3
"""Owned frozen native weapon components, two compilers and wrong-code controls.

This checks component ABI/memory/FP against bounded unchanged-original inputs.
It does not establish rendered effects, network playback or whole-graph closure.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'
NAMES=('dispatch','request','location','iterator','state','creation','initialization','allocation','selector','dispatch_strict','lifecycle_strict','location_strict','datum_allocate','sound_local','location_copy','marker_matrix','marker_parent','marker_mode','marker_parent_composed','marker_lookup','marker_emission_graph')
FIXTURES=(*NAMES,'restart','attached_strict','direction_strict')


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    paths=[UNIT,'src/gameplay/weapons/native_weapon_marker_matrix_tu15_internal.hpp','src/gameplay/weapons/native_weapon_marker_lookup_tu15_internal.hpp','src/gameplay/weapons/native_weapon_marker_math_tu15_internal.hpp','src/gameplay/weapons/native_weapon_marker_emission_tu15_internal.hpp','tests/native_tu15_sound_test_support.hpp','tests/native_weapon_effect_compiled_route.hpp','tools/test_native_weapon_compressed_reference.py',
        'tests/native_weapon_zoom_reset_tu15_tests.cpp','tests/fixtures/weapon_zoom_reset_tu15_original.json',
        'tests/native_weapon_zoom_reset_strict_tu15_tests.cpp','tests/fixtures/weapon_zoom_reset_strict_tu15_original.json',
        'tests/native_weapon_audio_request_strict_tu15_tests.cpp','tests/fixtures/weapon_audio_request_strict_tu15_original.json',
        *('tests/native_weapon_effect_'+name+'_tu15_tests.cpp' for name in NAMES),
        *('tests/fixtures/'+('weapon_sound_local_tu15_original.json.gz' if name=='sound_local' else 'weapon_effect_'+name+'_tu15_original.json'+('.gz' if name in ('datum_allocate','location_copy','marker_matrix','marker_parent','marker_mode','marker_parent_composed','marker_lookup','marker_emission_graph') else '')) for name in FIXTURES)]
    for relative in paths:
        target=project/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/relative,target)
    frozen={str(p.relative_to(project)):sha(p) for p in project.rglob('*') if p.is_file()}
    cmake=['cmake_minimum_required(VERSION 3.20)','project(native_weapon_components LANGUAGES CXX)',
        'include(CTest)','find_package(Python3 COMPONENTS Interpreter REQUIRED)','find_package(nlohmann_json REQUIRED)','find_package(glm CONFIG REQUIRED)',
        f'add_library(native_weapon_component_kernel STATIC {UNIT})',
        'target_include_directories(native_weapon_component_kernel PUBLIC include)',
        'target_compile_features(native_weapon_component_kernel PUBLIC cxx_std_20)',
        'target_compile_options(native_weapon_component_kernel PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)',
        'target_link_libraries(native_weapon_component_kernel PUBLIC glm::glm-header-only)',
        'if(HALO2_SANITIZERS)\n target_compile_options(native_weapon_component_kernel PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\n target_link_options(native_weapon_component_kernel INTERFACE -fsanitize=address,undefined)\nendif()']
    for name in (*NAMES,'zoom_reset','zoom_reset_strict','audio_request_strict'):
        target='native_weapon_'+name+'_tu15_tests' if name.startswith(('zoom_reset','audio_request')) else 'native_weapon_effect_'+name+'_tu15_tests'
        test='native_weapon_'+name+'_tu15' if name.startswith(('zoom_reset','audio_request')) else 'native_weapon_effect_'+name+'_tu15'
        fixture='weapon_'+name+'_tu15_original.json' if name.startswith(('zoom_reset','audio_request')) else 'weapon_effect_'+name+'_tu15_original.json'
        fixture_command=f'{target} "${{PROJECT_SOURCE_DIR}}/tests/fixtures/{fixture}"'
        if name in ('datum_allocate','sound_local','location_copy','marker_matrix','marker_parent','marker_mode','marker_parent_composed','marker_lookup','marker_emission_graph'):
            if name=='sound_local':fixture='weapon_sound_local_tu15_original.json'
            fixture_command=f'"${{Python3_EXECUTABLE}}" "${{PROJECT_SOURCE_DIR}}/tools/test_native_weapon_compressed_reference.py" --probe "$<TARGET_FILE:{target}>" --fixture "${{PROJECT_SOURCE_DIR}}/tests/fixtures/{fixture}.gz"'
        cmake.extend([f'add_executable({target} tests/{target}.cpp)',
            f'target_include_directories({target} PRIVATE include tests)',
            f'target_compile_features({target} PRIVATE cxx_std_20)',
            f'target_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)',
            f'target_link_libraries({target} PRIVATE native_weapon_component_kernel nlohmann_json::nlohmann_json glm::glm-header-only)',
            f'if(HALO2_SANITIZERS)\n target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\n target_link_options({target} PRIVATE -fsanitize=address,undefined)\nendif()',
            f'add_test(NAME {test} COMMAND {fixture_command})'])
    cmake.append('add_test(NAME native_weapon_effect_restart_tu15 COMMAND native_weapon_effect_request_tu15_tests "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_restart_tu15_original.json")')
    cmake.append('add_test(NAME native_weapon_effect_attached_strict_tu15 COMMAND native_weapon_effect_lifecycle_strict_tu15_tests "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_attached_strict_tu15_original.json")')
    cmake.append('add_test(NAME native_weapon_effect_direction_strict_tu15 COMMAND native_weapon_effect_location_strict_tu15_tests "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_direction_strict_tu15_original.json")')
    (project/'CMakeLists.txt').write_text('\n'.join(cmake)+'\n')
    (output/'source-cmake.txt').write_text('\n'.join(cmake)+'\n')
    report=dict(passed=False,scope=__doc__,frozen_source_sha256=frozen,scratch_owner=str(scratch),
        commands=[],negative_controls_rejected=[],negative_kernel_rebuilds={},retained_large_artifacts=[],full_user_goal_complete=False,
        generated_cmake_sha256=sha(project/'CMakeLists.txt'),owned_negative_worker_build=str(scratch/'build'),
        owned_negative_worker_cleanup='Enclosing tools/run_test.py supervisor; no permanent build retained')
    def save():(output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    def validate(label,compiler,sanitizers=False,owned_worker=False):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
            '--scratch-root',str(scratch/'builds'),'--output',str(output/label),'--jobs','3',
            '--filter','^(native_weapon_effect_(dispatch|request|restart|location|iterator|state|creation|initialization|allocation|selector|dispatch_strict|lifecycle_strict|attached_strict|location_strict|direction_strict|datum_allocate|sound_local|location_copy|marker_matrix|marker_parent|marker_mode|marker_parent_composed|marker_lookup|marker_emission_graph)_tu15|native_weapon_zoom_reset(_strict)?_tu15|native_weapon_audio_request_strict_tu15)$',
            '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
        for name in NAMES:command+=['--target','native_weapon_effect_'+name+'_tu15_tests']
        command+=['--target','native_weapon_zoom_reset_tu15_tests','--target','native_weapon_zoom_reset_strict_tu15_tests','--target','native_weapon_audio_request_strict_tu15_tests']
        if sanitizers:command.append('--sanitizers')
        if owned_worker:command.append('--build-worker')
        report['commands'].append(command);save();return subprocess.run(command,check=False,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    save()
    try:
        emit='src/gameplay/weapons/native_weapon_marker_emission_tu15_internal.hpp'
        math='src/gameplay/weapons/native_weapon_marker_math_tu15_internal.hpp'
        source=(project/emit).read_text();end='instruction(0x1e1331);a.gp[2]=read(m,a.gp[1]+4);';prefix=source[source.index('void marker_emission('):source.index(end)+len(end)];cached=prefix.replace('IteratorRegisters r{a.gp,a.eflags};','IteratorRegisters r{a.gp,a.eflags};std::uint32_t cached_count;').replace('iterator_flags(r,read(m,a.gp[1]+4),a.gp[3],4,true);','iterator_flags(r,cached_count=read(m,a.gp[1]+4),a.gp[3],4,true);').replace(end,'instruction(0x1e1331);a.gp[2]=cached_count;');assert cached!=prefix
        graph_controls=[('emission-filter-sign',emit,'a.gp[2]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(a.gp[0]))));','a.gp[2]=a.gp[0]&255u;'),('emission-capacity-sign',emit,'instruction(0x1e127f);if(!signed_less())goto finished;','instruction(0x1e127f);if(!(a.eflags&1u))goto finished;'),('emission-node-sign',emit,'instruction(0x1e12dd);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[6],2)))));','instruction(0x1e12dd);a.gp[1]=read(m,a.gp[6],2);'),('emission-fresh-group-count',emit,prefix,cached),('quaternion-quiet-compare',math,'"comiss %4,%3; setb %0; setp %1; sete %2"','"ucomiss %4,%3; setb %0; setp %1; sete %2"'),('quaternion-zero-scale',math,'instruction(0x144952);if(a.eflags&65u)goto zero_scale;','instruction(0x144952);if(a.eflags&64u)goto zero_scale;'),('product-packed-lanes',math,'instruction(0x1455aa);vector(a,0,_mm_mul_ps(vector(a,0),vector(a,4)));','instruction(0x1455aa);vector(a,0,_mm_mul_ss(vector(a,0),vector(a,4)));'),('product-half-load',math,'std::memcpy(a.xmm[n].data()+2,bytes.data(),8);','std::memcpy(a.xmm[n].data(),bytes.data(),8);'),('product-half-store',math,'std::memcpy(event.written.data(),a.xmm[n].data()+2,8);','std::memcpy(event.written.data(),a.xmm[n].data(),8);')]
        mutations=[(name,before,after)for name,path,before,after in graph_controls]+[('lookup-signed-count','instruction(0x1e11dc);if((a.eflags&64u)||signed_less())goto pop_return;','instruction(0x1e11dc);if(!a.gp[6])goto pop_return;'),
            ('lookup-record-stride','instruction(0x1e11e6);a.gp[1]=iterator_flags(r,a.gp[1],12,4,false);','instruction(0x1e11e6);a.gp[1]=iterator_flags(r,a.gp[1],8,4,false);'),
            ('lookup-result-flags','instruction(0x1e11ef);a.gp[0]=a.gp[2];','instruction(0x1e11ef);a.gp[0]=a.gp[2];iterator_logic_flags(r,a.gp[0],4);'),
            ('mode-return-upper-byte','instruction(0xfc52c);a.gp[0]&=0xffffff00u;iterator_logic_flags(r,0,1);','instruction(0xfc52c);a.gp[0]=0;iterator_logic_flags(r,0,1);'),
            ('mode-header-upper-byte','instruction(0xfc526);a.gp[2]=(a.gp[2]&0xffffff00u)|read(m,a.gp[1]+0x12c,1);','instruction(0xfc526);a.gp[2]=read(m,a.gp[1]+0x12c,1);'),
            ('mode-return-flags','instruction(0xfc53c);a.gp[0]=(a.gp[0]&0xffffff00u)|1u;','instruction(0xfc53c);a.gp[0]=(a.gp[0]&0xffffff00u)|1u;iterator_logic_flags(r,1,1);'),
            ('parent-tail-argument','instruction(0xbad17);store(m,o,0xbad17,a.gp[4]+4,a.gp[0]);','instruction(0xbad17);store(m,o,0xbad17,a.gp[4]+4,a.gp[7]);'),
            ('parent-definition-upper-byte','instruction(0xbad0a);a.gp[2]=(a.gp[2]&0xffffff00u)|read(m,a.gp[1]+2,1);','instruction(0xbad0a);a.gp[2]=read(m,a.gp[1]+2,1);'),
            ('parent-kind-upper-byte','instruction(0xbad26);a.gp[1]=(a.gp[1]&0xffffff00u)|read(m,a.gp[6]+0xaa,1);','instruction(0xbad26);a.gp[1]=read(m,a.gp[6]+0xaa,1);'),
            ('marker-return-upper-word','instruction(0xbaefe);a.gp[0]=(a.gp[0]&0xffff0000u)|(a.gp[5]&65535u);',
             'instruction(0xbaefe);a.gp[0]=a.gp[5]&65535u;'),
            ('marker-fresh-header-read','instruction(0xbae98);a.gp[1]=read(m,a.gp[1]+0x44);',
             'instruction(0xbae98);a.gp[1]=0x1002000u;'),
            ('marker-rep-direction','const auto delta=(a.eflags&1024u)?std::uint32_t(-4):4u;a.gp[6]+=delta;a.gp[7]+=delta;--a.gp[1];',
             'const auto delta=4u;a.gp[6]+=delta;a.gp[7]+=delta;--a.gp[1];'),
('location-copy-direction','instruction(0x17ddb4);const auto value=read(m,a.gp[6]);store(m,o,0x17ddb4,a.gp[7],value);\n        const auto delta=(a.eflags&1024u)?std::uint32_t(-4):4u;',
             'instruction(0x17ddb4);const auto value=read(m,a.gp[6]);store(m,o,0x17ddb4,a.gp[7],value);\n        const auto delta=4u;'),
            ('location-copy-node-sign','instruction(0x17dda5);a.gp[6]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[2]))));',
             'instruction(0x17dda5);a.gp[6]=a.gp[2]&65535u;'),
            ('location-copy-selected-length','instruction(0x17ddf4);a.gp[1]=13;',
             'instruction(0x17ddf4);a.gp[1]=12;'),
('sound-local-upper-byte','instruction(0x18be00);a.gp[0]&=0xffffff00u;iterator_logic_flags(a,0,1);',
             'instruction(0x18be00);a.gp[0]=0;iterator_logic_flags(a,0,1);'),
            ('sound-local-shift-mask','const auto count=a.gp[1]&31u;',
             'const auto count=a.gp[1]&7u;'),
            ('sound-local-definition-flag','instruction(0x18be1c);iterator_logic_flags(a,read(m,a.gp[2],1)&64u,1);',
             'instruction(0x18be1c);iterator_logic_flags(a,read(m,a.gp[2],1)&128u,1);'),
            ('datum-signed-capacity','instruction(0x16e6de);if(!signed_less())goto done;',
             'instruction(0x16e6de);if(a.gp[7]>=read(m,a.gp[2]+0x20))goto done;'),
            ('datum-fresh-stride','instruction(0x16e727);a.gp[1]=read(m,a.gp[2]+0x24);',
             'instruction(0x16e727);a.gp[1]=12u;'),
            ('datum-rep-direction','store(m,o,0x16e733,a.gp[7],a.gp[0]);a.gp[7]+=(a.eflags&1024u)?std::uint32_t(-4):4u;--a.gp[1];',
             'store(m,o,0x16e733,a.gp[7],a.gp[0]);a.gp[7]+=4u;--a.gp[1];'),
            ('datum-salt-wrap','instruction(0x16e74e);store(m,o,0x16e74e,a.gp[2]+0x40,0x8000,2);',
             'instruction(0x16e74e);store(m,o,0x16e74e,a.gp[2]+0x40,0,2);'),
            ('origin-unit-scale','instruction(0xbbe75);if(a.eflags&4u) {','instruction(0xbbe75);if(!(a.eflags&4u)) {'),
            ('direction-read-order','instruction(0x179216);a.gp[6]=read(m,a.gp[1]+4);',
             'instruction(0x179216);a.gp[6]=read(m,a.gp[1]+8);'),
            ('restart-real-sound-result','store(m,o,0x17a221,a.gp[3]+8,a.gp[0]);',
             'store(m,o,0x17a221,a.gp[3]+8,none);'),
            ('reset-producer-local-filter','if(observe_reset && previous!=0xffu)observe_reset(',
             'if(observe_reset && previous!=0xffu && a.gp[3]!=none)observe_reset('),
            ('iterator-cursor-read-order',
             'instruction(0x17dd06);store(m,o,0x17dd06,a.gp[6],a.gp[1]);\n        instruction(0x17dd08);a.gp[1]=(a.gp[1]&0xffff0000u)|read(m,a.gp[0]+2,2);',
             'const auto wrong_node=read(m,a.gp[0]+2,2);instruction(0x17dd06);store(m,o,0x17dd06,a.gp[6],a.gp[1]);\n        instruction(0x17dd08);a.gp[1]=(a.gp[1]&0xffff0000u)|wrong_node;'),
            ('iterator-mode3-filter','unfiltered=a.gp[1]==0;','unfiltered=false;'),
            ('state-signed-generation',
             'instruction(0x17b167);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[0]))));',
             'instruction(0x17b167);a.gp[0]&=65535u;'),
            ('state-x87-rounding','const auto cw=a.floating_point->x87_control_word;',
             'const auto cw=std::uint16_t(0x27f);'),
            ('state-bound-comparison','if(!(a.eflags&4u)) {',
             'if(a.xmm[0][0]==read(m,a.gp[1]+12)) {'),
            ('creation-final-admission','instruction(0x179081);if(!(a.eflags&64u))goto completed;',
             'instruction(0x179081);goto completed;'),
            ('creation-position-read-order',
             'instruction(0x178f55);store(m,o,0x178f55,a.gp[2],a.gp[0]);\n        instruction(0x178f57);a.gp[0]=read(m,a.gp[1]+4);',
             'const auto wrong_source=read(m,a.gp[1]+4);instruction(0x178f55);store(m,o,0x178f55,a.gp[2],a.gp[0]);\n        instruction(0x178f57);a.gp[0]=wrong_source;'),
            ('creation-attachment-result','instruction(0x178f39);store(m,o,0x178f39,a.gp[6]+0x58,a.gp[5]);',
             'instruction(0x178f39);store(m,o,0x178f39,a.gp[6]+0x58,none);'),
            ('initialization-eager-default-read','instruction(0x17b0de);const auto fallback=read(m,0x474c70);',
             'instruction(0x17b0de);const auto fallback=(a.eflags&64u)?read(m,0x474c70):a.gp[1];'),
            ('initialization-final-handle','for(unsigned slot=0;slot<32;++slot)',
             'for(unsigned slot=0;slot<31;++slot)'),
            ('initialization-vector-read-order',
             'instruction(0x17b0f2);store(m,o,0x17b0f2,a.gp[7],a.gp[3]);\n    instruction(0x17b0f4);a.gp[3]=read(m,a.gp[1]+4);',
             'const auto wrong_source=read(m,a.gp[1]+4);instruction(0x17b0f2);store(m,o,0x17b0f2,a.gp[7],a.gp[3]);\n    instruction(0x17b0f4);a.gp[3]=wrong_source;'),
            ('allocation-retry-result','instruction(0x17b028);a.gp[3]=a.gp[0];',
             'instruction(0x17b028);a.gp[3]=none;'),
            ('allocation-vector-read-order',
             'instruction(0x17b058);store(m,o,0x17b058,a.gp[1],a.gp[2]);\n        instruction(0x17b05a);a.gp[2]=read(m,a.gp[0]+4);',
             'const auto wrong_source=read(m,a.gp[0]+4);instruction(0x17b058);store(m,o,0x17b058,a.gp[1],a.gp[2]);\n        instruction(0x17b05a);a.gp[2]=wrong_source;'),
            ('allocation-final-location-pair','do {\n        instruction(0x17b970);',
             'a.gp[1]=15;do {\n        instruction(0x17b970);'),
            ('selector-return-flags','instruction(0x106fe4);(void)pop(m,a);',
             'instruction(0x106fe4);a.eflags=0x202;(void)pop(m,a);'),
            ('selector-eager-tag-read','instruction(0x106fc5);a.gp[6]=read(m,a.gp[6]+a.gp[7]+8);',
             'instruction(0x106fc5);if(a.eflags&64u)a.gp[6]=read(m,a.gp[6]+a.gp[7]+8);'),
            ('dispatch-none-flags','instruction(0x108b28);(void)pop(m,a);a.gp[4]+=12;',
             'instruction(0x108b28);a.eflags=0x202;(void)pop(m,a);a.gp[4]+=12;'),
            ('dispatch-signed-shift-carry','a.eflags=(a.eflags&~0x801u)|((before>>28u)&1u);',
             'a.eflags&=~0x801u;'),
            ('dispatch-owner-read-parity','instruction(0x108aac);iterator_logic_flags(r,a.gp[2]&1u,1);',
             'instruction(0x108aac);iterator_logic_flags(r,a.gp[2]&1u,1);a.eflags^=4u;'),
            ('reset-tail-flags','instruction(0xcad17);compare(a.gp[0],none);','instruction(0xcad17);'),
            ('reset-camera-multiply-carry','instruction(0xcaca4);a.gp[1]=multiply(a.gp[1],0x94);',
             'instruction(0xcaca4);a.gp[1]=multiply(a.gp[1],0x94);a.eflags|=1u;'),
            ('audio-direct-return-flags','instruction(0x18bea0);a.gp[4]=iterator_flags(r,a.gp[4],0x28,4,false);',
             'instruction(0x18bea0);a.gp[4]+=0x28;'),
            ('audio-unowned-call-flags','instruction(0x18c4d5);a.gp[1]=none;iterator_logic_flags(r,a.gp[1],4);',
             'instruction(0x18c4d5);a.gp[1]=none;'),
            ('audio-faulting-push-esp','const auto next_stack=a.gp[4]-4u;',
             'const auto next_stack=(a.gp[4]-=4u);'),
            ('refresh-signed-bound','instruction(0x17e00c);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto no_handle;',
             'instruction(0x17e00c);if(a.gp[1]>=a.gp[0])goto no_handle;'),
            ('refresh-return-byte','instruction(0x17e04b);a.gp[0]=(a.gp[0]&0xffffff00u)|(a.gp[3]&255u);',
             'instruction(0x17e04b);a.gp[0]=a.gp[3]&255u;'),
            ('restart-signed-location','instruction(0x17a1fc);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[0]))));',
             'instruction(0x17a1fc);a.gp[0]&=65535u;'),
            ('attached-rep-direction','        a.gp[7]+=(a.eflags&1024u)?std::uint32_t(-4):4u;--a.gp[1];',
             '        a.gp[7]+=4u;--a.gp[1];'),
            ('attached-argument-read-order','instruction(0x179652);store(m,o,0x179652,a.gp[4]+12,a.gp[0]);\n    instruction(0x179656);a.gp[0]=read(m,a.gp[4]+0xbc);',
             'const auto wrong_argument=read(m,a.gp[4]+0xbc);instruction(0x179652);store(m,o,0x179652,a.gp[4]+12,a.gp[0]);\n    instruction(0x179656);a.gp[0]=wrong_argument;'),
            ('origin-copy-flags','instruction(0xbbe1d);a.gp[0]=iterator_flags(r,a.gp[0],0x64,4,false);',
             'instruction(0xbbe1d);a.gp[0]+=0x64;'),
            ('origin-signed-matrix-offset','instruction(0xbbe59);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[2]+0x116,2)))));',
             'instruction(0xbbe59);a.gp[1]=read(m,a.gp[2]+0x116,2);'),
            ('direction-return-flags','instruction(0x17922b);(void)pop(m,a);',
             'instruction(0x17922b);a.eflags=0x202;(void)pop(m,a);')]
        body='src/gameplay/weapons/native_weapon_marker_matrix_tu15_internal.hpp'
        def mutation_path(name):
            if name.startswith('emission-'):return emit
            if name.startswith(('quaternion-','product-')):return math
            return 'src/gameplay/weapons/native_weapon_marker_lookup_tu15_internal.hpp' if name.startswith('lookup-') else (body if name.startswith(('marker-','parent-','mode-')) else UNIT)
        for name,before,_ in mutations:
            source_preflight=(project/mutation_path(name)).read_text()
            if source_preflight.count(before)!=1:raise ValueError('Ambiguous mutation '+name)
        for label,compiler,sanitizers in [('gcc','g++',False),('clang','clang++',False),('clang-sanitized','clang++',True)]:
            if validate(label,compiler,sanitizers):raise ValueError('Failed native component '+label)
        # This whole validator is inside an enclosing run_test.py owner. The
        # official worker can reuse that owner's build while each declared
        # mutation gives its private kernel source a fresh modification time.
        if validate('clang-owned-worker','clang++',owned_worker=True):raise ValueError('Failed owned component worker baseline')
        for name,before,after in mutations:
            target=project/mutation_path(name);original=target.read_text()
            if original.count(before)!=1:raise ValueError('Ambiguous mutation '+name)
            target.write_text(original.replace(before,after,1))
            result=validate('negative-'+name,'clang++',owned_worker=True)
            build_log=(output/('negative-'+name)/'build.log').read_text()
            rebuilt=re.findall(r'Building CXX object[^\n]*native_weapon_effect_dispatch_tu15\.cpp\.o',build_log)
            if len(rebuilt)!=1:raise ValueError('Owned mutation did not rebuild exactly its one kernel object: '+name)
            if re.search(r'Building CXX object[^\n]*/tests/[^\n]*\.cpp\.o',build_log):raise ValueError('Owned mutation unexpectedly rebuilt unchanged test drivers: '+name)
            report['negative_kernel_rebuilds'][name]=len(rebuilt)
            if result==0 or 'Different' not in (output/('negative-'+name)/'tests.log').read_text() and 'differs' not in (output/('negative-'+name)/'tests.log').read_text():
                raise ValueError('Wrong-code control was not rejected: '+name)
            target.write_text(original);report['negative_controls_rejected'].append(name);save()
        if validate('clang-owned-worker-restored','clang++',owned_worker=True):raise ValueError('Failed restored owned component worker')
        if any(sha(project/p)!=digest for p,digest in frozen.items()):raise ValueError('Frozen component source changed')
        report.update(passed=True,original_cases=4759,exhaustive_iterator_node_mode_cases=196608,instrumented_components=29,
            source_unchanged=True,current_root_matches_frozen=all(sha(ROOT/p)==digest for p,digest in frozen.items()))
    except BaseException as error:report['error']=str(error);raise
    finally:save()
    print(json.dumps({key:report[key] for key in ('passed','original_cases','negative_controls_rejected','current_root_matches_frozen')}))


if __name__=='__main__':main()
