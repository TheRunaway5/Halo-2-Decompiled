#pragma once
#include "halo2/native_tu15_instanced_ray_callbacks.hpp"
#include "physics/native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2::instanced_ray_child_value_reference {
void narrow(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o){e.before.resize(width);m.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(e);
}
std::uint32_t resolve(SourcePhysicsContactMemory&memory,
    std::uint32_t collection,std::uint32_t key,std::uint32_t child,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedRayCallbackStats*stats){
    if(stats)++stats->provider_calls;if(!child){if(stats)++stats->provider_null;return 0;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    try{joint_constraint_numeric::Memory m(memory,observe);const auto scenario=m.word(collection+12u);
        m.word(0x24decbu,child+12u,scenario);narrow(memory,observe,0x24ded2u,child+6u,1u,2u);
        m.word(0x24ded8u,child+8u,0u);m.word(0x24dedfu,child,0x46625cu);
        m.word(0x24dee5u,child+16u,key);return child;
    }catch(...){if(stats)++stats->refusals;throw;}
#else
    (void)memory;(void)collection;(void)key;(void)observe;throw std::runtime_error("Native instanced child backend unavailable");
#endif
}
}
