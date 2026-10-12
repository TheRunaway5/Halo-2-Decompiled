#pragma once
#include "halo2/sound_startup.hpp"

namespace halo2 {
// Addresses of retained mutable globals. Startup, active map/cache metadata,
// voice/controller allocation and driver backing need their actual producers.
struct SoundSourceLifecycleBindings {
  std::uint32_t system,sources,static_voices,controllers,effects;
  std::uint32_t tag_table,ugh,cache_pool,shared_voices,driver;
};
struct SoundSourceCallbackRequest {
  std::uint32_t owner,method,context,source,reason;
};
struct SoundSourceLifecycleServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Actual source callback. It may change globals/pools; the CPU owner reloads
  // them in original order. Missing service is not a successful callback.
  std::function<void(const SoundSourceCallbackRequest &)> source_callback;
  // Actual retained native platform owners for2B47C0. No status, buffer or
  // stream is created by this CPU adapter. HRESULTs unused by the source are
  // not manufactured as successful guest returns.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> buffer_status;
  std::function<void(std::uint32_t)> buffer_stop;
  std::function<void(std::uint32_t,std::uint32_t)> buffer_stop_ex;
  std::function<void(std::uint32_t,std::uint32_t)> stream_flush;
};
// Complete127B50/128A70 CPU owners and their effect/controller/cache/source/
// static/shared voice cleanup,16D4C0 datum deletion and2B47C0 stream-stop caller.
// Observer instruction0 identifies portable writes; ordered addresses, widths
// and complete raw output remain comparable with original instructions.
// This does not initialize sound, activate a map, reset the world or admit a
// variant. Required reached callback/platform dependencies throw when absent.
void source_sound_round_reset(const SoundSourceLifecycleBindings &,
    const SoundSourceLifecycleServices &,const SoundStartupObserver & = {});
void source_sound_stop(const SoundSourceLifecycleBindings &,std::uint32_t source,
    std::uint32_t reason,const SoundSourceLifecycleServices &,
    const SoundStartupObserver & = {});
} // namespace halo2
