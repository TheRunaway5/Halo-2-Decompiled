#pragma once
#include "halo2/cache.hpp"
#include <optional>

namespace halo2::path_tracing {
// The light a glow layer (see glow_material) adds to the frame, averaged over
// its surface. The layer's own NV2A register-combiner program is evaluated on
// a grid of its authored texels and weighted by the layer's blend: an
// additive teleporter field's blue plasma, not its grey noise texture.
//
// Environment cube stages contribute black (reflection, not emission) and
// volume stages their mean colour. Empty when an active stage holds an image
// the CPU cannot read; callers keep their constant tint.
std::optional<Vec3> average_glow_color(const Material&);
// Mean RGBA of an authored image, or empty when the CPU cannot read it.
std::optional<ShaderVector> average_image_rgba(const Texture&);
}
