#!/usr/bin/env python3
"""Additive Host-key lifetime interface; freeze the qualified numeric source."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';name='src/physics/native_tu15_instanced_ray_callbacks.cpp';before=(ROOT/name).read_text();text=before.replace('#include "halo2/native_tu15_instanced_ray_callbacks.hpp"','#include "halo2/native_tu15_instanced_ray_host_callbacks.hpp"',1)
 anchor='void native_tu15_collect_instanced_scene_ray(SourcePhysicsContactMemory&memory,\n    std::uint32_t collector,std::uint32_t body,NativeTu15PhysicsFrameFp&fp,unsigned depth,';assert text.count(anchor)==1;text=text.replace(anchor,'static void collect(SourcePhysicsContactMemory&memory,\n    std::uint32_t collector,const std::function<std::uint32_t()>&read_key,NativeTu15PhysicsFrameFp&fp,unsigned depth,',1)
 needle='const auto instance=m.word(body+4u),stamp=m.word(0x4f44ecu);';assert text.count(needle)==1;text=text.replace(needle,'const auto instance=read_key(),stamp=m.word(0x4f44ecu);',1)
 text=text.replace('(void)memory;(void)collector;(void)body;(void)fp;(void)depth;(void)services;(void)observe;','(void)memory;(void)collector;(void)read_key;(void)fp;(void)depth;(void)services;(void)observe;')
 wrapper='''void native_tu15_collect_instanced_scene_ray(SourcePhysicsContactMemory&memory,
    std::uint32_t collector,std::uint32_t body,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15InstancedRayCallbackServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedRayCallbackStats*stats){
    collect(memory,collector,[&]{joint_constraint_numeric::Memory m(memory,observe);return m.word(body+4u);},fp,depth,services,observe,stats);
}
void native_tu15_collect_instanced_scene_ray_key(SourcePhysicsContactMemory&memory,
    std::uint32_t collector,std::uint32_t key,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15InstancedRayCallbackServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedRayCallbackStats*stats){
    collect(memory,collector,[key]{return key;},fp,depth,services,observe,stats);
}
void native_tu15_forward_instanced_child_ray_key(SourcePhysicsContactMemory&memory,
    std::uint32_t key,std::uint32_t collector,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15InstancedRayHostForwardServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedRayCallbackStats*stats){
    if(stats)++stats->forward_calls;
    try{joint_constraint_numeric::Memory m(memory,observe);
        [[maybe_unused]]const auto unconsumed_fraction=m.word(0x46a0d0u);
        const auto table=m.word(collector),owner=m.word(table);
        if(!services.collect_key){if(stats)++stats->unbound_collect;throw std::runtime_error("Native instanced Host forward requires actual collector owner");}
        services.collect_key(owner,collector,key,fp);
    }catch(...){if(stats)++stats->refusals;throw;}
}
'''
 anchor='} // namespace halo2';assert text.count(anchor)==1;text=text.replace(anchor,wrapper+anchor)
 target=stage/name;target.parent.mkdir(parents=True);target.write_text(text);new='include/halo2/native_tu15_instanced_ray_host_callbacks.hpp';target=stage/new;target.parent.mkdir(parents=True);shutil.copyfile(ROOT/new,target)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),text.splitlines(True),fromfile='frozen/'+name,tofile='private/'+name)))
 pins=dict(qualified=False,root_reviewed=False,base_hashes={name:sha(ROOT/name)},staged_hashes={name:sha(stage/name),new:sha(stage/new)},scope='Sole numeric collector algorithm plus Host raw-key getter. Numeric fullread/alias path unchanged. Known24DEF0→24DF20 private unconsumed hitword is native DATA; no PC/GP/rawprivate serialization. Every native consumer must use same guardedmemory.',remaining=['Numeric public87 and453 returnedchild composition requalification onexactoverlay','Hostkey productionhelper fullSourceparent/data/FP/lifetime proof andunknownownerrefusals','Independentpeer andWorld/Look liveintegration'])
 (a.output/'source-pins.json').write_text(json.dumps(pins,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
