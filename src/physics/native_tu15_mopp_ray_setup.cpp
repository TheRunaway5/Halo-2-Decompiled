#include "halo2/native_tu15_mopp_private_ray_setup.hpp"
#include "halo2/native_tu15_instanced_ray_lifetime.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2 {
namespace {
void byte(SourcePhysicsContactMemory&memory,const SourcePhysicsContactRecordObserver&observe,
    std::uint32_t pc,std::uint32_t address,std::uint8_t value){
    const std::array bytes{std::byte(value)};SourcePhysicsContactRecordStore event{pc,address,{},{}};
    if(observe){event.before.resize(1);memory.read(address,event.before);event.written.assign(bytes.begin(),bytes.end());}
    memory.write(address,bytes);if(observe)observe(event);
}
}
static void prepare(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t collection,std::uint32_t code,
    std::uint32_t ray_input,std::uint32_t query_context,std::uint32_t collector,bool copy_filter_info,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15MoppRaySetupServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MoppRaySetupStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        NativeTu15MoppTraversalVariables variables{};NativeTu15MoppTraversalBounds bounds;
        {
            Environment environment(fp,depth);Memory m(memory,observe);
            push(m.word(0x46a0d0u));m.word(0x30b8a2u,cache+0x64u,collection);
            m.word(0x30b8a8u,cache+0x60u,query_context);m.word(0x30b8aeu,cache+0x5cu,collector);
            m.word(0x30b8b3u,cache+0x58u,0u);const auto fraction=m.word(collector+4u);
            m.word(0x30b8b9u,cache+0x54u,fraction);m.word(0x30b8bfu,cache+0x10u,code);
            const auto scale=m.word(code+12u);asm volatile("fdivs %0"::"m"(scale):"memory","st");
            m.pop_float(0x30b8d2u,cache+0x14u);
            push(m.word(code+12u));const auto translation=m.vector(code);multiply_scalar(m.word(0x46a1bcu));
            const auto start=m.vector(ray_input);auto end_delta=m.vector(ray_input+16u);
            asm volatile("subps %1,%0":"+x"(end_delta):"x"(translation):"memory");
            std::uint32_t variable_scale;asm volatile("fsts %0":"=m"(variable_scale)::"memory");
            auto start_delta=start;asm volatile("subps %1,%0":"+x"(start_delta):"x"(translation):"memory");
            std::uint32_t vector_scale;asm volatile("fstps %0":"=m"(vector_scale)::"memory","st");
            Vector scalar{};std::memcpy(&scalar,&vector_scale,4);const auto packed_scale=lane<0u>(scalar);
            const auto normalized_start=multiply(packed_scale,start_delta);m.vector(0x30b90cu,cache+32u,start);
            const auto current_end=m.vector(ray_input+16u);m.vector(0x30b91bu,cache+48u,current_end);
            if(copy_filter_info){const auto first_meta=m.word(ray_input+32u);m.word(0x30b922u,cache+64u,first_meta);}
            const auto filter=m.word(ray_input+36u);m.word(0x30b928u,cache+68u,filter);
            byte(memory,observe,0x30b92bu,cache+80u,0u);
            const auto normalized_end=multiply(packed_scale,end_delta);
            std::memcpy(bounds.start.data(),&normalized_start,16);std::memcpy(bounds.end.data(),&normalized_end,16);
            variables.fields10_1c[1]=variable_scale;
        }
        // No older x87 temporary crosses the typed child. Publish actual
        // hardware FP through the native scope before its own kernel begins.
        if(stats)++stats->traversal_requests;
        if(!services.traverse){if(stats)++stats->unbound_traversals;throw std::runtime_error("Native MOPP ray requires30AA70 traversal owner");}
        services.traverse(cache,code+0x30u,variables,bounds,fp,depth);if(stats)++stats->completed;
#else
        (void)memory;(void)cache;(void)collection;(void)code;(void)ray_input;(void)query_context;(void)collector;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native MOPP ray setup backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
void native_tu15_prepare_mopp_ray(SourcePhysicsContactMemory&memory,
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
void native_tu15_cast_instanced_ray_shape(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,std::uint32_t ray_input,std::uint32_t query_context,std::uint32_t collector,
    std::uint32_t cache,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15MoppRaySetupServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MoppRaySetupStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(stats)++stats->shape_calls;
    native_tu15_initialize_instanced_ray_cache(memory,cache,observe);if(stats)++stats->cache_initializations;
    joint_constraint_numeric::Memory m(memory,observe);const auto code=m.word(shape+16u),collection=m.word(shape+12u);
    native_tu15_prepare_mopp_ray(memory,cache,collection,code,ray_input,query_context,collector,fp,depth,services,observe,stats);
#else
    (void)memory;(void)shape;(void)ray_input;(void)query_context;(void)collector;(void)cache;(void)fp;(void)depth;(void)services;(void)observe;(void)stats;
    throw std::runtime_error("Exact native MOPP shape cast backend unavailable");
#endif
}
} // namespace halo2
