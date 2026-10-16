#include "halo2/hud.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
constexpr std::uint32_t base=0x1000000u;
struct Memory:SourcePhysicsContactMemory {
  std::vector<std::byte> bytes=std::vector<std::byte>(0x10000);
  bool later{};
  void read(std::uint32_t p,std::span<std::byte> out)const override {
    if(p==(later?0x4ecbd0u:0x4e78d0u) || p==(later?0x4f0414u:0x4eb114u) || p==(later?0x4f3218u:0x4edf18u)) {
      const auto value=p==(later?0x4ecbd0u:0x4e78d0u)?base:p==(later?0x4f0414u:0x4eb114u)?base+0x4000:base+0x9000;
      if(out.size()!=4)throw FormatError("Wrong HUD global width");
      for(unsigned n=0;n<4;++n)out[n]=std::byte(value>>(8*n));return;
    }
    if(p<base || p-base+out.size()>bytes.size())throw FormatError("Unbacked HUD input");
    std::copy_n(bytes.begin()+(p-base),out.size(),out.begin());
  }
  void write(std::uint32_t p,std::span<const std::byte> in)override {
    if(p<base || p-base+in.size()>bytes.size())throw FormatError("Unbacked HUD input store");
    std::copy(in.begin(),in.end(),bytes.begin()+(p-base));
  }
  void put(std::uint32_t p,std::uint32_t v,unsigned size=4) {
    for(unsigned n=0;n<size;++n)bytes.at(p-base+n)=std::byte(v>>(8*n));
  }
};
void check(bool condition,const char *why){if(!condition)throw FormatError(why);}
}
int main(int argc,char **argv)try {
  check(argc==3 || argc==4,"Supply original ownership fixture, maps and optional imported HUD pack");
  const std::filesystem::path assets=argc==4?argv[3]:"";
  std::ifstream file(argv[1]);if(!file)return 77;nlohmann::json fixture;file>>fixture;unsigned comparisons{};
  for(const auto &row:fixture.at("cases"))for(bool later:{false,true}) {
    const auto &input=row.at("input");Memory memory;memory.later=later;auto &m=memory;
    m.put(base+0x44,base+0x1000);
    auto objects=nlohmann::json::array({nlohmann::json::object(),{{"weapon",input.at("parent_weapon")}}});
    for(const auto &o:input.at("children"))objects.push_back(o);
    for(unsigned n=0;n<objects.size();++n) {
      const auto &o=objects[n];const auto p=base+0x2000+n*0x400,definition=base+0x5000+n*0x400;
      m.put(base+0x1008+n*12,p);m.put(base+0x4008+n*16,definition);m.put(p,0x10000+n);
      m.put(p+0x14,null_datum);m.put(p+0xc,null_datum);m.put(p+0x10,null_datum);
      m.put(p+0x212,o.value("weapon",false)?0:0xff,1);
      m.put(p+0x218,o.value("weapon",false)?0xb3210080+n:null_datum);
      m.put(p+4,o.value("flags",0u));m.put(p+0xaa,o.value("type",0u),1);
      m.put(definition+0xbc,o.value("unit_flags",0u));
      if(n>=2 && n+1<objects.size())m.put(p+0xc,0xa1230000+n+1);
    }
    m.put(base+0x2014,input.at("parent").get<bool>()?0xa1230001:null_datum);
    m.put(base+0x21fc,std::uint16_t(input.at("seat").get<int>()),2);
    m.put(base+0x55cc,base+0x8000);m.put(base+0x8000,input.at("seat_flags").get<unsigned>());
    m.put(base+0x807c,input.at("hud_count").get<unsigned>());m.put(base+0x8080,base+0x8800);
    for(unsigned n=0;n<input.at("hud_count");++n)m.put(base+0x8804+n*8,0xa3210900+n);
    m.put(base+0x9008,input.at("multiplayer").get<bool>()?2:1);
    if(!input.at("children").empty())m.put(base+0x2410,0xa1230002);
    const auto actual=weapon_hud_ownership(m,0xa1230000,later?SourcePhysicsContactImage::TitleUpdate15:SourcePhysicsContactImage::MapPack11);
    const auto &expected=row.at("expected");
    check(actual.seated==expected.at("seated") && actual.handheld_visible==expected.at("handheld_visible") &&
        actual.vehicle_weapon==expected.at("vehicle_weapon") && actual.parent_hud==expected.at("parent_hud"),
        "Native vehicle HUD ownership differs from original execution");
    ++comparisons;
  }
  Cache cache(std::filesystem::path(argv[2])/"shared.map");unsigned vehicle_huds{},draws{};
  const auto biped=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &t){return t.name=="objects\\characters\\masterchief\\masterchief_mp";});
  check(biped!=cache.tags().end(),"Missing original HUD biped");
  for(const auto &tag:cache.tags())if(tag.group=="weap" &&
      (tag.name.starts_with("objects\\vehicles\\") || tag.name.starts_with("objects\\weapons\\fixed\\"))) {
    auto authored=cache.load_hud(biped->datum,tag.datum);authored.action_messages.enabled=false;
    std::erase_if(authored.widgets,[&](const auto &w){return w.weapon_type!=1 || w.is_text || w.bitmap==null_datum ||
        !authored.images.at(w.bitmap).name.starts_with("ui\\hud\\bitmaps\\new_hud\\crosshairs\\");});
    if(authored.widgets.empty())continue;
    for(auto &w:authored.widgets)w.weapon_type=4;
    Mesh mesh;MultiplayerHud hud(authored,std::filesystem::path(argv[2])/"fonts",mesh,4,"Slayer",
        cache.original_data()->hud_bitmap_geometry_constants,assets,2);
    MultiplayerState state;auto &p=state.players[0];p.connected=true;p.health=p.shield=1;
    p.source_vehicle_hud={{true,false,0x12340000},tag.datum,{}};
    auto &weapon=p.source_vehicle_hud.weapon;weapon.datum=0x12340000;weapon.present=1;
    weapon.heat=.75f;weapon.age=.125f;weapon.charge=.5f;
    for(unsigned players:{1u,2u,4u})for(unsigned width:{640u,1280u}) {
      hud.animate(state,0,0,0,0,0);
      check(!hud.draw(state,0,0,players,width,width==640?480:720,0,0).empty(),"Authored vehicle crosshair missing");++draws;
    }
    const auto projected=multiplayer_hud_weapon_state(p,authored,2);
    check(projected.datum==weapon.datum && projected.heat==.75f && projected.age==.125f && projected.charge==.5f,
        "Vehicle HUD consumed handheld weapon state");++vehicle_huds;
  }
  std::set<std::uint32_t> parent_definitions;
  for(const auto &tag:cache.tags())if(tag.group=="vehi")
    for(auto id:cache.load_unit_seat_huds(tag.datum))parent_definitions.insert(id);
  unsigned seat_huds{};
  for(auto id:parent_definitions) {
    auto authored=cache.load_hud_definition(biped->datum,id);
    std::erase_if(authored.widgets,[&](const auto &w){return w.is_text || w.bitmap==null_datum ||
        !authored.images.at(w.bitmap).name.starts_with("ui\\hud\\bitmaps\\new_hud\\crosshairs\\");});
    if(authored.widgets.empty())continue;
    Mesh mesh;MultiplayerHud hud(authored,std::filesystem::path(argv[2])/"fonts",mesh,4,"Slayer",
        cache.original_data()->hud_bitmap_geometry_constants,assets);
    MultiplayerState state;state.players[0].connected=true;state.players[0].health=state.players[0].shield=1;
    for(unsigned players:{1u,2u,4u})for(unsigned width:{640u,1280u}) {
      hud.animate(state,0,0,0,0,0);
      check(!hud.draw(state,0,0,players,width,width==640?480:720,0,0).empty(),"Authored seat HUD crosshair missing");++draws;
    }
    const auto tag=cache.find_tag({id});
    check(tag!=nullptr,"Authored seat HUD lost its cache tag");
    std::cout<<"SEAT HUD="<<tag->name<<'\n';
    ++seat_huds;
  }
  check(comparisons==1296 && vehicle_huds==11 && seat_huds>=1,"Incomplete vehicle HUD coverage");
  std::cout<<comparisons<<" original ownership comparisons, "<<vehicle_huds<<" authored vehicle/turret crosshairs, "
      <<seat_huds<<" seat HUD crosshairs, "<<draws<<" layout draws passed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
