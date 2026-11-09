#!/usr/bin/env python3
"""Freeze the typed native304350 provider binding over its exact source union."""
import argparse,hashlib,json,shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/physics/native-contact-producer-actual1479-stage-v2-20261102'
WRITERS=ROOT/'analysis/physics/native-moving-shape-writer-observation-stage-20261102/staged'

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def replace(text,old,new):
    assert text.count(old)==1,(old,text.count(old))
    return text.replace(old,new)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    before=json.loads((BASE/'source-pins.json').read_text())
    assert all(sha(BASE/'staged'/n)==row['sha256']for n,row in before['files'].items())
    stage=a.output/'staged';shutil.copytree(BASE/'staged',stage)
    text=(ROOT/'tools/native_contact_producer_actual_graph_tu15.hpp').read_text()
    text=replace(text,'namespace {\nusing namespace halo2;\nconstexpr std::uint32_t PARENT_DATA=0x74000000u;','namespace halo2 {\nstruct NativeTu15ContactProducerBindingScratch {std::uint32_t identity{},bytes{};};\nstruct NativeTu15ContactProducerNativeServices {\n std::function<std::uint64_t()>timestamp_counter;\n std::function<void(SourcePhysicsContactMemory&,std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)>activate_island;\n NativeTu15ContactCreationGameplayServices gameplay;\n};')
    text=text.replace('ParentActivities','NativeTu15ContactProducerBindingActivity')
    text=replace(text,'struct NativeTu15ContactProducerBindingActivity {','struct NativeTu15ContactProducerBindingActivity {\n std::uint64_t attempts{},completed{},refusals{},resource_refusals{},depth_refusals{},fp_refusals{},unprovided_reads{};')
    begin=text.index('void run_native_parent(');end=text.index(' const auto unbound=',begin)
    text=text[:begin]+'''// Same native kernels and caller DATA aliases as the genuine selected304 proof.
// One owned64KiB availability lifetime spans candidates, queries, constructor,
// records and publications. No original executor or CPU/control image is input.
inline void native_tu15_produce_bound_contacts(SourcePhysicsContactMemory&storage,
 NativeTu15ContactProducer producer,NativeTu15ContactPayload first,
 NativeTu15ContactPayload second,NativeTu15ContactContext configuration,
 NativeTu15ContactOutput output,NativeTu15ContactProducerBindingScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ContactProducerNativeServices&host,
 NativeTu15PairDeletionNativeScope&scope,NativeTu15ContactProducerBindingActivity*activity=nullptr,
 const NativeTu15RetainedStoreObserver&retained_observer={}){
 NativeTu15ContactProducerBindingActivity local;auto&stats=activity?*activity:local;
 ++stats.attempts;NativeTu15PairDeletionNativeScopeGuard all_native(scope,&stats.providers.native_only);
 try{
 const auto p=scratch.identity;
 if(!p||(p&15u)||scratch.bytes<0x10000u||std::uint64_t(p)+0x10000u>0x100000000ull){
  ++stats.resource_refusals;throw std::runtime_error("Native producer requires owned aligned64KiB DATA");}
 if(depth){++stats.depth_refusals;throw std::runtime_error("Native producer requires qualified empty live x87");}
 if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
  (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
  ++stats.fp_refusals;throw std::runtime_error("Native producer requires its qualified masked FP domain");}
 const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x10000u}}};
 const std::array<NativeTu15ContactPrivateRange,5>retained{{
  {producer.identity,0x40u},{first.identity,0x28u},{second.identity,0x28u},
  {configuration.identity,0x40u},{output.identity,0x10u}}};
 NativeTu15ContactCandidatePrivateData private_data(storage,owned,retained);
 NativeTu15ConvexSimplexMemory m(private_data,retained_observer);
 m.withhold(p,0x10000u);
 struct AvailabilityActivity {const NativeTu15ConvexSimplexMemory&memory;NativeTu15ContactProducerBindingActivity&stats;
  ~AvailabilityActivity(){stats.unprovided_reads+=memory.refused_reads();}} availability{m,stats};
 const SourcePhysicsContactRecordObserver empty_observer;
 const auto&o=native_tu15_effective_store_observer(m,empty_observer);
''' +text[end:]
    text=text.replace(' const auto p=PARENT_DATA;\n','')
    # Relocate every ordinary DATA location; the binding accepts no fixed host
    # address. Preserve authenticated candidate/query/parent offsets370/400.
    import re
    text=re.sub(r'0x7400([0-9a-f]{4})u',lambda m:'p+0x'+m[1]+'u',text)
    text=replace(text,' NativeTu15ProducerNativeProviders native;native.owner_memory=&m;native.candidates=providers;',' NativeTu15ProducerNativeProviders native;native.owner_memory=&m;native.candidates=providers;native.timestamp_counter=host.timestamp_counter;')
    text=replace(text,'},support},0,{},&stats.solver);','},support},0,host.timestamp_counter,&stats.solver);')
    text=replace(text,' published.creation.allocation=',' published.creation.activate_island=[&](auto&view,auto world,auto island,auto&current){\n  if(!host.activate_island)unbound(0x316d40u);host.activate_island(view,world,island,current);\n };\n published.creation.allocation=')
    text=replace(text,'  native_tu15_dispatch_contact_creation_gameplay_listener(view,listener,owner,event,','  m.require_retained_extent(listener,4u);\n  native_tu15_dispatch_contact_creation_gameplay_listener(view,listener,owner,event,')
    text=replace(text,'current,0,{},&stats.gameplay);','current,0,host.gameplay,&stats.gameplay);')
    text=replace(text,'  (void)native_tu15_append_contact_record(view,array,constructor,current,allocation);','  (void)native_tu15_append_contact_record(view,array,constructor,current,allocation,o);')
    text=replace(text,'   {p+0x8000u,0x8000u},current,0,{},scope,&stats.moving);','   {p+0x8000u,0x8000u},current,0,{host.activate_island,host.gameplay,host.timestamp_counter},scope,&stats.moving,retained_observer);')
    text=replace(text,'m,scratch,0,native,scope,{},&stats.providers','m,provider_scratch,0,native,scope,o,&stats.providers')
    text=replace(text,' const NativeTu15ProducerProviderScratch scratch{',' const NativeTu15ProducerProviderScratch provider_scratch{')
    text=replace(text,' native_tu15_produce_contacts(m,{input.at("agent")},{input.at("first")},{input.at("second")},\n  {input.at("configuration")},{input.at("output")},{p+0x400u,p+0x4a00u},fp,0,services);',' native_tu15_produce_contacts(m,producer,first,second,configuration,output,\n  {p+0x400u,p+0x4a00u},fp,0,services,o);\n ++stats.completed;\n }catch(...){++stats.refusals;throw;}')
    assert 'input.at'not in text and '0x7400'not in text and 'PARENT_DATA'not in text
    target=stage/'include/halo2/native_tu15_contact_producer_binding.hpp';target.write_text(text)
    # The query-v5 arithmetic remains intact. Add only the four independently
    # observed cache SourcePC labels already qualified by the moving helper.
    target=stage/'src/physics/native_tu15_convex_simplex_solver.cpp';solver=target.read_text();writer=(WRITERS/target.name).read_text()if(WRITERS/target.name).is_file()else(WRITERS/'src/physics'/target.name).read_text()
    old=solver[solver.index('void narrow_store('):solver.index('\n}',solver.index('void narrow_store('))+2]
    new=writer[writer.index('void narrow_store('):writer.index('\n}',writer.index('void narrow_store('))+2]
    solver=replace(solver,old,new)
    for field,pc in (('input.support_cache+8u','0x30f68fu'),('input.support_cache+9u','0x30f694u'),('input.support_cache+written*2u','0x30f6a3u'),('target','0x30f6c3u')):
        solver=replace(solver,'narrow_store(memory,'+field+',','narrow_store(memory,'+pc+','+field+',')
    if '#include "halo2/native_tu15_retained_store_observer.hpp"'not in solver:
        solver='#include "halo2/native_tu15_retained_store_observer.hpp"\n'+solver
    target.write_text(solver)
    # These producer-only writers were outside the moving-process writer set.
    # A retained observer may not request an unknown local preimage. Default
    # public memory still emits every same event after the committed write.
    for name,member in (('native_tu15_contact_query.cpp','observer_'),('native_tu15_convex_contact_query.cpp','observe_')):
        target=stage/'src/physics'/name;content=target.read_text()
        old='SourcePhysicsContactRecordStore event{pc,p,{},{}};if('+member+'){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if('+member+')'+member+'(event);'
        new='const bool observed=native_tu15_observes_retained_store(memory_,'+member+',pc,p,b.size());SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observed){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observed)'+member+'(event);'
        target.write_text('#include "halo2/native_tu15_retained_store_observation.hpp"\n'+replace(content,old,new))
    target=stage/'src/physics/native_tu15_contact_pair_eligibility.cpp';content=target.read_text()
    content=replace(content,'    if(observe){event.before.resize(1);','    const bool observed=native_tu15_observes_retained_store(memory,observe,site,result,1u);\n    if(observed){event.before.resize(1);')
    content=replace(content,'memory.write(result,bytes);if(observe)observe(event);','memory.write(result,bytes);if(observed)observe(event);')
    target.write_text('#include "halo2/native_tu15_retained_store_observation.hpp"\n'+content)
    files={str(p.relative_to(stage)):{'sha256':sha(p)}for p in stage.rglob('*')if p.is_file()}
    report=dict(qualified=False,production_applied=False,base_manifest_sha256=sha(BASE/'source-pins.json'),files=files,units=before['units'],preparer_sha256=sha(Path(__file__)),remaining=['Same production-helper genuine original BOTH4FP and typed-observer proof','Admission/lifetime/default/fault and mutation controls','Independent peer','World and visible imported-MCC multiplayer delivery'])
    (a.output/'source-pins.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(dict(files=len(files),source_pins_sha256=sha(a.output/'source-pins.json')))
if __name__=='__main__':main()
