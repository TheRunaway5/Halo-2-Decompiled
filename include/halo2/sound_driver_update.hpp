#pragma once
#include "halo2/sound_driver_owners.hpp"
namespace halo2 {
// Actual selected-image operand addresses. Mutable source values are read
// from retained ownership; none is a source-image default or readiness flag.
struct SoundDriverUpdateBindings {
  std::uint32_t driver_pointer,effects_generation,effects_context_pointer;
  std::uint32_t optional_sound_flag,quiesce_flag,quiesce_clear_word;
  std::uint32_t section_name,quiesce_record;
};
struct SoundDriverEffectsRequest {
  std::uint32_t section_name_address;
  std::array<std::uint32_t,2> effect_locations;
  std::uint32_t flags,output_address;
};
struct SoundDriverUpdateServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Genuine section lifetime and DSP/device work. A missing owner stops;
  // loaded bytes alone are not a successful XLoadSection operation.
  std::function<std::uint32_t(std::uint32_t)> load_section;
  std::function<void(std::uint32_t)> free_section;
  std::function<void(const SoundDriverEffectsRequest &)> download_effects;
  std::function<void()> do_work;
  // Complete required source CPU dependencies, not empty success callbacks.
  std::function<void(std::uint32_t)> source_quiesce; //550A0, EAX record
  std::function<void()> source_restore; //53500
  std::function<void(std::uint32_t)> source_release_packets; //2B4BF0, ECX record
  // Actual status/release methods. GetStatus receives the original explicit
  // caller-owned four-byte backing; no zero/status/default is supplied here.
  std::function<void(const SoundDriverVirtualRequest &)> native_virtual_call;
};
// Complete193020 CPU caller. Section/DSP and required source callees remain
// mandatory providers with their own real ownership and error/lifetime state.
void source_sound_driver_reload_effects(const SoundDriverUpdateBindings &,
    const SoundDriverUpdateServices &,const SoundDriverOwnersObserver & = {});
// Complete220D10 CPU caller. status_backing is retained caller backing, not
// a synthesized original stack value. All real methods must own their outputs.
void source_sound_driver_update(const SoundDriverUpdateBindings &,
    std::span<std::byte,4> status_backing,const SoundDriverUpdateServices &,
    const SoundDriverOwnersObserver & = {});
} // namespace halo2
