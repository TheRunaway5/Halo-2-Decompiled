#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace halo2 {
struct SourceWeatherExclusionVolume {
    std::array<float,3> center{};
    float radius{};
    std::int32_t plane_count{};
    std::span<const std::array<float,4>> planes; // nx,ny,nz,d
};
// Complete38390 over owned point/volume/plane inputs. Sphere membership is
// strict; a plane boundary at epsilon is included. Signed count<=0 consumes
// no plane storage. No visibility, resource readiness or particle update is
// inferred. Source SSE arithmetic order requires contraction disabled.
bool source_weather_point_in_exclusion(const std::array<float,3> &point,
    const SourceWeatherExclusionVolume &volume);
} // namespace halo2
