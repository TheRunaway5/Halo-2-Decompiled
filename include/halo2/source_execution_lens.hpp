#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <span>
#include <array>
#include <optional>
#include <map>

namespace halo2 {
class PhysicsLiveWorldTu15;
// The renderer owns the device ABI, camera/projection inputs and completed
// GPU query metadata. Replay consumes their recorded reads, executes the
// original CPU query kernel, and validates every private-device write.
void source_execute_lens_query(PhysicsLiveWorldTu15 &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsContactMemory *device=nullptr);
struct SourceLensQueryRollback {
    std::uint32_t id{},area{};
    bool bit{};
};
// Actual source-ring rollback/presentation-count publication. Private Vulkan
// query/fence lifetimes still belong to the rendering executor.
void source_finish_lens_query(PhysicsLiveWorldTu15 &,bool submitted,
    std::span<const SourceLensQueryRollback>);
struct SourceLensDeviceRegion {std::uint32_t address{},bytes{};};
using SourceLensDeviceDomains=std::map<std::uint32_t,std::size_t>;
using SourceLensWorldPass=std::optional<std::array<std::uint32_t,6>>;
bool source_lens_draw_execution_entry(std::uint32_t);
// A present result is the original parent's interrupted world-pass call,
// including its actual six arguments. It is never a fabricated child return.
SourceLensWorldPass source_execute_lens_draw(PhysicsLiveWorldTu15 &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,bool planning,std::span<const SourceLensDeviceRegion>,
    SourcePhysicsContactMemory *device=nullptr);
SourceLensWorldPass source_execute_lens_draw(PhysicsLiveWorldTu15 &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,bool planning,const SourceLensDeviceDomains &,
    SourcePhysicsContactMemory &device);
}
