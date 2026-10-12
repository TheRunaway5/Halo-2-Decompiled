#pragma once
#include "halo2/source_weather_plate_caller.hpp"
#include <span>

namespace halo2 {
struct SourceWeatherPlateSubmissionInput {
    std::span<const std::byte> plate; // Current owned 0x3a8 bytes, including all four views.
    std::int16_t slot{};
    std::array<std::int16_t,4> rectangle{}; // Original top,left,bottom,right.
    std::array<float,2> selection{},projection{};
    std::uint16_t source_fpcw=0x023f;
    std::uint32_t source_mxcsr=0x1f80;
};
struct SourceWeatherPlateImmediateVertex {std::array<ShaderVector,5> registers{};};
static_assert(sizeof(SourceWeatherPlateImmediateVertex)==80);
struct SourceWeatherPlateSubmission {
    SourceWeatherPlateCallerInput caller;
    SourceWeatherPlateCallerResult result;
    std::array<ShaderVector,3> scroll{};
    std::array<SourceWeatherPlateImmediateVertex,4> vertices{};
};
// Original 37A60's current-row compositor, c21..23 and immediate v0..4,
// composed with the proved 3A5C0/37DBE caller. NoDraw retains the scroll
// commands but has no quad/program. The original function returns AL1 in
// both cases. This owns no SDK, texture readiness, camera, depth or GPU.
SourceWeatherPlateSubmission source_weather_plate_submission(const SourceWeatherPlateSubmissionInput &);
}
