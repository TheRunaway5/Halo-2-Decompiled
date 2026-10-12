#pragma once
#include "halo2/frontend_vista.hpp"
#include <chrono>
#include <bit>
#include <cstdint>
#include <optional>
#include <span>
namespace halo2 {
// Retail 147F4F reads channel five. Choose Profile (10) is on channel three,
// and the virtual keyboard (167) is on channel two. Screens are supplied in
// native stack order, from bottom to top; only these known overlays may be
// skipped while resolving the retained title (9) or main menu (6).
inline std::optional<unsigned> resolved_frontend_channel5_screen(
    std::span<const unsigned> screens) {
  for (auto i = screens.rbegin(); i != screens.rend(); ++i) {
    const auto id=is_vista_menu(*i)?(*i&0xffffu):*i;
    if (id == 9 || id == 6)
      return id;
    if (id != 10 && id != 167)
      return {};
  }
  return {};
}
// Retail 235B9C owns both the logical playing state and last transition time.
// Initial starts and restarts after a movie use the same strict >6000 ms rule.
class FrontendMusicTransitions {
public:
  explicit FrontendMusicTransitions(bool playing = false,
                                    std::uint32_t last_change = 0)
      : playing_(playing), last_change_(last_change) {}
  std::optional<bool> request(bool wanted, std::uint32_t now) {
    const auto elapsed = std::bit_cast<std::int32_t>(now - last_change_);
    if (playing_ == wanted || (last_change_ && elapsed <= 6000))
      return {};
    playing_ = wanted;
    last_change_ = now;
    return wanted;
  }
  bool playing() const { return playing_; }
  std::uint32_t last_change() const { return last_change_; }

private:
  bool playing_;
  std::uint32_t last_change_;
};
// Retail 223976 uses the later of the last input and movie/title reset.
// The host monotonic clock starts counting once the title can actually draw.
class FrontendIdleTimer {
public:
  using Clock = std::chrono::steady_clock;
  explicit FrontendIdleTimer(Clock::time_point now) : reset_(now) {}
  void activity(Clock::time_point now) { reset_ = now; }
  // Host decoding and the first GPU submission are outside a visible menu's
  // interval. Movie exit/title entry arm it on the next presented menu frame.
  void await_presentation() { pending_presentation_ = true; }
  bool presented(Clock::time_point now) {
    if (pending_presentation_) {
      activity(now);
      pending_presentation_ = false;
      return true;
    }
    return false;
  }
  void screen(unsigned id, Clock::time_point now) {
    if (id == 9 && screen_ != 9) {
      activity(now); // Retail 22F1F7.
      await_presentation();
    }
    screen_ = id;
  }
  std::chrono::milliseconds elapsed(Clock::time_point now) const {
    if (pending_presentation_) return std::chrono::milliseconds::zero();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now-reset_);
  }
  bool eligible() const { return screen_ == 9 || screen_ == 6; }
  bool due(Clock::time_point now) const {
    return eligible() && elapsed(now) >= std::chrono::milliseconds(75000);
  }
private:
  Clock::time_point reset_;
  unsigned screen_ = 9;
  bool pending_presentation_ = true;
};
// Retail 235BFA: profile/settings/dialog screens retain the prior preference.
inline bool frontend_music_enabled(unsigned screen, bool previous) {
  // The namespace bit belongs to the native content catalog. Original
  // 235BFA/PC638C60 compare the underlying screen ID, never that bit.
  const bool vista=is_vista_menu(screen);
  if(vista)screen&=0xffffu;
  if (screen == 9 || screen == 6) return true;
  if (screen == 13 || screen == 14 || screen == 186) return false;
  if (vista && screen == 247) return false; // Actual PC638C60 case247.
  return previous;
}
inline bool frontend_music_wanted(bool enabled, const FrontendIdleTimer &idle,
    FrontendIdleTimer::Clock::time_point now, unsigned silence_ms) {
  return enabled && !(idle.eligible() &&
      idle.elapsed(now).count() >= std::uint32_t(75000u - silence_ms));
}
}
