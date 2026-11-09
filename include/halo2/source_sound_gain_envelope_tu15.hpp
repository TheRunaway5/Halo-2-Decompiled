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

struct SourceCombinedDriverPacketTu15 {
    std::uint32_t flags, common_gain, voice_gain;
    std::array<std::uint32_t,4> channel_gains;
};
struct SourceCombinedMixBinTu15 {
    std::uint32_t bin;
    std::int32_t attenuation;
    bool operator==(const SourceCombinedMixBinTu15&) const = default;
};
struct SourceCombinedMixBinsTu15 {
    std::array<SourceCombinedMixBinTu15,8> routes{};
    unsigned count{};
};
struct SourceCombinedDriverRoutingTu15 {
    std::uint32_t category, plugin_send_gain;
    std::uint16_t mode, auxiliary_bin;
    std::uint8_t layout;
};

// Combined branch of 226490. Accept the driver voice's flags, gain and
// four gains after source publication. Preserve the raw per-bin gains
// alongside the common gain derived from their combined amplitude.
SourceCombinedDriverPacketTu15 source_combined_driver_packet_tu15(
    std::uint32_t driver_flags, std::uint32_t voice_gain,
    const std::array<std::uint32_t,4>& channel_gains,
    const SoundDriverFloatingPointContext&);

// 2B9450 and 2B96B0 append to the original empty eight-slot route list.
// The caller supplies the plugin's send gain, original category, layout
// and driver route selectors. Auxiliary bin 0xffff selects no auxiliary send.
SourceCombinedMixBinsTu15 source_combined_driver_routes_tu15(
    const SourceCombinedDriverPacketTu15&, const SourceCombinedDriverRoutingTu15&,
    const SoundDriverFloatingPointContext&);
}
