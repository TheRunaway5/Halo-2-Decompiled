#pragma once
#include "halo2/transform.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2 {
// Retail 163DB0 output. The retained fields are not written by that routine.
// Bounds use minX,maxX,minY,maxY,minZ,maxZ; plane equations are n.p - d.
struct SourceCameraFrustum {
    std::uint32_t source_group{};
    std::array<std::uint32_t,3> runtime_retained{};
    ShaderVector rectangle{};
    float metric_retained{};
    std::array<Vec3,4> corners{};
    Vec3 eye{};
    std::array<float,6> bounds{};
    std::array<ShaderVector,6> planes{};
    std::array<Vec3,4> rays{};
};
// Exact retail 441B0 output, including the embedded 0x108-byte frustum.
struct SourceFrustumCamera {
    std::uint8_t orthographic{};
    std::array<std::uint8_t,3> padding0{};
    Transform inverse_view{};
    Transform camera_to_world{};
    std::uint8_t near_plane_enabled{};
    std::array<std::uint8_t,3> padding6d{};
    float near_distance{};
    ShaderVector near_plane{};
    std::uint8_t far_plane_enabled{};
    std::array<std::uint8_t,3> padding85{};
    float far_distance{};
    std::uint8_t full_frustum_enabled{};
    std::array<std::uint8_t,3> padding8d{};
    SourceCameraFrustum full_frustum{};
    std::uint32_t polygon_count{};
    std::array<Vec2,4> polygon{};
};
struct SourceFrustumCameraInput {
    Vec3 position{},forward{},up{};
    float near_distance{},far_distance{};
    // Original 2FD90 view/projection output +0x68..0x74: xmin,xmax,ymin,ymax.
    ShaderVector rectangle{};
};
struct SourceFrustumProjectionInput {
    float projection_angle{}; // Source camera+28 vertical angle, after2F800.
    // Literal camera+30/+38 short banks, ordered top,left,bottom,right.
    std::array<std::int16_t,4> outer{},safe{};
    bool rectangle_override_enabled{}; // Camera+58.
    Vec2 rectangle_center{}; // Camera+5C/+60.
    float rectangle_scale{}; // Camera+64.
    bool screen_offset_enabled{}; // Camera+68.
    Vec2 screen_offset{}; // Camera+6C/+70, in original pixel units.
    bool widescreen{}; // Original global485AC2.
    bool layout_state_present{}; // Global4E6948 pointer.
    std::uint8_t layout_state_flag{}; // State+1.
    std::int16_t layout_state_index=-1; // State+2.
    std::int32_t layout_state_mode{}; // State+8; mode3 excludes widescreen.
    bool auxiliary_state_present{}; // Global510C50 pointer.
    std::uint8_t auxiliary_state_flag{}; // State+5.
    bool scenario_present{}; // Global4E0350 pointer, for2F970 modifier.
    std::int16_t scenario_type{}; // Scenario+10.
    bool use_projection_bounds=true; // Non-null2FB70 EDX.
};
struct SourceFrustumProjection {
    ShaderVector projection_bounds{}; // Complete2F970 result.
    std::array<float,16> projection_program{}; // Complete2FB70 result.
    bool view_modifier{}; // Original4BA019 side effect.
    ShaderVector clipping_rectangle() const {
        return {projection_program[12],projection_program[13],
                projection_program[14],projection_program[15]};
    }
};
static_assert(sizeof(SourceCameraFrustum)==0x108);
static_assert(offsetof(SourceCameraFrustum,rectangle)==0x10);
static_assert(offsetof(SourceCameraFrustum,corners)==0x24);
static_assert(offsetof(SourceCameraFrustum,eye)==0x54);
static_assert(offsetof(SourceCameraFrustum,bounds)==0x60);
static_assert(offsetof(SourceCameraFrustum,planes)==0x78);
static_assert(offsetof(SourceCameraFrustum,rays)==0xd8);
static_assert(sizeof(SourceFrustumCamera)==0x1bc);
static_assert(offsetof(SourceFrustumCamera,inverse_view)==4);
static_assert(offsetof(SourceFrustumCamera,camera_to_world)==0x38);
static_assert(offsetof(SourceFrustumCamera,near_plane_enabled)==0x6c);
static_assert(offsetof(SourceFrustumCamera,near_distance)==0x70);
static_assert(offsetof(SourceFrustumCamera,near_plane)==0x74);
static_assert(offsetof(SourceFrustumCamera,far_plane_enabled)==0x84);
static_assert(offsetof(SourceFrustumCamera,far_distance)==0x88);
static_assert(offsetof(SourceFrustumCamera,full_frustum_enabled)==0x8c);
static_assert(offsetof(SourceFrustumCamera,full_frustum)==0x90);
static_assert(offsetof(SourceFrustumCamera,polygon_count)==0x198);
static_assert(offsetof(SourceFrustumCamera,polygon)==0x19c);

// Retail 441B0. The input contains only fields read by the original constructor;
// it does not infer field values from a host projection or viewport.
SourceFrustumCamera source_frustum_camera(const SourceFrustumCameraInput&);

// Original2F970→2FB70 projection rectangle producer. Actual feature/context
// fields are explicit inputs; neither physical host aspect nor active flags
// are inferred. Pass clipping_rectangle() to441B0's separate rectangle input.
SourceFrustumProjection source_frustum_projection(const SourceFrustumProjectionInput&);

// Retail 163DB0, including 1429D0/11F770 corner/bounds generation and 30BF0
// normalization. Invalid rectangles leave every output byte unchanged.
// Optional diagnostic is the condition that clears original global+47FF86;
// it has no effect on the returned frustum or admission result.
bool source_frustum_rectangle(const SourceFrustumCamera&,ShaderVector rectangle,
    std::uint32_t source_group,SourceCameraFrustum& output,
    bool* plane_inconsistent=nullptr);
}
