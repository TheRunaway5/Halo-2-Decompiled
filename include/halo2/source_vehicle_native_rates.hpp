#pragma once
#include <cstdint>

namespace halo2 {
// Process-local read-only diagnostic counts; never gameplay or replay inputs.
struct SourceVehicleNativeRateActivity {
    std::uint64_t acceleration_calls{},approach_calls{},live_cursor_calls{};
};
SourceVehicleNativeRateActivity source_vehicle_native_rate_activity();
} // namespace halo2
