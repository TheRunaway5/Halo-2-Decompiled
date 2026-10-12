#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace halo2 {

// halo2.exe SHA256 33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// Explicit caller-owned context. The bounded instruction windows do not
// initialize it. A retained owned WineGE loading-thread hardware probe measures
// FPCW027F/MXCSR1F80 (later1FA0 sticky precision flag); physical Windows remains
// unmeasured. See native-loading-thread-original/wine-fp-hardware-20261011.
struct VistaLoadingFloatingPoint {
  std::uint16_t x87_control_word;
  std::uint32_t mxcsr;
};

// Raw binary32 words preserve the original retained values, including NaNs and
// signed zero. This is the D78E08/D78E0C/D78E10 projection, not loader ownership.
struct VistaLoadingProgressState {
  std::uint32_t baseline_bits;
  std::uint32_t progress_bits;
  std::uint32_t previous_clock;
  bool operator==(const VistaLoadingProgressState&) const = default;
};
struct VistaLoadingProgressQueryResults {
  std::uint32_t map_progress_bits;      // 4317D2 return, spilled to binary32
  std::int32_t duration;               // 60B183 return
  std::uint32_t clock;                 // 437E51 return
  std::uint32_t secondary_progress_bits; // 60B2B9/638D4C binary32 return
};
enum class VistaLoadingProgressService {
  path_conversion, map_progress, duration, clock, secondary_progress
};
struct VistaLoadingProgressUpdate {
  std::array<VistaLoadingProgressService,5> calls;
};

// Original 647F38..6480D9 arithmetic and ordered query intents. Supplied query
// results exclude external side effects, reentrancy, actual path conversion,
// loader readiness, and loading-screen dismissal.
VistaLoadingProgressUpdate vista_loading_progress_update(
    VistaLoadingProgressState&, const VistaLoadingProgressQueryResults&,
    const VistaLoadingFloatingPoint&);

// E4AFDC/E4AFE0/868530. Active is a byte, not a normalized native bool.
struct VistaLoadingFadeState {
  std::uint8_t active;
  std::uint32_t start_seconds_bits;
  std::uint32_t gain_bits;
  bool operator==(const VistaLoadingFadeState&) const = default;
};
enum class VistaLoadingFadeService { clock, gain };
struct VistaLoadingFadeBegin {
  std::array<VistaLoadingFadeService,1> calls;
};
struct VistaLoadingFadeUpdate {
  std::uint32_t elapsed_seconds_bits;
  bool complete;
  // Actual 782052 argument intent; its side effects remain caller-owned.
  std::optional<std::uint32_t> gain_service_bits;
  // Actual 47602F packing of {alpha=1, R=gain, G=gain, B=gain}. The original
  // loading_fade.fx copies this vertex color, with DESTCOLOR/ZERO blending.
  std::optional<std::uint32_t> packed_argb;
};

// Original 66A0B0: stamp wrapped (clock-origin) seconds, set active=1, retain
// the previous gain. Its 66370F caller waits for the loading renderer thread;
// this API does not establish a normal multiplayer/gameplay invocation.
VistaLoadingFadeBegin vista_loading_fade_begin(VistaLoadingFadeState&,
    std::uint32_t clock_origin, std::uint32_t clock,
    const VistaLoadingFloatingPoint&);

// Original elapsed conversion 66A585..66A5BB, gain/completion 66A9B7 and
// 66AB1B..66AB5D, plus 47602F color packing. Completion uses strict elapsed>
// start+1.5, independently of gain reaching zero. No rendering or scheduling.
VistaLoadingFadeUpdate vista_loading_fade_update(VistaLoadingFadeState&,
    std::uint32_t clock_origin, std::uint32_t clock,
    const VistaLoadingFloatingPoint&);

} // namespace halo2
