#pragma once
#include "halo2/transform.hpp"
#include "halo2/source_effect_location.hpp"
#include "halo2/source_effect_event.hpp"
#include <cstdint>

namespace halo2 {
class SourceRuntimeMemory;
struct ShaderFunctionTables;
// Read-only presentation fields from a completed original effect allocation.
// A transported identity names that peer's effect; it never authenticates a
// local simulation pool, authorizes a local lookup, or supplies an admission.
struct SourceEffectReplicaRenderTu15 {
    std::uint32_t identity=0xffffffffu,attached_owner=0xffffffffu,local_view=0xffffffffu;
    std::uint16_t flags{};
    Vec3 color,velocity;
    std::uint32_t root_owner=0xffffffffu;
    std::uint8_t root_type=0xffu;
};
SourceEffectReplicaRenderTu15 source_effect_replica_render_tu15(
    const SourceRuntimeMemory &,std::uint32_t full_effect,std::uint32_t definition);
void validate_source_effect_replica_render_tu15(const SourceEffectReplicaRenderTu15 &);
// Closed original178E30/17B0D0 WORLD branch. Dynamic attached color, moving
// locations and FP ownership require their own current-frame producers.
bool source_effect_replica_static_world_tu15(const SourceEffectReplicaRenderTu15 &);
// Current-frame attached WORLD packet with a fully observed non-biped root.
bool source_effect_replica_attached_world_tu15(const SourceEffectReplicaRenderTu15 &);
struct SourceEffectPartLensControlsTu15 {
    std::uint32_t scale_a{},scale_b{};
    std::uint16_t fade_curve{};
};
// Literal17C81A..17C85D and17CB0D..17CB7A. The current event clock,
// actual controls and authenticated lens transition table remain callers.
float source_effect_part_lens_opacity_tu15(const SourceEffectPartLensControlsTu15 &,
    SourceEffectControls,const SourceEffectEventState &,const ShaderFunctionTables &);
}
