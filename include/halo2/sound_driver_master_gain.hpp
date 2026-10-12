#pragma once
#include "halo2/sound_driver_startup.hpp"
#include "halo2/sound_driver_parameters.hpp"

namespace halo2 {
struct SoundDriverMasterGainConstants {
  std::uint32_t one_bits,logarithm_scale_bits,quarter_bits;
};
struct SoundDriverMasterGainState {
  std::uint32_t driver,output_globals;
};
struct SoundDriverMasterGainServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
};
// Complete2230E0 CPU owner and original32909C low-DWORD conversion.
// Input is actual raw24-byte parameter backing. Driver/output globals must be
// retained by their owners; this routine never creates or admits a device.
void source_sound_driver_master_gain(const SoundDriverMasterGainState &,
    std::uint32_t input,const SoundDriverMasterGainConstants &,
    const SoundDriverFloatingPointContext &,const SoundDriverMasterGainServices &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
