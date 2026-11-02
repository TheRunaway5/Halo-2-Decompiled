#!/usr/bin/env python3
"""Reject state-transport mutants using one owned unchanged physical golden."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

import validate_native_tu15_normalize_vector as reference

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    from oracle_smg_firing_1_1 import Xbe
    gold={};owners={}
    originals={'tu15':Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'),
        'mp11':ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'}
    for image,path in originals.items():
        assert hashlib.sha256(path.read_bytes()).hexdigest()==reference.PINS[image]
        rows=reference.rows(image);proof,owners[image]=reference.physical(Xbe(path),rows,scratch/image)
        gold[image]=dict(rows=rows,proof=proof)
    fixture=scratch/'gold.json';fixture.write_text(json.dumps(gold,separators=(',',':')))
    project=scratch/'source';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    for name in ('physics_step_internal.hpp','physics_solver_internal.hpp','physics_contact_runtime_internal.hpp','physics_world_runtime_internal.hpp','physics_instruction_remap_tu15.hpp'):
        target=project/'src/physics/runtime'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/'src/physics/runtime'/name,target)
    shutil.copyfile(ROOT/'tests/native_tu15_normalize_vector_tests.cpp',project/'probe.cpp')
    shutil.copyfile(ROOT/'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp',project/'math.cpp')
    bridge=(ROOT/'src/physics/runtime/native_tu15_normalize_vector_bridge.hpp').read_text()
    base=(ROOT/'src/physics/runtime/physics_contact_runtime_internal.hpp').read_text();base_variants={}
    variants={'positive':bridge}
    def edit(needle,change):
        assert bridge.count(needle)==1,(needle,bridge.count(needle));return bridge.replace(needle,change)
    variants['ecx_on_refusal']=edit('s.gp[1]=result.magnitude_bits','s.gp[1]=result.xmm0_bits()')
    variants['skip_xmm1']=edit('    if(result.normalized()) {','    s.xmm[1]=lane(0);\n    if(result.normalized()) {')
    variants['xmm_upper_lanes']=edit('s.xmm[0]=lane(result.xmm0_bits());','s.xmm[0]=_mm_move_ss(s.xmm[0],lane(result.xmm0_bits()));')
    variants['comparison_flags']=edit('s.flags.zf=result.compare==Compare::equal||result.compare==Compare::unordered;','s.flags.zf=false;')
    variants['retain_c1']=edit('s.fp.status&=~0x200u;','')
    variants['depth_four']=edit('if(s.fp.count>3)throw','if(s.fp.count>4)throw')
    variants['wrong_constants']=edit('native_tu15_normalize_mp11:native_tu15_normalize_tu15;','native_tu15_normalize_tu15:native_tu15_normalize_mp11;')
    variants['xyz_mapping']=edit('s.xmm[1]=lane(result.published[2]);s.xmm[2]=lane(result.published[1]);','s.xmm[1]=lane(result.published[1]);s.xmm[2]=lane(result.published[2]);')
    variants['observer_admitted']=edit('if(s.observe)return refuse(Refusal::Observer);','')
    variants['trace_admitted']=edit('if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))\n            return refuse(Refusal::Trace);','')
    variants['readonly_admitted']=edit('if(!page->writable)return std::nullopt;','')
    variants['exit_pc_missing']=edit('if(cursor)*cursor->position=normalized?0x30c37u:0x30c3cu;','')
    variants['literal_profile_admitted']=edit('if(profile==NativeTu15NormalizeProfile::Tu15Literal && s.image!=SourcePhysicsContactImage::TitleUpdate15)\n        return refuse(Refusal::Profile);','')
    variants['physical_alias_admitted']=edit('overlaps(scratch,8,backing(*vector),12)','false')
    variants['bounded_constant_alias_admitted']=edit('overlaps(scratch,8,backing(*epsilon),4)||overlaps(scratch,8,backing(*one),4)','false')
    variants['bounded_constant_capability_dropped']=edit('return s.memory.try_read_view(address,4);','return std::nullopt;')
    variants['policy_refusal_ignored']=edit('if(!result)return refuse(Refusal::Policies);','if(!result)return true;')
    variants['renderer_census_missing']=edit('++stats->renderer_admitted;','')
    variants['renderer_site_census_missing']=edit('++stats->renderer_sites[renderer_site].admitted;','')
    variants['writable_extent_capability_dropped']=edit('return s.memory.try_write_extent(address,bytes);','return std::nullopt;')
    variants['writable_extent_without_guarded_owner']=edit('if(s.services.normalize_vector_30bd0_guarded)return s.memory.try_write_extent(address,bytes);','return s.memory.try_write_extent(address,bytes);')
    variants['short_writable_extent_admitted']=edit('||vector->size()!=12||stack->size()!=8','')
    needle='target==0x30bd0u && image==SourcePhysicsContactImage::TitleUpdate15 &&'
    assert base.count(needle)==1
    variants['legacy_required_intercepted']=bridge
    base_variants['legacy_required_intercepted']=base.replace(needle,'target==0x30bd0u &&')
    (project/'check.py').write_text(f'''import json,sys
sys.path.insert(0,{str(ROOT/'tools')!r})
import validate_native_tu15_normalize_vector as reference
from pathlib import Path
gold=json.loads(Path(sys.argv[1]).read_text());difference=None
try:
 for image,g in gold.items():reference.compare(g['rows'],g['proof'],Path(sys.argv[2]))
except (ValueError,AssertionError) as error:difference=str(error)or 'Guard rejection changed'
if sys.argv[3]=='positive':assert difference is None,difference
else:assert difference is not None,'Mutation escaped physical state qualification'
print('Physical normalizer control',sys.argv[3],difference or 'all cases matched')
''')
    cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NormalizeMutations LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
add_library(math STATIC math.cpp)
target_include_directories(math PRIVATE include)
target_compile_features(math PRIVATE cxx_std_20)
target_compile_options(math PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    for name,source in variants.items():
        target=project/name/'physics/runtime/native_tu15_normalize_vector_bridge.hpp';target.parent.mkdir(parents=True,exist_ok=True);target.write_text(source)
        if name in base_variants:
            (target.parent/'physics_contact_runtime_internal.hpp').write_text(base_variants[name])
        cmake+=f'''add_executable(probe_{name} probe.cpp)
target_include_directories(probe_{name} PRIVATE {name} include src)
target_compile_features(probe_{name} PRIVATE cxx_std_20)
target_compile_options(probe_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(probe_{name} PRIVATE math nlohmann_json::nlohmann_json)
add_test(NAME normalizer_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/check.py" "{fixture}" $<TARGET_FILE:probe_{name}> {name})
'''
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
        '--output',str(a.output/'cmake'),'--jobs','2','--test-jobs','1','--filter','^normalizer_',
        '--retain-report','Testing/Temporary/LastTest.log']
    for name in variants:command+=['--target','probe_'+name]
    run=subprocess.run(command)
    paths=['src/physics/runtime/native_tu15_normalize_vector_bridge.hpp','src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp',
        'tests/native_tu15_normalize_vector_tests.cpp','src/physics/runtime/physics_contact_runtime_internal.hpp',
        'tools/validate_native_tu15_normalize_vector.py',str(Path(__file__).relative_to(ROOT))]
    (a.output/'validation.json').write_text(json.dumps(dict(passed=run.returncode==0,command=command,mutants=list(variants)[1:],
        physical_owners=owners,cases_per_control=sum(len(g['rows'])for g in gold.values()),
        source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in paths},
        owned_scratch=str(scratch),large_artifacts_retained=[]),indent=2)+'\n')
    return run.returncode


if __name__=='__main__':raise SystemExit(main())
