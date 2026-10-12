#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
// Supplied PC44A82E initializes these five manager flags to zero. PC44B553
// admits an event only when online(+4) and earning-achievements(+1) are set.
bool vista_pcr_achievement_dispatch_admitted(const std::array<std::uint8_t,5>&) noexcept;
struct VistaPcrMedalAchievementRequest {
  std::int32_t type{},distinct{};
  bool dispatched{};
};
// Bounded owner for an actual controller0 locally signed-in, offline session.
// PC5AC4A3 tests the actual sign-in state for2 (Live); state1 is offline.
// This owns the original initialized flag projection and reached type12 events.
// It does not own the41 achievement children, Live submission or persistence.
class VistaPcrOfflineAchievementOwner {
public:
  VistaPcrMedalAchievementRequest request_medals(unsigned distinct,
      std::int32_t actual_signin_state);
  std::span<const VistaPcrMedalAchievementRequest> requests() const noexcept{return requests_;}
  const std::array<std::uint8_t,5>& flags() const noexcept{return flags_;}
private:
  std::array<std::uint8_t,5> flags_{};
  std::vector<VistaPcrMedalAchievementRequest> requests_;
};
} // namespace halo2
