#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
struct SourceWeatherPlateSimulationInput {
    // Literal485600 view bank, including its signed slot and both transforms.
    std::span<const std::byte> view;
    // Actual owning weat root:36E5A supplies the resolved tag definition.
    std::span<const std::byte> weather;
    float elapsed{},intensity{}; // Actual509418 and4670FC caller values.
};
// Complete retail37260 over a separately owned936-byte plate and supplied
// source view/weather banks. No clock, camera, RNG chronology or publication
// producer is inferred. Returns the literal successful AL value.
bool source_weather_step_plate(std::span<std::byte> plate,
    const SourceWeatherPlateSimulationInput &,std::uint32_t &auxiliary);
}
