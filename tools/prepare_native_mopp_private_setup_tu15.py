#!/usr/bin/env python3
"""Private caller-specialization overlay; keep the frozen public source unchanged."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';name='src/physics/native_tu15_mopp_ray_setup.cpp';base=ROOT/name;before=base.read_text();assert sha(base)=='d27bf4717bafbdd8912e54875df3504c27db03cc9c76f0a4508556b595f15aac';after=before
 after=after.replace('#include "halo2/native_tu15_mopp_ray_setup.hpp"','#include "halo2/native_tu15_mopp_private_ray_setup.hpp"',1)
 after=after.replace('void native_tu15_prepare_mopp_ray(SourcePhysicsContactMemory&memory,','static void prepare(SourcePhysicsContactMemory&memory,',1).replace('std::uint32_t ray_input,std::uint32_t query_context,std::uint32_t collector,\n    NativeTu15PhysicsFrameFp&fp','std::uint32_t ray_input,std::uint32_t query_context,std::uint32_t collector,bool copy_filter_info,\n    NativeTu15PhysicsFrameFp&fp',1)
 anchor='            const auto first_meta=m.word(ray_input+32u);m.word(0x30b922u,cache+64u,first_meta);';assert after.count(anchor)==1;after=after.replace(anchor,'            if(copy_filter_info){const auto first_meta=m.word(ray_input+32u);m.word(0x30b922u,cache+64u,first_meta);}')
 at=after.index('void native_tu15_cast_instanced_ray_shape(')
 wrappers='''void native_tu15_prepare_mopp_ray(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t collection,std::uint32_t code,std::uint32_t ray_input,
    std::uint32_t query_context,std::uint32_t collector,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15MoppRaySetupServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15MoppRaySetupStats*stats){
    prepare(memory,cache,collection,code,ray_input,query_context,collector,true,fp,depth,services,observe,stats);
}
void native_tu15_prepare_private_mopp_ray(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t collection,std::uint32_t code,std::uint32_t ray_input,
    std::uint32_t query_context,std::uint32_t collector,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15MoppPrivateRayServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15MoppPrivateRayActivity*activity){
    NativeTu15MoppPrivateRayActivity local;auto&out=activity?*activity:local;++out.attempts;
    try{
        NativeTu15MoppPrivateRayMemory guarded(memory,ray_input,cache,out);
        NativeTu15MoppRaySetupServices bound;
        if(services.traverse)bound.traverse=[&](auto identity,auto program,const auto&variables,
            const auto&bounds,auto&current,auto older){
            services.traverse(guarded,identity,program,variables,bounds,current,older);
        };
        prepare(guarded,cache,collection,code,ray_input,query_context,collector,false,
            fp,depth,bound,observe,&out.setup);++out.completed;
    }catch(...){++out.refusals;throw;}
}
'''
 after=after[:at]+wrappers+after[at:];target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after)
 new='include/halo2/native_tu15_mopp_private_ray_setup.hpp';target=stage/new;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/new,target)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='frozen/'+name,tofile='private/'+name)))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,peer_review_pending=True,base_hashes={name:sha(base)},staged_hashes={name:sha(stage/name),new:sha(stage/new)},scope='Shared sole arithmetic/control30B890implementation. Exactpublicnumericpath stillcopiesdefinedword.24DC90privatevariant omitsonlyunprovided filter-info copy/read, leavesrawprivateledger unclaimed, ALLnativeconsumers throughsameview guardbothword/overlappingvectorreads andwrites.',remaining=['Publicpath requalification, guard models, concreteactual caller+nativecore allretainedDATA/FP proof','Independentpeer beforeanyWorldadmission; callbackmustnevercaptureunguardedmemory','Sourceactualno-leaf/non-escape lifetime only; no arbitraryobserver/privatebyte-equivalence claim']))
if __name__=='__main__':main()
