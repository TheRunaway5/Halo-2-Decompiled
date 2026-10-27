#pragma once
#include "halo2/netcode_mode.hpp"
#include <cstdint>
#include <deque>

namespace halo2 {
// Non-host adaptive interpolation buffer (docs/NETCODE_ENHANCED_GOAL.md item
// 3a). Presentation only. Original keeps the fixed one-tick delay; Balanced and
// Full play the replicated world back at a host tick that trails the earliest
// snapshot arrival path by a delay sized from measured arrival jitter.
//
// * Arrival line: the sliding-window minimum of (arrival time - tick time).
//   On a clean link the newest tick arrives exactly on it.
// * Jitter: the RFC 3550 EWMA of inter-arrival deviation, and the largest
//   lateness behind the arrival line over the same window (bursts). Lateness
//   is measured from the tick after the previous newest one, so ticks lost or
//   dropped as stale count as waiting time.
// * Target delay: one tick, plus processing, plus the larger of
//   jitter_scale x jitter and the held lateness, less the frame interval that
//   polling already absorbs (a snapshot polled at a frame was not needed by the
//   frame before it), within [one tick + processing, maximum]. An underrun
//   hold also proves its own shortfall (a gap whose older snapshots were lost
//   or dropped as stale has no late arrival to measure); the delay that would
//   have avoided it is held over the same window.
// * Playback: slewed toward the target at a rate within 1 +/- slew, so delay
//   changes never move the presented time discontinuously. With no newer
//   snapshot it holds at the newest tick (an underrun) instead of
//   extrapolating; the delay that hold adds is slewed out afterwards. Errors
//   larger than snap_seconds (the first snapshot, long stalls) re-place it.
struct AdaptiveInterpolationSettings {
  double tick_seconds=1.0/30.0;
  double processing_seconds=.004; // Margin above one tick on a clean link.
  double maximum_seconds=.25;     // Largest delay (the Full rewind cap).
  double jitter_gain=1.0/16;      // RFC 3550 inter-arrival jitter EWMA gain.
  double jitter_scale=3;          // Delay per unit of jitter (uniform jitter: its full width).
  double window_seconds=3;        // Arrival-line minimum and lateness peak hold.
  double frame_gain=1.0/16;       // Frame-interval EWMA gain.
  double slew=.08;                // Largest playback-rate change.
  double slew_seconds=.5;         // Time constant removing a small delay error.
  double snap_seconds=.5;         // Larger delay errors re-place the playback clock.
};
// Original presents the 2004 fixed delay; only Balanced and Full adapt.
inline constexpr bool adaptive_interpolation_enabled(NetcodeMode mode) {
  return mode!=NetcodeMode::original;
}

class AdaptiveInterpolationClock {
public:
  struct Statistics {
    double delay{};       // Presented time behind the arrival line, seconds.
    double target{};      // Target delay, seconds.
    double jitter{};      // RFC 3550 inter-arrival jitter, seconds.
    double peak{};        // Largest lateness behind the arrival line in the window.
    double deficit{};     // Largest delay above the minimum a recent hold needed.
    double frame{};       // Frame-interval estimate, seconds.
    double underrun_seconds{};
    std::uint64_t arrivals{},underruns{},snaps{};
    bool holding{};       // The newest frame held at the newest tick.
  };
  explicit AdaptiveInterpolationClock(AdaptiveInterpolationSettings={});
  // A newly accepted snapshot of host `tick` polled at local `seconds` (any
  // monotonic clock). Same-tick refreshes and older ticks are ignored.
  void receive(std::uint32_t tick,double seconds);
  // Advance playback to `seconds`, once per presented frame, and return the
  // presented host tick (fractional, never beyond the newest tick).
  double advance(double seconds);
  double playback()const noexcept{return playback_;}
  bool active()const noexcept{return have_arrival_;}
  std::uint32_t newest()const noexcept{return newest_tick_;}
  double target_delay()const;
  // Presented time behind the arrival line at `seconds`.
  double delay(double seconds)const;
  Statistics statistics(double seconds)const;
  const AdaptiveInterpolationSettings &settings()const noexcept{return settings_;}
  void reset();
private:
  struct Offset {double seconds{},value{};};
  double base()const{return minimum_.front().value;}
  AdaptiveInterpolationSettings settings_;
  std::deque<Offset> minimum_,maximum_,deficit_;
  bool have_arrival_{},placed_{},holding_{},advanced_{};
  std::uint32_t newest_tick_{};
  double newest_seconds_{},jitter_{},frame_{},playback_{},advanced_at_{},underrun_seconds_{};
  double hold_started_{},hold_delay_{};
  std::uint64_t arrivals_{},underruns_{},snaps_{};
};
} // namespace halo2
