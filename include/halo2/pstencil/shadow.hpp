#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace halo2::pstencil {
// These names belong to the optional native presentation layer. The 2003
// executable did not have a retail/2003 graphics-mode selector.
enum class GraphicsMode : std::uint8_t { Original, pStencil, PathTracing };
constexpr const char* graphics_mode_name(GraphicsMode mode) {
    return mode==GraphicsMode::Original?"Original":mode==GraphicsMode::pStencil?"pStencil":
        mode==GraphicsMode::PathTracing?"Path Tracing":"Unknown";
}

struct StitchQuad {
    // Original doubled point indices: even=unextruded, odd=extruded.
    // The source writes all four scratch words even when count is zero.
    std::array<std::uint16_t,4> indices;
    std::uint32_t count;
    bool operator==(const StitchQuad&) const=default;
};
// May 2003 halo2debug XBE1773D0, linker566870. Callers own the actual
// adjacency, plane-facing bits, destination capacity and draw admission.
// No position welding or guessed replacement ISQ topology occurs here.
StitchQuad stitch_quad(std::uint16_t first,std::uint16_t second,
                      bool first_face,bool second_face);

struct RenderState {
    std::uint32_t index,value;
    bool operator==(const RenderState&) const=default;
};
// Ordered state writes from XBE1CE3A0's ordinary stencil branch, with its
// debug-color path disabled. Target, pixel-program, clear and scissor services
// remain independent; this list alone is not a complete render pass.
std::span<const RenderState> shadow_stencil_states();
// May2003 E81A0, separate layer7 lightmap-shadow composite. The shipped
// 48F444 opacity is .4; x87 trunc((1-float32(.4))*255) is RGB byte152.
// This is applied once to the baked-light shadow mask, not per dynamic light.
inline constexpr std::uint8_t lightmap_shadow_rgb=152;
std::span<const RenderState> lightmap_shadow_states();
inline constexpr std::uint8_t shadow_stencil_clear=0x80;
// F7F20 passes this exact value to A4C50/A3CF0, then c[-76].w. This is
// extrusion distance, not a guessed light radius or shadow fade distance.
inline constexpr float shadow_extrusion_distance=1024.f;

enum class LightPhase : std::uint32_t { Shadows=13,Utility=16,Lighting=17 };
// F7F20's actual render_scene_geometry(scene_type, layer) calls. The
// shadowless branch also disables stencil separately before these draws.
struct LightPass {std::uint32_t scene;LightPhase phase;};
std::span<const LightPass> light_passes(bool shadows);
} // namespace halo2::pstencil
