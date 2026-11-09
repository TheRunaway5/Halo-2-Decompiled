#pragma once
#include "halo2/native_tu15_pair_handlers.hpp"
#include <map>
#include <stdexcept>
#include <string>
namespace halo2 {
struct NativeTu15PairDeletionBindingServices {
    std::function<void(NativeTu15ContactAgent,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> cleanup_agent;
    std::function<void(NativeTu15ActivationContactManager,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> release_manager;
};
struct NativeTu15PairDeletionBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},agent_cleanups{},manager_releases{},unqualified_owners{};
    std::map<std::uint32_t,std::uint64_t> cleanup_owners,release_owners;
};
// Actual320260 with ordinary caller-owned formal DATA. Services must execute
// their qualified native cleanup bodies; the World owner also scopes any
// generic remaining-owner adapter to reject before execution. No CPU state,
// old afterimage or callback result is accepted by this void operation.
inline void native_tu15_delete_bound_pair(SourcePhysicsContactMemory&memory,
    std::uint32_t handler,std::uint32_t pair,NativeTu15PairDeletionScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15PairDeletionBindingServices&services,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15PairDeletionBindingActivity*activity=nullptr){
    NativeTu15PairDeletionBindingActivity local;auto&result=activity?*activity:local;++result.attempts;
    const auto missing=[&](std::uint32_t owner){++result.unqualified_owners;
        throw std::runtime_error("Native TU1.5 pair deletion requires qualified native child owner="+std::to_string(owner));};
    NativeTu15PairHandlerServices children;
    children.destroy_agent=[&](auto agent,auto owner,auto&state){
        ++result.agent_cleanups;++result.cleanup_owners[owner.identity];
        if(owner.identity!=0x31d400u&&owner.identity!=0x2f3100u&&owner.identity!=0x2f7a20u&&
           owner.identity!=0x323220u&&owner.identity!=0x2f82d0u&&owner.identity!=0x2f6c80u&&
           owner.identity!=0x2f0da0u&&owner.identity!=0x2fbb10u&&owner.identity!=0x2f9960u&&
           owner.identity!=0x2f7790u)missing(owner.identity);
        if(!services.cleanup_agent)missing(owner.identity);
        services.cleanup_agent(agent,owner,state);
    };
    children.agents.release_manager=[&](auto manager,auto owner,auto&state){
        ++result.manager_releases;++result.release_owners[owner.identity];
        if(owner.identity!=0x323220u||!services.release_manager)missing(owner.identity);
        services.release_manager(manager,owner,state);
    };
    try{native_tu15_delete_pair(memory,handler,pair,scratch,fp,children,observe);++result.completed;}
    catch(...){++result.refusals;throw;}
}
}
