#pragma once
#include "halo2/source_vertex_declaration.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace halo2 {
// Retail ranges pack a nine-bit first index and seven-bit element count.
struct ShaderRange {
  std::uint16_t packed{};
  std::size_t first() const { return packed & 511; }
  std::size_t count() const { return packed >> 9; }
};
struct ShaderFragment {
  std::int16_t switch_index{-1};
  ShaderRange permutations;
};
struct ShaderPermutation {
  std::int16_t value{-1};
  std::uint16_t flags{};
  ShaderRange combiners;
};
struct ShaderCombiner {
  std::array<std::uint32_t, 8> words{};
};
struct ShaderRemap {
  std::uint16_t destination{};
  std::uint8_t component{}, source{};
};
using ShaderVector = std::array<float, 4>;
using ShaderConstants = std::array<std::array<ShaderVector, 16>, 2>;
// Draw-specific source globals beyond the ordinary c18..33 material bank.
// PRT binds c68 onward and supplies its own authored v5/v6 stream.
struct VertexProgramBank {
  unsigned first_constant = 34;
  std::vector<ShaderVector> constants;
  bool prt_inputs{};
  // An explicitly supplied original c0..11 bank. It remains independent of
  // the current Vulkan target viewport (for example, fixed shadow target10).
  // This requires first_constant0 and at least twelve complete vectors.
  bool camera_inputs{};
};
struct PackedVertexProgramBank {
  std::array<std::uint32_t,4> range{};
  std::array<ShaderVector,158> constants{};
  bool operator==(const PackedVertexProgramBank &) const = default;
};
static_assert(sizeof(PackedVertexProgramBank)==2544);
PackedVertexProgramBank pack_vertex_program_bank(const VertexProgramBank &);
// The 60 retail D3DPIXELSHADERDEF words, in original order. No host GPU
// handles.
struct PixelProgram {
  std::array<std::uint32_t, 60> words{};
  std::array<std::uint8_t, 8> fragment_offsets{};
};
struct ShaderTextureBinding {
  std::uint8_t parameter{}, external{}, stage{}, flags{};
};
struct ShaderState {
  std::uint8_t index{};
  std::uint32_t value{};
};
// Retail 000189A0 consumes one 24-byte record for each texture stage.
struct ShaderTextureState {
  std::uint32_t address_u = 1, address_v = 1, address_w = 1;
  std::uint32_t mag_filter = 2, min_filter = 2, mip_filter = 2;
  std::uint32_t lod_bias{}, max_mip{}, anisotropy{}, color_key_op{};
  std::uint32_t color_sign{}, alpha_kill{}, border_color{}, key_color{};
  auto operator<=>(const ShaderTextureState &) const = default;
};
ShaderTextureState decode_texture_state(std::span<const std::byte>);
struct ShaderBitmap {
  std::uint32_t datum{}, index{};
  float log_dimension{};
};
struct ShaderLod {
  ShaderRange layers;
  std::uint32_t available_layers{};
  float projected_height{};
};
struct MaterialPass {
  std::string name, vertex_shader;
  std::uint32_t template_pass{}, implementation{};
  std::vector<ShaderTextureBinding> textures;
  std::vector<ShaderState> states;
  std::vector<ShaderTextureState> texture_states;
  std::vector<ShaderRemap> bitmap_remaps, pixel_remaps, vertex_remaps;
  std::vector<ShaderRemap> pixel_externs, vertex_externs, state_externs;
  ShaderConstants constants{};
  PixelProgram program;
  PixelProgram definition;
  std::vector<ShaderFragment> fragments;
  std::vector<ShaderPermutation> permutations;
  std::vector<ShaderCombiner> combiners;
  std::vector<std::byte> vertex_code;
  // The selected compiled world class's literal register semantic list.
  // Local/skinned variants require their own decoded source streams.
  std::optional<SourceVertexDeclaration> vertex_declaration;
};

// Port of retail 0001BA00. Switch values index each fragment's permutation
// range, matching the executable; unknown or out-of-range switches are errors.
PixelProgram
assemble_pixel_program(const PixelProgram &definition,
                       std::span<const ShaderFragment> fragments,
                       std::span<const ShaderPermutation> permutations,
                       std::span<const ShaderCombiner> combiners,
                       std::span<const std::int32_t> switches);
// Ports of 00017000 and 00017170; preserve components absent from a remap.
// Vertex bank 0 sets constant vertex attributes, bank 1 sets shader constants.
void remap_vertex_constants(ShaderConstants &destination,
                            std::span<const ShaderVector> source,
                            std::span<const ShaderRemap> remaps);
void remap_pixel_constants(PixelProgram &destination,
                           std::span<const std::uint32_t> source,
                           std::span<const ShaderRemap> remaps,
                           std::span<const ShaderFragment> fragments,
                           std::span<const std::int32_t> switches);
// Retail 00019CA0 extern binding, including scalar operations, RGB vectors,
// signed-to-unsigned conversion and permutation filtering before evaluation.
void remap_pixel_externs(PixelProgram &destination,
                        std::span<const float> scalars,
                        std::span<const ShaderVector> colors,
                        std::span<const ShaderRemap> remaps,
                        std::span<const ShaderFragment> fragments,
                        std::span<const std::int32_t> switches);
} // namespace halo2
