#!/usr/bin/env python3
"""Stage the selected Host18A2A0 handoff against a frozen multiplayer source."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
from prepare_native_look_initialization_world_tu15 import ROOT,source_file,once

FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals',
        'kernel_calls':'reorientation.calls','kernel_refusals':'reorientation.refusals',
        'yaw_wraps':'reorientation.yaw_wraps'}
EXISTING=('src/physics/physics_live_world_tu15.cpp','include/halo2/native_tu15_physics_frame_residuals.hpp',
          'tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake')
NEW=('include/halo2/native_tu15_look_reorientation.hpp','include/halo2/native_tu15_look_reorientation_binding.hpp',
     'src/physics/native_tu15_look_reorientation.cpp')
INHERITED=('include/halo2/native_tu15_look_pipeline_binding.hpp','src/physics/native_tu15_joint_constraint_numeric_internal.hpp',
           'tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged'
    sources={rel:source_file(a.source_root,rel)for rel in EXISTING};before={rel:path.read_text()for rel,path in sources.items()};after=dict(before)
    live=EXISTING[0]
    after[live]=once(after[live],'#include "halo2/native_tu15_look_reorientation_input_observer.hpp"',
        '#include "halo2/native_tu15_look_reorientation_input_observer.hpp"\n#include "halo2/native_tu15_look_reorientation_binding.hpp"')
    after[live]=once(after[live],'    NativeTu15LookInitializationBindingActivity native_look_initialization;',
        '    NativeTu15LookInitializationBindingActivity native_look_initialization;\n    NativeTu15LookReorientationBindingActivity native_look_reorientation;')
    start=after[live].index('    void reorient_retained_input(');end=after[live].index('    void flush_vehicle_input_resets(',start)
    method=after[live][start:end]
    method=once(method,'        auto a=frame({});a.gp[0]=forward;a.gp[1]=0;', '        auto a=frame({});')
    handoff='''        source_execution_untracked(bank,0x18a2a0u);Execution execution(*this);
        SourcePhysicsContactRecordObserver observed;
        if(physics_store_observer)observed=[this,collector=physics_store_observer](const auto&store){
            last_instruction=store.instruction;collector(store);
        };
        native_tu15_reorient_bound_look_controller(bank,0u,forward,
            {a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr,a.x87_depth},
            !physics_store_observer||physics_store_observer_read_only,observed,&native_look_reorientation);
'''
    method=once(method,'        run_actual(0x18a2a0u,a);\n',handoff)
    after[live]=after[live][:start]+method+after[live][end:]
    getter='    const auto&look_reorientation=impl_->native_look_reorientation;\n'+''.join(f'    result.native_look_reorientation_{name}=look_reorientation.{member};\n'for name,member in FIELDS.items())
    after[live]=once(after[live],'    const auto&look_initialization=impl_->native_look_initialization;',getter+'    const auto&look_initialization=impl_->native_look_initialization;')
    header=EXISTING[1];anchor='  std::uint64_t native_look_initialization_attempts'
    after[header]=once(after[header],anchor,'  std::uint64_t '+','.join('native_look_reorientation_'+field+'{}'for field in FIELDS)+';\n'+anchor)
    acceptance=EXISTING[2];anchor='        {"native_look_initialization_attempts",native_frames.native_look_initialization_attempts},';assert after[acceptance].count(anchor)==2
    rows=''.join(f'        {{"native_look_reorientation_{name}",native_frames.native_look_reorientation_{name}}},\n'for name in FIELDS)
    after[acceptance]=after[acceptance].replace(anchor,rows+anchor)
    gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_REORIENTATION")) {
    const bool native_reorientation=native_frames.native_look_reorientation_calls>0&&
      native_frames.native_look_reorientation_attempts==native_frames.native_look_reorientation_calls&&
      native_frames.native_look_reorientation_kernel_calls==native_frames.native_look_reorientation_calls&&
      native_frames.native_look_reorientation_refusals==0&&native_frames.native_look_reorientation_kernel_refusals==0;
    report["native_look_reorientation_gate"]=native_reorientation;passed&=native_reorientation;
  }
'''
    after[acceptance]=once(after[acceptance],'  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION"))',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION"))')
    after[EXISTING[3]]+='''
# Sole native18A2A0 owner for the selected retained-input reorientation Host route.
target_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_look_reorientation.cpp)
if(MSVC)
    set_source_files_properties(src/physics/native_tu15_look_reorientation.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics/native_tu15_look_reorientation.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
'''
    for rel,text in after.items():dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(text)
    for rel in NEW:
        dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes((ROOT/rel).read_bytes())
    for rel in INHERITED:
        dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(source_file(a.source_root,rel).read_bytes())
    (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[rel].splitlines(keepends=True),after[rel].splitlines(keepends=True),fromfile='a/'+rel,tofile='b/'+rel))for rel in EXISTING))
    (a.output/'host-handoff.cpp').write_text('        auto a=frame({});\n'+method[method.index('        (void)native_tu15_observe_look_reorientation_input('):method.index('        bank.write(retained,')])
    save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,production_applied=False,base_source_root=str(a.source_root.resolve()),
        base_source_files={rel:str(path.resolve())for rel,path in sources.items()},base_hashes={rel:sha(path)for rel,path in sources.items()},
        staged_hashes={rel:sha(stage/rel)for rel in(*EXISTING,*NEW,*INHERITED)},delta_files=list(EXISTING),new_files=list(NEW),inherited_files=list(INHERITED),fields=FIELDS,
        positive_gate='HALO2_REQUIRE_NATIVE_LOOK_REORIENTATION',scope='Only selected retained-input Host18A2A0 route; current forward12B/current FP fields/readonly typed BEFORE observer and SourcePC store tee preserved. Other Source-generated/internal18A2A0 consumers and all unrelated physics remain outside admission.'))
    print(a.output/'source-pins.json')

if __name__=='__main__':main()
