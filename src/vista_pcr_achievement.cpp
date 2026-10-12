#include "halo2/vista_pcr_achievement.hpp"
#include "halo2/cache.hpp"
#include <bit>

namespace halo2 {
bool vista_pcr_achievement_dispatch_admitted(const std::array<std::uint8_t,5>& flags) noexcept {
  return flags[4]!=0 && flags[1]!=0;
}
VistaPcrMedalAchievementRequest VistaPcrOfflineAchievementOwner::request_medals(
    unsigned distinct,std::int32_t actual_signin_state) {
  if(actual_signin_state!=1)throw FormatError("Vista offline achievement owner requires actual local sign-in state1");
  // Original44A7B5's offline notification clears+4,+0,+1. The native offline
  // service supplies the actual state; it cannot enable an online manager.
  flags_[4]=0;flags_[0]=0;flags_[1]=0;
  const VistaPcrMedalAchievementRequest request{12,
      std::bit_cast<std::int32_t>(std::uint32_t(distinct)),
      vista_pcr_achievement_dispatch_admitted(flags_)};
  requests_.push_back(request);
  return request;
}
} // namespace halo2
