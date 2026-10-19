#pragma once
#include "halo2/menu_world_source_pass.hpp"

namespace halo2 {
// Exact caller-selected layer21 pass. External target19 is a dependency of
// stage0, not a resident bitmap or evidence that a scene copy has occurred.
// The owner must bind the produced320x240 copy before submitting this pass.
struct SourceHologramMaterial {
    Material material;
    MenuWorldSourcePass pass;
    std::array<unsigned,4> external_targets{};
};
SourceHologramMaterial prepare_hologram_material(const Cache &,const Material &,
    const MenuWorldSourcePassChoice &);
}
