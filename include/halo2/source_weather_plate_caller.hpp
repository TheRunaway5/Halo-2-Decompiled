#pragma once
#include "halo2/source_weather_plate_program.hpp"

namespace halo2 {
// The52byte compositor already produced by original37A60. Byte0 is1; its
// three padding bytes are ignored. Alpha triples are {+0,clamped alpha,+0};
// tint is clamped by the caller. The separate camera banks must stay separate.
struct SourceWeatherPlateCallerInput {
    SourceWeatherPlateCompositor compositor;
    float depth_start{},depth_end{};
    float selection_near{},selection_far{};
    float projection_near{},projection_far{};
    std::uint16_t source_fpcw=0x023f;
    std::uint32_t source_mxcsr=0x1f80;
};
enum class SourceWeatherPlateCallerProfile {
    NoDraw,
    AboveNearWDepth,
    // Original030C=0 and0290 bit16=0 at the quad. The existing native plate
    // graphics path must reject this profile until its separate proof/pass.
    NearDepthDisabledUnsupported
};
struct SourceWeatherPlateCallerResult {
    SourceWeatherPlateCallerProfile profile=SourceWeatherPlateCallerProfile::NoDraw;
    std::optional<PixelProgram> program;
    std::optional<float> depth;
    // Exact original37DBE..37E45 immediate Z/W at all four positions. This
    // result does not certify screen/UV/scroll/clip/binding/producer ownership.
    std::optional<std::array<float,2>> projected_zw;
};
// Owns originalmode3/center0/end0/start1/skip1/requested-depth arguments.
// Uses the strict pure builder, then the caller's separately rounded SSE
// projection order and literal D24 clamps. Rejects unsupported compositor,
// arithmetic or nonfinite projection intermediates before publishing output.
// NoDraw leaves every optional output empty. AboveNearWDepth classifies the
// source raster profile; it is not a forged or complete GPU admission token.
SourceWeatherPlateCallerResult source_weather_plate_caller_profile(
    const SourceWeatherPlateCallerInput &);
}
