#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_geometry_lod.hpp"

namespace halo2 {
// 443B2..443DF consumes the actual 2C490 feature bank. Availability,
// source context and draw-owner admission remain separate inputs.
bool source_render_layer_enabled(const SourceGeometryLodPolicy &,unsigned layer);
// Highest-LOD template layer for the explicitly prepared forward pass.
// Procedural passes without template metadata have no source layer here.
std::optional<unsigned> source_material_layer(const Material &);
}
