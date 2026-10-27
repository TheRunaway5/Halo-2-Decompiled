#include "halo2/predicted_impacts.hpp"
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/cache.hpp"
#include "halo2/material_effects.hpp"
#include "halo2/math.hpp"
#include "halo2/native_grenade.hpp"
#include "halo2/projectile_material_response.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace halo2 {
namespace {
constexpr std::uint32_t none=null_datum;
std::uint32_t tag_word(const Cache &cache,std::uint32_t datum,std::size_t offset) {
  const auto *tag=cache.query_tag({.handle=TagHandle{datum}});
  if(!tag)throw FormatError("Predicted impact projectile tag is absent");
  auto selected=*tag;
  bool loaded=false;
  if(!selected.size)
    for(const auto &backing:cache.select_tags({.group=tag->type(),.address=tag->address,.minimum_size=1})) {
      (void)cache.tag_payload(backing.handle());selected.size=backing.size;loaded=true;break;
    }
  if(!loaded)(void)cache.tag_payload({datum});
  if(offset+4>selected.size)throw FormatError("Predicted impact projectile tag is truncated");
  const auto bytes=cache.read_tag_bytes(selected.address+std::uint32_t(offset),4);
  std::uint32_t value{};std::memcpy(&value,bytes.data(),4);return value;
}
// Closest distance between segments [a0,a1] and [b0,b1].
float segment_distance(Vec3 a0,Vec3 a1,Vec3 b0,Vec3 b1) {
  const auto d1=a1-a0,d2=b1-b0,r=a0-b0;
  const float a=dot(d1,d1),e=dot(d2,d2),f=dot(d2,r);
  float s=0,t=0;
  if(a<=1e-12f && e<=1e-12f)return length(r);
  if(a<=1e-12f)t=std::clamp(f/e,0.f,1.f);
  else {
    const float c=dot(d1,r);
    if(e<=1e-12f)s=std::clamp(-c/a,0.f,1.f);
    else {
      const float b=dot(d1,d2),denominator=a*e-b*b;
      s=denominator>1e-12f?std::clamp((b*f-c*e)/denominator,0.f,1.f):0.f;
      t=(b*s+f)/e;
      if(t<0){t=0;s=std::clamp(-c/a,0.f,1.f);}
      else if(t>1){t=1;s=std::clamp((b-c)/a,0.f,1.f);}
    }
  }
  return length((a0+d1*s)-(b0+d2*t));
}
}

std::optional<PredictedShotWeapon> predicted_shot_weapon(const WeaponDefinition &weapon) {
  // The lag-compensation precision classification (tag data, not names).
  if(!weapon.instantaneous || weapon.projectile==none || weapon.triggers.empty() ||
     weapon.triggers.front().behavior==0)return std::nullopt;
  // Magazine rounds are replicated; heat and overheat are not.
  if(weapon.energy || weapon.capacity<=0)return std::nullopt;
  PredictedShotWeapon out;
  out.weapon=weapon.datum;out.projectile=weapon.projectile;
  const int burst=weapon.barrel_timing?weapon.barrel_timing->definition.burst_minimum:std::int16_t(weapon.burst);
  out.rounds_per_press=unsigned(std::clamp(burst,1,8));
  const float rate=weapon.barrel_timing?std::max(weapon.barrel_timing->definition.rate_lower,
      weapon.barrel_timing->definition.rate_upper):weapon.rounds_per_second;
  out.round_seconds=rate>0?1.0/double(rate):1.0/30.0;
  out.cycle_seconds=double(out.rounds_per_press)*out.round_seconds+std::max(0.0,double(weapon.fire_recovery));
  out.ready_seconds=std::max(0.f,weapon.ready_seconds);
  out.rounds_per_shot=std::max(1,weapon.rounds_per_shot);
  out.range=weapon.range>0?weapon.range:100.f;
  return out;
}

