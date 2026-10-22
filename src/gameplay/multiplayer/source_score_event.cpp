#include "halo2/source_score_event.hpp"
#include <stdexcept>

namespace halo2 {
void source_score_changed_event(std::uint32_t player,std::int32_t score,const SourceScoreEventServices& services) {
  if(!services.options)throw std::invalid_argument("Score publication requires its actual options owner");
  const auto* options=&services.options();
  constexpr std::array<std::uint32_t,3> thresholds{10,30,60},subtypes{47,9,7};
  for(unsigned i=0;i<thresholds.size();++i) {
    if(std::uint32_t(options->score_limit-std::uint32_t(score))!=thresholds[i])continue;
    if(!services.engine_present)throw std::invalid_argument("Score publication requires selected-engine ownership");
    const bool team=services.engine_present() && (options->player_flags&1u);
    if(!services.membership)throw std::invalid_argument("Score publication requires its actual player slot");
    const auto membership=services.membership(player);
    const VistaAnnouncerEvent event{{0,subtypes[i]+unsigned(team),0xffffffffu,player,
      std::uint32_t(std::int32_t(membership)),0xffffffffu,0xffffffffu,0},0xffff};
    if(options->raw_mode==4)continue;
    if(!services.network)throw std::invalid_argument("Score event network publication owner is unavailable");
    services.network(event);
    if(!services.local)throw std::invalid_argument("Score event local dispatch owner is unavailable");
    services.local(event);
    if(i<2)options=&services.options();
  }
}
} // namespace halo2
