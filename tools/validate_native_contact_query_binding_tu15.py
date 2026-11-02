#!/usr/bin/env python3
"""Qualify the sole native query binding; the offline30E930 oracle stays explicit."""
import inspect
import validate_native_contact_query_tu15 as parent

def binding_fragment(text):
    text='#include <memory>\n'+text
    old='const auto result=native_tu15_query_contact(memory,{std::uint32_t(entry.rcx)},args[0],{args[1]},{args[2]},{args[3]},args[4],{scratch},fp,0,services,observer,&stats);'
    assert text.count(old)==1
    text=text.replace(old,'''const NativeTu15ContactShapeScratch convex_child{scratch-0xe4u,scratch-0xe0u,nullptr};
 const NativeTu15ContactShapeScratch triangle_child{scratch-0xd4u,scratch-0xd0u,nullptr};
 const NativeTu15ContactQueryBindingScratch bound_scratch{{scratch},{scratch-0x200u,scratch-0x224u},
     {{scratch-0xa0u},{scratch-0xb4u,0,&convex_child}},
     {{scratch-0x90u},{scratch-0xa4u,0,&triangle_child}}};
 NativeTu15ConvexContactServices native_geometry;NativeTu15PairDeletionNativeScope scope;
 native_geometry.solve_simplex=[&](auto simplex,auto a,auto b,auto transform,auto cache,auto plane,auto&state){
     // This is a declared OFFLINE remaining-child oracle, not native closure.
     Bridge solver{native,scratch-0x200u,before_fp,report};
     const auto value=solver.call(0x30e930u,simplex,{a.identity,b.identity,transform,cache,plane},0x2f876au,state);
     for(const auto&call:solver.calls)simplex_calls.push_back(call);return value;
 };
 NativeTu15ContactQueryBindingActivity binding_activity;
 const auto result=native_tu15_query_bound_contact(memory,{std::uint32_t(entry.rcx)},args[0],
     {args[1]},{args[2]},{args[3]},args[4],bound_scratch,fp,0,native_geometry,scope,{observer},&binding_activity);
 stats=binding_activity.query;native_cache_parents=binding_activity.convex_caches+binding_activity.triangle_caches;
 native_convex_parents=binding_activity.convex_queries;''')
    marker=' }catch(const std::exception&e){report["error"]'
    assert text.count(marker)==1
    guards=''' if(report.at("passed").get<bool>()&&variation){
  std::unique_ptr<Child> guard_child;std::unique_ptr<QueryMemory> guard_memory;
  const auto reset_binding=[&]{guard_memory.reset();guard_child=std::make_unique<Child>();guard_child->start(argv[1],ordinal);guard_child->go(0x2e4fe0u);for(const auto&bank:banks)guard_child->write(bank.address,bank.before);guard_memory=std::make_unique<QueryMemory>(*guard_child,private_first,private_end,scratch,args[0],args[4]);fp={before_fp.cwd,before_fp.swd,before_fp.mxcsr};};
  // QueryMemory's write ledger requires an observer pre-read. Keep that test
  // adapter contract for the guard controls as well as the main comparison.
  const auto invoke_binding=[&](unsigned depth,const auto&geometry,auto&activity){return native_tu15_query_bound_contact(*guard_memory,{std::uint32_t(entry.rcx)},args[0],{args[1]},{args[2]},{args[3]},args[4],bound_scratch,fp,depth,geometry,scope,{[](const auto&){}},&activity);};
  reset_binding();NativeTu15ContactQueryBindingActivity missing;bool refused=false;
  try{invoke_binding(0,NativeTu15ConvexContactServices{},missing);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("30E930")!=std::string::npos;}
  report["binding_guard_debug"]={{"missing_refused",refused},{"missing_attempts",missing.attempts},{"missing_completed",missing.completed},{"missing_refusals",missing.refusals},{"missing_convex_queries",missing.convex_queries},{"scope_active",scope.active()}};
  check(refused&&missing.refusals==1&&missing.completed==0&&!scope.active(),"Bound query missing solver refusal/restore differs");
  reset_binding();NativeTu15ContactQueryBindingActivity native_only;auto rejected_geometry=native_geometry;
  rejected_geometry.solve_simplex=[&](auto,auto,auto,auto,auto,auto,auto&)->std::uint32_t{scope.reject_unconverted(0x30e930u);throw std::runtime_error("Unreachable source execution");};refused=false;
  try{invoke_binding(0,rejected_geometry,native_only);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("refuses unconverted")!=std::string::npos;}
  check(refused&&native_only.refusals==1&&native_only.native_only.refused==1&&native_only.native_only.owners.at(0x30e930u)==1&&!scope.active(),"Bound query native-only solver refusal/restore differs");
  reset_binding();NativeTu15ContactQueryBindingActivity depth;refused=false;
  try{invoke_binding(1,native_geometry,depth);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("empty live x87")!=std::string::npos;}
  check(refused&&depth.refusals==1&&guard_memory->ledger.empty()&&!scope.active(),"Bound query nonempty FP pre-DATA refusal differs");
  report["binding_guard_models"]=J::array({"Missing solver refuses without fallback","Native-only solver scope refuses before transport","Nonempty FP refuses before DATA"});
 }
'''
    text=text.replace(marker,guards+marker)
    marker='catch(const std::exception&e){report["error"]'
    assert text.count(marker)==1
    return text.replace(marker,'catch(const std::exception&e){report["passed"]=false;report["error"]')

