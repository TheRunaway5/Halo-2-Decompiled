#pragma once
#include "halo2/cache.hpp"

namespace halo2::pstencil {
// Private pStencil forward-pipeline phases. Target9 stores world normals,
// target10 stores completed per-light utility, and the final phase adds light.
inline constexpr unsigned light_utility_blend_2003=13;
inline constexpr unsigned dependent_light_blend_2003=14;
inline constexpr unsigned normal_accumulation_blend_2003=15;
inline constexpr unsigned normal_light_utility_blend_2003=16;
inline bool normal_accumulation_material_2003(const Material& m) {
    return m.forward_pass && m.forward_vertex==6 && m.forward_blend==normal_accumulation_blend_2003;
}
inline bool normal_light_utility_material_2003(const Material& m) {
    return m.forward_pass && m.forward_vertex==6 && m.forward_blend==normal_light_utility_blend_2003;
}
inline bool light_utility_material_2003(const Material& m) {
    return m.forward_pass && m.forward_vertex==6 && m.forward_blend==light_utility_blend_2003;
}
inline bool dependent_light_material_2003(const Material& m) {
    return m.forward_pass && m.forward_vertex==6 && m.forward_blend==dependent_light_blend_2003;
}
}
