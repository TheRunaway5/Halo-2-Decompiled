#pragma once

#include "halo2/vista_loading.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace halo2 {

// Expanded by unchanged 66E828 from descriptor63 at 7E4F23. These are D3D9
// declaration values, including COLOR0 at byte24 and both float2 texcoords.
struct VistaLoadingVertexElement {
  std::uint16_t stream,offset;
  std::uint8_t type,method,usage,usage_index;
  bool operator==(const VistaLoadingVertexElement&) const = default;
};
inline constexpr std::array<VistaLoadingVertexElement,5> vista_loading_vertex_declaration{{
    {0,0,1,0,0,0},{0,8,1,0,5,0},{0,16,1,0,5,1},{0,24,4,0,10,0},{255,0,17,0,0,0}}};

struct VistaLoadingVertex {
  std::array<std::uint32_t,2> position;
  std::array<std::uint32_t,2> texcoord0;
  std::array<std::uint32_t,2> texcoord1;
  std::uint32_t argb;
  bool operator==(const VistaLoadingVertex&) const = default;
};
static_assert(sizeof(VistaLoadingVertex)==28);

enum class VistaLoadingBlend { one_one,destination_color_zero,source_alpha_inverse_source_alpha };
struct VistaLoadingDraw {
  std::uint32_t primitive; // Actual D3DPT_TRIANGLESTRIP5 / TRIANGLEFAN6; two primitives.
  std::uint32_t shader;    // Literal loading program0..3.
  std::uint32_t texture;   // Actual loading.bin table index; fade program does not sample.
  VistaLoadingBlend blend;
  std::optional<std::array<std::uint32_t,4>> pixel_constant0;
  std::array<VistaLoadingVertex,4> vertices;
};

inline constexpr std::array<std::uint32_t,20> vista_loading_vertex_constants177{{
    0x40000000,0,0,0xbf800000,0,0xc0000000,0,0x3f800000,
    0,0,0,0x3f000000,0,0,0,0x3f800000,0x3f800000,0x3f800000,0,0x3f800000}};

// Native projection of normal-mode66A512 and 66A0E2 draw arguments. The elapsed
// word is the original unsigned clock conversion after its binary32 spill.
// Sweep progress remains an explicit E4AFE8 input: no producer is invented.
// Optional fade color comes from vista_loading_fade_update/47602F.
// This API owns geometry/math only, not D3D state/lifetime, scheduling, texture
// decoding, vertex-program selection, shader precision or pixel equivalence.
std::vector<VistaLoadingDraw> vista_loading_geometry(std::uint32_t elapsed_seconds_bits,
    std::uint32_t width,std::uint32_t height,std::uint32_t sweep_progress_bits,
    std::optional<std::uint32_t> fade_argb,const VistaLoadingFloatingPoint&);

} // namespace halo2
