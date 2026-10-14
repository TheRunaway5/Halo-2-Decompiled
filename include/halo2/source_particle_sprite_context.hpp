#pragma once
#include "halo2/particle_sprite.hpp"
#include <optional>

namespace halo2 {
struct SourceParticleSpriteContext {
    std::int32_t selected_sequence=-1;
    float width=1;
    bool has_bitmap=true;
};
// Original TU1.5 22AE16..22AEC4: the selected bitmap's first data row supplies
// width/height, and the authored first sequence selects the initial atlas.
// No bitmap data means the original caller emits no sprite. Simulation,
// material admission and per-particle random sequence selection are separate.
std::optional<SourceParticleSpriteContext> source_particle_sprite_context(
    const Cache &,std::uint32_t particle,const ParticleSpriteDefinition &);
}
