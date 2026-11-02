#include "halo2/native_tu15_instanced_scene_ray.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2 {
namespace {
void narrow(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width){
    std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore e{pc,p,{},{}};
    if(o){e.before.resize(width);m.read(p,e.before);e.written.assign(bytes.begin(),bytes.begin()+width);}
    m.write(p,std::span(bytes).first(width));if(o)o(e);
}
}
bool native_tu15_query_instanced_scene_ray(SourcePhysicsContactMemory&memory,
    const NativeTu15InstancedSceneRayRequest&q,NativeTu15InstancedSceneRayScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15InstancedSceneRayServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedSceneRayStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        Memory m(memory,observe);const auto scenario=m.word(0x4ecc18u);
        const auto clusters=m.word(scenario+0xa0u),cluster=clusters+q.cluster*0xb0u;
        if(std::int32_t(m.word(cluster+0xa8u))<=0){if(stats){++stats->empty_clusters;++stats->completed;}return false;}
        if(stats)++stats->positive_clusters;
        if((scratch.identity&15u)||scratch.identity>0xfffffe60u)
            throw std::runtime_error("Native instanced scene ray requires distinct aligned DATA");
        const auto collection=scratch.identity,shape=collection+0x20u,collector=collection+0x40u,
            body=collection+0x60u,transform=collection+0x90u,ray=collection+0x100u,cache=collection+0x130u;
        m.word(0x24dcc3u,collection+16u,cluster);const auto code=m.word(cluster+0xacu);
        m.word(0x24dccdu,collection+12u,scenario);
        narrow(memory,observe,0x24dcdbu,collection+6u,1u,2u);
        m.word(0x24dce2u,collection+8u,0u);m.word(0x24dce6u,collection,0x466284u);
        native_tu15_initialize_instanced_ray_shape(memory,shape,collection,code,observe);
        if(stats)++stats->shape_initializations;
        {
            Environment environment(fp,depth);
            const auto scalar=[](std::uint32_t bits){Vector v{};std::memcpy(&v,&bits,4);return v;};
            const auto large_w=m.word(0x46a520u);auto origin_x=m.word(q.origin);
            auto endpoint_y=scalar(m.word(q.displacement+4u));
            auto endpoint_z=scalar(m.word(q.displacement+8u));
            endpoint_y=add_scalar(endpoint_y,scalar(m.word(q.origin+4u)));
            endpoint_z=add_scalar(endpoint_z,scalar(m.word(q.origin+8u)));
            const Vector zero=_mm_setzero_ps();
            m.word(0x24dd20u,body+8u,transform);
            m.vector(0x24dd27u,transform,zero);m.word(0x24dd2fu,transform+12u,large_w);
            m.vector(0x24dd38u,transform+16u,zero);m.word(0x24dd40u,transform+28u,large_w);
            const auto fraction=m.word(q.result+4u);
            m.word(0x24dd52u,body,shape);m.word(0x24dd59u,collector+28u,q.result);
            m.word(0x24dd5du,ray,origin_x);const auto origin_y=m.word(q.origin+4u);
            m.word(0x24dd6bu,collector+8u,q.mask);m.word(0x24dd6fu,collector+4u,fraction);
            auto endpoint_x=scalar(m.word(q.origin));endpoint_x=add_scalar(endpoint_x,scalar(m.word(q.displacement)));
            m.word(0x24dd82u,ray+4u,origin_y);const auto origin_z=m.word(q.origin+8u);
            m.word(0x24dd94u,collector+12u,q.bsp_filter);m.word(0x24dda0u,ray+8u,origin_z);
            m.word(0x24ddacu,ray+16u,scalar_bits(endpoint_x));const auto one=m.word(0x46a0d0u);
            m.vector(0x24ddbdu,transform+32u,zero);m.vector(0x24ddc5u,transform+48u,zero);
            m.vector(0x24ddcdu,transform+64u,zero);
            m.word(0x24dddau,ray+36u,0u);m.word(0x24dde1u,body+12u,0u);
            m.word(0x24dde5u,body+4u,0xffffffffu);m.word(0x24ddedu,body+16u,0u);
            m.word(0x24ddf1u,body+24u,0u);m.word(0x24ddf5u,body+20u,0u);
            m.word(0x24ddf9u,body+28u,0u);m.word(0x24ddfdu,body+32u,0u);
            m.word(0x24de01u,collector,0x4662c0u);m.word(0x24de09u,collector+16u,q.origin);
            m.word(0x24de0du,collector+20u,q.displacement);narrow(memory,observe,0x24de11u,collector+24u,0u,1u);
            narrow(memory,observe,0x24de15u,collection+4u,0u,2u);
            narrow(memory,observe,0x24de1au,shape+4u,0u,2u);
            m.word(0x24de1fu,ray+12u,0u);m.word(0x24de28u,ray+20u,scalar_bits(endpoint_y));
            m.word(0x24de31u,ray+24u,scalar_bits(endpoint_z));m.word(0x24de3au,ray+28u,0u);
            m.word(0x24de43u,transform+32u,one);m.word(0x24de4cu,transform+52u,one);
            m.word(0x24de55u,transform+72u,one);m.vector(0x24de5eu,transform+80u,zero);
        }
        native_tu15_initialize_instanced_ray_cache(memory,cache,observe);
        const auto current_code=m.word(shape+16u),current_collection=m.word(shape+12u);
        if(stats)++stats->shape_casts;
        native_tu15_prepare_private_mopp_ray(memory,cache,current_collection,current_code,ray,body,
            collector,fp,depth,services.traversal,observe,stats?&stats->setup:nullptr);
        const auto hit=m.word(collector+24u,1u)!=0u;
        m.word(0x24de7au,collector,0x45f8b8u);
        native_tu15_destroy_instanced_ray_shape(memory,shape,fp,services.lifetime,observe);
        if(stats)++stats->shape_destructions;
        const auto flags=m.word(collection+4u);m.word(0x24de8du,collection,0x45eddcu);
        if(flags&0x8000u){
            const auto current=m.word(collection+5u,1u);
            narrow(memory,observe,0x24de97u,collection+5u,current&0x7fu,1u);
            if(!services.lifetime.allocation_mode){if(stats)++stats->unbound_allocation_modes;
                throw std::runtime_error("Native instanced scene ray requires collection allocation-mode owner");}
            services.lifetime.allocation_mode(collection,0u,fp);
        }
        if(stats){++stats->completed;stats->hits+=hit;}return hit;
#else
        (void)memory;(void)q;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native instanced scene ray backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
