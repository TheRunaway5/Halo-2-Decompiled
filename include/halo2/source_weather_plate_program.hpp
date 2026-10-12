#pragma once
#include "halo2/render_program.hpp"
#include <optional>

namespace halo2 {
// Original 3A5C0 record: one byte flag (three ignored padding bytes), then
// four float3 triples at offsets4,10h,1Ch,28h. Preserve the source bit patterns.
struct SourceWeatherPlateCompositor {
    std::array<std::uint32_t,13> words{};
};
static_assert(sizeof(SourceWeatherPlateCompositor)==52);
struct SourceWeatherPlateProgramInput {
    SourceWeatherPlateCompositor compositor;
    float depth_start{},depth_end{},near_clip{},far_clip{};
    std::uint32_t mode=3;
    // Original arguments inspect only the low byte. Their upper bytes are
    // retained here so callers cannot accidentally replace this with !=0.
    std::uint32_t center{},anchor_end{},anchor_start=1,skip_state=1;
    bool depth_requested=true;
    std::uint16_t source_fpcw=0x023f;
    std::uint32_t source_mxcsr=0x1f80;
};
struct SourceWeatherPlateProgramResult {
    // Empty on the original pre-mutation false return. An existing program
    // and optional depth bank must then remain owned and unchanged by caller.
    std::optional<PixelProgram> program;
    std::optional<float> depth;
};
// Pure mode3 producer, matching original source order and packed60word output.
// Requires skip_state's low byte nonzero and nearest/PC53/gradual arithmetic.
// Host control inspection is supported on x86 SSE (including MSVC x86/x64);
// other host control implementations are rejected until independently proved.
// Unsupported modes, arithmetic controls, nonfinite/overflow classifications
// throw std::invalid_argument; they are not invented original false returns.
// Source RGB packing rounds nearest-even and masks bytes; it does not clamp.
// Raster setup and the unconditional successful1CCF0 SDK installation are
// separate boundaries. This function owns no SDK, texture, simulation or GPU.
SourceWeatherPlateProgramResult source_weather_plate_program(
    const SourceWeatherPlateProgramInput &);
}
