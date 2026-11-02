#!/usr/bin/env python3
"""Explicit retained Host capability plus exact-invocation World witness."""
from pathlib import Path
import hashlib,json,difflib,shutil
ROOT=Path(__file__).resolve().parents[1]
PARENT=ROOT/'analysis/physics/native-moving-shape-world-stage-20261102'
OUT=ROOT/'analysis/physics/native-moving-shape-retained-world-stage-20261102'
BASE=PARENT/'base'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def replace(s,a,b,n=1):assert s.count(a)==n,(a[:90],s.count(a),n);return s.replace(a,b)
def main():
 OUT.mkdir(exist_ok=False);stage=OUT/'staged';shutil.copytree(PARENT/'staged',stage);parent=json.loads((PARENT/'source-pins.json').read_text());before=dict(parent['base_hashes']);after=dict(parent['source_sha256']);diffs=[]
 def edit(name,fn,inherited=True):
  p=stage/name if inherited else BASE/name;old=p.read_text();new=fn(old);assert old!=new;target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new)
  if not inherited:before[name]=sha(p)
  after[name]=sha(target);diffs.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+name,tofile='b/'+name))
 name='include/halo2/native_tu15_world_retained_store_observation.hpp';p=stage/name;p.write_text('''#pragma once
#include "halo2/native_tu15_retained_store_observer.hpp"
namespace halo2 {
enum class NativeTu15WorldStoreObservationPolicy {retained_data_only};
// Explicit consent belongs to ONE actual callable object and memory owner.
// The caller keeps both alive/unchanged for the synchronous World invocation.
// A global read-only flag never supplies this witness. No private events are
// promised and callback reentry/mutation of source state is not permitted.
class NativeTu15WorldRetainedStoreWitness {
    const SourcePhysicsContactMemory*memory_;
    const SourcePhysicsContactRecordObserver*actual_;
    NativeTu15RetainedStoreObserver retained_;
public:
    NativeTu15WorldRetainedStoreWitness(const SourcePhysicsContactMemory&memory,
        const SourcePhysicsContactRecordObserver&actual,NativeTu15WorldStoreObservationPolicy)
        :memory_(&memory),actual_(&actual),retained_(SourcePhysicsContactRecordObserver{
            [&actual](const auto&event){if(actual)actual(event);}}){}
    NativeTu15WorldRetainedStoreWitness(const NativeTu15WorldRetainedStoreWitness&)=delete;
    NativeTu15WorldRetainedStoreWitness&operator=(const NativeTu15WorldRetainedStoreWitness&)=delete;
    const NativeTu15RetainedStoreObserver*for_invocation(const SourcePhysicsContactMemory&memory,
        const SourcePhysicsContactRecordObserver&actual)const noexcept{
        return memory_==&memory&&actual_==&actual?&retained_:nullptr;
    }
};
} // namespace halo2
''');before[name]=None;after[name]=sha(p)
 def world(s):
  s=replace(s,'  const std::function<std::uint64_t()>&actual_filetime){','  const std::function<std::uint64_t()>&actual_filetime,\n  const NativeTu15WorldRetainedStoreWitness*retained_stores){')
  s=replace(s,'    if(observe){if(stats)++stats->native_moving_process_observer_refusals;',
   '    const auto*retained=retained_stores?retained_stores->for_invocation(m,observe):nullptr;\n    if((observe&&!retained)||(retained_stores&&!retained)){if(stats)++stats->native_moving_process_observer_refusals;')
  needle='    NativeTu15MovingShapeNativeServices native;native.timestamp_counter=collision.timestamp_counter;'
  s=replace(s,needle,'''    NativeTu15RetainedStoreObserver moving_observer;
    if(retained){
      if(stats)++stats->native_moving_process_retained_observer_calls;
      moving_observer=NativeTu15RetainedStoreObserver{[retained,stats](const auto&event){
        if(stats)++stats->native_moving_process_retained_observer_events;
        const auto&callback=retained->callback();if(callback)callback(event);
      }};
    }
'''+needle)
  return replace(s,'      stats?&stats->moving_shape_process:nullptr);','      stats?&stats->moving_shape_process:nullptr,moving_observer);')
 edit('src/physics/native_tu15_world_integration_residuals.cpp',world)
 def api(s):
  s=replace(s,'#include "halo2/native_tu15_world_integration.hpp"','#include "halo2/native_tu15_world_integration.hpp"\n#include "halo2/native_tu15_world_retained_store_observation.hpp"')
  return replace(s,'  const std::function<std::uint64_t()>& actual_recorded_filetime = {});','  const std::function<std::uint64_t()>& actual_recorded_filetime = {},\n  const NativeTu15WorldRetainedStoreWitness* retained_store_witness = nullptr);')
 edit('include/halo2/native_tu15_world_integration_residuals.hpp',api,False)
 def live(s):
  s=replace(s,'    bool physics_store_observer_read_only{};','    bool physics_store_observer_read_only{};\n    bool physics_store_observer_retained{};')
  s=replace(s,'        native_tu15_step_physics_frame_with_residuals(bank,context,state,','''        std::optional<NativeTu15WorldRetainedStoreWitness> retained_store_witness;
        if(physics_store_observer_retained)
            retained_store_witness.emplace(bank,observed,NativeTu15WorldStoreObservationPolicy::retained_data_only);
        native_tu15_step_physics_frame_with_residuals(bank,context,state,''')
  s=replace(s,'[this,&state,continuation,&observed](auto world,const auto &step,auto &context)', '[this,&state,continuation,&observed,&retained_store_witness](auto world,const auto &step,auto &context)')
  needle='''                        return source_execution_input(bank,SourceExecutionInputKind::UtcFiletime,0,
                            [this]{return clocks->utc_filetime()+utc_adjustment;});
                    });'''
  s=replace(s,needle,needle[:-3]+'},retained_store_witness?&*retained_store_witness:nullptr);')
  s=replace(s,'  impl_->physics_store_observer_read_only=false;','  impl_->physics_store_observer_retained=false;\n  impl_->physics_store_observer_read_only=false;')
  s=replace(s,'  impl_->physics_store_observer_read_only=true;','  impl_->physics_store_observer_retained=false;\n  impl_->physics_store_observer_read_only=true;')
  needle='void PhysicsLiveWorldTu15::observe_body_velocity_inputs('
  index=s.index(needle)
  setter='''void PhysicsLiveWorldTu15::observe_physics_retained_stores(SourcePhysicsStoreObserverTu15 observer){Impl::PublicCall active(*impl_);
  impl_->physics_store_observer_read_only=true;
  impl_->physics_store_observer_retained=bool(observer);
  if(!observer){impl_->physics_store_observer={};return;}
  const NativeTu15RetainedStoreObserver retained{[owner=impl_.get(),observer=std::move(observer)](const auto&store){
    observer(owner->last_instruction,store);
  }};
  impl_->physics_store_observer=[retained](const auto&store){retained.callback()(store);};
}
'''
  return s[:index]+setter+s[index:]
 edit('src/physics_live_world_tu15.cpp',live)
 for name in ('include/halo2/physics_live_world_tu15.hpp','include/halo2/multiplayer_object_runtime.hpp'):
  edit(name,lambda s:replace(s,'    void observe_physics_stores_read_only(SourcePhysicsStoreObserverTu15);','''    void observe_physics_stores_read_only(SourcePhysicsStoreObserverTu15);
    // Explicit read-only retained-DATA observation: private workspace events
    // may be omitted. Callbacks preserve source state and cannot reenter;
    // instrumentation runs under protected registration-time host FP policy.
    void observe_physics_retained_stores(SourcePhysicsStoreObserverTu15);'''),False)
 def runtime(s):
  needle='void MultiplayerObjectRuntime::observe_phantom_phase(';index=s.index(needle)
  return s[:index]+'''void MultiplayerObjectRuntime::observe_physics_retained_stores(SourcePhysicsStoreObserverTu15 observer){Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Retained physics observation requires its TU1.5 live world");
  tu15_->observe_physics_retained_stores(std::move(observer));
}
'''+s[index:]
 edit('src/multiplayer_object_runtime.cpp',runtime,False)
 edit('tests/physics_multiplayer_acceptance.cpp',lambda s:replace(s,'world.source_objects->observe_physics_stores_read_only([&](std::uint32_t actual_instruction,const SourcePhysicsContactRecordStore &store)', 'world.source_objects->observe_physics_retained_stores([&](std::uint32_t actual_instruction,const SourcePhysicsContactRecordStore &store)'))
 # Import only frozen observer and mechanical writer sources; arithmetic is
 # based on c58 and all other0bf source pins, never external current solver.
 interface=ROOT/'analysis/physics/native-simplex-retained-observation-interface-qualified-v3-20261102/source'
 writers=ROOT/'analysis/physics/native-moving-shape-writer-observation-stage-20261102/staged'
 base_manifest=json.loads((ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-broad-phase-edges-original-v1480-20261102/source-manifest.json').read_text())['source_files_sha256']
 for folder in (interface,writers):
  for source in folder.rglob('*'):
   if not source.is_file():continue
   name=str(source.relative_to(folder));target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(source.read_bytes());before[name]=base_manifest.get(name);after[name]=sha(target)
 # Add two truthful retained-capability counters without weakening existing gate.
 def stat(s):return replace(s,'native_moving_process_resource_refusals{};','native_moving_process_resource_refusals{},native_moving_process_retained_observer_calls{},native_moving_process_retained_observer_events{};')
 edit('include/halo2/native_tu15_world_integration.hpp',stat)
 names=('retained_observer_calls','retained_observer_events')
 edit('include/halo2/native_tu15_physics_frame_residuals.hpp',lambda s:replace(s,'  std::map<std::uint32_t,std::uint64_t> native_moving_process_refused_owners;','  std::uint64_t native_moving_process_retained_observer_calls{},native_moving_process_retained_observer_events{};\n  std::map<std::uint32_t,std::uint64_t> native_moving_process_refused_owners;'))
 edit('src/physics_live_world_tu15.cpp',lambda s:replace(s,'    result.native_moving_process_attempts=world_integration.native_moving_process_dispatches;', '    result.native_moving_process_attempts=world_integration.native_moving_process_dispatches;\n'+''.join('    result.native_moving_process_'+n+'=world_integration.native_moving_process_'+n+';\n'for n in names).rstrip()))
 def acceptance(s):
  anchor='        {"native_moving_process_attempts",native_frames.native_moving_process_attempts},';extra=''.join('\n        {"native_moving_process_'+n+'",native_frames.native_moving_process_'+n+'},'for n in names);s=replace(s,anchor,anchor+extra,2)
  return replace(s,'    const bool moving=native_frames.native_moving_process_calls>0&&','    const bool moving=(!std::getenv("HALO2_TU15_PHANTOM_TRACE")||\n      (native_frames.native_moving_process_retained_observer_calls==native_frames.native_moving_process_calls&&\n       native_frames.native_moving_process_retained_observer_events>0))&&native_frames.native_moving_process_calls>0&&')
 edit('tests/physics_multiplayer_acceptance.cpp',acceptance)
 (OUT/'retained-world.diff').write_text(''.join(diffs))
 fields=dict(parent['fields']);fields.update({n:'native_moving_process_'+n for n in names});r=dict(parent);r.update(qualified=False,source_sha256=after,base_hashes=before,fields=fields,scope='Explicit retained Host registration and exact memory+raw-instance World witness; no read-only inference or arbitrary raw observer drop. Pending unionproofs/context/model/peer/live.',parent_stage_sha256=sha(PARENT/'source-pins.json'));(OUT/'source-pins.json').write_text(json.dumps(r,indent=2)+'\n');print(len(after),sha(OUT/'source-pins.json'))
if __name__=='__main__':main()
