#!/usr/bin/env python3
"""Guarded source-only 2EA380 World binding over exact frozen MP source."""
from pathlib import Path
import argparse,difflib,hashlib,json,shutil
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals',**{x:'edges.'+x for x in ('objects','shifted_edges','created_pairs','removed_pairs','special_additions','special_removals','array_growths')}}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();base=a.source_root.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);stage=out/'staged';before={};after={};diffs=[]
 def edit(name,fn):
  q=base/name;old=q.read_text();new=fn(old);assert new!=old,name;target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new);before[name]=sha(q);after[name]=sha(target);diffs.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+name,tofile='b/'+name))
 def replace(s,x,y,count=1):assert s.count(x)==count,(x[:100],s.count(x),count);return s.replace(x,y)
 component=ROOT/'analysis/physics/native-broad-phase-edges-qualified-20261102';manifest=json.loads((component/'source-pins.json').read_text());assert manifest['qualified']
 for name in ('include/halo2/native_tu15_broad_phase_edges.hpp','src/physics/native_tu15_broad_phase_edges.cpp'):
  q=component/'source'/name;assert sha(q)==manifest['source_sha256'][name];target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(q,target);before[name]=None;assert not(base/name).exists(),name;after[name]=sha(target)
 name='include/halo2/native_tu15_broad_phase_edges_binding.hpp';target=stage/name;assert not(base/name).exists();target.write_text('''#pragma once
#include "halo2/native_tu15_broad_phase_edges.hpp"
namespace halo2 {
struct NativeTu15BroadPhaseEdgeBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15BroadPhaseEdgeActivity edges;
};
// Sole semantic sweep and sole existing native array-growth service closure.
// The caller selects authenticated 2EA380; no CPU/executor callback is accepted.
inline void native_tu15_update_bound_broad_phase_edges(
    SourcePhysicsContactMemory&memory,const NativeTu15BroadPhaseEdgeUpdate&input,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&storage,
    const SourcePhysicsContactRecordObserver&observer={},
    NativeTu15BroadPhaseEdgeBindingActivity*activity=nullptr) {
    if(activity)++activity->attempts;
    try {
        native_tu15_update_broad_phase_edges(memory,input,fp,storage,observer,
            activity?&activity->edges:nullptr);
        if(activity)++activity->completed;
    } catch(...) {if(activity)++activity->refusals;throw;}
}
} // namespace halo2
''');before[name]=None;after[name]=sha(target)
 def world(s):
  old='''  broad_phase.sweep=[&](auto dispatcher,auto owner,auto entries,auto bounds,auto count,auto first,auto second,auto&){
    const std::array args{entries,bounds,count,first.identity,second.identity};call(owner.identity,args,dispatcher.identity,20);
  };
''';s=replace(s,old,'');anchor='  // Broad-phase and contact-buffer slow paths use the same qualified arena\n';new='''  broad_phase.sweep=[&](auto dispatcher,auto owner,auto entries,auto bounds,auto count,auto first,auto second,auto&context){
    if(owner.identity==0x2ea380u) {
      native_tu15_update_bound_broad_phase_edges(m,{dispatcher.identity,entries,bounds,count,
        {first.identity},{second.identity}},context,pair_storage,observe,
        stats?&stats->broad_phase_edges:nullptr);
    } else {
      const std::array args{entries,bounds,count,first.identity,second.identity};
      call(owner.identity,args,dispatcher.identity,20);
    }
  };
''';return replace(s,anchor,new+anchor)
 edit('src/physics/native_tu15_world_integration_residuals.cpp',world)
 edit('include/halo2/native_tu15_world_integration.hpp',lambda s:replace(replace(s,'#include <map>','#include "halo2/native_tu15_broad_phase_edges_binding.hpp"\n#include <map>'),'  std::uint64_t native_broad_phase_calls{};','  std::uint64_t native_broad_phase_calls{};\n  NativeTu15BroadPhaseEdgeBindingActivity broad_phase_edges;'))
 declaration='  std::uint64_t '+','.join('native_broad_phase_edge_'+n+'{}'for n in FIELDS)+';\n'
 edit('include/halo2/native_tu15_physics_frame_residuals.hpp',lambda s:replace(s,'  std::uint64_t native_broad_phase_calls{};','  std::uint64_t native_broad_phase_calls{};\n'+declaration))
 getter=''.join('    result.native_broad_phase_edge_'+n+'=world_integration.broad_phase_edges.'+path+';\n'for n,path in FIELDS.items())
 edit('src/physics_live_world_tu15.cpp',lambda s:replace(s,'    const auto&world_integration=impl_->native_physics_integrations;','    const auto&world_integration=impl_->native_physics_integrations;\n'+getter))
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_BROAD_PHASE_EDGES")) {
    const bool native_edges=native_frames.native_broad_phase_edge_calls>0&&
      native_frames.native_broad_phase_edge_attempts==native_frames.native_broad_phase_edge_calls&&
      native_frames.native_broad_phase_edge_objects>0&&native_frames.native_broad_phase_edge_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x2ea380u);
    report["native_broad_phase_edges_gate"]=native_edges;passed&=native_edges;
  }
