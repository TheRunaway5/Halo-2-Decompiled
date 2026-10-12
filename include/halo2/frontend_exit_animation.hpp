#pragma once
#include "halo2/frontend.hpp"
#include <algorithm>
#include <cstdint>

namespace halo2 {
// Original22E65E type3 selects trackB, forward, with no intro delay.
//22E89C resets its frame/end time and scale.22E3CD then advances this state
//once per UI update, including its integer frame time and1ms minimum blend.
//Kept separate from the existing intro/ambient sampler so this bounded exit
//correction does not change unrelated screen presentation.
struct FrontendExitAnimation {
  unsigned frame{};
  std::uint32_t end_ms{};
  UiKey key;
  bool queued{};

  UiKey update(const UiTrack &track, std::uint32_t elapsed_ms, bool screen) {
    if (queued) return key;
    if (track.keys.empty() || !track.milliseconds) {
      queued = screen;
      return key;
    }
    if (track.keys.size() < 2 ||
        track.milliseconds / (track.keys.size() - 1) == 0)
      throw FormatError("unsupported frontend exit animation frame time");
    const auto frame_time = std::uint32_t(track.milliseconds /
                                         (track.keys.size() - 1));
    auto elapsed = std::int64_t(elapsed_ms) - end_ms;
    auto next_time = std::uint64_t(end_ms) + frame_time;
    while (elapsed_ms >= next_time) {
      if (frame + 1 == track.keys.size()) {
        if (screen) {
          //148119 queues removal without performing another interpolation.
          queued = true;
          return key;
        }
      } else {
        ++frame;
      }
      next_time += frame_time;
      elapsed -= frame_time;
      end_ms = elapsed_ms;
    }
    const auto next = std::min(frame + 1, unsigned(track.keys.size() - 1));
    const float t = float(std::max<std::int64_t>(elapsed, 1)) / float(frame_time);
    const auto &a = track.keys[frame], &b = track.keys[next];
    key.alpha = (b.alpha - a.alpha) * t + a.alpha;
    key.position.x = (b.position.x - a.position.x) * t + a.position.x;
    key.position.y = (b.position.y - a.position.y) * t + a.position.y;
    key.position.z = (b.position.z - a.position.z) * t + a.position.z;
    return key;
  }
};
} // namespace halo2
