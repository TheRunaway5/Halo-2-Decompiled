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
  for(unsigned representation:{2u,3u}) {
    const auto name=representation==2?"objects\\characters\\masterchief\\masterchief_mp":
        "objects\\characters\\elite\\elite_mp";
    const auto biped=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.group=="bipd" && t.name==name;});
    check(biped!=cache.tags().end(),"Missing authored HUD representation");
    for(const auto &tag:cache.tags()) {
      if(tag.group!="weap" || !tag.address || tag.size<0x2d0)continue;
      const auto metadata=cache.load_source_tag_metadata(tag.datum);
      const Bytes root(metadata.bytes_at(tag.address,tag.size));
      if(root.u32(0x2b4)==null_datum)continue;
      auto content=cache.load_hud(biped->datum,tag.datum);content.action_messages.enabled=false;
      std::erase_if(content.widgets,[&](const auto &w) {
        return w.weapon_type!=1 || w.is_text || w.bitmap==null_datum ||
            (!content.images.at(w.bitmap).name.starts_with("ui\\hud\\bitmaps\\new_hud\\scope_masks\\") &&
             !content.images.at(w.bitmap).name.starts_with("ui\\hud\\bitmaps\\new_hud\\crosshairs\\"));
      });
      if(content.widgets.empty())continue;
      const auto levels=std::int16_t(root.u16(0x1fe));
      check(levels>=0 && levels<=2,"Authored optic needs another power consumer: "+tag.name);
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
              (representation==3?0x2000u:0u));
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
  check(weapons>=60 && optics.size()==6 && scopes==12 && variants>=150,"Incomplete all-weapon HUD coverage");
  Cache gameplay(maps/"lockout.map");const auto level=gameplay.load_content(0,true);
  Mesh mesh;NativeWeaponHud campaign(gameplay,level,mesh,maps/"fonts",assets);
  unsigned campaign_weapons{};
  for(const auto &[id,w]:level.weapons)if(w.first_person_model!=null_datum) {
    Unit unit;unit.exists=true;unit.vitality.body=unit.vitality.shield=1;
    unit.weapon=native_weapon_state(w,0x10000u);unit.source_inventory.current[0]=0;
    unit.source_inventory.slots[0]=unit.weapon.source_datum;
    check(!campaign.frame(unit,0,640,480).empty(),"Campaign crosshair missing after weapon selection: "+w.name);
    if(w.zoom.levels) {
      unit.source_zoom.current=0;
      check(!campaign.frame(unit,1,1280,720).empty(),"Campaign zoom HUD missing: "+w.name);
    }
    ++campaign_weapons;
  }
  check(campaign_weapons>=15,"Incomplete campaign HUD weapon selection coverage");
  std::cout<<weapons<<" weapon/representation HUDs, "<<rows<<" authored rows, "<<optics.size()
      <<" optics, "<<scopes<<" scope masks, "<<draws<<" real visibility/layout draws, "
      <<campaign_weapons<<" campaign weapon selections passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
