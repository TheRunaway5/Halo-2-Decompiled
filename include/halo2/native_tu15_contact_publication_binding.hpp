#pragma once
#include "halo2/native_tu15_contact_publication.hpp"
#include "halo2/native_tu15_contact_point_create_binding.hpp"
#include <string>
namespace halo2 {
struct NativeTu15ContactPublicationNativeServices {
    NativeTu15ContactPointCreationNativeServices creation;
    // The World supplies its sole qualified native point-removal composition.
    // The actual manager virtual8 owner is checked before calling it.
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t manager,
        std::uint16_t point,NativeTu15PhysicsFrameFp&)> remove_point;
};
struct NativeTu15ContactPublicationBindingActivity {
    std::uint64_t point_requests{},removal_requests{},allocation_requests{},release_requests{};
    NativeTu15ContactPointCreationBindingActivity creation;
    NativeTu15PairDeletionNativeScopeActivity native_only;
};
// Shared30CD00/30CF40/30CAA0 service composition for304350 and2F8B60.
// Provider functions are copied into the result. memory,scope and activity
// remain borrowed operation owners and must outlive all synchronous calls.
// Every provider receives the current guarded DATA view, never a CPU context.
inline NativeTu15ContactPublicationServices native_tu15_make_contact_publication_services(
    SourcePhysicsContactMemory&memory,NativeTu15ContactPointCreationBindingScratch scratch,
    unsigned depth,NativeTu15ContactPublicationNativeServices native,
    NativeTu15PairDeletionNativeScope&scope,
    NativeTu15ContactPublicationBindingActivity*activity=nullptr){
    if(!native.creation.remove_bound_point)native.creation.remove_bound_point=native.remove_point;
    NativeTu15ContactPublicationServices services;
    services.create_point=[&memory,&scope,scratch,depth,native,activity](auto manager,
        auto owner,auto first,auto second,auto context,auto packet,auto&fp){
        if(owner.identity!=0x322d20u)
            throw std::runtime_error("Native contact publication requires its point creation owner="+std::to_string(owner.identity));
        if(activity)++activity->point_requests;
        return native_tu15_create_bound_contact_point(memory,manager,first,second,context,
            packet,scratch,fp,depth,native.creation,scope,{},activity?&activity->creation:nullptr);
    };
    services.removal.remove_point=[&memory,&scope,native,activity](auto manager,
        auto owner,auto point,auto&fp){
        if(owner.identity==0x2e4d60u)return; // Actual RET4, no DATA/FP effect.
        if(owner.identity!=0x322f70u)
            throw std::runtime_error("Native contact publication requires its point removal owner="+std::to_string(owner.identity));
        if(!native.remove_point)throw std::runtime_error("Native contact publication requires native point removal services");
        if(activity)++activity->removal_requests;
        NativeTu15PairDeletionNativeScopeGuard guard(scope,activity?&activity->native_only:nullptr);
        native.remove_point(memory,manager,point,fp);
    };
    services.removal.allocation.allocate=[&memory,&scope,native,activity](const auto&request,auto&fp){
        NativeTu15PairDeletionNativeScopeGuard guard(scope,activity?&activity->native_only:nullptr);
        if(!native.creation.allocation)
            throw std::runtime_error("Native contact publication requires native allocation services");
        const auto allocation=native.creation.allocation(memory);
        if(!allocation.allocate)throw std::runtime_error("Native contact publication requires its native allocation owner");
        if(activity)++activity->allocation_requests;return allocation.allocate(request,fp);
    };
    services.removal.allocation.release=[&memory,&scope,native,activity](const auto&request,auto&fp){
        NativeTu15PairDeletionNativeScopeGuard guard(scope,activity?&activity->native_only:nullptr);
        if(!native.creation.allocation)
            throw std::runtime_error("Native contact publication requires native allocation services");
        const auto allocation=native.creation.allocation(memory);
        if(!allocation.release)throw std::runtime_error("Native contact publication requires its native release owner");
        if(activity)++activity->release_requests;allocation.release(request,fp);
    };
    return services;
}
} // namespace halo2
