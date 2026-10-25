#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/detail/damage_product_native.hpp"
#include "halo2/source_damage_material_native_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <atomic>

namespace halo2::physics_world_runtime_detail {
inline std::atomic<std::uint64_t> damage_material_native_calls{},
    damage_material_groups_found{},damage_material_modifiers_found{};

// Complete D8110: two authored damage keys, each with two authored armor
// keys. The original search/comparator execute through search; they retain
// duplicate selection and signed wrapping subtraction. Preserve the caller's
// original memory, stack, GP/SSE state and all observation/fault boundaries.
template<class State,class Site,class Search>
void damage_material_lookup_native_tu15(State &s,Site site,Search search) {
    SourcePhysicsEntryObservation diagnostic(s.memory,0xd8110u,[&]{return s.save();});
    damage_material_native_calls.fetch_add(1,std::memory_order_relaxed);
    const auto at=[&](std::uint32_t offset){site(0xd71c0u+offset,0xd8110u+offset);};
    const auto store=[&](std::uint32_t offset,std::uint32_t address,std::uint32_t value){
        w(s.memory,s.observe,site(0xd71c0u+offset,0xd8110u+offset),address,4,value);
    };
    // site returns the observer PC for stores as well as publishing it.
    at(0x00);s.gp[4]=s.flags.minus(s.gp[4],0x14u,4);
    at(0x03);s.gp[0]=r(s.memory,0x4ecc1cu,4);
    at(0x08);s.xmm[0]=_mm_set_ss(f(r(s.memory,0x46a0d0u,4)));
    at(0x10);s.push(site(0xd71d0u,0xd8120u,false),s.gp[3]);
    at(0x11);s.gp[3]=r(s.memory,s.gp[0]+0xd4u,4);
    at(0x17);s.gp[0]=r(s.memory,s.gp[4]+0x1cu,4);
    at(0x1b);s.push(site(0xd71dbu,0xd812bu,false),s.gp[5]);
    store(0x1c,s.gp[4]+0xcu,s.gp[1]);
    at(0x20);s.gp[1]=r(s.memory,s.gp[4]+0x24u,4);
    at(0x24);s.push(site(0xd71e4u,0xd8134u,false),s.gp[6]);
    at(0x25);s.push(site(0xd71e5u,0xd8135u,false),s.gp[7]);
    store(0x26,s.gp[4]+0x10u,u(_mm_cvtss_f32(s.xmm[0])));
    store(0x2c,s.gp[4]+0x18u,s.gp[2]);
    store(0x30,s.gp[4]+0x1cu,s.gp[0]);
    store(0x34,s.gp[4]+0x20u,s.gp[1]);
    at(0x38);s.gp[5]=s.flags.logic(0u,4);
    at(0x3a); // Original alignment LEA changes neither state nor flags.
    for(;;) {
        at(0x40);s.gp[0]=r(s.memory,s.gp[3]+4u,4);
        at(0x43);s.gp[2]=r(s.memory,s.gp[4]+s.gp[5]*4u+0x14u,4);
        at(0x47);s.push(site(0xd7207u,0xd8157u,false),0u);
        at(0x49);s.push(site(0xd7209u,0xd8159u,false),0xd8100u);
        at(0x4e);s.push(site(0xd720eu,0xd815eu,false),12u);
        at(0x50);s.push(site(0xd7210u,0xd8160u,false),s.gp[0]);
        at(0x51);s.gp[0]=r(s.memory,s.gp[3],4);
        at(0x53);s.gp[1]=s.gp[4]+0x38u;
        at(0x57);s.push(site(0xd7217u,0xd8167u,false),s.gp[1]);
        store(0x58,s.gp[4]+0x3cu,s.gp[2]);
        at(0x5c);s.push(site(0xd721cu,0xd816cu,false),0xd8171u);search();
        at(0x61);s.flags.minus(s.gp[0],0xffffffffu,4);
        at(0x64);
        if(!s.flags.zf) {
            damage_material_groups_found.fetch_add(1,std::memory_order_relaxed);
            at(0x66);s.gp[2]=s.gp[0]*3u;
            at(0x69);s.gp[0]=r(s.memory,s.gp[3]+4u,4);
            at(0x6c);s.gp[6]=s.gp[0]+s.gp[2]*4u;
            at(0x6f);s.gp[7]=s.flags.logic(0u,4);
            for(;;) {
                at(0x71);s.gp[2]=r(s.memory,s.gp[6]+8u,4);
                at(0x74);s.gp[1]=r(s.memory,s.gp[4]+s.gp[7]*4u+0x1cu,4);
                at(0x78);s.push(site(0xd7238u,0xd8188u,false),0u);
                at(0x7a);s.push(site(0xd723au,0xd818au,false),0xd8100u);
                at(0x7f);s.push(site(0xd723fu,0xd818fu,false),8u);
                at(0x81);s.push(site(0xd7241u,0xd8191u,false),s.gp[2]);
                at(0x82);s.gp[0]=s.gp[4]+0x3cu;
                at(0x86);s.push(site(0xd7246u,0xd8196u,false),s.gp[0]);
                at(0x87);s.gp[0]=r(s.memory,s.gp[6]+4u,4);
                store(0x8a,s.gp[4]+0x40u,s.gp[1]);
                at(0x8e);s.push(site(0xd724eu,0xd819eu,false),0xd81a3u);search();
                at(0x93);s.flags.minus(s.gp[0],0xffffffffu,4);
                at(0x96);
                if(!s.flags.zf) {
                    damage_material_modifiers_found.fetch_add(1,std::memory_order_relaxed);
                    at(0x98);s.gp[1]=r(s.memory,s.gp[6]+8u,4);
                    at(0x9b);s.xmm[0]=_mm_set_ss(f(r(s.memory,s.gp[1]+s.gp[0]*8u+4u,4)));
                    at(0xa1);
                    const auto modifier=_mm_cvtss_f32(s.xmm[0]);
                    const auto accumulated=f(r(s.memory,s.gp[4]+0x10u,4));
                    s.xmm[0]=_mm_set_ss(detail::damage_product(modifier,accumulated));
                    store(0xa7,s.gp[4]+0x10u,u(_mm_cvtss_f32(s.xmm[0])));
                }
                at(0xad);s.gp[7]=s.flags.increment(s.gp[7],4,false);
                at(0xae);s.flags.minus(s.gp[7],2u,4);
                at(0xb1);if(!s.flags.cf)break;
            }
        }
        at(0xb3);s.gp[5]=s.flags.increment(s.gp[5],4,false);
        at(0xb4);s.flags.minus(s.gp[5],2u,4);
        at(0xb7);if(!s.flags.cf)break;
    }
    at(0xb9);s.xmm[0]=_mm_set_ss(f(r(s.memory,s.gp[4]+0x10u,4)));
    at(0xbf);s.gp[7]=s.pop();
    at(0xc0);s.gp[6]=s.pop();
    at(0xc1);s.gp[5]=s.pop();
    at(0xc2);s.gp[3]=s.pop();
    at(0xc3);s.gp[4]=s.flags.plus(s.gp[4],0x14u,4);
    at(0xc6);s.pop();s.gp[4]+=8u;
}
}
