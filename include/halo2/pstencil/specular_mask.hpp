#pragma once
#include "halo2/cache.hpp"
namespace halo2::pstencil {
// Validate and preserve the authored seven-combiner program. A separate
// fragment-stage sample supplies texture accumulation alpha as its mask;
// normal, tint, lobe, scratch registers and light remain authored.
PixelProgram texture_masked_specular(const PixelProgram&);
bool texture_masked_specular_material(const Material&);
bool texture_mask_pass(const Material&);
// Select the same shader and CPU input transform. Combined shader LODs use
// the resident independent texture pass only to populate the pStencil mask.
std::vector<unsigned> texture_mask_materials(std::span<const Material>);
}
