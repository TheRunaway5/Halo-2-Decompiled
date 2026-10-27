#pragma once
#include "halo2/path_tracing/scene.hpp"

namespace halo2::path_tracing {
// CPU reference for the transported, source-qualified emissive programs.
// UV is the source mesh coordinate before the pass's c18..22 transforms.
Vec3 material_emission(const Scene&,std::uint32_t material,Vec2 uv);
}
