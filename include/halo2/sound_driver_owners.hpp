#pragma once
#include "halo2/sound_driver_startup.hpp"

namespace halo2 {
struct SoundDriverOwnersState {
  std::uint32_t driver_address,cache_address,static_queue_address;
};
enum class SoundDriverVirtualThis { Stack, Ecx };
struct SoundDriverVirtualRequest {
  std::uint32_t identity,vtable_address,slot,method_address;
  SoundDriverVirtualThis this_abi;
  // First status request has an explicit DWORD output. Its value is not
  // invented or consumed by this CPU owner; the real method owns it.
  std::span<std::byte> output;
};
enum class SoundDriverStaticBufferOperation {
  MixBins,MaxDistance,MinDistance,DopplerFactor,RolloffFactor,ConeOutsideVolume,
};
struct SoundDriverStaticBufferRequest {
  SoundDriverStaticBufferOperation operation;
  std::uint32_t identity,first_word,second_word;
  std::span<const std::uint32_t> mix_bin_pairs;
};
struct SoundDriverOwnersServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Each genuinely retained method is required. Missing/null/unsupported
  // backing stops; no method, voice, result or status is synthesized.
  std::function<void(const SoundDriverVirtualRequest &)> native_virtual_call;
  std::function<std::int32_t(const SoundDriverBufferCreateRequest &)> native_buffer_create;
  std::function<void(const SoundDriverStaticBufferRequest &)> native_buffer_operation;
};
using SoundDriverOwnersStore=SoundDriverStartupStore;
using SoundDriverOwnersObserver=SoundDriverStartupObserver;
// Complete original CPU owners2213A0/2B4470, with their complete packet
// helpers2B4BF0/2B48A0. The static buffer constructor is complete222B80.
// Real virtual/buffer services and actual cache/packet identities are required.
// Differential coverage includes naturally empty source-created owners and
// explicit raw-state controls which stop at unallocated voices/packets.
void source_sound_driver_stream_flush(const SoundDriverOwnersState &,
    const SoundDriverOwnersServices &,const SoundDriverOwnersObserver & = {});
void source_sound_driver_queue_shutdown(const SoundDriverOwnersState &,
    const SoundDriverOwnersServices &,const SoundDriverOwnersObserver & = {});
void source_sound_driver_release_packets(const SoundDriverOwnersState &,
    std::uint32_t record,const SoundDriverOwnersServices &,
    const SoundDriverOwnersObserver & = {});
void source_sound_driver_complete_packet(const SoundDriverOwnersState &,
    std::uint32_t record,std::uint32_t packet,const SoundDriverOwnersServices &,
    const SoundDriverOwnersObserver & = {});
bool source_sound_driver_static_buffer_construct(const SoundDriverOwnersState &,
    std::uint32_t index,const SoundDriverOwnersServices &,
    const SoundDriverOwnersObserver & = {});
} // namespace halo2
