#include "halo2/native_weapon_hud.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <algorithm>
namespace halo2 {
MultiplayerState::Player native_weapon_hud_player(const Unit &unit,const LevelContent &content) {
  MultiplayerState::Player p;p.connected=unit.exists;p.dead=unit.vitality.dead;
  p.health=unit.vitality.body;p.shield=unit.vitality.shield;
  p.source_unit=unit.source_datum;p.weapon=unit.weapon.definition;p.source_weapon=unit.weapon.source_datum;
  p.loaded=unit.weapon.loaded;p.reserve=unit.weapon.reserve;p.weapon_shader=unit.weapon.shader;
  p.weapon_trigger={unit.weapon.smg.input_flags,unit.weapon.smg.trigger_ticks,
      unit.weapon.smg.trigger_state,unit.weapon.charge_fraction};
  p.ammunition_available=native_weapon_ammunition_available(unit,unit.weapon);
  p.source_zoom=unit.source_zoom;p.zoom_reset_serial=unit.zoom_reset_serial;
  const auto definition=content.weapons.find(unit.weapon.definition);
  if(definition!=content.weapons.end()) {
    p.source_zoom_magazine_mode=definition->second.smg?unit.weapon.smg.magazine_mode:unit.weapon.magazine.mode;
    const std::array modes{p.source_zoom_magazine_mode};
    const ZoomWeaponView weapon{definition->second.zoom,definition->second.magazine?
        std::span(modes):std::span<const std::int16_t>{}};
    p.source_hud_zoom_mask=source_zoom_hud_mask(source_zoom_hud_status(unit.source_zoom.current,&weapon));
  }
  if(const auto *weapon=native_hand_weapon(unit,1)) {
    auto &s=p.secondary_weapon;s.definition=weapon->definition;s.datum=weapon->source_datum;
    s.loaded=weapon->loaded;s.reserve=weapon->reserve;s.shader=weapon->shader;
    s.trigger={weapon->smg.input_flags,weapon->smg.trigger_ticks,weapon->smg.trigger_state,weapon->charge_fraction};
    s.ammunition_available=native_weapon_ammunition_available(unit,*weapon);
  }
  return p;
}
NativeWeaponHud::NativeWeaponHud(const Cache &cache,const LevelContent &content,Mesh &mesh,
    const std::filesystem::path &fonts,const std::filesystem::path &assets):content_(content) {
  auto unit=cache.load_hud(content.player.datum,null_datum);
  unit.action_messages.enabled=false;
  std::erase_if(unit.widgets,[](const auto &w){return !(w.yes[0]&0x400u);});
  binocular_=std::make_unique<MultiplayerHud>(std::move(unit),fonts,mesh,1,"",
      cache.original_data()->hud_bitmap_geometry_constants,assets);
  for(const auto &[id,weapon]:content.weapons) {
    if(weapon.first_person_model==null_datum)continue;
    auto hud=cache.load_hud(content.player.datum,id);hud.action_messages.enabled=false;
    std::erase_if(hud.widgets,[](const auto &w){return w.weapon_type!=1;});
    primary_.emplace(id,std::make_unique<MultiplayerHud>(hud,fonts,mesh,1,"",
        cache.original_data()->hud_bitmap_geometry_constants,assets));
    if(weapon.dual_wieldable)secondary_.emplace(id,std::make_unique<MultiplayerHud>(std::move(hud),
        fonts,mesh,1,"",cache.original_data()->hud_bitmap_geometry_constants,assets,1));
  }
}
std::vector<Vertex> NativeWeaponHud::frame(const Unit &unit,std::uint32_t tick,
    unsigned width,unsigned height,std::optional<Vec2> aim_anchor) {
  MultiplayerState state;state.tick=tick;state.players[0]=native_weapon_hud_player(unit,content_);
  std::vector<Vertex> result;
  const auto append=[&](MultiplayerHud &hud) {
    hud.animate(state,0,0,0,0,double(tick)*tick_seconds);
    auto vertices=hud.draw(state,0,0,1,width,height,0,0,aim_anchor);
    result.insert(result.end(),vertices.begin(),vertices.end());
  };
  if(const auto found=primary_.find(unit.weapon.definition);found!=primary_.end())append(*found->second);
  if(const auto *other=native_hand_weapon(unit,1))
    if(const auto found=secondary_.find(other->definition);found!=secondary_.end())append(*found->second);
  append(*binocular_);return result;
}
}
