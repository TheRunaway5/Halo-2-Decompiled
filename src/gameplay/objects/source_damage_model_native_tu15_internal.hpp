#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_damage_model_native_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <atomic>
#include <bit>

namespace halo2::physics_world_runtime_detail {
inline std::atomic<std::uint64_t> damage_model_native_calls{},damage_model_null_object{},
    damage_model_null_model{},damage_model_no_information{},damage_model_resolved{};

constexpr std::uint32_t damage_model_tag_row_tu15(std::uint32_t handle) {
    return (handle&0xffffu)*0x10u;
}
constexpr bool damage_model_has_information_tu15(std::uint32_t count) {
    return std::bit_cast<std::int32_t>(count)>0;
}

// Complete D80A0 owner: object -> definition -> model -> damage-information
// block. Original lookup masks the handle indices and uses a signed count;
// adding salt checks or treating a nonzero negative count as present changes it.
// The adapter keeps the original ABI, read/fault order and observation sites.
template<class State,class Site>
void damage_model_lookup_native_tu15(State &s,Site site) {
    SourcePhysicsEntryObservation diagnostic(s.memory,0xd80a0u,[&]{return s.save();});
    damage_model_native_calls.fetch_add(1,std::memory_order_relaxed);
    const auto object=s.gp[1];
    site(0xd7160u,0xd80a0u);s.gp[0]=s.flags.logic(0u,4);
    site(0xd7162u,0xd80a2u);s.flags.minus(object,0xffffffffu,4);
    site(0xd7165u,0xd80a5u);
    if(object!=0xffffffffu) {
        site(0xd7167u,0xd80a7u);s.gp[2]=r(s.memory,0x4ecbd0u,4);
        site(0xd716du,0xd80adu);s.gp[2]=r(s.memory,s.gp[2]+0x44u,4);
        site(0xd7170u,0xd80b0u);s.gp[1]=s.flags.logic(object&0xffffu,4);
        site(0xd7176u,0xd80b6u);s.gp[1]*=3u;
        site(0xd7179u,0xd80b9u);s.gp[1]=r(s.memory,s.gp[2]+s.gp[1]*4u+8u,4);
        site(0xd717du,0xd80bdu);s.gp[1]=r(s.memory,s.gp[1],4);
        const auto definition=s.gp[1];
        site(0xd717fu,0xd80bfu);s.gp[2]=r(s.memory,0x4f0414u,4);
        site(0xd7185u,0xd80c5u);s.gp[1]=s.flags.logic(definition&0xffffu,4);
        site(0xd718bu,0xd80cbu);s.flags.shift(s.gp[1],4u,4);s.gp[1]=damage_model_tag_row_tu15(definition);
        site(0xd718eu,0xd80ceu);s.gp[1]=r(s.memory,s.gp[2]+s.gp[1]+8u,4);
        site(0xd7192u,0xd80d2u);s.gp[1]=r(s.memory,s.gp[1]+0x38u,4);
        const auto model=s.gp[1];
        site(0xd7195u,0xd80d5u);s.flags.minus(model,0xffffffffu,4);
        site(0xd7198u,0xd80d8u);
        if(model!=0xffffffffu) {
            site(0xd719au,0xd80dau);s.gp[1]=s.flags.logic(model&0xffffu,4);
            site(0xd71a0u,0xd80e0u);s.flags.shift(s.gp[1],4u,4);s.gp[1]=damage_model_tag_row_tu15(model);
            site(0xd71a3u,0xd80e3u);s.gp[1]=r(s.memory,s.gp[2]+s.gp[1]+8u,4);
            site(0xd71a7u,0xd80e7u);s.gp[2]=r(s.memory,s.gp[1]+0x60u,4);
            const auto count=s.gp[2];
            site(0xd71aau,0xd80eau);s.flags.logic(count,4);
            site(0xd71acu,0xd80ecu);
            if(damage_model_has_information_tu15(count)) {
                site(0xd71aeu,0xd80eeu);s.gp[0]=r(s.memory,s.gp[1]+0x64u,4);
                damage_model_resolved.fetch_add(1,std::memory_order_relaxed);
            } else damage_model_no_information.fetch_add(1,std::memory_order_relaxed);
        } else damage_model_null_model.fetch_add(1,std::memory_order_relaxed);
    } else damage_model_null_object.fetch_add(1,std::memory_order_relaxed);
    site(0xd71b1u,0xd80f1u);s.pop();
}
}
