#pragma once
#include "halo2/sound_driver_startup.hpp"
namespace halo2 {
struct SourceSoundWetReturnRouteTu15 {
    std::uint32_t input_bin,output_bin,driver_slot_offset;
    std::uint32_t flags,volume;
    bool operator==(const SourceSoundWetReturnRouteTu15&)const=default;
};
// Authored224EC0:225333..225452 mix-in voices. This is routing metadata,
// not a physical frame clock, volume converter or successful device result.
SourceSoundWetReturnRouteTu15 source_sound_wet_return_route_tu15(unsigned);
// Preserve original external-call order and re-read each returned slot after
// volume/routing callbacks. A failed creation skips only that voice's setters;
// later voices are still attempted. Only actual supplied services can create,
// configure or play a resource. This function never creates an admission flag.
void source_sound_driver_wet_returns_tu15(SoundDriverStartupState&,
                                        const SoundDriverStartupServices&);
}
