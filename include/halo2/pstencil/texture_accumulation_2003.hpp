#pragma once
#include "halo2/cache.hpp"
namespace halo2::pstencil {
inline constexpr unsigned texture_accumulation_blend_2003=17;
PixelProgram texture_accumulation_2003();
// Authenticate the current base/detail and color-change subgraph before using
// its completed color factor with the original constructor's RGB/alpha stages.
bool supports_texture_accumulation_2003(const Material&);
Material adapt_texture_accumulation_2003(Material);
inline bool texture_accumulation_material_2003(const Material& m) {
    return m.forward_pass && m.forward_vertex==3 && m.forward_blend==texture_accumulation_blend_2003;
}
}
