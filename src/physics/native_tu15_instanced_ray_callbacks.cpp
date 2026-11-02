#include "halo2/native_tu15_instanced_ray_host_callbacks.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2 {
namespace {
void narrow(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o){e.before.resize(width);m.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(e);
}
}
std::uint32_t native_tu15_resolve_instanced_ray_child(SourcePhysicsContactMemory&memory,
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
void native_tu15_forward_instanced_child_ray(SourcePhysicsContactMemory&memory,
    std::uint32_t body,std::uint32_t collector,std::uint32_t packet,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15InstancedRayCallbackServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedRayCallbackStats*stats){
    if(stats)++stats->forward_calls;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    try{joint_constraint_numeric::Memory m(memory,observe);const auto one=m.word(0x46a0d0u),table=m.word(collector);
        m.word(0x24df0eu,packet+20u,one);const auto owner=m.word(table);
        if(!services.collect){if(stats)++stats->unbound_collect;throw std::runtime_error("Native instanced child requires actual collector owner");}
        services.collect(owner,collector,body,packet,fp);
    }catch(...){if(stats)++stats->refusals;throw;}
#else
    (void)memory;(void)body;(void)collector;(void)packet;(void)fp;(void)services;(void)observe;throw std::runtime_error("Native instanced forward backend unavailable");
#endif
}
static void collect(SourcePhysicsContactMemory&memory,
    std::uint32_t collector,const std::function<std::uint32_t()>&read_key,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15InstancedRayCallbackServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedRayCallbackStats*stats){
    if(stats)++stats->collector_calls;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    try{using namespace joint_constraint_numeric;Memory m(memory,observe);const auto instance=read_key(),stamp=m.word(0x4f44ecu);
        const auto index=std::uint32_t(std::int32_t(std::int16_t(instance&0xffffu))),stamp_address=0x4f44f0u+index*4u;
        if(m.word(stamp_address)==stamp){if(stats)++stats->duplicate_instances;return;}
        const auto result=m.word(collector+28u);m.word(0x24df40u,stamp_address,stamp);
        const auto displacement=m.word(collector+20u),origin=m.word(collector+16u),filter=m.word(collector+12u),mask=m.word(collector+8u);
        if(stats)++stats->instance_queries;
        if(!services.query_instance){if(stats)++stats->unbound_query;throw std::runtime_error("Native instanced collector requires16C000 query owner");}
        if(!services.query_instance(instance,mask,filter,origin,displacement,result,fp))return;
        if(stats)++stats->accepted_instances;
        if(m.word(collector+8u)&0x10000000u){m.word(0x24df6cu,collector+4u,0u);narrow(memory,observe,0x24df71u,collector+24u,1u,1u);return;}
        const auto current_result=m.word(collector+28u);const auto current=m.word(collector+4u),candidate=m.word(current_result+4u);
        bool keep;{Environment environment(fp,depth);Vector a{},b{};std::memcpy(&a,&current,4);std::memcpy(&b,&candidate,4);unsigned char below_or_equal;
            asm volatile("comiss %2,%1;setbe %0":"=qm"(below_or_equal):"x"(a),"x"(b):"cc","memory");keep=below_or_equal!=0;}
        if(keep){const auto value=m.word(collector+4u);m.word(0x24dfa0u,collector+4u,value);narrow(memory,observe,0x24dfa5u,collector+24u,1u,1u);}
        else{const auto value=m.word(current_result+4u);m.word(0x24df8du,collector+4u,value);narrow(memory,observe,0x24df92u,collector+24u,1u,1u);}
    }catch(...){if(stats)++stats->refusals;throw;}
#else
    (void)memory;(void)collector;(void)read_key;(void)fp;(void)depth;(void)services;(void)observe;throw std::runtime_error("Native instanced collector backend unavailable");
#endif
}
void native_tu15_collect_instanced_scene_ray(SourcePhysicsContactMemory&memory,
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
} // namespace halo2