PredictedShotGate::PredictedShotGate(Settings settings):settings_(settings) {}
void PredictedShotGate::reset() {
  pending_.clear();recent_rounds_.clear();fire_held_=false;
  last_press_=last_admitted_=hold_until_=weapon_since_=-1e9;weapon_object_=none;
}
unsigned PredictedShotGate::input(double seconds,const Controls &controls,const Player &player,
    const PredictedShotWeapon *weapon) {
  // Any other weapon action may delay or cancel the next shot on the host.
  if(controls.reload || controls.melee || controls.grenade || controls.switch_weapon) {
    hold_until_=std::max(hold_until_,seconds+settings_.action_hold_seconds);pending_.clear();
  }
  if(player.weapon_object!=weapon_object_) {
    weapon_object_=player.weapon_object;weapon_since_=seconds;pending_.clear();recent_rounds_.clear();
  }
  if(!player.armed)pending_.clear();
  const bool press=controls.fire && !fire_held_;
  fire_held_=controls.fire;
  if(!press)return 0;
  ++stats_.presses;
  const double previous_press=last_press_;
  last_press_=seconds;
  const auto refuse=[&]{++stats_.refused;return 0u;};
  if(!weapon || !player.armed || player.unit==none || player.weapon!=weapon->weapon)return refuse();
  if(seconds<hold_until_)return refuse();
  if(seconds-weapon_since_<weapon->ready_seconds+settings_.ready_margin_seconds)return refuse();
  // Both the previous press (fired or not) and the previous predicted press
  // must be a full cycle back, and the host's own last shot too.
  const double spacing=weapon->cycle_seconds+settings_.margin_seconds;
  if(seconds-previous_press<spacing || seconds-last_admitted_<spacing)return refuse();
  if(player.shot_tick && player.tick>=player.shot_tick &&
     double(player.tick-player.shot_tick)*settings_.tick_seconds<weapon->cycle_seconds)return refuse();
  while(!recent_rounds_.empty() && recent_rounds_.front()<seconds-settings_.ammunition_seconds)recent_rounds_.pop_front();
  const int available=player.loaded-int(recent_rounds_.size())*weapon->rounds_per_shot;
  const int rounds=std::min(int(weapon->rounds_per_press),available/weapon->rounds_per_shot);
  if(rounds<=0)return refuse();
  for(int index=0;index<rounds;++index) {
    const double due=seconds+double(index)*weapon->round_seconds;
    pending_.push_back({due,player.tick,player.player,player.unit,weapon->weapon,weapon->projectile,
        unsigned(index),weapon->range});
    recent_rounds_.push_back(due);
  }
  last_admitted_=seconds;++stats_.admitted;stats_.rounds+=unsigned(rounds);
  return unsigned(rounds);
}
PredictedShotGate::Player predicted_shot_player(const MultiplayerState::Player &p,std::uint32_t tick) {
  PredictedShotGate::Player out;
  out.armed=p.connected && !p.dead && p.source_unit!=none && p.source_weapon!=none &&
      p.source_parent==none && p.secondary_weapon.datum==none && p.source_zoom_magazine_mode==0 &&
      p.source_action_word==0 && !p.melee_action.active();
  out.player=p.source_player;out.unit=p.source_unit;out.weapon_object=p.source_weapon;out.weapon=p.weapon;
  out.loaded=p.loaded;out.tick=tick;out.shot_tick=p.shot_tick;
  return out;
}
std::vector<PredictedShotGate::Round> PredictedShotGate::take_due(double seconds) {
  std::vector<Round> out;
  while(!pending_.empty() && pending_.front().seconds<=seconds) {out.push_back(pending_.front());pending_.pop_front();}
  return out;
}

PredictedImpactLedger::PredictedImpactLedger(Settings settings):settings_(settings) {}
void PredictedImpactLedger::predict(const Prediction &prediction) {
  pending_.push_back(prediction);++stats_.predicted;
}
void PredictedImpactLedger::expire(double seconds) {
  while(!pending_.empty() && pending_.front().seconds<seconds-settings_.expiry_seconds) {
    pending_.pop_front();++stats_.expired;
  }
}
bool PredictedImpactLedger::suppress(const HostImpact &host,double seconds) {
  expire(seconds);
  if(!host.world)return false;
  for(auto at=pending_.begin();at!=pending_.end();++at) {
    const auto &p=*at;
    if(host.player!=none && host.player!=p.player)continue;
    if(host.unit!=none && host.unit!=p.unit)continue;
    if(host.projectile!=none && host.projectile!=p.projectile)continue;
    if(host.weapon!=none && host.weapon!=p.weapon)continue;
    if(host.player==none && host.unit==none && host.weapon==none)continue; // No shooter identity at all.
    if(host.effect!=none) {
      if(std::find(p.effects.begin(),p.effects.end(),host.effect)==p.effects.end())continue;
    } else if(host.material!=p.material)continue;
    if(host.tick+settings_.early_ticks<p.tick_floor || host.tick>std::uint64_t(p.tick_floor)+settings_.window_ticks)continue;
    if(length(host.position-p.position)>settings_.tolerance+settings_.tolerance_per_unit*p.distance)continue;
    pending_.erase(at);++stats_.suppressed;
    return true;
  }
  return false;
}
PredictedImpactLedger::HostImpact predicted_host_impact(const NativeGrenadeDetonation &event) {
  PredictedImpactLedger::HostImpact out;
  out.player=event.player;out.unit=event.unit;out.projectile=event.definition;out.effect=event.effect;
  out.tick=event.tick;
  out.position=event.source_locations.empty()?event.position:event.source_locations.front().frame.position;
  return out;
}
PredictedImpactLedger::HostImpact predicted_host_impact(const BulletImpact &hit,std::uint32_t projectile) {
  PredictedImpactLedger::HostImpact out;
  out.weapon=hit.weapon;out.projectile=projectile;out.material=hit.material;out.world=hit.terrain;
  out.tick=hit.tick;out.position=hit.position;
  return out;
}

