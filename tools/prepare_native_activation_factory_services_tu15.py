#!/usr/bin/env python3
"""Private typed factory option; preserve the existing activation public APIs."""
import argparse,difflib
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 files=['include/halo2/native_tu15_object_activation_residuals.hpp','src/physics/native_tu15_object_activation_residuals.cpp'];before={r:(a.source_root/r).read_text()for r in files}
 h=before[files[0]];h=once(h,'struct NativeTu15ObjectActivationBindingActivity {','''// A native caller can supply its already-qualified recursive factory graph.
// Inputs are ordinary semantic DATA and actual FP/depth, never GP registers.
struct NativeTu15ObjectActivationNativeServices {
    std::function<NativeTu15ContactAgent(NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
        NativeTu15ActivationContactManager,NativeTu15PhysicsFrameFp&,unsigned)> create_agent;
};
struct NativeTu15ObjectActivationBindingActivity {''')
 h=once(h,'    std::uint64_t setters{},native_agent_factories{},','    std::uint64_t native_bound_agent_factory_attempts{},native_bound_agent_factory_calls{},native_bound_agent_factory_refusals{};\n    std::uint64_t setters{},native_agent_factories{},')
 declaration='''// Same316D40 and shared services, with an explicit native factory supplier.
// Existing entry points retain their signatures and original refusal policy.
void native_tu15_activate_island_with_native_services(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,NativeTu15ObjectActivationScratch,
    const NativeTu15ObjectActivationNativeServices&,
    const NativeTu15ObjectActivationResidualCall&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectActivationBindingActivity* = nullptr);
''';h=once(h,'} // namespace halo2',declaration+'} // namespace halo2')
 cpp=before[files[1]];cpp=once(cpp,'const SourcePhysicsContactRecordObserver&observer,NativeTu15ObjectActivationBindingActivity*activity,\n const std::function<void(const NativeTu15IslandActivationServices&)>&use)',
  'const SourcePhysicsContactRecordObserver&observer,NativeTu15ObjectActivationBindingActivity*activity,\n const NativeTu15ObjectActivationNativeServices*native,\n const std::function<void(const NativeTu15IslandActivationServices&)>&use)')
 anchor='        if(activity)++activity->unconverted_agent_factories;'
 callback='''        if(native&&native->create_agent){
            if(activity)++activity->native_bound_agent_factory_attempts;
            try{
                const auto result=native->create_agent(owner,first,second,context,manager,state,depth);
                if(activity){++activity->native_bound_agent_factory_calls;++activity->native_agent_factories;}
                return result;
            }catch(...){if(activity)++activity->native_bound_agent_factory_refusals;throw;}
        }
''';cpp=once(cpp,anchor,callback+anchor)
 old='with_activation_services(memory,depth,scratch,invoke,observer,activity,[&](const auto&activation)'
 assert cpp.count(old)==2;cpp=cpp.replace(old,'with_activation_services(memory,depth,scratch,invoke,observer,activity,nullptr,[&](const auto&activation)')
 definition='''void native_tu15_activate_island_with_native_services(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,NativeTu15ObjectActivationScratch scratch,
 const NativeTu15ObjectActivationNativeServices&native,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){
    with_activation_services(memory,depth,scratch,invoke,observer,activity,&native,[&](const auto&activation){
        native_tu15_activate_physics_island(memory,world,island,fp,activation,observer,
            activity?&activity->activation:nullptr);
    });
}
''';cpp=once(cpp,'} // namespace halo2',definition+'} // namespace halo2')
 for relative,s in zip(files,(h,cpp)):
  target=a.output/'staged'/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(s)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[r].splitlines(True),(a.output/'staged'/r).read_text().splitlines(True),fromfile='a/'+r,tofile='b/'+r))for r in files))
 save(a.output/'source-pins.json',dict(qualified=False,production_applied=False,base_source_root=str(a.source_root),base_hashes={r:sha(a.source_root/r)for r in files},staged_hashes={r:sha(a.output/'staged'/r)for r in files},
  scope='New typed native factory option; old APIs/unknown-owner refusal unchanged. Count callback attempt/completion/refusal separately; increment native factory success after real typed callback returns. No residual counter/oldCPU fallback in native option.',
  remaining=['Actual2F8030 BEFORE direct original factory/Worldcallback proof','Shared316D40 native option interface/oldAPI regression','Worldcallback/gate wiring and coherent models','Visible actual multiplayer/fullframe']))
 print(dict(prepared=True,semantic_files=2));return 0
if __name__=='__main__':raise SystemExit(main())
