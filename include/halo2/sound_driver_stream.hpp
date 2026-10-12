#pragma once
#include "halo2/sound_driver_startup.hpp"

namespace halo2 {
struct SoundDriverStreamConstants {
  std::uint32_t one_bits,stereo_bits,pcm_queue,adpcm_queue;
  std::uint32_t pcm_initialize_method,adpcm_initialize_method,completion_callback;
};
struct SoundDriverStreamScratch {
  // Explicit raw caller backing. PCM writes only18 of20 format bytes;
  // the final two bytes must not be silently initialized by this adapter.
  std::uint32_t format,descriptor,queue_slot;
};
struct SoundDriverStreamCreateRequest {
  std::uint32_t device_identity,descriptor_address,output_address,outer_identity;
  std::array<std::uint32_t,6> descriptor;
  std::array<std::byte,20> format;
};
struct SoundDriverStreamServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Required actual platform creation, including real output-slot ownership.
  // No absent device/voice/result is supplied by the CPU owner.
  std::function<std::int32_t(const SoundDriverStreamCreateRequest &)> native_create_stream;
  // Original calls Pause(stream,0) even after failed creation.
  std::function<void(std::uint32_t stream,std::uint32_t pause)> native_pause_stream;
};
// Complete220520 CPU format writes. The original CRT conversion is represented
// only for its authenticated immutable1.0/2.0 operands; another operand stops.
void source_sound_driver_stream_format(std::uint32_t category,
    std::uint32_t format,std::uint32_t queue_slot,const SoundDriverStreamConstants &,
    const SoundDriverStreamServices &,const SoundDriverStartupObserver & = {});
// Complete2215F0 and2B4970 CPU owners, including real PCM/ADPCM queue methods
// authenticated through their original vtables. Scratch, queue/driver storage
// and actual native services are mandatory; this does not admit the driver.
bool source_sound_driver_stream_construct(std::uint32_t driver,std::uint32_t index,
    std::uint32_t category,const SoundDriverStreamScratch &,
    const SoundDriverStreamConstants &,const SoundDriverStreamServices &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
