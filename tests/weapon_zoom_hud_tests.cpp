#include "halo2/native_weapon_hud.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool yes,const std::string &why){if(!yes)throw FormatError(why);}
}
int main(int argc,char **argv)try {
  check(argc==2 || argc==3,"Supply original maps and optional imported HUD pack");
  const std::filesystem::path maps=argv[1],assets=argc==3?argv[2]:"";
  Cache cache(maps/"shared.map");
  unsigned weapons{},rows{},draws{},scopes{},variants{};
  std::set<std::string> optics;
  for(unsigned representation:{0u,1u,2u,3u}) {
    const auto name=std::array{"objects\\characters\\masterchief\\masterchief",
        "objects\\characters\\dervish\\dervish","objects\\characters\\masterchief\\masterchief_mp",
        "objects\\characters\\elite\\elite_mp"}.at(representation);
    const auto biped=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.group=="bipd" && t.name==name;});
    check(biped!=cache.tags().end(),"Missing authored HUD representation");
    for(const auto &tag:cache.tags()) {
      if(tag.group!="weap" || !tag.address || tag.size<0x2d0)continue;
      const auto metadata=cache.load_source_tag_metadata(tag.datum);
      const Bytes root(metadata.bytes_at(tag.address,tag.size));
      if(root.u32(0x2b4)==null_datum) {
        if(representation==0)std::cout<<"HUD_NONE weapon="<<tag.name<<'\n';
        continue;
      }
      auto content=cache.load_hud(biped->datum,tag.datum);content.action_messages.enabled=false;
      check(content.unit_representation==representation,"HUD lost its authored campaign/multiplayer biped representation");
      std::erase_if(content.widgets,[&](const auto &w) {
        return w.weapon_type!=1 || w.is_text || w.bitmap==null_datum ||
            (!content.images.at(w.bitmap).name.starts_with("ui\\hud\\bitmaps\\new_hud\\scope_masks\\") &&
             !content.images.at(w.bitmap).name.starts_with("ui\\hud\\bitmaps\\new_hud\\crosshairs\\"));
      });
      if(content.widgets.empty())continue;
      const auto levels=std::int16_t(root.u16(0x1fe));
      check(levels>=0 && levels<=2,"Authored optic needs another power consumer: "+tag.name);
      if(representation==0)std::cout<<"CATALOG weapon="<<tag.name<<" levels="<<levels
          <<" rows="<<content.widgets.size()<<'\n';
      const std::array<std::int16_t,1> modes{};
      const ZoomWeaponView weapon{{root.u32(0x12c),levels,root.f32(0x200),root.f32(0x204)},
          content.weapon_magazine_count?std::span(modes):std::span<const std::int16_t>{}};
      if(levels) {
        optics.insert(tag.name);
        std::int16_t selected=-1;
        for(int level=0;level<levels;++level) {
          selected=source_zoom_next(selected,&weapon,{1,2,2});
          const auto value=source_zoom_magnification(selected,&weapon,{1,2,2});
          check(selected==level && value && *value==(level?weapon.definition.maximum:weapon.definition.minimum),
              "Authored optic selector/magnification lost: "+tag.name);
        }
        check(source_zoom_next(selected,&weapon,{1,2,2})==-1,"Optic failed to cycle out");
      }
      ++weapons;
      for(const auto &widget:content.widgets) {
        auto isolated=content;isolated.widgets={widget};
        Mesh mesh;MultiplayerHud hud(isolated,maps/"fonts",mesh,4,"Slayer",
            cache.original_data()->hud_bitmap_geometry_constants,assets);
        ++rows;scopes+=widget.name=="scope_mask";
        bool visible{};
        for(int level=-1;level<std::max<int>(1,levels);++level)
        for(std::uint32_t target:{0u,1u,4u,8u,0x10u,0x11u,0x14u})
        for(std::uint8_t trigger:{std::uint8_t(0),std::uint8_t(5),std::uint8_t(6)}) {
          MultiplayerState state;auto &p=state.players[0];p.connected=true;p.health=p.shield=1;
          p.loaded=10;p.weapon_trigger.mode=trigger;p.weapon_trigger.target_acquired=true;
          p.weapon_trigger.target_position=std::array{10.f,0.f,0.f};
          p.source_hud_target_flags=target;
          if(representation==3){p.source_configuration=source_default_player_configuration();(*p.source_configuration)[0x44]=1;}
          p.source_hud_zoom_mask=source_zoom_hud_mask(source_zoom_hud_status(std::int8_t(level),&weapon));
          const auto aim=std::uint16_t((target&1u)|((target&8u)?2u:0u)|((target&0x10u)?4u:0u)|((target&4u)?8u:0u));
          const std::uint16_t unit=std::uint16_t(1u|2u|0x10u|0x800u|0x1000u|p.source_hud_zoom_mask|
              ((representation==1 || representation==3)?0x2000u:0u));
          const auto projected=multiplayer_hud_weapon_state(p,content,0);
          const auto weapon_mask=weapon_hud_cutoffs(widget.weapon_type==1?&projected:nullptr,widget.cutoffs,
              weapon_hud_mask(widget.weapon_type==1?&projected:nullptr,widget.weapon_type));
          const bool expected=hud_widget_visible(widget,{unit,aim,weapon_mask,0x294});
          for(unsigned players:{1u,2u,4u})for(unsigned width:{640u,1280u}) {
            const auto vertices=hud.draw(state,0,0,players,width,width==640?480:720,0,0,{}, {},
                SourceHudAimCamera{{},{1,0,0},{0,0,1},1.f,false});
            check(vertices.empty()!=expected,"Reticle/scope visibility lost: "+tag.name+" / "+widget.name);
            visible|=!vertices.empty();++draws;
          }
        }
        check(visible,"Authored reticle/scope never admitted: "+tag.name+" / "+widget.name);++variants;
      }
    }
  }
  check(weapons>=120 && optics.size()==6 && scopes==24 && variants>=300,"Incomplete all-weapon HUD coverage");
  Cache gameplay(maps/"lockout.map");const auto level=gameplay.load_content(0,true);
  const auto smg=std::find_if(level.weapons.begin(),level.weapons.end(),[](const auto &w){return w.second.smg.has_value();});
  check(smg!=level.weapons.end(),"Missing original SMG reticle for shader-input check");
  auto reticle_content=gameplay.load_hud(level.player_representations.at(2).biped.datum,smg->first);
  reticle_content.action_messages.enabled=false;
  std::erase_if(reticle_content.widgets,[](const auto &w){return w.name!="crosshair";});
  Mesh reticle_mesh;MultiplayerHud reticle(reticle_content,maps/"fonts",reticle_mesh,1,"Slayer",
      gameplay.original_data()->hud_bitmap_geometry_constants,assets);
  MultiplayerState aim_state;aim_state.players[0].connected=true;
  reticle.animate(aim_state,0,0,0,0,0);
  const auto vertices=reticle.draw(aim_state,0,0,1,640,480,0,0);
  check(!vertices.empty(),"Missing actual SMG reticle shader binding");
  const auto material=vertices[0].material;
  const auto base_words=reticle_mesh.materials[material].forward_program.words;
  const auto base_constants=reticle_mesh.materials[material].program_constants;
  aim_state.players[0].source_hud_autoaimed=true;reticle.animate(aim_state,0,0,0,0,0);
  check(reticle_mesh.materials[material].forward_program.words!=base_words ||
      reticle_mesh.materials[material].program_constants!=base_constants,
      "Actual reticle shader did not consume the original confidence selector18");
  aim_state.players[0].source_hud_autoaimed=false;aim_state.players[0].source_hud_target_flags=8;
  reticle.animate(aim_state,0,0,0,0,0);
  check(reticle_mesh.materials[material].forward_program.words==base_words &&
      reticle_mesh.materials[material].program_constants==base_constants,
      "Plasma-tracking mask was substituted for the independent source confidence input");
  Mesh mesh;NativeWeaponHud campaign(gameplay,level,mesh,maps/"fonts",assets);
  unsigned campaign_weapons{},source_items{};
  for(const auto &[id,w]:level.weapons)if(w.first_person_model!=null_datum) {
    const auto authored=gameplay.load_hud(level.player_representations.at(2).biped.datum,id);
    if(std::ranges::none_of(authored.widgets,[](const auto &row){return row.weapon_type==1;}))continue;
    Unit unit;unit.exists=true;unit.vitality.body=unit.vitality.shield=1;
    unit.weapon=native_weapon_state(w,0x10000u);unit.source_inventory.current[0]=0;
    unit.source_inventory.slots[0]=unit.weapon.source_datum;
    check(!campaign.frame(unit,0,640,480).empty(),"Campaign crosshair missing after weapon selection: "+w.name);
    if(w.zoom.levels) {
      unit.source_zoom.current=0;
      check(!campaign.frame(unit,1,1280,720).empty(),"Campaign zoom HUD missing: "+w.name);
    }
    ++campaign_weapons;
    source_items+=w.source_runtime_only;
  }
  check(campaign_weapons==19 && source_items==4,"Incomplete ordinary/objective/depleted weapon HUD selection coverage");
  auto limited_level=level;
  std::erase_if(limited_level.weapons,[](const auto &row){return row.second.source_runtime_only;});
  Mesh limited_mesh;NativeWeaponHud limited_hud(gameplay,limited_level,limited_mesh,maps/"fonts",assets);
  for(const auto &[id,w]:level.weapons)if(w.source_runtime_only) {
    const auto authored=gameplay.load_hud(level.player_representations.at(2).biped.datum,id);
    if(std::ranges::none_of(authored.widgets,[](const auto &row){return row.weapon_type==1;}))continue;
    MultiplayerState::Player player;player.connected=true;player.health=player.shield=1;player.weapon=id;
    player.source_hud_zoom_mask=0x40;
    check(!limited_hud.frame(player,0,1280,720).empty(),"Authored item HUD depends on native projectile support: "+w.name);
  }
  std::set<std::string> campaign_catalog,campaign_optics;
  unsigned campaign_levels{},campaign_maps{},arbiter_bindings{};
  for(const auto map:{"01b_spacestation.map","04b_floodlab.map","05b_deltatowers.map"}) {
    Cache campaign_cache(maps/map);
    const auto campaign_level=campaign_cache.load_content(0);
    const auto representation=campaign_cache.load_hud(campaign_level.player.datum,null_datum).unit_representation;
    check(representation.has_value(),"Campaign player lacks an authored HUD representation");
    Mesh campaign_mesh;
    NativeWeaponHud campaign_hud(campaign_cache,campaign_level,campaign_mesh,maps/"fonts",assets);
    unsigned resident{};
    for(const auto &[id,w]:campaign_level.weapons) {
      if(w.first_person_models.empty())continue;
      const auto preferred=*representation==1 || *representation==3 ? 1u:0u;
      const auto &binding=w.first_person_models.at(w.first_person_models.size()>preferred?preferred:0u);
      check(w.first_person_model==binding.model && w.first_person_animation==binding.animation,
          "Campaign weapon selected the wrong original first-person record: "+w.name);
      if(binding.model==null_datum)continue;
      if(*representation==1)++arbiter_bindings;
      const auto authored=campaign_cache.load_hud(campaign_level.player.datum,id);
      if(std::ranges::none_of(authored.widgets,[](const auto &row){return row.weapon_type==1;}))continue;
      Unit unit;unit.exists=true;unit.vitality.body=unit.vitality.shield=1;
      unit.weapon=native_weapon_state(w,0x10000u);unit.source_inventory.current[0]=0;
      unit.source_inventory.slots[0]=unit.weapon.source_datum;
      check(!campaign_hud.frame(unit,0,1280,720).empty(),"Resident campaign crosshair missing: "+w.name);
      std::cout<<"CAMPAIGN map="<<map<<" weapon="<<w.name<<" levels="<<w.zoom.levels
          <<" source_only="<<w.source_runtime_only<<'\n';
      if(!w.source_runtime_only)campaign_catalog.insert(w.name);
      ++resident;
      for(int zoom=0;zoom<w.zoom.levels;++zoom) {
        unit.source_zoom.current=std::int8_t(zoom);
        check(!campaign_hud.frame(unit,1,1280,720).empty(),"Resident campaign scope missing: "+w.name);
        if(campaign_optics.insert(w.name).second)campaign_levels+=unsigned(w.zoom.levels);
      }
    }
    check(resident>0,"Campaign cache prepared no handheld HUDs");++campaign_maps;
  }
  check(campaign_catalog.size()==16 && campaign_optics.size()==6 && campaign_levels==8 && arbiter_bindings>=8,
      "Incomplete campaign weapon/species/optic catalog");
  std::cout<<weapons<<" weapon/representation HUDs, "<<rows<<" authored rows, "<<optics.size()
      <<" optics, "<<scopes<<" scope masks, "<<draws<<" real visibility/layout draws, "
      <<campaign_weapons<<" multiplayer-cache weapon selections, "<<campaign_catalog.size()
      <<" campaign handhelds across "<<campaign_maps<<" maps, "<<campaign_levels<<" campaign optic levels and "
      <<arbiter_bindings<<" Arbiter FP bindings and "<<source_items<<" objective/depleted HUDs passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
