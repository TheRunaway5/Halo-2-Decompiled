#include "halo2/multiplayer_world.hpp"
#include "halo2/hud.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <set>
using namespace halo2;
namespace {void check(bool yes,const std::string &why){if(!yes)throw FormatError(why);}}
int main(int argc,char **argv)try {
  check(argc==4,"Supply original MAP SHARED TU1.5_XBE");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto authored=cache.load_content(0,true);
  std::set<std::uint32_t> tested;
  unsigned worlds{},levels{};
  for(unsigned option=3;option<=20;++option) {
    const auto id=cache.slayer_starting_weapon(option);const auto &weapon=authored.weapons.at(id);
    if((!weapon.zoom.levels && !weapon.smg) || !tested.insert(id).second)continue;
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.local_players=2;options.starting_weapon=option;options.source_object_world=true;
    options.session_random_seed=0x105240u;
    MultiplayerPreparedWorld world(options);
    check(world.source_objects && world.source_objects->owns_physics(),"Actual TU1.5 world required");
    const auto local_slots=world.local_slots;world.bind_source_local_views(local_slots);
    std::array<Controls,multiplayer_capacity> controls{};
    for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
    auto state=world.match.state();
    const auto player=world.local_slots.at(0),other=world.local_slots.at(1);
    check(state.players[player].weapon==id && state.players[player].source_unit!=null_datum,
        "Original starting equipment did not select "+weapon.name);
    auto content=cache.load_hud(authored.player_representations.at(2).biped.datum,id);
    content.action_messages.enabled=false;
    std::erase_if(content.widgets,[](const auto &w){return w.weapon_type!=1;});
    Mesh mesh;MultiplayerHud hud(content,std::filesystem::path(argv[2])/"fonts",mesh,2,"Slayer",
        cache.original_data()->hud_bitmap_geometry_constants);
    const int count=std::max<int>(1,weapon.zoom.levels);
    for(int selector=0;selector<count;++selector) {
      controls[player].zoom=std::int16_t(selector);
      controls[player].zoom_reset_serial=state.players[player].zoom_reset_serial;
      world.match.step(controls);state=world.match.state();
      const auto &p=state.players[player];
      check(p.source_zoom.current==selector && p.source_zoom.requested==selector,
          "Delivered source zoom did not reach the actual unit/HUD: "+weapon.name);
      const auto body=world.source_objects->object_body(p.source_unit);
      const auto raw=world.source_objects->memory().view(body+0x240u,2);
      check(std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(raw[0]))==p.source_zoom.current &&
          std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(raw[1]))==p.source_zoom.requested,
          "HUD snapshot differs from actual TU1.5 unit bytes");
      check(p.source_hud_zoom_mask==std::uint16_t((selector?0x100:0x80)|(weapon.zoom.levels?0:0x400)),
          "Authored optic/binocular conditions lost: "+weapon.name);
      check(state.players[other].source_zoom.current==-1 && state.players[other].source_hud_zoom_mask==0x40,
          "Zoom leaked into another actual source unit");
      hud.animate(state,player,0,0,0,state.tick*tick_seconds);
      if(weapon.zoom.levels)check(!hud.draw(state,player,0,2,1280,720,0,0).empty(),
          "Actual scoped weapon HUD submitted no widgets: "+weapon.name);
      ++levels;
    }
    controls[player].zoom=-1;world.match.step(controls);state=world.match.state();
    check(state.players[player].source_hud_zoom_mask==0x40,"Source optic failed to zoom out");
    std::cout<<"PASS actual TU1.5 "<<weapon.name<<" ("<<count<<" levels)\n"<<std::flush;
    ++worlds;
  }
  check(worlds==7 && levels==9,"Incomplete actual TU1.5 optic/binocular coverage");
  std::cout<<worlds<<" original source worlds, "<<levels<<" actual zoom levels, independent units/HUD snapshots passed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
