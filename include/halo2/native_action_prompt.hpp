#pragma once
#include "halo2/source_control_target.hpp"
namespace halo2 {
struct Unit;
struct LevelContent;
class SourceDeviceWorld;
// Authoritative selected action. Bindings and button artwork belong to the
// local HUD view; the producer retains the full target generation.
struct NativeActionPrompt {
  SourcePlayerActionTarget target;
  std::uint32_t definition=0xffffffffu, message{};
};
// Shared native adapter for campaign and multiplayer control owners. A
// preselected weapon context retains its original rank and root distance.
NativeActionPrompt native_control_action_prompt(const Unit &,const LevelContent &,
    const SourceDeviceWorld &,NativeActionPrompt current={},Vec3 current_root={});
}
