#include "halo2/campaign_text.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
void CampaignSubtitle::start(std::uint32_t id, float seconds) {
  if (!std::isfinite(seconds) || seconds < 0)
    throw FormatError("Invalid cinematic subtitle duration");
  text = id;
  remaining = seconds + 1.5f;
}
void CampaignSubtitle::advance(float seconds) {
  if (!std::isfinite(seconds) || seconds < 0)
    throw FormatError("Invalid cinematic subtitle clock");
  if (text) {
    remaining -= seconds;
    if (remaining <= 0) {
      text = 0;
      remaining = 0;
    }
  }
}
float CampaignSubtitle::alpha() const {
  return text
             ? remaining > 1.5f
                   ? 1.f
                   : float(std::pow(std::max(0.f, remaining) * .6666667f, 1.9f))
             : 0.f;
}
bool campaign_subtitles_visible(unsigned preference, unsigned game_language,
                                unsigned map_language) {
  if (preference > 2)
    throw FormatError("Invalid subtitle preference");
  return preference == 1 || (preference == 0 && game_language != map_language);
}
void AutomaticTrainingState::advance(std::span<const TrainingEventDefinition> definitions,
    const std::array<bool,32> &admitted,const std::array<bool,32> &completed) {
  if(definitions.size()>32)throw FormatError("Too many automatic training events");
  const auto definition=[&](unsigned i)->const TrainingEventDefinition * {
    return i<definitions.size()?&definitions[i]:nullptr;
  };
  const auto finish_one=[&](unsigned i) {
    auto &t=transitions[i];const auto *d=definition(i);
    if(!d){t={4,150};return;}
    if(d->limit && d->limit<=3) {
      if(learned[i]+1>=d->limit){t={5,0};learned[i]=3;return;}
      ++learned[i];
    }
    t={4,std::int16_t(30*d->cooldown_seconds)};
  };
  const auto finish=[&](unsigned i) {
    if(i==21)finish_one(22);
    finish_one(i);
  };
  int selected=current;
  for(unsigned i=0;i<32;++i) {
    auto &t=transitions[i];
    if(t.state==0) {
      if((selected==-1 || selected>int(i) || i==31) && admitted[i])selected=int(i);
    } else if(t.state==4 && --t.ticks<=0)t={0,0};
  }
  if(selected!=-1) {
    if(selected!=current) {
      if(current!=-1)transitions[current]={0,0};
      current=selected;elapsed=0;baseline=false;
    }
    auto &t=transitions[selected];const auto *d=definition(selected);
    switch(t.state) {
    case 0:t={1,std::int16_t(d?std::nearbyint(d->delay_seconds*30):0)};break;
    case 1:
      if(completed[selected]){finish(selected);current=-1;}
      else if(admitted[selected]){if(--t.ticks<=0)t={2,0};}
      else {t={0,0};current=-1;}
      break;
    case 2:
      ++elapsed;
      if(completed[selected]){finish(selected);current=-1;elapsed=0;}
      else if(!admitted[selected])t={3,std::int16_t(d?30*d->grace_seconds:0)};
      break;
    case 3:
      ++elapsed;
      if(completed[selected]){finish(selected);current=-1;elapsed=0;}
      else if(admitted[selected])t={2,0};
      else if(--t.ticks<=0){t={0,0};current=-1;elapsed=0;}
      break;
    case 4:if(--t.ticks<=0)t={0,0};break;
    }
  }
  for(unsigned i:{15u,17u,18u,23u,30u})
    if(current!=int(i) && learned[i]!=3 && completed[i])finish(i);
}
std::uint32_t AutomaticTrainingState::text(std::span<const TrainingEventDefinition> definitions) const {
  if(current<0 || std::size_t(current)>=definitions.size())return 0;
  const auto &t=transitions[current];const auto &d=definitions[current];
  if((t.state!=2 && t.state!=3) || (d.display_seconds && 30*d.display_seconds<=elapsed))return 0;
  return d.text;
}
} // namespace halo2
