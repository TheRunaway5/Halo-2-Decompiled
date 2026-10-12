#pragma once
#include "halo2/cache.hpp"
namespace halo2 {
Texture prepare_cubemap(const Bytes&, std::uint32_t width, std::uint16_t format,
                        bool swizzled, unsigned additional_mips);
// Retail 000359E0: resident mip count; BC chains stop when the largest side
// reaches 4 texels. Linear and non-power-of-two images have no mip chain.
unsigned retail_mip_count(unsigned width, unsigned height, unsigned depth,
                          unsigned format, bool linear, unsigned maximum);
// Keep BC data compressed for Vulkan; convert Xbox swizzled byte/BGRA textures to RGBA8.
Texture prepare_texture(const Bytes& raw, std::uint32_t width, std::uint32_t height,
                        std::uint16_t format, bool swizzled, unsigned additional_mips = 0);
// Xemu Vulkan's original BC1 expansion uses integer truncation for endpoints
// and interpolation. Expand before filtering for source-matched title effects.
Texture prepare_xemu_bc1_texture(const Texture &);
// Pinned Xemu DXT3: opaque four-entry RGB interpolation and independent
// explicit four-bit alpha, expanded before filtering.
Texture prepare_xemu_bc2_texture(const Texture &);
// Same source integer BC3 kernel used by prepare_ui_bitmap; this 2D API
// preserves that caller's existing behavior and endpoint/alpha arithmetic.
Texture prepare_xemu_bc3_texture(const Texture &);
// Explicit pinned-Xemu Vulkan storage preparation. BC1/BC2/BC3, 2D or
// six-face cube, retaining every logical mip extent and face-major order.
// Volumes and malformed/incomplete face chains are rejected.
Texture prepare_xemu_bc_texture(const Texture &);
// NV2A volume Morton order interleaves X/Y/Z while each dimension has bits.
// Mips are consecutive XYZ volumes with X varying fastest after decoding.
Texture prepare_volume_texture(const Bytes& raw, std::uint32_t width,
    std::uint32_t height,std::uint32_t depth,std::uint16_t format,
    bool swizzled,unsigned additional_mips=0);
// P8 indices and the bound Xbox BGRA palette are expanded before interpolation.
Texture prepare_palettized_texture(const Bytes& raw, std::uint32_t width, std::uint32_t height,
    bool swizzled, std::span<const std::byte> palette, unsigned additional_mips = 0);
// A native-only storage format: mixed signed RGB/unsigned alpha must be
// decoded before filtering, which a single RGBA8 SNORM view cannot express.
inline constexpr std::uint16_t native_rgba32f = 0xfffe;
Texture prepare_signed_texture(Texture, std::uint32_t color_sign);
}
