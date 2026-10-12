#pragma once
#include "halo2/render_program.hpp"

namespace halo2 {
// Original19BE0 ->1B2C0. The extern's destination packs bank/register,
// component and texture selector; ShaderRemap::component (byte+2) is ignored.
// Original19BE0 passes scalar mode0 to1B2C0. Its texture selector has no effect
// on admitted scalar components0..3 or RGB component4. Dimension components
// require their separate source texture owner and throw here.
// Components without a mapping survive. Returned masks are ORed into the
// caller's existing dirty banks by the original owner.
std::array<std::uint32_t,2> remap_source_vertex_externs(
    ShaderConstants &,std::span<const float> scalars,
    std::span<const ShaderVector> colors,std::span<const ShaderRemap>);
}
