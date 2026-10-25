#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_vehicle_compiled_symbols_tu15.hpp"
#include <array>
#include <atomic>

namespace halo2::physics_world_runtime_detail {

enum class VehicleChoiceFamilyTu15 { light, heavy, turret, flying };
inline std::atomic<std::uint64_t> vehicle_choice_native_calls{};
inline std::atomic<std::uint64_t> vehicle_choice_native_random_draws{};
inline std::atomic<std::uint64_t> vehicle_choice_native_flying_calls{};

// Unsigned arithmetic is the original wrapping LCG. A rejected NONE reference
// consumes its draw, then the same owner draws again. Cache preparation never
// calls this function or advances the live world's seed.
constexpr std::uint32_t vehicle_choice_next_seed_tu15(std::uint32_t seed) {
    return seed*0x19660du+0x3c6ef35fu;
}

struct VehicleChoiceSitesTu15 {
    std::uint32_t entry,dispatch,table,none,finish,random,draw,ret;
    unsigned count;
    std::array<std::uint32_t,5> fixed;
    std::array<unsigned,5> reference;
};
constexpr VehicleChoiceSitesTu15 vehicle_choice_sites_tu15(VehicleChoiceFamilyTu15 family) {
    switch(family) {
    case VehicleChoiceFamilyTu15::light:
        return {0x15cb60,0x15cb9b,0x15cc50,0x15cc45,0x15cc48,0x15cc0b,0x15cc10,0x15cc3f,
            7,{0x15cba2,0x15cbb7,0x15cbcc,0x15cbe1,0x15cbf6},{0,0,1,6,5}};
    case VehicleChoiceFamilyTu15::heavy:
        return {0x15cc70,0x15cca7,0x15cd1c,0x15cd14,0x15cd17,0x15ccd8,0x15cce0,0x15cd0e,
            4,{0x15ccae,0x15ccc3},{2,3}};
    case VehicleChoiceFamilyTu15::turret:
        return {0x15cdf0,0x15ce2b,0x15ced0,0x15cec5,0x15cec8,0x15ce86,0x15ce90,0x15cebf,
            6,{0x15ce32,0x15ce47,0x15ce5c,0x15ce71},{7,9,8,10}};
    case VehicleChoiceFamilyTu15::flying:
        return {0x15cd30,0x15cd67,0x15cddc,0x15cdd4,0x15cdd7,0x15cd98,0x15cda0,0x15cdce,
            4,{0x15cd6e,0x15cd83},{4,4}};
    }
    throw std::runtime_error("Unknown native vehicle-choice family");
}

// Complete selector owners15CB60/15CC70/15CD30/15CDF0. The native branch structure
// selects authored runtime references; the ABI adapter retains original ordered
// accesses, recursive frames, flags and site observations for its callers.
template<class State,class Site>
void vehicle_choice_native_tu15(State& s,VehicleChoiceFamilyTu15 family,Site site) {
    vehicle_choice_native_calls.fetch_add(1,std::memory_order_relaxed);
    if(family==VehicleChoiceFamilyTu15::flying)
        vehicle_choice_native_flying_calls.fetch_add(1,std::memory_order_relaxed);
    const auto p=vehicle_choice_sites_tu15(family);
    const bool light=family==VehicleChoiceFamilyTu15::light;
    const bool heavy=family==VehicleChoiceFamilyTu15::heavy || family==VehicleChoiceFamilyTu15::flying;
    const auto signed_byte=[](std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    };
    site(p.entry);s.push(p.entry,s.gp[6]);
    site(p.entry+1);s.gp[6]=s.gp[1];
    site(p.entry+3);s.gp[1]=r(s.memory,0x4ecc1cu,4);
    site(p.entry+9);s.gp[2]=r(s.memory,s.gp[1]+0x16cu,4);
    site(p.entry+15);s.gp[1]=r(s.memory,0x4f0414u,4);
    site(p.entry+21);s.gp[2]=s.flags.logic(s.gp[2]&0xffffu,4);
    site(p.entry+27);s.gp[2]=s.flags.shift(s.gp[2],4u,4);
    site(p.entry+30);s.gp[2]=r(s.memory,s.gp[1]+s.gp[2]+8u,4);
    const unsigned choice_reg=heavy?1u:2u;
    const unsigned runtime_reg=heavy?2u:1u;
    if(heavy) {
        site(p.entry+34);s.gp[1]=signed_byte(r(s.memory,s.gp[4]+12u,1));
        site(p.entry+39);s.gp[2]=r(s.memory,s.gp[2]+12u,4);
    } else {
        site(p.entry+34);s.gp[1]=r(s.memory,s.gp[2]+12u,4);
        site(p.entry+37);s.gp[2]=signed_byte(r(s.memory,s.gp[4]+12u,1));
    }
    site(p.entry+0x2au);s.push(p.entry+0x2au,s.gp[7]);
    site(p.entry+0x2bu);s.gp[7]=r(s.memory,s.gp[4]+12u,4);
    site(p.entry+0x2fu);s.gp[choice_reg]=s.flags.increment(s.gp[choice_reg],4,true);
    site(p.entry+0x30u);s.flags.minus(s.gp[choice_reg],p.count-1u,4);
    site(p.entry+0x33u);s.gp[0]=s.gp[7];
    site(p.entry+0x35u);
    if(s.gp[choice_reg]<p.count) {
        site(p.dispatch);
        const auto selected=s.gp[choice_reg];
        const auto target=r(s.memory,p.table+selected*4u,4);
        const unsigned fixed_count=light?5u:heavy?2u:4u;
        const auto expected=selected<fixed_count?p.fixed[selected]:selected==fixed_count?p.random:p.none;
        if(target!=expected)throw std::runtime_error("Native vehicle choice lost its authenticated jump-table owner");
        if(selected<fixed_count) {
            const auto at=p.fixed[selected];
            const auto pointer_reg=selected==0?0u:2u;
            site(at);s.gp[pointer_reg]=r(s.memory,s.gp[runtime_reg]+0x74u,4);
            site(at+3);s.gp[1]=r(s.memory,s.gp[4]+20u,4);
            site(at+7);s.gp[0]=r(s.memory,s.gp[pointer_reg]+p.reference[selected]*8u+4u,4);
            site(at+10);s.gp[7]=s.pop();
            site(at+11);
            const auto symbol=family==VehicleChoiceFamilyTu15::heavy?0x0b00078bu:light && selected==1?0x05000694u:0u;
            w(s.memory,s.observe,at+11,s.gp[1],4,
                symbol?source_vehicle_compiled_symbol_tu15(s.memory,at+11,symbol):0u);
            site(at+17);s.gp[6]=s.pop();
            site(at+18);s.pop();s.gp[4]+=12u;return;
        }
        if(selected==fixed_count) {
            site(p.random);s.push(p.random,s.gp[3]);
            site(p.random+1);s.gp[3]=r(s.memory,s.gp[4]+24u,4);
            if(!light)site(p.random+5); // Original alignment instruction / jump.
            do {
                vehicle_choice_native_random_draws.fetch_add(1,std::memory_order_relaxed);
                site(p.draw);s.gp[1]=r(s.memory,0x4f3cd8u,4);
                site(p.draw+6);s.gp[0]=r(s.memory,s.gp[1],4);
                site(p.draw+8);s.gp[0]=s.flags.product(s.gp[0],0x19660du);
                site(p.draw+14);s.gp[0]=s.flags.plus(s.gp[0],0x3c6ef35fu,4);
                site(p.draw+19);w(s.memory,s.observe,p.draw+19,s.gp[1],4,s.gp[0]);
                site(p.draw+21);s.gp[0]=s.flags.right(s.gp[0],16u,4,false);
                const unsigned result_reg=light?2u:0u;
                site(p.draw+24);
                if(light)s.gp[2]=s.gp[0]*5u;
                else s.gp[0]=s.flags.shift(s.gp[0],heavy?1u:2u,4);
                site(p.draw+(heavy?26u:27u));s.gp[result_reg]=s.flags.right(s.gp[result_reg],16u,4,false);
                site(p.draw+(heavy?29u:30u));s.set(result_reg,
                    s.flags.increment(s.reg(result_reg,1,0),1,false),1,0);
                const auto args=p.draw+(heavy?31u:32u);
                site(args);s.push(args,s.gp[3]);
                site(args+1);s.gp[1]=s.gp[6];
                site(args+3);s.push(args+3,s.gp[result_reg]);
                site(args+4);s.push(args+4,s.gp[7]);
                site(args+5);s.push(args+5,args+10);
                vehicle_choice_native_tu15(s,family,site);
                site(args+10);s.flags.minus(s.gp[0],0xffffffffu,4);
                site(args+13);
            } while(s.gp[0]==0xffffffffu);
            site(p.ret);s.gp[3]=s.pop();
            site(p.ret+1);s.gp[7]=s.pop();
            site(p.ret+2);s.gp[6]=s.pop();
            site(p.ret+3);s.pop();s.gp[4]+=12u;return;
        }
        site(p.none);s.gp[0]=s.flags.logic(0xffffffffu,4);
    }
    site(p.finish);s.gp[7]=s.pop();
    site(p.finish+1);s.gp[6]=s.pop();
    site(p.finish+2);s.pop();s.gp[4]+=12u;
}
}
