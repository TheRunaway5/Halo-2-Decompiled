#!/usr/bin/env python3
"""Private additive Host Vec4/active API using the sole qualified algorithm."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';name='src/physics/native_tu15_linear_jacobian_emitter.cpp';before=(ROOT/name).read_text();assert sha(ROOT/name)=='665e36a2f3a752526d04e59513b2e9badf6a506cbe5b6e9bc146436c63cc7ea0';text=before.replace('#include "halo2/native_tu15_linear_jacobian_emitter.hpp"','#include "halo2/native_tu15_linear_jacobian_host.hpp"',1)
 start=text.index('        const auto first=m.word(input)');end=text.index('\n#else',start);body=text[start:end];body=body.replace('m.vector(first_point)','point(true)').replace('m.vector(second_point)','point(false)')
 at=text.index('\n#endif\n}\nvoid native_tu15_emit_world_linear_jacobians(')
 shared='''
template<class Point>void emit(Memory&m,std::uint32_t input,std::uint32_t packet,
    Point&&point,NativeTu15LinearJacobianEmitterStats*stats){
'''+body+'''
}
'''
 text=text[:at]+shared+text[at:];start=text.index('        const auto first=m.word(input)',text.index('void native_tu15_emit_world_linear_jacobians('));end=text.index('\n#else',start);text=text[:start]+'''        emit(m,input,packet,[&](bool first){return m.vector(first?first_point:second_point);},stats);'''+text[end:]
 at=text.rfind('\n}')
 active='''
void native_tu15_emit_world_linear_jacobians_active(SourcePhysicsContactMemory&memory,
    const NativeTu15ConstraintVector4&first_point,const NativeTu15ConstraintVector4&second_point,
    std::uint32_t input,std::uint32_t packet,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15LinearJacobianEmitterStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth>6u){if(stats)++stats->unsupported_fp_profiles;throw std::runtime_error("Native active linear Jacobian requires x87 capacity owner");}
        Memory m(memory,observe,stats);
        emit(m,input,packet,[&](bool first){V v;const auto&point=first?first_point:second_point;std::memcpy(&v,point.data(),16);return v;},stats);
#else
        (void)memory;(void)first_point;(void)second_point;(void)input;(void)packet;(void)depth;(void)observe;throw std::runtime_error("Exact native active linear Jacobian backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
'''
 text=text[:at]+active+text[at:];target=stage/name;target.parent.mkdir(parents=True);target.write_text(text);new='include/halo2/native_tu15_linear_jacobian_host.hpp';target=stage/new;target.parent.mkdir(parents=True);shutil.copyfile(ROOT/new,target)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),text.splitlines(True),fromfile='frozen/'+name,tofile='private/'+name)))
 pins=dict(qualified=False,root_reviewed=False,base_hashes={name:sha(ROOT/name)},staged_hashes={name:sha(stage/name),new:sha(stage/new)},scope='Sole numeric3294E0 arithmetic+currentmemory algorithm. Numericpoint readers unchanged; Hostpoints areallfour Sourceinitialized transformedlanes; active realhardwareFP belongs to nativeparent, no CPUframe.',remaining=['Exactnumeric214profile requalification onoverlay','HostVec4 definedDATA physicalcomparison andwhole31D930 callerproof','NativeWorldbinding/genuineBefore/liveinput'])
 (a.output/'source-pins.json').write_text(json.dumps(pins,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
