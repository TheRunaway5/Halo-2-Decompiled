#pragma once
#include "halo2/native_tu15_pair_creation_queues.hpp"
namespace halo2 {
struct NativeTu15PairCreationQueueBindingActivity {
    std::uint64_t merge_attempts{},merge_calls{},merge_refusals{},defer_attempts{},defer_calls{},defer_refusals{};
};
inline void native_tu15_queue_bound_body_pair_merge(SourcePhysicsContactMemory&memory,
    std::uint32_t world,std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15PairCreationQueueBindingActivity*activity=nullptr){
    NativeTu15PairCreationQueueBindingActivity local;auto&result=activity?*activity:local;++result.merge_attempts;
    try{native_tu15_queue_body_pair_merge(memory,world,first,second,fp,allocation,observe);++result.merge_calls;}
    catch(...){++result.merge_refusals;throw;}
}
inline void native_tu15_defer_bound_collision_pair(SourcePhysicsContactMemory&memory,
    std::uint32_t island,std::uint32_t pair,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15PairCreationQueueBindingActivity*activity=nullptr){
    NativeTu15PairCreationQueueBindingActivity local;auto&result=activity?*activity:local;++result.defer_attempts;
    try{native_tu15_defer_collision_pair(memory,island,pair,fp,allocation,observe);++result.defer_calls;}
    catch(...){++result.defer_refusals;throw;}
}
}
