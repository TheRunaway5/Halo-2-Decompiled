#pragma once
#include "halo2/transform.hpp"
#include "halo2/render_view.hpp"
#include <cstddef>

namespace halo2 {
// Original 2FD90 input fields. projection_program is the actual complete
// 2FB70 result, already available from source_frustum_projection.
struct SourceViewProjectionInput {
    Vec3 position{},forward{},up{};
    float near_distance{},far_distance{};
    ShaderVector oblique_near_plane{}; // Camera+48..54, read only if near==0.
    std::array<float,16> projection_program{};
};
// Literal 2FD90 output. It is distinct from 441B0's clipping/frustum camera:
// the render camera reconstructs perpendicular up before normalizing.
struct SourceViewProjectionBank {
    Transform inverse_view{};
    Transform camera_to_world{};
    ShaderVector clipping_rectangle{};
    std::array<float,16> projection{};
    Vec2 viewport_half_metrics{};
};
static_assert(sizeof(SourceViewProjectionBank)==0xc0);
static_assert(offsetof(SourceViewProjectionBank,camera_to_world)==0x34);
static_assert(offsetof(SourceViewProjectionBank,clipping_rectangle)==0x68);
static_assert(offsetof(SourceViewProjectionBank,projection)==0x78);
static_assert(offsetof(SourceViewProjectionBank,viewport_half_metrics)==0xb8);

SourceViewProjectionBank source_view_projection(const SourceViewProjectionInput&);

// Original 12D50 ordered view/projection products followed by 12FA0's
// c4/c5/c6 basis copies. The first four vectors are source GPU constants,
// with no Vulkan clip conversion or GLM matrix reconstruction.
// actual_w_distance is submitting camera+44. In the canonical image,2BA10
// and132D0 publish48565C; TU1.5's2BAEF/132B0 publish491E7C instead.
// These image-specific addresses must not be interchanged.
// first_person selects original 12D50 BL nonzero, multiplying c0..3 by .15.
std::array<ShaderVector,7> source_view_shader_constants(
    const SourceViewProjectionBank&,float actual_w_distance,bool first_person);
// Complete12FA0 c0..11 bank, with the actual submitting camera position and
// source viewport/scale/mode inputs. first_person selects the later12D50
// foreground replacement of c0..3; c4..11 remain the ordinary view bank.
// A fixed shadow target does not replace the submitting view's viewport here.
std::array<ShaderVector,12> source_view_shader_bank(
    const SourceViewProjectionBank&,float actual_w_distance,Vec3 submitting_eye,
    RenderViewport,bool first_person);
}
