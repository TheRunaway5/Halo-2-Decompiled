#pragma once
#include "halo2/native_tu15_object_activation_residuals.hpp"
#include "halo2/native_tu15_contact_agent_factory_input_observer.hpp"
#include <stdexcept>
#include <string>
namespace halo2 {
struct NativeTu15PairIslandWakeActivity {
    std::uint64_t attempts{},completed{},refusals{};
};
// Actual320EF0 calls316D40 with typed World/island identities. The caller
// supplies disjoint ordinary DATA for its nested native activation services.
// A remaining owner is counted by the shared binding and refused before it
// executes; this boundary never accepts guest registers or original results.
inline void native_tu15_activate_pair_island(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15ObjectActivationScratch scratch,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15PairIslandWakeActivity*activity=nullptr,
    NativeTu15ObjectActivationBindingActivity*activation=nullptr){
    NativeTu15PairIslandWakeActivity local;auto&result=activity?*activity:local;
    ++result.attempts;
    try{
        native_tu15_activate_island_with_residuals(memory,world,island,fp,depth,scratch,
            [&](std::uint32_t owner,std::span<const std::uint32_t>args,
                std::optional<std::uint32_t>receiver,unsigned pop,NativeTu15PhysicsFrameFp&current)->std::uint32_t{
                if(owner==0x2f8030u){
                    // Actual28296F publishes its context receiver as well as
                    // all four cdecl factory arguments. Observe before refusal.
                    if(args.size()!=4u||!receiver||*receiver!=args[2]||pop!=0u)
                        throw std::runtime_error("Native TU1.5 wake factory INPUT call contract differs");
                    native_tu15_observe_contact_agent_factory_input(memory,
                        {owner,args[0],args[1],args[2],args[3],current,depth,receiver,pop});
                }
                throw std::runtime_error("Native TU1.5 pair island activation requires native owner="+
                    std::to_string(owner));
            },observe,activation);
        ++result.completed;
    }catch(...){++result.refusals;throw;}
}
}