'''
 def accept(s):
  anchor='        {"native_broad_phase_calls",native_frames.native_broad_phase_calls},';extra=''.join('        {"native_broad_phase_edge_'+n+'",native_frames.native_broad_phase_edge_'+n+'},\n'for n in FIELDS);s=replace(s,anchor,anchor+'\n'+extra.rstrip(),2)
  return replace(s,'  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_CONSUMER")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_CONSUMER")) {')
 edit('tests/physics_multiplayer_acceptance.cpp',accept)
 edit('cmake/PhysicsLiveWorldTu15.cmake',lambda s:s+'\n# Sole semantic 2EA380 broad-phase edge reorder; reuse existing array/pool services.\ntarget_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_broad_phase_edges.cpp)\n')
 # Real World service routing prefix; no old sweep callback or invented result.
 model='''  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    const auto broad=world+0x5d40u,nodes=world+0xc000u;
    m.put(world+0x6c14u,0x2ea380u);
    for(unsigned lane=0;lane<4;++lane){m.put(broad+0x10u+4u*lane,0);m.put(broad+0x20u+4u*lane,0);m.put(broad+0x30u+4u*lane,0x3f800000u);m.put(0x41c680u+4u*lane,0x47800000u);m.put(0x41c690u+4u*lane,0);m.put(0x41c6a0u+4u*lane,0x477ffc00u);}
    m.put(broad+0x40u,nodes);m.put(broad+0x78u,world+0xc800u);
    constexpr std::array<unsigned,3>low{8,0,2},high{10,4,6};
    for(unsigned axis=0;axis<3;++axis){const auto edges=world+0xc100u+axis*0x100u;m.put(broad+std::array<unsigned,3>{0x4c,0x58,0x64}[axis],edges);m.put(edges,0);m.put(edges+9u*4u,0xffffu);
      for(unsigned n=0;n<4;++n){m.put(edges+(n+1u)*4u,(n+1u)<<16u);m.put(edges+(n+5u)*4u,((n+1u)<<16u)|1u);m.put(nodes+(n+1u)*16u+low[axis],n+1u,2);m.put(nodes+(n+1u)*16u+high[axis],n+5u,2);m.put(nodes+(n+1u)*16u+12u,body0+n*0x100u+0x1cu);m.put(body0+n*0x100u+0x1cu,n+1u);}}
    struct SweepPrefixStop{};bool stopped=false;
    NativeTu15PairConsumerInputObserverBinding boundary(m,[&](const auto&,const auto&input){if(input.phase==NativeTu15PairConsumerInputPhase::Entry)throw SweepPrefixStop{};});
    try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});}
    catch(const SweepPrefixStop&){stopped=true;}
    check(stopped&&stats.broad_phase_edges.attempts==1&&stats.broad_phase_edges.completed==1&&stats.broad_phase_edges.refusals==0&&stats.broad_phase_edges.edges.objects==4&&stats.broad_phase_edges.edges.array_growths==0&&std::count(calls.begin(),calls.end(),dispatcherSweep)==0&&!stats.residual_owner_calls.contains(0x2ea380u),"World sweep bypassed semantic2EA380 or supplied an old result");
  }
'''
 edit('tests/native_tu15_world_residuals_tests.cpp',lambda s:replace(s,'  std::cout<<"World residual model:',model+'  std::cout<<"World residual model:'))
 # Model dependencies are frozen source, not current canonical Root imports.
 helpers=['tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_world_island_regroup_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']
 inherited={}
 for name in helpers:
  p=base/name
  if p.exists():q=stage/name;q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q);inherited[name]=sha(q)
 (out/'native-broad-phase-edges-world.diff').write_text(''.join(diffs))
 r=dict(qualified=False,root_reviewed=False,base_source_root=str(base),base_hashes=before,source_sha256=after,inherited_source_sha256=inherited,component_manifest=str(component/'source-pins.json'),component_manifest_sha256=sha(component/'source-pins.json'),new_gate='HALO2_REQUIRE_NATIVE_BROAD_PHASE_EDGES',fields=FIELDS,remaining=['Coherent World models','Counter/getter/twoJSON/gate proofs','Bound helper physical requalification','Actual current sweep/live strict'],no_root_source_changes=True)
 (out/'source-pins.json').write_text(json.dumps(r,indent=2)+'\n');print(sha(out/'source-pins.json'))
if __name__=='__main__':main()
