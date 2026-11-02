#pragma once
#include "halo2/native_tu15_wrapped_contact_agent.hpp"
#include "halo2/native_tu15_convex_agent_factory.hpp"
#include "halo2/native_tu15_contact_shape.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include <limits>
#include <stdexcept>
#include <string>
namespace halo2 {
struct NativeTu15WrappedFactoryBindingScratch {std::uint32_t first{},bytes{};};
struct NativeTu15WrappedFactoryBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},wrapped_calls{},transformed_calls{},
        convex_calls{},class_queries{},unqualified_owners{};
};
// Typed composition for source2F0EB0/2F1CC0/2F7190 and their actual native convex
// descendants. Allocation is an explicit native service owned by the caller.
// Each recursive constructor owns a disjoint512B DATA slot until its child
// returns. No source control stack, GP/SIMD file or output pointer is supplied.
inline NativeTu15ContactAgent native_tu15_create_bound_wrapped_contact_factory(
    SourcePhysicsContactMemory&memory,NativeTu15ContactVirtualOwner owner,
    const NativeTu15WrappedContactArguments&arguments,
    NativeTu15WrappedFactoryBindingScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned incoming_x87_depth,const NativeTu15ArrayGrowthServices&allocation,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15WrappedFactoryBindingActivity*activity=nullptr){
    NativeTu15WrappedFactoryBindingActivity local;auto&stats=activity?*activity:local;
    const auto shape_class=[&](auto,auto method,auto&)->std::uint32_t{
        ++stats.class_queries;
        if(method.identity==0xa9520u)return native_tu15_contact_child_class_23();
        if(method.identity==0xab4b0u)return native_tu15_contact_child_class_24();
        if(const auto kind=native_tu15_activation_type(method.identity))return *kind;
        ++stats.unqualified_owners;
        throw std::runtime_error("Native wrapped factory requires qualified class owner="+std::to_string(method.identity));
    };
    std::function<NativeTu15ContactAgent(NativeTu15ContactVirtualOwner,
        const NativeTu15WrappedContactArguments&,unsigned)>create;
    create=[&](auto target,const auto&input,unsigned depth)->NativeTu15ContactAgent{
        ++stats.attempts;
        try{
            if(incoming_x87_depth)throw std::runtime_error("Native wrapped factory requires qualified empty live x87 domain");
            const auto offset=std::uint64_t(depth)*512u;
            if((scratch.first&15u)||offset+512u>scratch.bytes||
                std::uint64_t(scratch.first)+offset+512u>std::numeric_limits<std::uint32_t>::max())
                throw std::runtime_error("Native wrapped factory exhausted its caller-owned DATA arena");
            const auto slot=scratch.first+std::uint32_t(offset);NativeTu15ContactAgent result;
            if(target.identity==0x2f0eb0u||target.identity==0x2f1cc0u||target.identity==0x2f7190u){
                NativeTu15WrappedContactServices services;services.allocation=allocation;
                services.dispatch.classify_shape=shape_class;
                services.dispatch.create_agent=[&](auto child,auto a,auto b,auto settings,auto data,auto&){
                    const NativeTu15WrappedContactArguments next{[&]{return a.identity;},[&]{return b.identity;},[&]{return settings.identity;},[&]{return data;}};
                    return create(child,next,depth+1u);
                };
                if(target.identity==0x2f0eb0u){++stats.wrapped_calls;result=native_tu15_create_bv_contact_agent(memory,input,{slot},fp,services,observe);}
                else if(target.identity==0x2f1cc0u){++stats.wrapped_calls;result=native_tu15_create_wrapped_contact_agent(memory,input,{slot},fp,services,observe);}
                else{++stats.transformed_calls;result=native_tu15_create_transformed_contact_agent(memory,input,{slot,slot+16u},fp,services,observe);}
            }else if(target.identity==0x2e4eb0u){
                ++stats.convex_calls;NativeTu15ConvexAgentServices services;services.allocation=allocation;services.shape_class=shape_class;
                services.cache.shape_center=[&](auto shape,auto method,auto output,auto&){native_tu15_contact_shape_center(memory,shape,method,output,observe);};
                services.cache.support_vertex=[&](auto shape,auto method,auto direction,auto output,auto&state){
                    const NativeTu15ContactShapeScratch child{slot+352u,slot+368u,nullptr};
                    native_tu15_contact_shape_support(memory,shape,method,direction,output,{slot+320u,slot+336u,&child},state,0,observe);
                };
                services.cache.vertex_encoding=[&](auto,auto method,auto&)->std::uint32_t{
                    if(method.identity==0x1cb030u)return native_tu15_contact_shape_vertex_encoding();
                    if(method.identity==0x31d520u||method.identity==0xad740u)
                        if(const auto encoding=native_tu15_activation_type(method.identity))return *encoding;
                    ++stats.unqualified_owners;throw std::runtime_error("Native wrapped factory requires qualified vertex encoding owner="+std::to_string(method.identity));
                };
                const NativeTu15ConvexAgentArguments next{input.first,input.second,input.user_data};
                result=native_tu15_create_convex_contact_agent(memory,next,
                    {slot+128u,{slot+192u}},fp,services,observe);
            }else{
                ++stats.unqualified_owners;throw std::runtime_error("Native wrapped factory requires qualified child factory owner="+std::to_string(target.identity));
            }
            ++stats.completed;return result;
        }catch(...){++stats.refusals;throw;}
    };
    return create(owner,arguments,0);
}
} // namespace halo2
