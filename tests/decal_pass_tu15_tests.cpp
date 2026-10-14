#include "halo2/source_decal_pass.hpp"
#include "halo2/source_decal_draw.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/content.hpp"
#include "halo2/simulation.hpp"
#include "halo2/multiplayer.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5)throw FormatError("Supply FIXTURE MAP SHARED TU15");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  unsigned cases{};
  for(const auto &row:fixture.at("passes")) {
    const auto layer=row.at("layer").get<unsigned>();if(layer<2)continue;
    const auto actual=source_decal_pass(layer);
    const auto cached=source_decal_layer_state_tu15(layer);
    if(cached.pixel.words!=actual.program.words || cached.source_blend!=actual.source ||
       cached.destination_blend!=actual.destination || cached.blend_equation!=actual.operation)
      throw FormatError("Source cache decal draw changed its original layer program/state");
    if(actual.program.words!=row.at("program").get<std::array<std::uint32_t,60>>())
      throw FormatError("Whole original decal program changed at layer"+std::to_string(layer));
    for(const auto &state:row.at("states")) {
      const unsigned index=state[0],value=state[1];
      if((index==62 && value!=actual.source) || (index==63 && value!=actual.destination) ||
         (index==74 && value!=actual.operation))throw FormatError("Original decal blend changed");
    }
    ++cases;
  }
  for(const auto &row:fixture.at("colors")) {
    const auto rgb=[&](const char *key) {const auto v=row.at(key).get<std::array<std::uint32_t,3>>();
      return Vec3{std::bit_cast<float>(v[0]),std::bit_cast<float>(v[1]),std::bit_cast<float>(v[2])};};
    if(source_decal_rgb(rgb("lower"),rgb("upper"),std::bit_cast<float>(row.at("fraction").get<std::uint32_t>()))!=
       row.at("packed").get<std::uint32_t>())throw FormatError("Original decal RGB interpolation/packing changed");
    ++cases;
  }
  for(const auto &row:fixture.at("fades")) {
    const float elapsed=float(row.at("tick").get<unsigned>())*
        std::bit_cast<float>(row.at("seconds").get<std::uint32_t>());
    if(source_decal_alpha(row.at("lifetime"),row.at("decay"),elapsed)!=row.at("alpha").get<unsigned>())
      throw FormatError("Whole original decal alpha fade changed");
    ++cases;
  }
  Cache cache(argv[2],argv[3],OriginalData::load_xbe(argv[4]));
  const auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);Mesh mesh;
  NativeWorldEffects effects(cache,mesh,collision,0x12345678);unsigned max{},add{};
  for(const auto &m:mesh.materials)if(m.forward_vertex==11) {
    max+=m.forward_pass_index==4 && m.forward_blend==12;
    add+=m.forward_pass_index==5 && m.forward_blend==2;
    if(m.forward_program.words!=source_decal_pass(m.forward_pass_index).program.words)
      throw FormatError("Actual decal material lost original program");
  }
  if(!max || !add)throw FormatError("Actual glowing impact layers were omitted");
  UiVariant variant{"Slayer",0,{}};variant.values.set(0x59,13);auto rules=slayer_rules(variant);
  rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  MultiplayerMatch match(content,0,cache.spawns(),rules);const auto player=match.join("Carbine decal");
  std::array<Controls,multiplayer_capacity> controls{};
  controls[player].yaw=match.unit(player).yaw;controls[player].pitch=-1.2f;
  std::uint64_t serial{};unsigned glows{};bool faded{};
  for(unsigned tick=0;tick<90;++tick) {
    controls[player].fire=tick==1;match.step(controls);
    for(const auto &hit:match.state().bullet_impacts)if(hit.serial>serial){serial=hit.serial;effects.impact(hit);}
    effects.advance(match.state().tick,tick_seconds);
    for(const auto &v:effects.decals(match.state().tick,0,tick_seconds))
      if(mesh.materials.at(v.material).name=="effects\\decals\\bullet_holes\\carbine_glow") {
        if(v.color.y<=v.color.x || v.color.y<=v.color.z)throw FormatError("Actual Carbine glow lost authored green");
        faded|=v.source_color_alpha>0 && v.source_color_alpha<1;++glows;
      }
  }
  if(!serial || !glows || !faded)throw FormatError("Real Carbine contact failed to create and fade its authored glow");
  std::cout<<"PASS "<<cases<<" original decal contracts; actual max/add materials="<<max<<'/'<<add
           <<" green Carbine decal vertices="<<glows<<"; alpha fading retained RGB\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
