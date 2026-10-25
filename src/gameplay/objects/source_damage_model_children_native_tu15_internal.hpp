#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <atomic>

namespace halo2::physics_world_runtime_detail {
inline std::atomic<std::uint64_t> damage_model_children_native_calls{};

// Complete original DC540. Match the authored child key/definition, walk the
// actual sibling handles and call D9110 only for the original bit26 gate.
// Site publishes the selected/canonical diagnostic PC. DamageChild executes
// the actual original callee with the current ABI and retained source bank.
// Qualification is separate from implementation; this header alone admits no
// route and supplies no destruction, retirement or child-construction result.
template<class State,class Site,class DamageChild>
void damage_model_children_native_tu15(State &s,Site site,DamageChild damage_child) {
    SourcePhysicsEntryObservation diagnostic(s.memory,0xdc540u,[&]{return s.save();});
    damage_model_children_native_calls.fetch_add(1,std::memory_order_relaxed);
    const auto at=[&](std::uint32_t pc){site(pc,true);};
    const auto store=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        w(s.memory,s.observe,site(pc,true),address,4,value);
    };
    at(0xdc540);s.gp[4]=s.flags.minus(s.gp[4],0x14u,4);
    at(0xdc543);s.gp[1]=r(s.memory,0x4ecbd0u,4);
    at(0xdc549);s.gp[2]=r(s.memory,s.gp[1]+0x44u,4);
    at(0xdc54c);s.gp[1]=r(s.memory,0x4f0414u,4);
    at(0xdc552);s.gp[0]=s.flags.logic(s.gp[0]&65535u,4);
    at(0xdc557);s.gp[0]*=3u;
    at(0xdc55a);s.gp[2]=r(s.memory,s.gp[2]+s.gp[0]*4u+8u,4);
    at(0xdc55e);s.gp[0]=r(s.memory,s.gp[2],4);
    at(0xdc560);s.gp[0]=s.flags.logic(s.gp[0]&65535u,4);
    at(0xdc565);s.gp[0]=s.flags.shift(s.gp[0],4u,4);
    at(0xdc568);s.gp[0]=r(s.memory,s.gp[0]+s.gp[1]+8u,4);
    at(0xdc56c);s.gp[0]=r(s.memory,s.gp[0]+0x38u,4);
    at(0xdc56f);s.flags.minus(s.gp[0],0xffffffffu,4);
    store(0xdc572,s.gp[4]+0xcu,s.gp[2]);
    at(0xdc576);
    if(!s.flags.zf) {
        at(0xdc57c);s.gp[0]=s.flags.logic(s.gp[0]&65535u,4);
        at(0xdc581);s.gp[0]=s.flags.shift(s.gp[0],4u,4);
        at(0xdc584);s.gp[1]=r(s.memory,s.gp[0]+s.gp[1]+8u,4);
        at(0xdc588);s.gp[0]=r(s.memory,s.gp[1]+0x50u,4);
        at(0xdc58b);s.flags.logic(s.gp[0],4);
        store(0xdc58d,s.gp[4]+0x10u,s.gp[1]);
        at(0xdc591);
        if(!s.flags.zf && s.flags.sf==s.flags.of) {
            at(0xdc597);s.push(site(0xdc597,false),s.gp[3]);
            at(0xdc598);s.push(site(0xdc598,false),s.gp[5]);
            at(0xdc599);s.push(site(0xdc599,false),s.gp[6]);
            store(0xdc59a,s.gp[4]+0xcu,0u);
            store(0xdc5a2,s.gp[4]+0x14u,s.gp[0]);
            at(0xdc5a6);s.push(site(0xdc5a6,false),s.gp[7]);
            at(0xdc5a7);
            for(;;) {
                at(0xdc5b0);s.gp[5]=r(s.memory,s.gp[1]+0x54u,4);
                at(0xdc5b3);s.gp[7]=r(s.memory,s.gp[4]+0x10u,4);
                at(0xdc5b7);s.gp[0]=r(s.memory,s.gp[5]+s.gp[7]+0x1cu,4);
                at(0xdc5bb);s.gp[5]=s.flags.plus(s.gp[5],s.gp[7],4);
                at(0xdc5bd);s.flags.logic(s.gp[0],4);
                at(0xdc5bf);
                if(!s.flags.zf && s.flags.sf==s.flags.of) {
                    at(0xdc5c1);s.gp[3]=s.flags.logic(0u,4);
                    store(0xdc5c3,s.gp[4]+0x14u,s.gp[0]);
                    at(0xdc5c7);
                    for(;;) {
                        at(0xdc5d0);s.gp[0]=r(s.memory,s.gp[5]+0x20u,4);
                        at(0xdc5d3);s.gp[1]=r(s.memory,s.gp[4]+0x28u,4);
                        at(0xdc5d7);s.gp[6]=r(s.memory,s.gp[0]+s.gp[3],4);
                        at(0xdc5da);s.gp[0]=s.flags.plus(s.gp[0],s.gp[3],4);
                        at(0xdc5dc);s.flags.minus(s.gp[6],s.gp[1],4);
                        at(0xdc5de);
                        if(s.flags.zf) {
                            at(0xdc5e0);s.gp[7]=r(s.memory,s.gp[0]+0xcu,4);
                            at(0xdc5e3);s.flags.minus(s.gp[7],0xffffffffu,4);
                            at(0xdc5e6);
                            if(!s.flags.zf) {
                                at(0xdc5e8);s.gp[1]=r(s.memory,s.gp[2]+0x10u,4);
                                at(0xdc5eb);s.flags.minus(s.gp[1],0xffffffffu,4);
                                at(0xdc5ee);
                                if(!s.flags.zf) {
                                    for(;;) {
                                        at(0xdc5f0);s.gp[0]=s.gp[1];
                                        at(0xdc5f2);s.gp[0]=s.flags.logic(s.gp[0]&65535u,4);
                                        at(0xdc5f7);s.gp[2]=s.gp[0]*3u;
                                        at(0xdc5fa);s.gp[0]=r(s.memory,0x4ecbd0u,4);
                                        at(0xdc5ff);s.gp[0]=r(s.memory,s.gp[0]+0x44u,4);
                                        at(0xdc602);s.gp[0]=r(s.memory,s.gp[0]+s.gp[2]*4u+8u,4);
                                        at(0xdc606);s.gp[2]=r(s.memory,s.gp[0]+4u,4);
                                        at(0xdc609);s.gp[6]=r(s.memory,s.gp[0]+0xcu,4);
                                        at(0xdc60c);s.gp[2]=s.flags.right(s.gp[2],26u,4,false);
                                        at(0xdc60f);s.flags.logic((s.gp[2]&255u)&1u,1);
                                        at(0xdc612);
                                        if(!s.flags.zf) {
                                            at(0xdc614);s.flags.minus(r(s.memory,s.gp[0],4),s.gp[7],4);
                                            at(0xdc616);
                                            if(s.flags.zf) {
                                                at(0xdc618);s.push(site(0xdc618,false),s.gp[1]);
                                                at(0xdc619);s.push(site(0xdc619,false),0xdc61eu);damage_child();
                                            }
                                        }
                                        at(0xdc61e);s.flags.minus(s.gp[6],0xffffffffu,4);
                                        at(0xdc621);s.gp[1]=s.gp[6];
                                        at(0xdc623);if(s.flags.zf)break;
                                    }
                                    at(0xdc625);s.gp[2]=r(s.memory,s.gp[4]+0x1cu,4);
                                }
                            }
                        }
                        at(0xdc629);s.gp[0]=r(s.memory,s.gp[4]+0x14u,4);
                        at(0xdc62d);s.gp[3]=s.flags.plus(s.gp[3],0x10u,4);
                        at(0xdc630);s.gp[0]=s.flags.increment(s.gp[0],4,true);
                        store(0xdc631,s.gp[4]+0x14u,s.gp[0]);
                        at(0xdc635);if(s.flags.zf)break;
                    }
                    at(0xdc637);s.gp[1]=r(s.memory,s.gp[4]+0x20u,4);
                }
                at(0xdc63b);s.gp[6]=r(s.memory,s.gp[4]+0x10u,4);
                at(0xdc63f);s.gp[0]=r(s.memory,s.gp[4]+0x18u,4);
                at(0xdc643);s.gp[6]=s.flags.plus(s.gp[6],0x38u,4);
                at(0xdc646);s.gp[0]=s.flags.increment(s.gp[0],4,true);
                store(0xdc647,s.gp[4]+0x10u,s.gp[6]);
                store(0xdc64b,s.gp[4]+0x18u,s.gp[0]);
                at(0xdc64f);if(s.flags.zf)break;
            }
            at(0xdc655);s.gp[7]=s.pop();
            at(0xdc656);s.gp[6]=s.pop();
            at(0xdc657);s.gp[5]=s.pop();
            at(0xdc658);s.gp[3]=s.pop();
        }
    }
    at(0xdc659);s.gp[4]=s.flags.plus(s.gp[4],0x14u,4);
    at(0xdc65c);s.pop();s.gp[4]+=4u;
}
}
