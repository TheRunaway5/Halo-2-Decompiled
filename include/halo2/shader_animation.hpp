#pragma once
#include "halo2/render_program.hpp"
#include <functional>
#include <memory>
#include <optional>

namespace halo2 {
class Bytes;
struct Material;
// Verified Xbox function tables retained in the port's owned material library.
struct ShaderFunctionTables {
  std::array<std::array<std::uint8_t, 1024>, 12> periodic{};
  std::array<std::array<std::uint8_t, 1024>, 16> transition{};
};
struct ShaderOverlay {
  std::uint32_t input{}, range{};
  float period{};
  std::vector<std::byte> curve;
};
struct ShaderOverlayReference {
  std::int16_t function{}, transform{};
};
struct ShaderAnimationImplementation {
  ShaderRange bitmap_transforms, pixel_constants, vertex_constants;
};
struct ShaderAnimationLod {
  std::uint32_t available_layers{};
  ShaderRange layers;
};
struct ShaderAnimation {
  std::vector<ShaderAnimationLod> lods;
  std::vector<ShaderRange> layers, passes, parameters;
  std::vector<ShaderAnimationImplementation> implementations;
  std::vector<ShaderOverlay> overlays;
  std::vector<ShaderOverlayReference> references;
  std::vector<ShaderRemap> mappings;
  std::shared_ptr<const ShaderFunctionTables> tables;
};
struct ObjectFunction {
  std::uint32_t flags{}, input{}, output{}, enable{};
  float threshold{};
  std::vector<std::byte> curve;
  std::uint32_t scale{};
};
struct FunctionValue {
  float value{};
  bool enabled{};
};
using ShaderInput = std::function<float(std::uint32_t)>;
using ObjectFunctionInput = std::function<std::optional<float>(std::uint32_t)>;
// 108D90's recognized scalar/active outputs are distinct from an interpolated
// 10AAC0 override. Supplying a recognized zero must retain inactive status.
using ObjectFunctionHandler = std::function<std::optional<FunctionValue>(std::uint32_t)>;
ShaderAnimation read_shader_animation(const Bytes &, std::size_t definition,
                                      std::int64_t magic);
float shader_function_fraction(std::span<const std::byte>, float input,
                               float range, const ShaderFunctionTables &);
float shader_function_value(std::span<const std::byte>, float input,
                            float range, const ShaderFunctionTables &);
std::uint32_t shader_function_color(std::span<const std::byte>, float fraction);
FunctionValue evaluate_object_function(std::span<const ObjectFunction>,
                                       const ShaderFunctionTables &,
                                       std::uint32_t name, float seconds,
                                       const ObjectFunctionInput &,
                                       const ObjectFunctionHandler & = {},
                                       bool original_builtin_constants = true);
// Retail 174D0/17550/176A0/17960. Retains the selected pass and changes only
// its animated constants. Per-object inputs are supplied by the scene owner.
bool animate_forward_material(Material &, double seconds, const ShaderInput &);
// Apply the same authored masks to an independent owner bank. The caller
// supplies the selected pass constants, including earlier object externs.
bool animate_forward_constants(const ShaderAnimation &, const MaterialPass &,
    const std::array<std::int32_t,8> &, double seconds, const ShaderInput &,
    PixelProgram &, ShaderConstants &);
// Retail 17550 property zero is the bitmap frame, independent of UV transforms.
void prepare_bitmap_animation(const class Cache &, Material &);
bool animate_material_bitmaps(Material &, double seconds, const ShaderInput &);
// 1B0A0 wraps property zero by the bitmap tag's resident image count.
int bitmap_frame_index(float value, unsigned image_count);
// Retail FF5F0 primary magazine digit inputs, normalized before the authored curve.
std::optional<float> weapon_ammunition_input(std::uint32_t name, int loaded);
} // namespace halo2
