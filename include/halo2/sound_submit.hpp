#pragma once
#include "halo2/sound_startup.hpp"

namespace halo2 {
// Live mutable owners: none is inferred from an unopened source image or host
// device. The literal tag table and active map roots must have real producers.
struct SoundSubmitState {
  std::uint32_t system,tag_table,current_matg,current_ugh,current_scenario,effect_pool,controller_pool,controller_root;
};
struct SoundSubmitConstants {
  // Complete source21FD70 callback identity imported from the owned image.
  // It is not a native voice or a default source callback implementation.
  std::uint32_t effect_callbacks;
};
struct SoundSubmitServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Complete147A20 clock/CRT producer called only on genuine first controller
  // reference. It must execute its original ordered time/rand/tick consumers;
  // arbitrary seeds and default results are not valid platform services.
  std::function<std::uint32_t(std::uint8_t source_flag)> controller_seed;
  // Complete128380 admission writes actual local selection into location;
  // caller sets its source-owned localDWORD to0 before this request.
  std::function<bool(std::uint32_t sound,std::span<std::byte,160> request,
                     std::uint32_t &location)> admit;
  // Complete128610 may modify sound for its caller; its integer result owns
  // the original rejection. It is not a bool device-open result.
  std::function<std::int32_t(std::uint32_t original_sound,std::uint32_t &sound)> promote;
  // Complete127750 source creation owns permutation/RNG/cache/voice requests.
  std::function<std::uint32_t(std::uint32_t sound,std::uint32_t location,
                  std::span<std::byte,160> request,std::uint32_t promoted_sound)> create_source;
  // Complete21F740 optional effect/controller update and originalcallbacks.
  std::function<void(std::uint32_t effect,unsigned phase)> update_effect;
};
// Complete18EAC0/18EBA0/21B5B0 CPU metadata lookup, preserving signed tag
// group indexing and the source interface0/1/2/3 branches.
std::uint32_t source_sound_custom_playback(const SoundSubmitState &,
    std::uint32_t sound,std::uint32_t interface,const SoundSubmitServices &);
std::uint32_t source_sound_default_playback(const SoundSubmitState &,
    std::uint32_t sound,const SoundSubmitServices &);
// Complete21FE40. Caller owns the preexisting outputDWORD; source only writes
// it on its original pointer/count branch. Its null-ESI fallback is preserved.
bool source_sound_controller_request(const SoundSubmitState &,std::uint32_t sound,
    std::uint32_t interface,std::uint32_t &output,const SoundSubmitServices &,
    const SoundStartupObserver & = {});
// Complete21F220/21FD70/21FE00 and raw16D2C0/16D4C0/16D680,
//21BA70/21BB40/21BBA0/21BD90/21BFA0 CPU control owners. Raw160 bytes are mandatory
// caller backing. The semanticFP request omits original unowned bytes and must
// never be zero-extended into this ABI. Missing dependencies throw at phase;
// they are not original false/NONE results or scheduler/device success.
std::uint32_t source_sound_submit(const SoundSubmitState &,std::uint32_t sound,
    std::uint32_t raw_request_address,const SoundSubmitConstants &,
    const SoundSubmitServices &,const SoundStartupObserver & = {});
} // namespace halo2
