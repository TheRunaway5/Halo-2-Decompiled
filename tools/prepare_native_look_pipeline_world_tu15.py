#!/usr/bin/env python3
"""Prepare a narrow native Host18A300 integration over a frozen multiplayer source."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals','context_calls':'controller.context.calls','context_refusals':'controller.context.refusals','camera_calls':'camera.calls','camera_completions':'camera.completed','camera_onfoot_calls':'camera.on_foot','camera_marker_calls':'camera.markers','marker_calls':'marker_calls','angle_calls':'angles.calls','angle_completions':'angles.completed','angle_refusals':'angles.refusals','query_refusals':'query_refusals','marker_observer_refusals':'marker_observer_refusals'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',type=Path,required=True);p.add_argument('--stage',type=Path,required=True);a=p.parse_args();a.base=a.base.resolve();a.stage=a.stage.resolve();a.stage.mkdir(parents=True,exist_ok=False);staged=a.stage/'staged';core=ROOT/'analysis/physics/native-look-pipeline-core-qualified-v2-20261102';pins=json.loads((core/'source-pins.json').read_text());assert pins['staged_hashes'];base_hashes={};staged_hashes={};sources={};deltas=[]
 def read(n):
  p=a.base/n
  if not p.exists()and n.startswith('src/physics/'):p=a.base/'src'/Path(n).name
  base_hashes[n]=sha(p);sources[n]=p;return p.read_text()
 def write(n,text):
  p=staged/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text);staged_hashes[n]=sha(p);deltas.append(n)
 world='src/physics/physics_live_world_tu15.cpp';s=read(world);include='#include "halo2/native_tu15_look_initialization_binding.hpp"';assert s.count(include)==1;s=s.replace(include,include+'\n#include "halo2/native_tu15_look_pipeline_binding.hpp"');needle='    NativeTu15LookInitializationBindingActivity native_look_initialization;';assert s.count(needle)==1;s=s.replace(needle,needle+'\n    NativeTu15LookPipelineActivity native_look_pipeline;')
 old='        run_actual(0x18a300u,a);';assert s.count(old)==1
 block='''        {
            source_execution_untracked(bank,0x18a300u);Execution execution(*this);
            SourcePhysicsContactRecordObserver observed;
            if(physics_store_observer)observed=[this,collector=physics_store_observer](const auto&store){
                last_instruction=store.instruction;collector(store);
            };
            // Owned Host arithmetic DATA, disjoint from retained player rows,
            // camera/model outputs and the original descending call stack.
            native_tu15_update_bound_look_controller(bank,a.gp[0],args[0],args[1],
                {{0x6005d000u},{0x6005d100u,{0x6005d200u}}},
                {a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr,a.x87_depth},
                !physics_store_observer||physics_store_observer_read_only,observed,&native_look_pipeline);
        }'''
 s=s.replace(old,block);s=s.replace('// Read-only diagnostic at the actual typed Hostcall; the existing\n        // controller implementation and its input/return behavior are unchanged.','// Read-only diagnostic at the actual typed Host look boundary,\n        // before the qualified native controller consumes its DATA formals.')
 needle='    const auto&look_initialization=impl_->native_look_initialization;';assert s.count(needle)==1
 getter='    const auto&look_pipeline=impl_->native_look_pipeline;\n'+''.join('    result.native_look_update_'+k+'=look_pipeline.'+v+';\n'for k,v in FIELDS.items());s=s.replace(needle,getter+needle);write(world,s)
 header='include/halo2/native_tu15_physics_frame_residuals.hpp';s=read(header);needle='  std::uint64_t native_look_initialization_attempts{}';at=s.index(needle);s=s[:at]+'  std::uint64_t '+','.join('native_look_update_'+k+'{}'for k in FIELDS)+';\n'+s[at:];write(header,s)
 accept='tests/physics_multiplayer_acceptance.cpp';s=read(accept);needle='        {"native_look_initialization_attempts",native_frames.native_look_initialization_attempts},';assert s.count(needle)==2;s=s.replace(needle,''.join('        {"native_look_update_'+k+'",native_frames.native_look_update_'+k+'},\n'for k in FIELDS)+needle)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_CONTROLLER")) {
    const bool native_look=native_frames.native_look_update_calls>0&&
      native_frames.native_look_update_attempts==native_frames.native_look_update_calls&&
      native_frames.native_look_update_refusals==0&&
      native_frames.native_look_update_context_calls==native_frames.native_look_update_calls&&
      native_frames.native_look_update_context_refusals==0&&
      native_frames.native_look_update_camera_calls==native_frames.native_look_update_calls&&
      native_frames.native_look_update_camera_completions==native_frames.native_look_update_camera_calls&&
      native_frames.native_look_update_angle_completions==native_frames.native_look_update_angle_calls&&
      native_frames.native_look_update_angle_refusals==0&&native_frames.native_look_update_query_refusals==0&&
      native_frames.native_look_update_marker_observer_refusals==0;
    report["native_look_controller_gate"]=native_look;passed&=native_look;
  }
'''
 needle='  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION")) {';assert s.count(needle)==1;s=s.replace(needle,gate+needle);write(accept,s)
 cmake='cmake/PhysicsLiveWorldTu15.cmake';s=read(cmake);new=[]
 for n,h in pins['staged_hashes'].items():
  if not n.endswith('.cpp'):continue
  if n in s:continue
  new.append(n)
 s+='\n# Source-authenticated native Host look/camera pipeline; unknown queries refuse.\ntarget_sources(halo2_native_pair_consumer PRIVATE\n'+''.join('    '+n+'\n'for n in new)+')\nif(MSVC)\n    set_source_files_properties(\n'+''.join('        '+n+'\n'for n in new)+'        PROPERTIES COMPILE_OPTIONS /fp:strict)\nelse()\n    set_source_files_properties(\n'+''.join('        '+n+'\n'for n in new)+'        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")\nendif()\n';write(cmake,s)
 for n,h in pins['staged_hashes'].items():
  # Already-qualified common math is not replaced in the frozen source.
  existing=a.base/n
  if existing.exists()and sha(existing)==h and n=='src/physics/native_tu15_constraint_auxiliary_math.cpp':continue
  src=core/'staged'/n;assert sha(src)==h;dst=staged/n;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst);staged_hashes[n]=sha(dst)
 inherited=[]
 for n in('tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp'):
  dst=staged/n;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.base/n,dst);staged_hashes[n]=sha(dst);inherited.append(n)
 patch=''
 for n in deltas:patch+=''.join(difflib.unified_diff(sources[n].read_text().splitlines(keepends=True),(staged/n).read_text().splitlines(keepends=True),fromfile='base/'+n,tofile='nativeLook/'+n))
 (a.stage/'delta.patch').write_text(patch)
 save(a.stage/'source-pins.json',dict(qualified=False,peer_review_pending=True,root_reviewed=False,live_qualified=False,base_source_root=str(a.base),base_source_files={n:str(p)for n,p in sources.items()},base_hashes=base_hashes,staged_hashes=staged_hashes,delta_files=deltas,inherited_files=inherited,core_qualification=str((core/'source-pins.json').relative_to(ROOT)),core_qualification_sha256=sha(core/'source-pins.json'),fields=FIELDS,host_data=dict(controller='[6005D000,6005D0B4)',camera_parent='[6005D100,6005D1A4)',adjustment='[6005D200,6005D228)',owner='Existing one-MiB native Host DATA/stack reservation; no original GP or return fields serialized'),scope='Sole prepare_player_look Host18A300 call native, actual index/delta/FP fields and readonly SourcePC observer; old volatile ABI results unused. Preserve constructor3/Body/Dirty/Pool/SeatedRO. Native-only builder, unknown16C620/CRT counts/refuses; no CPUfallback.',remaining=['Exact Host/controller admission models/counter truth/both serializers/gate','Coherent full existing World models and both actual same-production receipts','Independent peer and visible multiplayer allnativeLOOKgate/actualvehiclecamera/strict','All post16C620 camera query paths remain explicit unqualified requirements']))
 print(sha(a.stage/'source-pins.json'));return 0
if __name__=='__main__':raise SystemExit(main())
