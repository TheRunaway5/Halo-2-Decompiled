#pragma once
#include "halo2/native_runtime_address.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_effect_replica_render_tu15.hpp"
#include <optional>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
class SourceRuntimeMemory;
struct SourceAttachedEffectLocation {
    std::uint32_t name{};
    std::int16_t node=-1;
    Transform local;
};
// Completed original object attachment slot and its actual effect allocation.
// NONE retains a retired/absent slot; it is not a new native effect request.
struct SourceAttachedEffect {
    std::uint16_t slot{},flags{};
    std::uint32_t instance=0xffffffffu,definition=0xffffffffu;
    float scale_a{},scale_b{};
    std::array<std::uint32_t,2> colors{};
    std::vector<SourceAttachedEffectLocation> locations;
    std::optional<SourceEffectReplicaRenderTu15> render;
    SourceEffectEventState render_clock;
};
// Read-only TU1.5 C0CD0's eight-byte object attachment bank, followed by the
// actual 400-byte effect record updated by179CD0/17C370/17C5C0. Full object
// and effect salts, occupancy and every descriptor extent are checked.
std::vector<SourceAttachedEffect> source_attached_effects_tu15(
    const SourceRuntimeMemory &,NativeRuntimeAddress object_pool,
    NativeRuntimeAddress effect_pool,std::uint32_t object);
void validate_source_attached_effects(std::span<const SourceAttachedEffect>);
}
