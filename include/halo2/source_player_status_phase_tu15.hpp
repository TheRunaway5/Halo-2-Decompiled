#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <functional>
#include <map>

namespace halo2 {
// Additive observation at15ED4F, after the complete original161610. The
// original instructions and active ABI are never replaced or modified.
using SourcePlayerStatusPhaseHandlerTu15=std::function<void(const SourcePhysicsWorldRuntimeAbi&)>;
// Function-local TLS preserves one registry per thread across translation
// units without MinGW namespace-scope TLS initializer aliases.
inline std::map<const SourcePhysicsContactMemory*,SourcePlayerStatusPhaseHandlerTu15>& source_player_status_phase_handlers_tu15() {
    static thread_local std::map<const SourcePhysicsContactMemory*,SourcePlayerStatusPhaseHandlerTu15> value;
    return value;
}
inline std::map<const SourcePhysicsContactMemory*,SourcePlayerStatusPhaseHandlerTu15>& source_player_scoreboard_gate_handlers_tu15() {
    static thread_local std::map<const SourcePhysicsContactMemory*,SourcePlayerStatusPhaseHandlerTu15> value;
    return value;
}
class SourcePlayerStatusPhaseBindingTu15 {
  const SourcePhysicsContactMemory* memory_;
  SourcePlayerStatusPhaseHandlerTu15 previous_;
  SourcePlayerStatusPhaseHandlerTu15 previous_gate_;
public:
  SourcePlayerStatusPhaseBindingTu15(const SourcePhysicsContactMemory& memory,SourcePlayerStatusPhaseHandlerTu15 handler,
      SourcePlayerStatusPhaseHandlerTu15 gate_handler={})
      :memory_(&memory) {
    auto& slot=source_player_status_phase_handlers_tu15()[memory_];
    previous_=std::move(slot);slot=std::move(handler);
    auto& gate=source_player_scoreboard_gate_handlers_tu15()[memory_];
    previous_gate_=std::move(gate);gate=std::move(gate_handler);
  }
  ~SourcePlayerStatusPhaseBindingTu15() {
    if(previous_)source_player_status_phase_handlers_tu15()[memory_]=std::move(previous_);
    else source_player_status_phase_handlers_tu15().erase(memory_);
    if(previous_gate_)source_player_scoreboard_gate_handlers_tu15()[memory_]=std::move(previous_gate_);
    else source_player_scoreboard_gate_handlers_tu15().erase(memory_);
  }
  SourcePlayerStatusPhaseBindingTu15(const SourcePlayerStatusPhaseBindingTu15&)=delete;
};
inline void source_observe_player_status_phase_tu15(const SourcePhysicsContactMemory& memory,
    const SourcePhysicsWorldRuntimeAbi& abi) {
  const auto found=source_player_status_phase_handlers_tu15().find(&memory);
  if(found!=source_player_status_phase_handlers_tu15().end() && found->second) {
    // Nested native queries may temporarily rebind this same bank.
    const auto handler=found->second;handler(abi);
  }
}
inline void source_observe_player_scoreboard_gate_tu15(const SourcePhysicsContactMemory& memory,
    const SourcePhysicsWorldRuntimeAbi& abi) {
  const auto found=source_player_scoreboard_gate_handlers_tu15().find(&memory);
  if(found!=source_player_scoreboard_gate_handlers_tu15().end() && found->second) {
    const auto handler=found->second;handler(abi);
  }
}
}
