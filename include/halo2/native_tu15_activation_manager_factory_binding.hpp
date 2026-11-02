#pragma once
#include "halo2/native_tu15_activation_contact_manager.hpp"
#include "halo2/native_tu15_base_lock.hpp"
namespace halo2 {
struct NativeTu15ActivationManagerFactoryBindingActivity {
  std::uint64_t attempts{},completed{},refusals{},unqualified_factories{},
    body_destructor_refusals{},base_lock_releases{},base_lock_refusals{};
  NativeTu15ActivationContactManagerStats managers;
};
// The same native-only builder serves World pair creation and direct proofs.
// Source3232E0's third context formal is unused. Allocation is caller-owned
// native storage; no residual executor is available to this service builder.
inline NativeTu15ActivationContactManager native_tu15_create_bound_activation_contact_manager(
    SourcePhysicsContactMemory&memory,std::uint32_t factory,
    NativeTu15ContactVirtualOwner owner,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,
    const SourcePhysicsContactRecordObserver&observer={},
    NativeTu15ActivationManagerFactoryBindingActivity*activity=nullptr){
  NativeTu15ActivationManagerFactoryBindingActivity local;auto&result=activity?*activity:local;
  ++result.attempts;
  try{
    if(owner.identity!=0x3232e0u){++result.unqualified_factories;
      throw std::runtime_error("Native TU1.5 manager factory requires its remaining native owner");}
    NativeTu15ActivationContactManagerServices services;services.allocation=allocation;
    services.destroy_body=[&](auto body,auto target,auto disposing,auto&){
      (void)body;(void)disposing;++result.body_destructor_refusals;
      throw std::runtime_error("Native TU1.5 manager construction requires native body destructor owner="+std::to_string(target.identity));
    };
    services.release_base_lock=[&](auto object,auto flags,auto&){
      try{
        if(flags)throw std::runtime_error("Native TU1.5 manager base lock requires its disposing profile");
        native_tu15_release_unregistered_base_lock(memory,object,observer);
        ++result.base_lock_releases;
      }catch(...){++result.base_lock_refusals;throw;}
    };
    const auto manager=native_tu15_create_activation_contact_manager(memory,factory,
      first,second,fp,services,observer,&result.managers);
    ++result.completed;return manager;
  }catch(...){++result.refusals;throw;}
}
} // namespace halo2
