#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

namespace halo2 {
inline constexpr std::string_view vista_announcer_queue_source_sha256 =
    "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109";
inline constexpr std::uint32_t vista_announcer_none = 0xffffffffu;

// Literal 12-byte slots at PC 8D8578. The unused byte and inactive fields are
// retained because enqueue writes only sound/delay and shift copies all bytes.
struct VistaAnnouncerQueueSlot {
  std::uint32_t sound{};
  std::uint32_t voice{};
  std::uint8_t active{};
  std::uint8_t retained_byte{};
  std::int16_t delay{};
  bool operator==(const VistaAnnouncerQueueSlot&) const = default;
};
struct VistaAnnouncerQueueState {
  std::array<VistaAnnouncerQueueSlot,5> slots{};
  std::uint32_t count{}; // PC 8D8614; admitted update counts are 0..5.
  bool operator==(const VistaAnnouncerQueueState&) const = default;
};

// Playback is original 4883B6's boundary: resolve/play the supplied sound with
// request scale 1 and return its actual voice handle, or NONE on failure.
// No PCM, mixer, event audience, language, duplicate hash or clock is invented.
using VistaAnnouncerPlayback =
    std::function<std::uint32_t(std::uint32_t sound,float request_scale)>;

// Queue-owned subset of 4D0E02. Original 47C0DB multiplies the caller's signed
// tick rate by literal 2 using float32 then FISTP; this integer product is exact.
// Original also resets a separate event-hash bank, outside this queue API.
void vista_announcer_queue_initialize(VistaAnnouncerQueueState&,std::int16_t tick_rate);
// 4D0E75: append only if unsigned count <=4. Delay retains its low signed16.
void vista_announcer_queue_enqueue(VistaAnnouncerQueueState&,std::uint32_t sound,
                                  std::int32_t delay_ticks);
// 4D0E62: completion clears active only when the returned voice matches front.
void vista_announcer_queue_completed(VistaAnnouncerQueueState&,std::uint32_t voice);
// 4D0FF4: one admitted original engine update, not a wall-time delta. Delay zero
// requests playback this update; a finished negative front shifts on a later
// update. Source stale slot fields and failed-playback behavior are preserved.
// Caller must supply a playback callback when a non-NONE cue can be requested.
// Counts above5 reject: they are outside the source queue's capacity invariant.
void vista_announcer_queue_update(VistaAnnouncerQueueState&,
                                 const VistaAnnouncerPlayback&);
} // namespace halo2
