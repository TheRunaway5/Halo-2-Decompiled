#pragma once
#include "halo2/sound_driver_parameters.hpp"
#include <array>
namespace halo2 {
struct SourceSoundGainEnvelopeTu15 {
    std::uint32_t class_lower,class_upper,scale_lower,scale_upper;
};
//127CF0 through127D2D: actual21F780/21F7F0 bounds,220000 amplitudes,
// extended interpolation and220200 dB conversion. No class/ducking owner,
// parent datum, voice allocation or device result is supplied here.
std::uint32_t source_sound_gain_envelope_tu15(const SourceSoundGainEnvelopeTu15&,
    std::uint32_t control,const SoundDriverFloatingPointContext&);
//2216D0's four independent numeric output gains. The retained parent and
// actual class/ducking owner supply these typed values after identity checks.
std::array<std::uint32_t,4> source_loop_combined_gain_tu15(
    const std::array<std::uint32_t,4>& amplitudes,const SourceSoundGainEnvelopeTu15&,
    std::uint32_t control,std::uint32_t parent_gain,std::uint32_t ducked_class_gain,
    const SoundDriverFloatingPointContext&);
}
