#pragma once

#include "halo2/source_physics_entry_observation.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_tu15_liquid_widget {
inline constexpr std::uint32_t retirement_entry=0xc5cf0u;

// C5CF0 is the tdtl (liquid widget) retirement callback. Its pool is
// independent of the object-widget list that the caller also retires.
inline void retire(SourcePhysicsContactMemory& memory,SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint controls,const SourcePhysicsWorldRuntimeServices& services,
    const SourcePhysicsContactRecordObserver& observer={}) {
    using namespace physics_world_runtime_detail;
    FpScope fp(controls);
    WorldState state(memory,observer,services,SourcePhysicsContactImage::TitleUpdate15);
    state.load(abi);
    SourcePhysicsEntryObservation phase(memory,retirement_entry,[&]{return state.save();});
    try {
        state.instruction(retirement_entry);
        state.gp[2]=r(memory,0x4ecbecu,4);
        state.instruction(0xc5cf6u);
        state.flags.logic(state.gp[2],4);
        state.instruction(0xc5cf8u);
        if(!state.flags.zf) {
            state.instruction(0xc5cfau);
            state.set(0,r(memory,state.gp[2]+0x29u,1),1);
            state.instruction(0xc5cfdu);
            state.flags.logic(state.reg(0,1),1);
            state.instruction(0xc5cffu);
            if(!state.flags.zf) {
                state.instruction(0xc5d01u);
                state.gp[0]=r(memory,state.gp[4]+4u,4);
                state.instruction(0xc5d05u);
                state.flags.minus(state.gp[0],0xffffffffu,4);
                state.instruction(0xc5d08u);
                if(!state.flags.zf) {
                    constexpr std::uint32_t call=0xc5d0au,continuation=0xc5d0fu;
                    state.instruction(call);
                    state.push(call,continuation);
                    state.required(call,0x16e8a0u,false);
                }
            }
        }
        state.instruction(0xc5d0fu);
        state.pop();
        state.gp[4]+=4u;
    } catch(...) {
        abi=state.save();
        throw;
    }
    abi=state.save();
}
}
