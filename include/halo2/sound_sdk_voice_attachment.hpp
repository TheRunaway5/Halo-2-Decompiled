#pragma once
#include "halo2/sound_sdk_sample_pages.hpp"
#include "halo2/sound_sdk_voice_initialization.hpp"

namespace halo2 {
struct SoundSdkVoiceAttachmentBindings {
  std::uint32_t stream_samples_pointer_address;
};
struct SoundSdkVoiceAttachmentServices {
  SoundSdkSamplePageServices pages;
  SoundSdkVoiceInitializationServices initialization;
  // Reached382DD2/381E0E stop/drain remain actual required owners. Cleanup
  // ignores stop's HRESULT and still drains, as the original caller does.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> stop;
  std::function<void(std::uint32_t)> wait_pending;
};
// Complete37F43E sample-to-byte helper, including ADPCM's unsigned shift.
std::uint32_t source_sound_sdk_voice_sample_bytes(std::uint32_t voice,std::uint32_t samples,
    const SoundSdkDeviceMemory &);
// Complete3824EE retained cursor packing under the actual device guard.
void source_sound_sdk_voice_cursor(std::uint32_t voice,std::uint32_t cursor,
    const SoundSdkDeviceServices &,const SoundDriverStartupObserver & = {});
// Complete382560/3824EA cleanup, composing native sample-page release.
void source_sound_sdk_detach_voice_samples(std::uint32_t voice,
    const SoundSdkVoiceAttachmentServices &,const SoundDriverStartupObserver & = {});
// Complete382B7E buffered attachment, composing native sample-page mapping.
std::uint32_t source_sound_sdk_attach_voice_buffer(std::uint32_t voice,
    const SoundSdkVoiceAttachmentServices &,const SoundDriverStartupObserver & = {});
// Complete382BE3 streamed attachment. Composes the complete native37C8AB
// buffer owner; its HRESULT and this caller's volatile EAX are ignored by
// their original callers. No platform/DSP/sample output is manufactured.
void source_sound_sdk_attach_voice_stream(std::uint32_t voice,
    const SoundSdkVoiceAttachmentBindings &,const SoundSdkVoiceInitializationData &,
    const SoundSdkVoiceAttachmentServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
