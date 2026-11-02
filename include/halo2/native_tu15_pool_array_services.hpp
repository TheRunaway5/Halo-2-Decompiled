#pragma once
#include "halo2/native_tu15_array_allocator.hpp"
#include <string>
namespace halo2 {
struct NativeTu15PoolArrayServiceActivity {
    std::uint64_t allocation_requests{},release_requests{},pool_allocations{},pool_releases{},refusals{};
};
// The shared qualified149DA0/E00→233250/233420 native pool domain.
// No original executor/default-heap result is accepted. Unported owners and
// large-free/default-heap branches refuse at their genuine callback phase.
// Returned callbacks borrow only memory/activity and copy the observer.
inline NativeTu15ArrayGrowthServices native_tu15_make_pool_array_services(
    SourcePhysicsContactMemory&memory,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15PoolArrayServiceActivity*activity=nullptr){
    const auto heap=[&memory,observe,activity](){
        NativeTu15ArrayAllocatorServices services;
        services.allocate_owner=[&memory,observe,activity](const auto&request,auto&){
            if(request.owner!=0x233250u)
                throw std::runtime_error("Native pool array requires its allocation owner="+std::to_string(request.owner));
            NativeTu15PoolMemory pool(memory,request.receiver,observe);
            const auto value=native_tu15_pool_allocate(pool,request.bytes);
            if(activity)++activity->pool_allocations;return value;
        };
        services.release_owner=[&memory,observe,activity](const auto&request,auto&){
            if(request.owner!=0x233420u)
                throw std::runtime_error("Native pool array requires its release owner="+std::to_string(request.owner));
            NativeTu15PoolMemory pool(memory,request.receiver,observe);
            if(!native_tu15_pool_release(pool,request.data,request.bytes))
                throw std::runtime_error("Native pool array requires its large-free owner");
            if(activity)++activity->pool_releases;
        };
        services.default_heap=[](auto&)->std::uint32_t{
            throw std::runtime_error("Native pool array requires its default-heap owner149930");
        };
        return services;
    };
    NativeTu15ArrayGrowthServices services;
    services.allocate=[&memory,heap,activity](const auto&request,auto&fp){
        if(activity)++activity->allocation_requests;
        try{
            if(request.owner!=0x149da0u)
                throw std::runtime_error("Native pool array requires its allocation route="+std::to_string(request.owner));
            NativeTu15ArrayAllocatorMemory router(memory);
            return native_tu15_route_array_allocation(router,request.bytes,request.memory_class,fp,heap());
        }catch(...){if(activity)++activity->refusals;throw;}
    };
    services.release=[&memory,heap,activity](const auto&request,auto&fp){
        if(activity)++activity->release_requests;
        try{
            if(request.owner!=0x149e00u)
                throw std::runtime_error("Native pool array requires its release route="+std::to_string(request.owner));
            NativeTu15ArrayAllocatorMemory router(memory);
            native_tu15_route_array_release(router,request.data,request.bytes,request.memory_class,fp,heap());
        }catch(...){if(activity)++activity->refusals;throw;}
    };
    return services;
}
} // namespace halo2
