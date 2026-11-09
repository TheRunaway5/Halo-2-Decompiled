#pragma once
#include "halo2/native_tu15_world_factory_scratch.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include <string>

// Declared native constructor/enter composition, not a recorded gameplay or
// original-XBE comparison. Memory/check come from the World residual model.
// Pool, property, object and tag inputs follow the physical phantom validator's
// references_1/append cases. Declared pool metadata feeds the genuine native
//149DA0/233250/149E00/233420 algorithms; no allocation outcome is supplied.
inline void native_world_wrapped_phantom_model() {
    constexpr std::uint32_t data=0x2200000u,private_first=0x2300000u;
    constexpr auto allocator=data,allocator_table=data+0x40u;
    constexpr auto component_pool=data+0x100u,object_pool=data+0x200u;
    constexpr auto components=data+0x1000u,objects=data+0x3000u,tags=data+0x4000u;
    constexpr auto self_body=data+0x5000u,other_body=data+0x5100u;
    constexpr auto own_object=data+0x6000u,other_object=data+0x6400u;
    constexpr auto definition=data+0x7000u,model_data=data+0x7100u,physics=data+0x7200u;
    constexpr auto phantoms=data+0x7400u,shape_rows=data+0x7500u;
    constexpr auto header=data+0x7600u,members=data+0x7700u;
    constexpr auto context=data+0x8000u,dispatch=data+0x8100u;
    constexpr auto wrapper_table=data+0xa000u,phantom_table=data+0xa040u,convex_table=data+0xa080u;
    constexpr auto shapes=data+0xb000u,self_root=data+0xb400u,other_root=data+0xb440u;
    constexpr std::uint32_t user=0x76543210u,pool=0x2400000u,pool_table=data+0xa100u;
    for(unsigned depth:{1u,3u})for(bool reversed:{false,true})for(unsigned failure=0;failure<4;++failure) {
        Memory m;m.banks[data].resize(0x10000u);m.banks[pool].resize(0xf1000u);m.banks[0x400000u].resize(0x160000u);
        m.banks[private_first].assign(0x10000u,std::byte{0xa5});
        const auto arena=native_tu15_world_factory_scratch({private_first,0x10000u},2u);
        m.put(0x48c938u,allocator);m.put(allocator,allocator_table);
        m.put(allocator_table+16u,0x149da0u);m.put(allocator_table+20u,0x149e00u);
        m.put(0x486064u,2u);m.put(0x48606cu,pool);m.put(pool,pool_table);
        m.put(pool_table+0x10u,0x233250u);m.put(pool_table+0x14u,0x233420u);
        m.put(pool+0xacu+20u,2u,1u);m.put(pool+0xacu+36u,3u,1u);
        m.put(pool+0x68u+2u*4u,32u);m.put(pool+0x68u+3u*4u,48u);
        m.put(pool+0x18u,pool+0x4340u);m.put(pool+0x20u,pool+0x4340u);
        m.put(pool+0x1cu,pool+0x6300u);m.put(pool+0xf0340u,1u);m.put(pool+0x2d0u,1u);
        m.put(0x52be5cu,component_pool);m.put(component_pool+0x44u,components);
        m.put(0x4ecbd0u,object_pool);m.put(object_pool+0x44u,objects);
        m.put(0x4f0414u,tags);
        m.put(self_body+0x30u,data+0x5200u);m.put(self_body+0x34u,2u);
        m.put(data+0x5200u,0x2001u);m.put(data+0x5204u,1u);
        m.put(data+0x5208u,0x2002u);m.put(data+0x520cu,7u);
        m.put(other_body+0x30u,data+0x5220u);m.put(other_body+0x34u,2u);
        m.put(data+0x5220u,0x2001u);m.put(data+0x5224u,2u);
        m.put(data+0x5228u,0x2002u);m.put(data+0x522cu,9u);
        m.put(components+0xa0u+8u,3u);m.put(components+0xa0u+0x94u,header);
        m.put(components+0x140u+8u,4u);
        m.put(objects+3u*12u+8u,own_object);m.put(objects+4u*12u+8u,other_object);
        m.put(own_object,5u);m.put(other_object+0x13cu,0xffffffffu);
        m.put(other_object+0x3e4u,0xffffffffu);
        m.put(tags+5u*16u+8u,definition);m.put(definition+0x38u,6u);
        m.put(tags+6u*16u+8u,model_data);m.put(model_data+0x24u,7u);
        m.put(tags+7u*16u+8u,physics);m.put(physics+0x44u,phantoms);
        m.put(physics+0x2cu,shape_rows);m.put(phantoms+2u*12u+8u,0u,2);
        m.put(shape_rows,0x1000000u);m.put(shape_rows+4u,0x00007f80u);
        m.put(header,members);m.put(header+8u,0x80000002u);
        m.put(context,dispatch);m.put(wrapper_table+20u,0xad1d0u);
        m.put(phantom_table+20u,failure==1u?0xdead0010u:0x308540u);
        m.put(phantom_table+40u,failure==3u?0xdead0028u:0x1f34d0u);
        m.put(convex_table+20u,0xa9520u);
        m.put(dispatch+((17u<<5u)+23u)*4u+0x18cu,0x2f0eb0u);
        m.put(dispatch+((22u<<5u)+23u)*4u+0x18cu,failure==2u?0xdead00f0u:0x2f78c0u);
        for(unsigned n=0;n<=depth;++n) {
            const auto shape=shapes+n*32u;
            m.put(shape,n==depth?phantom_table:wrapper_table);
            if(n<depth)m.put(shape+12u,shape+32u);
            else m.put(shape+8u,2u);
        }
        m.put(shapes+0x200u,convex_table);
        m.put(self_root,shapes);m.put(self_root+4u,0x12340000u);m.put(self_root+8u,0x12350000u);
        m.put(self_root+0x18u,1u);m.put(self_root+0x20u,self_body);
        m.put(other_root,shapes+0x200u);m.put(other_root+0x18u,1u);m.put(other_root+0x20u,other_body);
        std::vector<std::uint32_t>allocated,released,sites;
        NativeTu15ArrayAllocatorMemory routing(m);NativeTu15ArrayAllocatorServices pool_services;
        pool_services.allocate_owner=[&](const auto&request,auto&){
            check(request.owner==0x233250u,"Wrapped phantom lost native pool allocator");
            NativeTu15PoolMemory native_pool(m,request.receiver);return native_tu15_pool_allocate(native_pool,request.bytes);
        };
        pool_services.release_owner=[&](const auto&request,auto&){
            check(request.owner==0x233420u,"Wrapped phantom lost native pool release");
            NativeTu15PoolMemory native_pool(m,request.receiver);
            check(native_tu15_pool_release(native_pool,request.data,request.bytes),"Wrapped phantom required unqualified large release");
        };
        NativeTu15ArrayGrowthServices allocation;
        allocation.allocate=[&](const auto&request,auto&state) {
            check(request.owner==0x149da0u&&request.memory_class==0x1au&&
                request.bytes==(allocated.size()<depth?20u:36u),
                "Wrapped phantom selected an unexpected allocator request");
            const auto result=native_tu15_route_array_allocation(routing,request.bytes,request.memory_class,state,pool_services);
            allocated.push_back(result);return result;
        };
        allocation.release=[&](const auto&request,auto&state) {
            check(request.owner==0x149e00u&&request.memory_class==0x1au&&request.bytes==20u,
                "Wrapped phantom changed native constructor unwind allocation");
            released.push_back(request.data);
            native_tu15_route_array_release(routing,request.data,request.bytes,request.memory_class,state,pool_services);
        };
        const SourcePhysicsContactRecordObserver observe=[&](const auto&event){sites.push_back(event.instruction);};
        const NativeTu15WrappedContactArguments arguments{
            [&]{return reversed?other_root:self_root;},[&]{return reversed?self_root:other_root;},
            [&]{return context;},[]{return user;}};
        NativeTu15PhysicsFrameFp fp{0x27fu,0u,0x1f80u};NativeTu15WrappedFactoryBindingActivity activity;
        std::uint32_t outer{};std::string refusal;
        try{outer=native_tu15_create_bound_wrapped_contact_factory(m,
            {reversed?0x2f1cc0u:0x2f0eb0u},arguments,arena,fp,0u,allocation,observe,&activity).identity;}
        catch(const std::runtime_error&error){refusal=error.what();}
        if(!failure) {
            if(!refusal.empty())throw std::runtime_error(
                "Closed BV chain refused native phantom composition: "+refusal);
            check(refusal.empty()&&released.empty()&&allocated.size()==depth+1u,
                "Closed BV chain did not construct its actual native phantom callback");
            auto agent=outer;
            for(unsigned n=0;n<depth;++n) {
                check(m.get(agent)==(reversed&&n==0?0x41caf0u:0x41ca9cu)&&
                    m.get(agent+8u)==user&&m.get(agent+4u,2)==20u,
                    "Wrapped phantom changed parent DATA or pair orientation");
                agent=m.get(agent+12u);
            }
            check(m.get(agent)==0x41cca4u&&m.get(agent+4u,2)==36u&&m.get(agent+8u)==user&&
                m.get(agent+12u)==self_root&&m.get(agent+16u)==other_root&&
                m.get(agent+20u)==shapes+depth*32u&&m.get(agent+24u)==0u&&
                m.get(agent+28u)==22u&&m.get(agent+32u)==23u,
                "Wrapped phantom lost actual roots, classes or retained enter shape");
            check(m.get(header+4u)==1u&&m.get(members)==0x07010902u&&m.get(members+4u)==4u&&
                m.get(other_object+0x3e4u)==3u&&
                std::count(sites.begin(),sites.end(),0x1da307u)==1&&
                std::count(sites.begin(),sites.end(),0x1da301u)==1&&
                activity.completed==depth+1u&&activity.refusals==0u&&activity.unqualified_owners==0u,
                "Wrapped phantom bypassed actual membership, reference or owner stores");
        }else{
            const auto expected=failure==1u?"class owner=3735879696":failure==2u?
                "child factory owner=3735879920":"enter owner=3735879720";
            check(refusal.find(expected)!=std::string::npos&&activity.completed==0u&&
                activity.unqualified_owners==1u&&released.size()==depth&&
                allocated.size()==depth+(failure==3u?1u:0u)&&m.get(header+4u)==0u&&
                std::count(sites.begin(),sites.end(),0x1da307u)==0,
                "Wrapped phantom suppressed an unknown owner or changed native refusal lifetime");
            for(unsigned n=0;n<depth;++n)check(released[n]==allocated[depth-1u-n],
                "Wrapped phantom did not release failed parents in native order");
        }
        const auto&scratch=m.banks.at(private_first);
        for(unsigned n=0;n<scratch.size();++n) {
            const bool child_payload=n>=arena.first-private_first&&
                n<arena.first-private_first+depth*512u&&((n-(arena.first-private_first))%512u)<16u;
            if(!child_payload)check(scratch[n]==std::byte{0xa5},
                "Wrapped phantom crossed retained child DATA or surrounding sentinels");
        }
    }
}
