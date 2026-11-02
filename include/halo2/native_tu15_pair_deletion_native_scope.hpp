#pragma once
#include "halo2/native_tu15_pair_deletion_binding.hpp"

namespace halo2 {
struct NativeTu15PairDeletionNativeScopeActivity {
    std::uint64_t refused{};
    std::map<std::uint32_t,std::uint64_t> owners;
};
class NativeTu15PairDeletionNativeScopeGuard;
// This instance belongs to one World operation. The generic remaining-owner
// adapter checks it before touching its ABI transport or executing a child.
class NativeTu15PairDeletionNativeScope {
    friend class NativeTu15PairDeletionNativeScopeGuard;
    bool active_{};
    NativeTu15PairDeletionNativeScopeActivity* activity_{};
public:
    bool active() const noexcept { return active_; }
    void reject_unconverted(std::uint32_t owner) const {
        if(!active_)return;
        if(activity_){++activity_->refused;++activity_->owners[owner];}
        throw std::runtime_error("Native TU1.5 pair deletion refuses unconverted child owner="+
            std::to_string(owner));
    }
};
class NativeTu15PairDeletionNativeScopeGuard {
    NativeTu15PairDeletionNativeScope& scope_;
    bool previous_active_;
    NativeTu15PairDeletionNativeScopeActivity* previous_activity_;
public:
    NativeTu15PairDeletionNativeScopeGuard(NativeTu15PairDeletionNativeScope&scope,
        NativeTu15PairDeletionNativeScopeActivity*activity=nullptr):scope_(scope),
        previous_active_(scope.active_),previous_activity_(scope.activity_){
        scope_.active_=true;scope_.activity_=activity;
    }
    NativeTu15PairDeletionNativeScopeGuard(const NativeTu15PairDeletionNativeScopeGuard&)=delete;
    NativeTu15PairDeletionNativeScopeGuard& operator=(const NativeTu15PairDeletionNativeScopeGuard&)=delete;
    ~NativeTu15PairDeletionNativeScopeGuard(){
        scope_.active_=previous_active_;scope_.activity_=previous_activity_;
    }
};
inline void native_tu15_delete_bound_pair_in_native_scope(SourcePhysicsContactMemory&memory,
    std::uint32_t handler,std::uint32_t pair,NativeTu15PairDeletionScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15PairDeletionBindingServices&services,
    NativeTu15PairDeletionNativeScope&scope,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15PairDeletionBindingActivity*activity=nullptr,
    NativeTu15PairDeletionNativeScopeActivity*scope_activity=nullptr){
    NativeTu15PairDeletionNativeScopeGuard guard(scope,scope_activity);
    native_tu15_delete_bound_pair(memory,handler,pair,scratch,fp,services,observe,activity);
}
}
