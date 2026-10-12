#pragma once
#include <cstdint>

namespace halo2 {
// Original146240's two words. The simulation owns the deterministic word;
// the application owns the auxiliary word across BSP/renderer handoffs.
//177C20 selects the deterministic word only for effe flag2. Particle-system,
// location, particle-birth and decal draws use the auxiliary word separately.
struct SourceEffectRandom {
    std::uint32_t &primary;
    std::uint32_t &auxiliary;
    std::uint32_t &event(std::uint32_t effect_flags) const {
        return effect_flags&4u?primary:auxiliary;
    }
};
}
