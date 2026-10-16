#pragma once
#include "halo2/source_vehicle_director_tu15.hpp"
#include "halo2/observer_motion.hpp"

namespace halo2 {
// Owning caller values for the actual TU1.5 private-view camera scheduler.
// Controller/observer words are source values, never native host pointers.
struct SourcePlayerCameraCall {
    std::uint32_t player{};
    float seconds{};
    SourceCameraControllerState controller{};
    SourceObserverState observer{};
    std::uint32_t random{};
    bool cycle{},mode_output{};
};
}
