#pragma once
#include "halo2/cache.hpp"
namespace halo2 {
using ObjectColors = std::array<Vec3, 4>;
//3DD10/BAD50 packs the current object RGB through131ED0;33980 then
//unpacks that byte bank through131BB0 before either vertex or pixel externs.
//Constructor/current object colors remain raw floats until this draw boundary.
ObjectColors source_object_render_colors(const ObjectColors &);
// Copy the selected authored pass before applying object externs, so changing
// player colors never alters another model's material.
void apply_object_colors(Material &, const ObjectColors &);
// Draw-owner color externs, after the source render-bank byte conversion.
void apply_object_render_color_bindings(PixelProgram &,ShaderConstants &,
    const MaterialPass &,const ObjectColors &,std::span<const std::int32_t> switches);
// Already packed/unpacked33980 draw colors. Resident shader alternatives keep
// this exact retained bank, without repeating the object's byte conversion.
void apply_object_render_colors(Material &, const ObjectColors &);
// Bind the retained object draw bank onto the caller's current authored bank.
// The original animated parameter masks run afterward. Does not recurse.
void reapply_object_colors(Material &);
void prepare_surface_material(const Cache &, Material &);
void prepare_model_environment(const Cache &, Material &);
// Shared authored transparency/emissive bindings for skies and frontend models.
void prepare_forward_materials(const Cache &, Mesh &);
bool prepare_forward_material(const Cache &, Material &);
} // namespace halo2
