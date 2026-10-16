#include "halo2/native_weapon_hud.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/native_unit_interaction.hpp"
#include <algorithm>
#include <set>
namespace halo2 {
namespace {
bool resident_campaign_hud(const Cache &cache,std::uint32_t id) {
  if(id==null_datum)return false;
  const auto tag=cache.query_tag({.handle=TagHandle{id},.group=TagGroup::from_string("nhdt")});
  if(!tag)throw FormatError("Campaign HUD references an invalid nhdt tag");
  if(!tag->address || !tag->size)return false;
  if(tag->size<0x28)throw FormatError("Truncated campaign HUD definition");
  return true;
}
}
std::vector<HandheldWeaponHudBinding> handheld_weapon_hud_bindings(const Cache &cache,unsigned representation) {
  std::vector<HandheldWeaponHudBinding> result;
  const unsigned preferred=representation==1 || representation==3 ? 1u:0u;
  for(const auto &tag:cache.select_tags({.group=tag_groups::weapon,.require_address=true})) {
    const Bytes root(cache.tag_payload(tag.handle()));
    const auto records=read_tag_block_descriptor(root,0x2a8);
    const auto extent=records.extent(16,256);
    if(!records.count)continue;
    const Bytes rows(cache.read_tag_bytes(records.address,extent));
    const auto record=(unsigned(records.count)>preferred?preferred:0u)*16u;
    if(rows.u32(record+4)==null_datum)continue;
    result.push_back({tag.datum,(root.u32(0x12c)&(1u<<22))!=0});
  }
  return result;
}
MultiplayerState::Player native_weapon_hud_player(const Unit &unit,const LevelContent &content) {
  MultiplayerState::Player p;p.connected=unit.exists;p.dead=unit.vitality.dead;
  p.health=unit.vitality.body;p.shield=unit.vitality.shield;
  p.position=unit.body.position;p.velocity=unit.body.velocity;p.yaw=unit.yaw;p.pitch=unit.pitch;p.crouch=unit.crouch;
  p.grenades=unit.grenades;p.grenade_current=unit.grenade_current;p.source_membership=unit.source_membership;
  p.source_parent=unit.source_parent;p.source_seat=unit.source_seat;p.source_action_word=unit.source_action_word;
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
  } else p.source_hud_zoom_mask=source_zoom_hud_mask(source_zoom_hud_status(unit.source_zoom.current,nullptr));
  if(const auto *weapon=native_hand_weapon(unit,1)) {
    auto &s=p.secondary_weapon;s.definition=weapon->definition;s.datum=weapon->source_datum;
    s.loaded=weapon->loaded;s.reserve=weapon->reserve;s.shader=weapon->shader;
    s.trigger={weapon->smg.input_flags,weapon->smg.trigger_ticks,weapon->smg.trigger_state,weapon->charge_fraction};
    s.ammunition_available=native_weapon_ammunition_available(unit,*weapon);
    const auto definition=content.weapons.find(weapon->definition);
    s.magazine_mode=definition!=content.weapons.end() && definition->second.smg?
        weapon->smg.magazine_mode:weapon->magazine.mode;
  }
  return p;
}
NativeWeaponHud::NativeWeaponHud(const Cache &cache,const LevelContent &content,Mesh &mesh,
    const std::filesystem::path &fonts,const std::filesystem::path &assets):content_(content) {
  auto unit=cache.load_hud(content.player.datum,null_datum);
  if(!unit.unit_representation)throw FormatError("Campaign weapon HUD lacks its authored player representation");
  const auto handhelds=handheld_weapon_hud_bindings(cache,*unit.unit_representation);
  unit.action_messages.enabled=false;
  std::erase_if(unit.widgets,[](const auto &w){return !(w.yes[0]&0x400u);});
  binocular_=std::make_unique<MultiplayerHud>(std::move(unit),fonts,mesh,1,"",
      cache.original_data()->hud_bitmap_geometry_constants,assets);
  for(const auto &weapon:handhelds) {
    const auto id=weapon.definition;
    auto hud=cache.load_hud(content.player.datum,id);hud.action_messages.enabled=false;
    std::erase_if(hud.widgets,[](const auto &w){return w.weapon_type!=1;});
    primary_.emplace(id,std::make_unique<MultiplayerHud>(hud,fonts,mesh,1,"",
        cache.original_data()->hud_bitmap_geometry_constants,assets));
    if(weapon.dual_wieldable)secondary_.emplace(id,std::make_unique<MultiplayerHud>(std::move(hud),
        fonts,mesh,1,"",cache.original_data()->hud_bitmap_geometry_constants,assets,1));
  }
  std::set<std::uint32_t> vehicle_weapons,seat_huds;
  if(content.unit_interactions)for(const auto &[id,unit]:content.unit_interactions->units) {
    const auto metadata=cache.load_source_tag_metadata(id);
    const Bytes descriptor(metadata.bytes_at(metadata.source_address()+0x1c0u,8));
    const auto count=descriptor.u32(0);
    if(count>4096)throw FormatError("Excessive authored campaign unit weapon list");
    if(count) {
      const Bytes rows(metadata.bytes_at(descriptor.u32(4),count*8u));
      for(unsigned n=0;n<count;++n)
        if(const auto weapon=rows.u32(n*8u+4u);weapon!=null_datum)vehicle_weapons.insert(weapon);
    }
    for(auto hud:cache.load_unit_seat_huds(id))seat_huds.insert(hud);
  }
  for(auto id:vehicle_weapons) {
    // Cinematic caches retain vehicle weapon definitions while pruning their
    // unused HUDs. The opening's banshee_gun nhdt is an actual size-zero tag.
    // Preload only resident HUDs; the live vehicle draw still requires its
    // authored owner if the source later admits that HUD to a player view.
    const Bytes weapon(cache.tag_payload(TagHandle{id}));
    if(!resident_campaign_hud(cache,weapon.u32(0x2b4)))continue;
    auto authored=cache.load_hud(content.player.datum,id);authored.action_messages.enabled=false;
    std::erase_if(authored.widgets,[](const auto &w){return w.weapon_type!=1;});
    for(auto &w:authored.widgets)w.weapon_type=4;
    vehicle_.emplace(id,std::make_unique<MultiplayerHud>(std::move(authored),fonts,mesh,1,"",
        cache.original_data()->hud_bitmap_geometry_constants,assets,2));
  }
  for(auto id:seat_huds) {
    if(!resident_campaign_hud(cache,id))continue;
    auto authored=cache.load_hud_definition(content.player.datum,id);authored.action_messages.enabled=false;
    parent_.emplace(id,std::make_unique<MultiplayerHud>(std::move(authored),fonts,mesh,1,"",
        cache.original_data()->hud_bitmap_geometry_constants,assets));
  }
}
std::vector<Vertex> NativeWeaponHud::frame(const Unit &unit,std::uint32_t tick,
    unsigned width,unsigned height,std::optional<Vec2> aim_anchor,std::optional<SourceHudAimCamera> target_camera) {
  return frame(native_weapon_hud_player(unit,content_),tick,width,height,aim_anchor,target_camera);
}
std::vector<Vertex> NativeWeaponHud::frame(const MultiplayerState::Player &player,std::uint32_t tick,
    unsigned width,unsigned height,std::optional<Vec2> aim_anchor,std::optional<SourceHudAimCamera> target_camera) {
  MultiplayerState state;state.tick=tick;state.players[0]=player;
  std::vector<Vertex> result;
  if(!player.connected || player.dead)return result;
  const auto append=[&](MultiplayerHud &hud) {
    hud.animate(state,0,0,0,0,double(tick)*tick_seconds);
    auto vertices=hud.draw(state,0,0,1,width,height,0,0,aim_anchor,{},target_camera);
    result.insert(result.end(),vertices.begin(),vertices.end());
  };
  if(player.source_vehicle_hud.weapon.present) {
    const auto found=vehicle_.find(player.source_vehicle_hud.definition);
    if(found==vehicle_.end())throw FormatError("Campaign vehicle weapon has no prepared authored HUD");
    append(*found->second);
  }
  if(player.source_vehicle_hud.ownership.handheld_visible) {
    if(const auto found=primary_.find(player.weapon);found!=primary_.end())append(*found->second);
    if(player.secondary_weapon.datum!=null_datum)
      if(const auto found=secondary_.find(player.secondary_weapon.definition);found!=secondary_.end())append(*found->second);
  }
  if(player.source_vehicle_hud.ownership.parent_hud!=null_datum) {
    const auto found=parent_.find(player.source_vehicle_hud.ownership.parent_hud);
    if(found==parent_.end())throw FormatError("Campaign seat has no prepared authored HUD");
    append(*found->second);
  }
  append(*binocular_);return result;
}
}
