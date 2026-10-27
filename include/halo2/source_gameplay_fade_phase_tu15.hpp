#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <functional>
#include <map>
namespace halo2 {
// Read-only additive observations; every original instruction still executes.
using SourceGameplayFadePhaseHandlerTu15=std::function<void(unsigned,const SourcePhysicsWorldRuntimeAbi&)>;
// Function-local TLS preserves one registry per thread across translation
// units without MinGW namespace-scope TLS initializer aliases.
inline std::map<const SourcePhysicsContactMemory*,SourceGameplayFadePhaseHandlerTu15>& source_gameplay_fade_handlers_tu15() {
    static thread_local std::map<const SourcePhysicsContactMemory*,SourceGameplayFadePhaseHandlerTu15> value;
    return value;
}
class SourceGameplayFadePhaseBindingTu15 {
 const SourcePhysicsContactMemory *memory_; SourceGameplayFadePhaseHandlerTu15 previous_;
public:
 SourceGameplayFadePhaseBindingTu15(const SourcePhysicsContactMemory &m,SourceGameplayFadePhaseHandlerTu15 h):memory_(&m){
   auto &slot=source_gameplay_fade_handlers_tu15()[memory_];previous_=std::move(slot);slot=std::move(h);
 }
 ~SourceGameplayFadePhaseBindingTu15(){if(previous_)source_gameplay_fade_handlers_tu15()[memory_]=std::move(previous_);else source_gameplay_fade_handlers_tu15().erase(memory_);}
 SourceGameplayFadePhaseBindingTu15(const SourceGameplayFadePhaseBindingTu15&)=delete;
};
inline void source_observe_gameplay_fade_tu15(const SourcePhysicsContactMemory &m,unsigned phase,const SourcePhysicsWorldRuntimeAbi &abi){
 const auto found=source_gameplay_fade_handlers_tu15().find(&m);
 if(found!=source_gameplay_fade_handlers_tu15().end()&&found->second){const auto h=found->second;h(phase,abi);}
}
}
