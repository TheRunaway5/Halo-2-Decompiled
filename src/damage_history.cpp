#include "halo2/damage_history.hpp"
namespace halo2 {
void record_damage_history(DamageHistory &history,std::uint32_t tick,float amount,
    std::uint32_t object,std::uint32_t player,std::uint8_t cause) {
  bool matched=false;
  for(auto &entry:history)
    if((player!=0xffffffffu && entry.player==player) ||
       (object!=0xffffffffu && entry.object==object)) {
      entry.amount=amount+entry.amount;
      entry.tick=tick;entry.cause=cause;matched=true;
    }
  if(matched)return;
  unsigned selected=0;
  while(selected<history.size() && history[selected].tick!=0xffffffffu)++selected;
  if(selected==history.size()) {
    unsigned protected_entry=0;
    for(unsigned i=1;i<history.size();++i)
      if(history[i].amount>history[protected_entry].amount)protected_entry=i;
    selected=protected_entry==0?1:0;
    for(unsigned i=0;i<history.size();++i)
      if(i!=protected_entry && history[i].tick<history[selected].tick)selected=i;
  }
  history[selected]={tick,amount,object,player,cause};
}
void record_damage_result_history(DamageHistory &history,std::uint32_t tick,
    float body_damage,float shield_damage,std::uint32_t object,
    std::uint32_t player,std::uint8_t cause) {
  if(player!=0xffffffffu || object!=0xffffffffu)
    record_damage_history(history,tick,shield_damage+body_damage,object,player,cause);
}
}
