#include "halo2/hud.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_runtime_memory.hpp"
#if defined(HALO2_OBJECTIVE_GPU)
#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#endif
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
using namespace halo2;
void check(bool value,const char *message){if(!value)throw FormatError(message);}
namespace {
std::string phase="objective query fixtures";
// Explicit bank stimuli, not gameplay constructors. The independent expected
// bytes come from both unchanged executables, including their palette helper.
SourceHudObjectives retained(const SourceHudObjectiveInput &in,std::span<const SourceHudScoreColor> palette) {
  SourceRuntimeMemory m;
  m.mount(0x400000,std::vector<std::byte>(0x140000),SourceRuntimeMemory::Access::ReadWrite,"HUD query globals stimulus");
  m.mount(0x1000000,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"HUD query owner stimulus");
  const auto w=[&](unsigned at,std::uint32_t value,unsigned bytes=4) {
    auto out=m.mutable_view(at,bytes);for(unsigned k=0;k<bytes;++k)out[k]=std::byte(value>>(k*8));
  };
  constexpr unsigned options=0x1000000,pool=0x1001000,rows=0x1001100,bitmap=0x1003300,engine=0x1003400;
  w(0x4f3218,options);w(options+0x180,in.engine);w(options+0x244,std::uint16_t(in.bomb_seconds),2);
  w(0x4f54f4,pool);w(pool+0x38,16);w(pool+0x24,0x21c);w(pool+0x44,rows);w(pool+0x48,bitmap);
  w(bitmap,in.active_players);w(0x4f63b8,engine);w(engine+0x210,std::uint16_t(in.territory_count),2);
  for(unsigned k=0;k<16;++k) {
    const auto &p=in.players[k];const auto row=rows+k*0x21c;
    w(row,p.datum>>16,2);w(row+0x2c,p.unit);w(row+0xc0,std::uint8_t(p.membership),1);
    w(engine+0x191+k*8,std::uint8_t(p.capturing),1);w(engine+0x192+k*8,p.capture,1);
  }
  for(unsigned k=0;k<8;++k) {
    w(engine+0x15c+k*2,std::uint16_t(in.territory_indices[k]),2);w(engine+0x16c+k*4,in.territory_owners[k]);
  }
  constexpr unsigned matg=0x1004000,tags=0x1004300,mulg=0x1004400,universal=0x1004500,colors=0x1004600;
  w(0x4ecc1c,matg);w(0x4f0414,tags);w(matg+0x16c,palette.empty()?0xffffffffu:1);w(tags+24,mulg);w(mulg+4,universal);
  w(universal+0x10,palette.size());w(universal+0x14,colors);
  for(unsigned k=0;k<palette.size();++k)for(unsigned c=0;c<3;++c)w(colors+k*12+c*4,palette[k][c]);
  constexpr unsigned objects=0x1005000,headers=0x1005100,unit=0x1005200,weapon=0x1005600,definition=0x1005800,
      clock=0x1005c00,bombs=0x1006000;
  w(0x4ecbd0,objects);w(objects+0x38,2);w(objects+0x24,12);w(objects+0x44,headers);
  w(headers,0x7100,2);w(headers+8,unit);w(headers+12,0x7200,2);w(headers+20,weapon);
  w(unit+0x212,in.assault_bomb?0:255,1);w(unit+0x218,0x72000001);w(weapon,2);
  w(weapon+0x17e,std::uint16_t(in.bomb_index),2);w(tags+40,definition);w(definition+0x290,in.assault_bomb,2);
  w(0x51df54,clock);w(clock+4,std::bit_cast<std::uint32_t>(in.tick_seconds));w(0x52d710,bombs);
  if(in.bomb_index>=0) {
    w(bombs+0x170+in.bomb_index,in.bomb_armed?8:0,1);w(bombs+0x1a0+in.bomb_index*2,std::uint16_t(in.bomb_ticks),2);
  }
  return source_hud_objectives_tu15(m,in.player);
}
void query_fixtures() {
  std::ifstream f(HALO2_OBJECTIVE_QUERY_FIXTURE);check(bool(f),"Missing original objective query fixture");
  std::string line;std::getline(f,line);std::getline(f,line);std::istringstream colors(line);
  std::vector<SourceHudScoreColor> palette;
  for(SourceHudScoreColor rgb;colors>>rgb[0]>>rgb[1]>>rgb[2];)palette.push_back(rgb);
  unsigned cases=0,neutral_cases=0;
  while(std::getline(f,line)) {
    std::istringstream row(line);SourceHudObjectiveInput in;unsigned slot{},alive{},active{},value{};
    row>>in.engine>>slot>>alive>>active>>in.territory_count;in.active_players=active;
    in.player=(0x7000u+slot)<<16|slot;
    for(unsigned k=0;k<16;++k) {
      auto &p=in.players[k];int member{},capture{};
      row>>member>>capture>>value;p.membership=member;p.capturing=capture;p.capture=value;
      p.datum=(0x7000u+k)<<16|k;p.unit=alive?0x71000000:0xffffffffu;
    }
    for(auto &i:in.territory_indices)row>>i;
    for(auto &i:in.territory_owners)row>>i;
    row>>in.assault_bomb>>in.bomb_armed>>in.bomb_index>>in.bomb_ticks>>in.bomb_seconds>>value;
    in.tick_seconds=std::bit_cast<float>(value);
    bool missing_palette{};row>>missing_palette;
    SourceHudObjectives expected;row>>expected.charge_visible>>value;expected.charge_fraction=std::bit_cast<float>(value);
    row>>expected.territory_count;
    for(auto &t:expected.territories)row>>t.color;
    for(auto &t:expected.territories){row>>value;t.fraction=std::bit_cast<float>(value);}
    check(bool(row),"Truncated original objective query");
    const auto selected=missing_palette?std::span<const SourceHudScoreColor>{}:std::span<const SourceHudScoreColor>(palette);
    const auto actual=source_hud_objectives(in,selected),bank=retained(in,selected);
    if(in.engine==9 && in.assault_bomb && in.bomb_index==8)++neutral_cases;
    if(actual!=expected || bank!=expected) {
      const auto describe=[](const auto &o){return std::to_string(o.charge_visible)+"/"+
          std::to_string(std::bit_cast<unsigned>(o.charge_fraction))+"/"+std::to_string(o.territory_count);};
      throw FormatError("Objective query differs from unchanged disc/TU1.5 at case "+std::to_string(cases)+
          " expected="+describe(expected)+" native="+describe(actual)+" retained="+describe(bank));
    }
    ++cases;
  }
  check(cases==168 && neutral_cases==8,"Original objective query or neutral-bomb coverage changed");
  std::cout<<cases<<" original query and retained-bank comparisons\n";
}
}
int main(int argc,char **argv)try {
  check(argc>=2 && argc<=4,"Supply maps, optional UI assets and GPU scratch output");
  const std::filesystem::path maps=argv[1],assets=argc>=3?argv[2]:"";
  if(!std::filesystem::is_regular_file(maps/"lockout.map") ||
      !std::filesystem::is_regular_file(HALO2_OBJECTIVE_QUERY_FIXTURE) ||
      !std::filesystem::is_regular_file(HALO2_OBJECTIVE_GEOMETRY_FIXTURE))return 77;
  query_fixtures();phase="authored HUD loading";
  Cache cache(maps/"lockout.map",maps,OriginalData::native_tu15());
  const auto level=cache.load_content(0,true);
  auto content=cache.load_hud(level.player.datum,cache.slayer_starting_weapon());
  std::erase_if(content.widgets,[](const auto &w){return w.name!="bomb_defusal" && w.special!=7;});
  check(content.widgets.size()==2,"Missing original objective widgets");
  Mesh mesh;MultiplayerHud hud(content,gameplay_font_directory(cache),mesh,4,"Territories",{},assets);
  MultiplayerState state;state.source_engine=8;
  auto &player=state.players[0];player.connected=true;player.health=player.shield=1;
  player.source_player=0x70000000;player.source_unit=0x71000000;player.source_flags=1;player.source_membership=0;
  auto &objective=player.source_hud_objectives;
  objective.territory_count=3;
  objective.territories[0]={0xff0000,.25f};objective.territories[1]={0x0000ff,.75f};
  objective.territories[2]={0,.0f};
  phase="objective publication";
  hud.animate(state,0,0,120,0,0);
  check(hud.draw(state,0,0,1,640,480,120,0).size()==18,"Original Territories HUD must submit one meter per active territory");
  objective.charge_visible=true;objective.charge_fraction=.5f;
  hud.animate(state,0,0,120,0,0);
  check(hud.draw(state,0,0,1,640,480,120,0).size()==24,"Original charge meter must draw with Territories capture progress");
  objective.charge_visible=false;objective.territory_count=0;
  check(hud.draw(state,0,0,1,640,480,120,0).empty(),"Empty source status retained stale objective meters");
  player.dead=true;objective.charge_visible=true;objective.territory_count=8;
  check(hud.draw(state,0,0,1,640,480,120,0).empty(),"Dead player retained objective widgets");player.dead=false;
  hud.animate(state,0,0,120,0,0);
  const auto first_view=hud.draw(state,0,0,2,640,480,120,0);
  const auto first_program=mesh.materials.at(first_view.back().material).forward_program.words;
  state.players[1]=player;state.players[1].source_player=0x70010001;state.players[1].source_unit=0x71000001;
  auto &second=state.players[1].source_hud_objectives;second.charge_visible=false;second.territory_count=1;
  second.territories[0]={0x00ff00,.625f};hud.animate(state,1,1,120,0,0);
  const auto second_view=hud.draw(state,1,1,2,640,480,120,0);
  check(second_view.size()==6 && first_view.size()==54 && first_view.back().material!=second_view.back().material &&
      mesh.materials.at(first_view.back().material).forward_program.words==first_program,
      "One local player's objective state overwrote another player's meters");
  state.players[1].connected=false;
  // Compare actual fill/outline and objective vertices with both originals.
  using Key=std::pair<unsigned,std::string>;
  struct Isolated {HudWidget widget;Mesh mesh;std::unique_ptr<MultiplayerHud> hud;};
  std::map<Key,Isolated> isolated;
  phase="species HUD loading";
  const auto *globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
  check(globals && globals->address,"Missing authored HUD representations");
  const Bytes representations(cache.read_tag_bytes(globals->address+0x138,8));
  check(representations.u32(0)>=4,"Missing original Spartan/Elite representation rows");
  for(unsigned elite=0;elite<2;++elite) {
    const Bytes biped(cache.read_tag_bytes(representations.u32(4)+(elite?3:2)*0xbc+0xb4,4));
    auto species=cache.load_hud(biped.u32(0),cache.slayer_starting_weapon());
    for(const auto &w:species.widgets)if(w.name=="bomb_defusal" || w.special==7 || w.name=="shield_meter" ||
        w.name=="shield_mask" || w.name=="motion_tracker_background") {
      if(isolated.contains({elite,w.name}))continue; //First radar row is the sensor-enabled outline.
      auto &i=isolated[{elite,w.name}];i.widget=w;auto one=species;one.widgets={w};
      i.hud=std::make_unique<MultiplayerHud>(std::move(one),gameplay_font_directory(cache),i.mesh,4,"Territories",std::nullopt,assets);
    }
  }
  std::ifstream geometry(HALO2_OBJECTIVE_GEOMETRY_FIXTURE);check(bool(geometry),"Missing objective/shield/radar original geometry");
  unsigned geometry_cases=0;
  for(std::string line;std::getline(geometry,line);) {
    if(line.empty() || line[0]=='#')continue;
    unsigned elite{},wide{},players{},view{},count{},index{};std::string name;std::array<float,4> expected;
    std::istringstream row(line);row>>elite>>wide>>players>>view>>name>>count>>index;
    for(auto &v:expected)row>>v;check(bool(row),"Truncated HUD alignment fixture");
    phase="geometry "+std::to_string(geometry_cases)+" "+name+" species="+std::to_string(elite)+
        " wide="+std::to_string(wide)+" players="+std::to_string(players)+" view="+std::to_string(view);
    auto &i=isolated.at({elite,name});objective.charge_visible=true;objective.territory_count=count;
    for(unsigned k=0;k<8;++k)objective.territories[k]={(k%2?0xffu:0xff0000u),float(k)/7};
    i.hud->animate(state,0,view,120,0,0);
    const unsigned width=wide?1280:640,height=wide?720:480;
    const auto vertices=i.hud->draw(state,0,view,players,width,height,120,0);
    std::array<float,4> bounds{1e6f,1e6f,-1e6f,-1e6f};
    for(unsigned k=index*6;k<index*6+6;++k) {
      const auto p=vertices.at(k).position;bounds[0]=std::min(bounds[0],p.x*width/640);
      bounds[1]=std::min(bounds[1],p.y*height/480);bounds[2]=std::max(bounds[2],p.x*width/640);
      bounds[3]=std::max(bounds[3],p.y*height/480);
    }
    if(wide) {
      const auto v=hud_viewport(players,view,true);const auto anchor=i.widget.anchor;
      const float oldx=(anchor==0 || anchor==3)?v.safe.right:(anchor==1 || anchor==2)?v.safe.left:
          std::floor((v.safe.left+v.safe.right)*.5f);
      const float l=v.outer.left*2+(v.safe.left-v.outer.left)*1.5f,
          r=v.outer.right*2-(v.outer.right-v.safe.right)*1.5f;
      const float newx=(anchor==0 || anchor==3)?r:(anchor==1 || anchor==2)?l:
          std::floor(((l+r)*.5f-160)/1.5f)*1.5f+160;
      for(unsigned k:{0u,2u})expected[k]=newx+(expected[k]-oldx)*1.5f;
      for(unsigned k:{1u,3u})expected[k]*=1.5f;
    }
    for(unsigned k=0;k<4;++k)if(std::abs(bounds[k]-expected[k])>.001f)
      throw FormatError(name+" alignment differs from disc/TU1.5 at geometry "+std::to_string(geometry_cases)+
          " edge "+std::to_string(k)+": "+std::to_string(bounds[k])+" != "+std::to_string(expected[k]));
    ++geometry_cases;
  }
  check(geometry_cases==860,"Objective/shield/radar source geometry coverage changed");
  std::cout<<geometry_cases<<" objective, shield fill/outline and radar outline quads match disc/TU1.5\n";
#if defined(HALO2_OBJECTIVE_GPU)
  if(argc==4) {
    phase="Vulkan objective rendering";
    if(!std::getenv("DISPLAY"))return 77;
    DesktopSession desktop;mesh.vertices.resize(3);Renderer renderer(desktop.window(640,480,true),mesh,true);
    const std::filesystem::path output=argv[3];std::filesystem::create_directories(output);
    auto capture=[&](std::span<const Vertex> vertices,const std::string &name) {
      renderer.update_material_constants(mesh.materials);const auto path=output/(name+".ppm");bool drawn=false;
      for(unsigned retry=0;retry<4 && !drawn;++retry)drawn=renderer.draw(Camera{},path,{},false,{},{},vertices);
      check(drawn,"Objective framebuffer did not render");std::ifstream f(path,std::ios::binary);
      std::string magic;unsigned w{},h{},max{};f>>magic>>w>>h>>max;f.get();
      check(magic=="P6" && w==640 && h==480 && max==255,"Invalid objective framebuffer");
      std::vector<unsigned char> pixels(w*h*3);f.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
      check(bool(f),"Truncated objective framebuffer");return pixels;
    };
    const auto empty=capture({},"empty");
    for(unsigned players=1;players<=4;++players) {
      std::set<unsigned> materials;
      for(unsigned view=0;view<players;++view) {
        objective.charge_visible=true;objective.charge_fraction=.25f;objective.territory_count=8;
        for(unsigned k=0;k<8;++k)objective.territories[k]={k%2?0xffu:0xff0000u,k%2?.75f:.25f};
        hud.animate(state,0,view,120,0,0);const auto vertices=hud.draw(state,0,view,players,640,480,120,0);
        check(vertices.size()==54,"Objective GPU path omitted source meters");
        for(unsigned k=0;k<vertices.size();k+=6)check(materials.insert(vertices[k].material).second,
            "Local views or territories share mutable progress/color");
        const auto a=capture(vertices,"meters-"+std::to_string(players)+"-"+std::to_string(view));
        for(unsigned k=0;k<vertices.size();k+=6) {
          unsigned changed=0,red=0,blue=0;float l=640,t=480,r=0,b=0;
          for(unsigned n=k;n<k+6;++n){const auto p=vertices[n].position;l=std::min(l,p.x);t=std::min(t,p.y);r=std::max(r,p.x);b=std::max(b,p.y);}
          for(int y=std::ceil(t);y<int(b);++y)for(int x=std::ceil(l);x<int(r);++x) {
            const auto at=(y*640+x)*3;changed+=!std::equal(a.begin()+at,a.begin()+at+3,empty.begin()+at);
            red+=a[at]>a[at+2]+8;blue+=a[at+2]>a[at]+8;
          }
          check(changed>0,"An authored objective meter is invisible in Vulkan");
          const bool territory=mesh.materials.at(vertices[k].material).name.ends_with("widget_meter_custom");
          if(territory) {
            const auto ordinal=(k/6)-1; // Authored charge row precedes territory row.
            check(ordinal%2?blue>0 && red==0:red>0 && blue==0,"Territory meter uses another owner's color");
          }
        }
        objective.charge_fraction=.75f;for(auto &t:objective.territories)t.fraction=1-t.fraction;
        hud.animate(state,0,view,120,0,0);
        check(capture(vertices,"progress-"+std::to_string(players)+"-"+std::to_string(view))!=a,
            "Objective shader did not respond to source progress");
      }
    }
    std::cout<<"Vulkan objective colors/progress visible in every1-4 player cell\n";
  }
#endif
  std::cout<<"PASS original objective HUD publication and source alignment\n";return 0;
}catch(const std::exception &e){std::cerr<<phase<<": "<<e.what()<<'\n';return 1;}
