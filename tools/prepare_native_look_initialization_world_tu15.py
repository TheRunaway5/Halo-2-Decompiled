#!/usr/bin/env python3
"""Stage only qualified prepare_player_look constructor routing and activity."""
import argparse,difflib,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals','cleared_records':'initialization.cleared_records','unit_directions':'initialization.unit_directions','unchanged':'initialization.unchanged'}
EXISTING=('src/physics/physics_live_world_tu15.cpp','include/halo2/native_tu15_physics_frame_residuals.hpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake')
NEW=('include/halo2/native_tu15_look_initialization.hpp','include/halo2/native_tu15_look_initialization_binding.hpp','src/physics/native_tu15_look_initialization.cpp')
INHERITED=('tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp')
def sha(p):return hashlib.file_digest(p.open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def source_file(base,relative):
 p=base/relative
 if p.exists():return p
 assert relative.startswith('src/');p=base/'src'/Path(relative).name;assert p.exists(),p;return p
def once(text,before,after):assert text.count(before)==1,before[:100];return text.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(exist_ok=False,parents=True);stage=a.output/'staged';sources={rel:source_file(a.source_root,rel)for rel in EXISTING};before={rel:p.read_text()for rel,p in sources.items()};after=dict(before)
 assert sha(sources[EXISTING[0]])=='14d31ad27c5f4c8fc5f4f0ea71b8948c3ab64f446b4ae37d0d7dec358e78a7b4'
 assert sha(sources[EXISTING[-1]])=='b160029299e84a2af724a58b6f966b8c6921b4ed0afdc194644c85d08140547b'
 live=EXISTING[0];after[live]=once(after[live],'#include "halo2/native_tu15_look_controller_input_observer.hpp"','#include "halo2/native_tu15_look_controller_input_observer.hpp"\n#include "halo2/native_tu15_look_initialization_binding.hpp"')
 after[live]=once(after[live],'    NativeTu15WorldIntegrationStats native_physics_integrations;','    NativeTu15WorldIntegrationStats native_physics_integrations;\n    NativeTu15LookInitializationBindingActivity native_look_initialization;')
 begin=after[live].index('    void prepare_player_look(');end=after[live].index('    void reorient_retained_input(',begin);method=after[live][begin:end]
 old='''        if(!initialized) {
            auto a=frame({});a.gp[0]=0;a.gp[1]=unit;run_actual(0x18a0c0u,a);
        }'''
 new='''        if(!initialized) {
            auto a=frame({});
            source_execution_untracked(bank,0x18a0c0u);Execution execution(*this);
            SourcePhysicsContactRecordObserver observed;
            if(physics_store_observer)observed=[this,collector=physics_store_observer](const auto&store){
                last_instruction=store.instruction;collector(store);
            };
            native_tu15_initialize_bound_look_controller(bank,0u,unit,
                {a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr,a.x87_depth},
                !physics_store_observer||physics_store_observer_read_only,observed,&native_look_initialization);
        }'''
 method=once(method,old,new);after[live]=after[live][:begin]+method+after[live][end:]
 getter='    const auto&look_initialization=impl_->native_look_initialization;\n'+''.join('    result.native_look_initialization_'+field+'=look_initialization.'+member+';\n'for field,member in FIELDS.items())
 after[live]=once(after[live],'    result.world_integrations=impl_->native_physics_integrations.world_integrations;',getter+'    result.world_integrations=impl_->native_physics_integrations.world_integrations;')
 header=EXISTING[1];anchor='  std::uint64_t native_contact_body_attempts';assert after[header].count(anchor)==1;after[header]=after[header].replace(anchor,'  std::uint64_t '+','.join('native_look_initialization_'+field+'{}'for field in FIELDS)+';\n'+anchor)
 acceptance=EXISTING[2];rows=''.join('        {"native_look_initialization_'+field+'",native_frames.native_look_initialization_'+field+'},\n'for field in FIELDS);anchor='        {"native_contact_body_attempts",native_frames.native_contact_body_attempts},';assert after[acceptance].count(anchor)==2;after[acceptance]=after[acceptance].replace(anchor,rows+anchor)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION")) {
    const bool native_initialization=native_frames.native_look_initialization_calls>0&&
      native_frames.native_look_initialization_attempts==native_frames.native_look_initialization_calls&&
      native_frames.native_look_initialization_refusals==0&&native_frames.native_look_initialization_unit_directions>0&&
      native_frames.native_look_initialization_cleared_records>=native_frames.native_look_initialization_unit_directions;
    report["native_look_initialization_gate"]=native_initialization;passed&=native_initialization;
  }
'''
 after[acceptance]=once(after[acceptance],'  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_BODY_UPDATE"))',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_BODY_UPDATE"))')
 after[EXISTING[3]]+='\n# Sole native92-site18A0C0 constructor selected by prepare_player_look.\ntarget_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_look_initialization.cpp)\n'
 for rel,text in after.items():dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(text)
 for rel in NEW:dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes((ROOT/rel).read_bytes())
 for rel in INHERITED:dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(source_file(a.source_root,rel).read_bytes())
 delta=''.join(''.join(difflib.unified_diff(before[rel].splitlines(keepends=True),after[rel].splitlines(keepends=True),fromfile='a/'+rel,tofile='b/'+rel))for rel in EXISTING);(a.output/'delta.patch').write_text(delta)
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,production_applied=False,base_source_root=str(a.source_root.resolve()),base_source_files={rel:str(p.resolve())for rel,p in sources.items()},base_hashes={rel:sha(p)for rel,p in sources.items()},staged_hashes={rel:sha(stage/rel)for rel in(*EXISTING,*NEW,*INHERITED)},delta_files=list(EXISTING),new_files=list(NEW),inherited_files=list(INHERITED),fields=FIELDS,positive_gate='HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION',scope='Only prepare_player_look fresh index0/unit constructor: native-only92-site kernel, actual caller FP fields, source store observer, six truthful activity counters. Other18A0C0 caller routes and18A300/CD4A0 remain explicit migration work.',remaining=['Visible positive constructor initialization and strict multiplayer frame/camera validation','Other reachable18A0C0 Host call sites remain original','Whole18A300/CD4A0 native closure remains open']))
 print(a.output/'source-pins.json')
if __name__=='__main__':main()
