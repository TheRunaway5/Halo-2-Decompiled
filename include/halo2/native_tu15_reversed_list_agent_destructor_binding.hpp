#pragma once
#include "halo2/native_tu15_reversed_list_agent_destructor.hpp"
#include "halo2/native_tu15_pair_deletion_native_scope.hpp"
namespace halo2 {
struct NativeTu15ReversedListAgentDestructorActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15PairDeletionNativeScopeActivity native_only;
};
inline NativeTu15ContactAgent native_tu15_delete_bound_reversed_list_contact_agent(
    SourcePhysicsContactMemory&memory,NativeTu15ContactAgent agent,std::uint32_t disposing,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15AgentCleanupChildrenServices&services,
    NativeTu15PairDeletionNativeScope&scope,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ReversedListAgentDestructorActivity*activity=nullptr){
    NativeTu15ReversedListAgentDestructorActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15PairDeletionNativeScopeGuard guard(scope,&out.native_only);
    try{auto result=native_tu15_delete_reversed_list_contact_agent(memory,agent,disposing,fp,services,observe);
        ++out.completed;return result;
    }catch(...){++out.refusals;throw;}
}
}
