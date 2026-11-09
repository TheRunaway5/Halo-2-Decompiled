#pragma once
#include "halo2/native_tu15_world_factory_scratch.hpp"
// Declared typed model, not a recorded gameplay collision or original-XBE
// comparison. Constructors and dispatch run; only allocation is a test service.
inline void native_world_factory_scratch_model() {
    constexpr std::uint32_t data=0x2200000u,private_first=0x2300000u;
    for(unsigned count:{2u,8u,32u,91u}) {
        Memory m;m.banks[data].resize(0x10000u);
        m.banks[0x48c000u].resize(0x1000u);
        m.banks[private_first].assign(0x10000u,std::byte{0xa5});
        const auto arena=native_tu15_world_factory_scratch({private_first,0x10000u},2u);
        check(arena.first==private_first+0x2000u&&arena.bytes/512u==92u,
            "World factory lost the genuinely available disjoint constructor DATA");
        m.put(0x48c938u,data);m.put(data,data+0x100u);m.put(data+0x110u,0x149da0u);
        m.put(data+0x114u,0x149e00u);
        constexpr auto context=data+0x200u,dispatch=data+0x400u;
        constexpr auto wrapper_table=data+0x2000u,sphere_table=data+0x2040u;
        m.put(context,dispatch);m.put(wrapper_table+20u,0xad1d0u);m.put(sphere_table+20u,0x2e8cf0u);
        m.put(dispatch+((17u<<5u)+4u)*4u+0x18cu,0x2f0eb0u);
        m.put(dispatch+((4u<<5u)+4u)*4u+0x18cu,0x2f9990u);
        const auto shapes=data+0x2800u,first=data+0x3000u,second=first+32u;
        for(unsigned i=0;i<=count;++i) {
            const auto shape=shapes+i*16u;
            m.put(shape,i==count?sphere_table:wrapper_table);
            if(i<count)m.put(shape+12u,shape+16u);
        }
        m.put(first,shapes);m.put(first+4u,0x12340000u);m.put(first+8u,0x12350000u);
        m.put(second,shapes+count*16u);
        std::uint32_t next=data+0x4000u;unsigned allocations{};
        NativeTu15ArrayGrowthServices allocation;
        allocation.allocate=[&](const auto&request,auto&) {
            check(request.owner==0x149da0u&&request.memory_class==0x1au&&
                request.bytes==(allocations<count?20u:16u),"Nested constructor allocation differs");
            ++allocations;const auto out=next;next+=64u;return out;
        };
        allocation.release=[](const auto&,auto&){throw std::runtime_error("Unexpected successful-chain unwind");};
        NativeTu15PhysicsFrameFp fp{0x27fu,0u,0x1f80u};NativeTu15WrappedFactoryBindingActivity activity;
        const NativeTu15WrappedContactArguments arguments{[&]{return first;},[&]{return second;},
            [&]{return context;},[]{return 0x76543210u;}};
        auto agent=native_tu15_create_bound_wrapped_contact_factory(m,{0x2f0eb0u},arguments,
            arena,fp,0u,allocation,{},&activity).identity;
        for(unsigned i=0;i<count;++i) {
            check(m.get(agent)==0x41ca9cu&&m.get(agent+4u,2)==20u&&
                m.get(agent+8u)==0x76543210u&&m.get(agent+16u)==0u,"Nested BV constructor DATA differs");
            agent=m.get(agent+12u);
        }
        check(m.get(agent)==0x41cd80u&&m.get(agent+12u,2)==0xffffu&&allocations==count+1u&&
            activity.completed==count+1u&&activity.refusals==0u,"Nested native leaf did not complete");
        const auto&bytes=m.banks.at(private_first);
        for(unsigned i=0;i<bytes.size();++i)
            if(i<0x2000u||i>=0xd800u)check(bytes[i]==std::byte{0xa5},
                "Native constructor overwrote wake, regroup, merge or parent DATA");
    }
    for(const auto input:std::array{
            NativeTu15PhysicsFrameScratch{private_first,0x4ff0u},
            NativeTu15PhysicsFrameScratch{private_first+1u,0x10000u},
            NativeTu15PhysicsFrameScratch{0xffff8000u,0x10000u}}) {
        bool refused{};try{(void)native_tu15_world_factory_scratch(input,2u);}catch(const std::runtime_error&){refused=true;}
        check(refused,"World constructor admitted unavailable DATA");
    }
    bool refused{};try{(void)native_tu15_world_factory_scratch({private_first,0x10000u},120u);}catch(const std::runtime_error&){refused=true;}
    check(refused,"World constructor crossed live parent/filter DATA");
}
