#pragma once
#include "halo2/path_tracing/scene.hpp"

namespace halo2::path_tracing {
// Source lightmap images are independent of material albedo and include the
// original per-cluster/instance palette choice. Cache only this RT owner.
void refresh_color_guides(Scene&,std::span<const Texture>);
std::array<std::uint32_t,4> color_guide_material(const Material&);
ShaderVector color_guide_triangle(const Vertex&,const Vertex&,const Vertex&,const GpuMaterial&);
// CPU reference for primary/secondary atlas transport and static-event hue.
std::optional<Vec3> sample_color_guide(const Scene&,const GpuMaterial&,std::uint32_t image,Vec2 uv,Vec3 vertex_radiance={});
std::optional<Vec3> sample_color_guide(const Scene&,const GpuTriangle&,Vec3 barycentric);
Vec3 guide_static_color(Vec3 incoming,const std::optional<Vec3>& guide);
}
