#include "halo2/damage_feedback.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
void accumulate(RecentDamageState &s,float amount,bool depleted) {
  if(depleted && !(amount>1.f))amount=1.f;
  if(!(amount>.0001f))return;
  const float accumulated=amount+s.accumulated;
  s.accumulated=accumulated>1.f?1.f:accumulated;
  const float visible=s.visible+amount;
  s.visible=visible>1.f?1.f:visible;
  s.ticks=0;
}
std::int32_t rounded_ticks(float value) {
  // FISTP's integer-indefinite result for an invalid conversion.
  const auto rounded=std::nearbyint(value);
  if(!(rounded>=-2147483648.f && rounded<2147483648.f))
    return std::numeric_limits<std::int32_t>::min();
  return static_cast<std::int32_t>(rounded);
}
void decay(float &value,int ticks,std::int16_t rate,float delay,float seconds) {
  if(!(value>0.f))return;
  const float delay_ticks=float(rate)*delay;
  if(ticks<rounded_ticks(delay_ticks))return;
  if(seconds>.0001f) {
    const float span=float(rate)*seconds;
    const float step=1.f/span;
    value=value-step;
    if(0.f>value)value=0.f;
  } else value=0.f;
}
}
void report_damage_feedback(DamageFeedbackState &s,float body,float shield,
                            std::uint32_t flags,bool special) {
  accumulate(s.body,body,(flags&1)!=0);
  if(!special)accumulate(s.shield,shield,(flags&2)!=0);
}
bool fade_recent_damage(RecentDamageState &s,std::int16_t rate,bool reset,
                        const DamageFadeDefinition &definition) {
  if(s.ticks==-1)return false;
  s.ticks=static_cast<std::int8_t>(std::min(int(s.ticks)+1,127));
  decay(s.visible,s.ticks,rate,definition.visible_delay,definition.visible_decay);
  decay(s.accumulated,s.ticks,rate,definition.accumulated_delay,definition.accumulated_decay);
  if(reset)s.visible=0.f;
  if((reset || s.visible==0.f) && s.accumulated==0.f)s.ticks=-1;
  return true;
}
bool update_damage_feedback(DamageFeedbackState &s,std::int16_t rate,bool body_depleted,
    bool shield_depleted,bool special,const DamageFadeDefinition &definition) {
  const bool body=fade_recent_damage(s.body,rate,body_depleted,definition);
  if(special) {
    if(s.shield.ticks>0) {
      s.shield.visible=s.shield.accumulated+s.shield.visible;
      --s.shield.ticks;
    }
    return true;
  }
  const bool shield=fade_recent_damage(s.shield,rate,shield_depleted,definition);
  return body|shield;
}
std::optional<DamageFeedbackInput> damage_feedback_input(const DamageFeedbackState &s,
    float body,float shield,bool dead,std::uint32_t name) {
  float value;
  switch(name) {
  case 0x0500055b:value=dead?0.f:1.f;break;
  case 0x0d000558:value=body;break;
  case 0x0f000559:value=shield;break;
  case 0x13000556:value=s.body.visible;break;
  case 0x15000557:value=s.shield.visible;break;
  case 0x1800055a:value=shield-1.f;break;
  default:return {};
  }
  if(0.f>value)value=0.f;
  else if(value>1.f)value=1.f;
  return DamageFeedbackInput{value,value>0.f};
}
float damage_feedback_shader_input(const DamageFeedbackState &s,float body,
    float shield,bool dead,std::span<const ObjectFunction> functions,
    const ShaderFunctionTables &tables,std::uint32_t name,float seconds) {
  const auto value=evaluate_object_function(functions,tables,name,seconds,{},
      [&](std::uint32_t input)->std::optional<FunctionValue> {
        if(const auto resolved=damage_feedback_input(s,body,shield,dead,input))
          return FunctionValue{resolved->value,resolved->active};
        return {};
      });
  return value.enabled?value.value:0.f;
}
}