PredictedImpactTracer::PredictedImpactTracer(const Cache &cache,const LevelContent &content,
    const BspSurfaceLighting &structure):cache_(cache),content_(content),structure_(structure) {
  for(const auto &triangle:content.collision) {
    const auto &s=triangle.source;
    if(s.bsp!=structure.bsp || s.surface<0)continue;
    surfaces_.try_emplace({s.record,s.instance,s.surface},std::int16_t(triangle.material==0xffff?-1:triangle.material),
        triangle.source_normal);
  }
}
std::pair<std::uint32_t,std::uint32_t> PredictedImpactTracer::projectile_effects(std::uint32_t projectile)const {
  const auto found=projectiles_.find(projectile);
  if(found!=projectiles_.end())return found->second;
  // proj+58 material effects and proj+144, as NativeWorldEffects::impact reads them.
  const std::pair value{tag_word(cache_,projectile,0x58),tag_word(cache_,projectile,0x144)};
  projectiles_.emplace(projectile,value);
  return value;
}
std::array<std::uint32_t,4> PredictedImpactTracer::effects(const WeaponDefinition &weapon,const BulletImpact &hit)const {
  std::array<std::uint32_t,4> out{none,none,none,none};
  if(weapon.projectile==none || (hit.presentation && !hit.presentation->admitted))return out;
  const auto rows=std::span<const std::byte>(weapon.projectile_material_responses);
  const auto response=source_projectile_material_response(rows,content_.material_parents,hit.material);
  if(!response && !hit.presentation)return out;
  std::int16_t type{};
  if(hit.presentation)type=hit.presentation->response;
  else std::memcpy(&type,rows.data()+*response*0x58+2,2);
  const auto slot=source_projectile_material_effect_slot(type);
  const auto [foot,contact]=projectile_effects(weapon.projectile);
  if(foot!=none && slot) {
    const auto visual=source_material_visual_effects(cache_,foot,-1,hit.material,*slot,0);
    std::copy(visual.begin(),visual.end(),out.begin());
  }
  if(hit.material!=-1 && contact!=none)out[3]=contact;
  return out;
}
std::optional<PredictedWorldImpact> PredictedImpactTracer::trace(const WeaponDefinition &weapon,Vec3 origin,
    Vec3 direction,float range,std::span<const PredictedImpactOccluder> occluders,std::uint64_t tick,
    std::uint32_t &random)const {
  if(!(range>0) || !(length(direction)>0))return std::nullopt;
  const auto dir=normalized(direction);
  const auto ray=dir*range;
  std::optional<MapVectorContact> contact;
  try{contact=original_map_vector_contact(structure_,origin,ray,nullptr,0x04800005u);}
  catch(const FormatError &){return std::nullopt;}
  if(!contact)return std::nullopt;
  const float distance=contact->contact.fraction*range;
  const auto end=origin+dir*distance;
  for(const auto &body:occluders) {
    const auto top=body.base+Vec3{0,0,body.height};
    if(segment_distance(origin,end,body.base,top)<body.radius)return std::nullopt;
  }
  BulletImpact hit;
  hit.tick=tick;hit.weapon=weapon.datum;hit.position=contact->point;hit.incoming=dir;hit.terrain=true;
  const auto surface=surfaces_.find({contact->source.record,contact->source.instance,contact->source.surface});
  if(surface==surfaces_.end())return std::nullopt; // Unknown surface: no authored material to respond with.
  hit.material=surface->second.first;
  hit.normal=length(surface->second.second)>0?normalized(surface->second.second):dir*-1.f;
  if(dot(hit.normal,dir)>0)hit.normal=hit.normal*-1.f;
  // The terrain branch of the native collision owner's response selection
  // (Simulation::trace): material response row, approach angles and scale.
  if(content_.original_data && content_.original_data->unit_animation_constants) {
    const auto velocity=dir*weapon.projectile_speed;
    const float speed=length(velocity);
    const auto index=source_projectile_material_response(weapon.projectile_material_responses,
        content_.material_parents,hit.material);
    const auto damage_scale=source_projectile_contact_damage_scale(weapon.projectile_flags,
        origin,hit.position,0.f,weapon.projectile_dry_damage_range,1.f);
    if(!index)hit.presentation=source_projectile_default_impact_presentation(hit.normal,velocity,damage_scale,0.f);
    else {
      const auto raw=std::span<const std::byte>(weapon.projectile_material_responses).subspan(*index*0x58,0x58);
      const auto &constants=*content_.original_data->unit_animation_constants;
      const SourceProjectileResponseServices services{
          std::bit_cast<float>(constants.one_bits),std::bit_cast<float>(constants.rng_scale_bits),
          [&]{return std::uint8_t(11);},[&]()->std::uint32_t&{return random;}};
      const auto angles=source_projectile_impact_angles(raw,hit.normal,hit.incoming,velocity,services);
      const auto response=choose_source_projectile_response(raw,{1,angles.angle,speed},services);
      hit.presentation=SourceProjectileImpactPresentation{response,
          source_projectile_effect_scale(raw,angles.angle,damage_scale),0.f,angles.approach>.25f};
    }
  }
  PredictedWorldImpact out;
  out.effects=effects(weapon,hit);
  if(std::all_of(out.effects.begin(),out.effects.end(),[](auto e){return e==none;}))return std::nullopt;
  out.impact=hit;out.distance=distance;
  return out;
}
} // namespace halo2
