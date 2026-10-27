#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <exception>
#include <map>
#include <stdexcept>
namespace halo2 {
// Read-only host diagnostics at a complete source entry/return boundary.
using SourcePhysicsEntryObserver=std::function<void(bool,std::uint32_t,const SourcePhysicsWorldRuntimeAbi &)>;
// Function-local TLS preserves one registry per thread across translation
// units without MinGW namespace-scope TLS initializer aliases.
inline std::map<const SourcePhysicsContactMemory *,SourcePhysicsEntryObserver>& source_physics_entry_observers() {
    static thread_local std::map<const SourcePhysicsContactMemory *,SourcePhysicsEntryObserver> value;
    return value;
}
class SourcePhysicsEntryObserverBinding {
 const SourcePhysicsContactMemory &memory_;
public:
 SourcePhysicsEntryObserverBinding(const SourcePhysicsContactMemory &memory,SourcePhysicsEntryObserver observer):memory_(memory) {
  if(!source_physics_entry_observers().emplace(&memory,std::move(observer)).second)
   throw std::runtime_error("Original physics entry diagnostics already bound");
 }
 ~SourcePhysicsEntryObserverBinding(){source_physics_entry_observers().erase(&memory_);}
 SourcePhysicsEntryObserverBinding(const SourcePhysicsEntryObserverBinding &)=delete;
};
class SourcePhysicsEntryObservation {
 SourcePhysicsEntryObserver observer_;
 std::function<SourcePhysicsWorldRuntimeAbi()> state_;
 std::uint32_t entry_{};int exceptions_=std::uncaught_exceptions();
public:
 SourcePhysicsEntryObservation(const SourcePhysicsContactMemory &memory,std::uint32_t entry,std::function<SourcePhysicsWorldRuntimeAbi()> state):entry_(entry) {
  const auto found=source_physics_entry_observers().find(&memory);if(found==source_physics_entry_observers().end())return;
  observer_=found->second;state_=std::move(state);observer_(true,entry_,state_());
 }
 ~SourcePhysicsEntryObservation() noexcept(false) {
  if(observer_ && exceptions_==std::uncaught_exceptions())observer_(false,entry_,state_());
 }
 SourcePhysicsEntryObservation(const SourcePhysicsEntryObservation &)=delete;
};
}
