#pragma once
#include "halo2/sound_driver_parameters.hpp"
#include <array>
#include <cstdint>
namespace halo2 {
struct SourceVehicleAudioListenerInputTu15 {
    std::array<std::uint32_t,3> position,forward,up,unit_point,unit_aim;
    std::uint32_t factor; // Actual authored seat+40, without clamping.
};
struct SourceVehicleAudioListenerTu15 {
    std::array<std::uint32_t,3> position,forward,up;
    std::array<std::uint32_t,4> rotation;
};
//129F10:12A05C..12A308 with complete120060/120310/11F9C0/11FFD0.
// Finite component operands and the original identity quaternion. Admission,
// actual CD440 point production and BSP location are the caller's owners.
// Arithmetic uses the caller's masked FP controls and restores its full state.
SourceVehicleAudioListenerTu15 source_vehicle_audio_listener_tu15(
    const SourceVehicleAudioListenerInputTu15&,const SoundDriverFloatingPointContext&);
}