def main():
    source=inspect.getsource(parent.main)
    marker='try:\n  assert sha(NEW_DEFAULT)'
    assert source.count(marker)==1
    source=source.replace(marker,'try:\n  assert a.native_convex,"Production binding proof requires --native-convex"\n  assert sha(NEW_DEFAULT)')
    source=source.replace('shared+fragment.read_text()', 'shared+binding_fragment(fragment.read_text())')
    marker="  if a.native_convex:\n   for path in"
    assert source.count(marker)==1
    source=source.replace(marker,"  for name in ('include/halo2/native_tu15_contact_query_binding.hpp','src/physics/native_tu15_activation_services.cpp'):\n   pins[name]=sha(ROOT/name)\n"+marker)
    marker="  (project/'CMakeLists.txt').write_text(cmake);results=[]"
    assert source.count(marker)==1
    source=source.replace(marker,"  cmake+='target_sources(dependencies PRIVATE \"'+str(ROOT/'src/physics/native_tu15_activation_services.cpp')+'\")\\n'\n  cmake+='target_compile_options(dependencies PRIVATE -ffunction-sections -fdata-sections)\\n'\n  for mode in ('native',*variants):cmake+='target_link_options(query_'+mode+' PRIVATE -Wl,--gc-sections)\\n'\n"+marker)
    source=source.replace("'#include \"halo2/native_tu15_contact_query.hpp\"\\n'", "'#include \"halo2/native_tu15_contact_query_binding.hpp\"\\n#include \"halo2/native_tu15_contact_query.hpp\"\\n'")
    marker='  unchanged=all('
    assert source.count(marker)==1
    checks='''  for compiler in a.compilers:
   for case in cases:
    retained=a.output/compiler/'evidence/evidence'/('native_'+case['name']+'.json')
    result=json.loads(retained.read_text());assert result['passed']and not result.get('error'),(retained,result.get('error'))
    if case['variant']:assert len(result.get('binding_guard_models',[]))==3,(retained,'Missing explicit guard models')
  report['generated_observer_sha256']=sha(project/'observer.cpp')
'''
    source=source.replace(marker,checks+marker)
    environment=dict(parent.__dict__,__file__=__file__,binding_fragment=binding_fragment)
    exec(source,environment)
    return environment['main']()

if __name__=='__main__':raise SystemExit(main())
