#pragma once
#include "halo2/sound_sdk_voice_attachment.hpp"
namespace halo2 {
// A value description of an already prepared original wet voice. Numeric
// source addresses remain identities, never native pointers or DMA receipts.
class SourceSoundWetVoiceLayoutTu15 final {
public:
    std::uint32_t input_bin()const noexcept{return input_bin_;}
    std::uint32_t byte_offset()const noexcept{return byte_offset_;}
    std::uint32_t byte_extent()const noexcept{return byte_extent_;}
    std::uint32_t cursor_word()const noexcept{return cursor_word_;}
    std::uint32_t hardware_format()const noexcept{return hardware_format_;}
private:
    std::uint32_t input_bin_{},byte_offset_{},byte_extent_{},cursor_word_{},hardware_format_{};
    SourceSoundWetVoiceLayoutTu15()=default;
    friend SourceSoundWetVoiceLayoutTu15 source_sound_wet_voice_layout_tu15(
        std::uint32_t,const SoundSdkVoiceAttachmentBindings&,const SoundSdkDeviceMemory&);
};
// Read actual initialized format/source owners and the published attachment
// cursor. The caller must exclude concurrent source changes. This identifies
// the eight literal startup wet voices, not arbitrary buffered/streamed audio.
// No physical sample alignment, playing state, consumed cursor or clock follows.
SourceSoundWetVoiceLayoutTu15 source_sound_wet_voice_layout_tu15(
    std::uint32_t voice,const SoundSdkVoiceAttachmentBindings&,const SoundSdkDeviceMemory&);
}
