#pragma once
#include <algorithm>
#include <cstdint>
#include <utility>
namespace halo2 {
// SDL clock/device adapter. Source action admission uses the authored matg
// threshold later; this tracks elapsed input time without a guessed tick count.
struct NativeInteractionHold {
  bool down{}, pending{};
  std::uint32_t began{};
  std::uint16_t released_milliseconds{};
  void set(bool value,std::uint32_t now) {
    if(value && !down) {began=now;pending=true;released_milliseconds=0;}
    if(!value && down) {
      released_milliseconds=std::uint16_t(std::min(now-began,65535u));
      pending=true;
    }
    down=value;
  }
  std::pair<bool,std::uint16_t> sample(std::uint32_t now) const {
    return {down || pending,down?std::uint16_t(std::min(now-began,65535u)):released_milliseconds};
  }
  void consume() {pending=false;released_milliseconds=0;}
  // The device clock keeps advancing while native input ownership is paused.
  // Rebase only an active hold; a completed duration is already immutable.
  void exclude_elapsed(std::uint32_t milliseconds) noexcept {
    if(down)began+=milliseconds;
  }
};
}
