#include "halo2/hud.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
struct Sample {std::array<float,22> input;bool visible;Vec2 anchor;};
struct Layout {
  unsigned players,view,width,height;
  std::optional<TwoPlayerSplit> split;
  std::array<float,8> rectangles;
};
// Original 2F600/2F6B0 rectangles supplied to the unchanged projection oracle.
const std::array layouts{
  Layout{1,0,640,480,{}, {0,0,640,480,48,36,544,408}},
  Layout{1,0,1280,720,{}, {0,0,1280,720,96,54,1088,612}},
  Layout{2,0,640,480,TwoPlayerSplit::Horizontal,{0,0,640,240,48,36,544,200}},
  Layout{4,3,640,480,{}, {320,240,320,240,324,244,268,200}},
  Layout{2,0,1280,720,TwoPlayerSplit::Vertical,{0,0,640,720,96,54,536,612}},
  Layout{2,1,1280,720,TwoPlayerSplit::Vertical,{640,0,640,720,648,54,536,612}},
  Layout{4,3,1280,720,{}, {640,360,640,360,648,366,536,300}}};
}
int main(int argc,char **argv)try {
  check(argc==3,"Supply original maps and frozen original target-anchor fixtures");
  const std::filesystem::path maps=argv[1];if(!std::filesystem::exists(maps/"shared.map"))return 77;
  Cache cache(maps/"shared.map");
  const auto unit=std::ranges::find_if(cache.tags(),[](const auto &t){return t.group=="bipd" && t.name.ends_with("\\masterchief_mp");});
  const auto weapon=std::ranges::find_if(cache.tags(),[](const auto &t){return t.group=="weap" && t.name.find("\\rocket_launcher\\")!=std::string::npos;});
  check(unit!=cache.tags().end() && weapon!=cache.tags().end(),"Original rocket HUD/biped unavailable");
  auto content=cache.load_hud(unit->datum,weapon->datum);content.action_messages.enabled=false;
  std::erase_if(content.widgets,[](const auto &w){return w.anchor!=5 || w.weapon_type!=1 || w.is_text;});
  check(!content.widgets.empty(),"Original rocket HUD has no target-owned widgets");
  std::ifstream file(argv[2]);check(bool(file),"Frozen original target anchors unavailable");
  std::vector<Sample> samples;
  for(std::string line;std::getline(file,line);) {
    if(line.empty() || line[0]=='#')continue;
    Sample sample;std::istringstream row(line);
    for(auto &v:sample.input)row>>v;row>>sample.visible>>sample.anchor.x>>sample.anchor.y;
    check(bool(row),"Malformed original target anchor");if(!sample.input[10])samples.push_back(sample);
  }
  unsigned widgets{},draws{},wide{},split{};
  for(const auto &widget:content.widgets) {
    auto isolated=content;isolated.widgets={widget};Mesh mesh,expected_mesh;
    MultiplayerHud hud(isolated,maps/"fonts",mesh,4,"Slayer",cache.original_data()->hud_bitmap_geometry_constants);
    // Use the original oracle's pixel anchor with the same authored bitmap
    // drawing primitive. This bypasses target projection in the expected draw
    // and checks absolute positions, including split-view center offsets.
    isolated.widgets[0].anchor=4;
    MultiplayerHud expected(isolated,maps/"fonts",expected_mesh,4,"Slayer",cache.original_data()->hud_bitmap_geometry_constants);
    bool admitted=false;
    for(auto mode:{std::uint8_t(0),std::uint8_t(5),std::uint8_t(6)}) {
      for(auto mask:{std::uint16_t(0x40),std::uint16_t(0x80),std::uint16_t(0x100)}) {
        MultiplayerState state;auto &p=state.players[0];p.connected=true;p.health=p.shield=1;
        p.weapon=weapon->datum;p.source_weapon=0x10000;p.loaded=2;p.reserve=8;
        p.source_hud_zoom_mask=mask;p.weapon_trigger.mode=mode;p.weapon_trigger.target_acquired=true;
        bool combination=false;
        for(const auto &sample:samples) {
          const auto &v=sample.input;
          const auto layout=std::ranges::find_if(layouts,[&](const auto &l){return std::equal(l.rectangles.begin(),l.rectangles.end(),v.begin()+11);});
          check(layout!=layouts.end(),"Original target fixture has no declared native layout");
          p.weapon_trigger.target_position=std::array{v[19],v[20],v[21]};
          const auto vertices=hud.draw(state,0,layout->view,layout->players,layout->width,layout->height,0,0,{},layout->split,
              SourceHudAimCamera{{v[0],v[1],v[2]},{v[3],v[4],v[5]},{v[6],v[7],v[8]},v[9],false});
          if(!sample.visible){check(vertices.empty(),"HUD drew an original rejected target");continue;}
          const auto reference=expected.draw(state,0,layout->view,layout->players,layout->width,layout->height,0,0,sample.anchor,layout->split);
          if(reference.empty())continue;combination=true;
          check(vertices.size()==reference.size(),"Target projection lost authored HUD geometry");
          for(unsigned n=0;n<vertices.size();++n)
            check(std::abs(vertices[n].position.x-reference[n].position.x)<.0003f &&
                std::abs(vertices[n].position.y-reference[n].position.y)<.0003f,
                "Authored target HUD differs from the original camera viewport projection");
          ++draws;wide+=layout->width*3>layout->height*4;split+=layout->players>1;
        }
        if(combination){admitted=true;break;}
      }
      if(admitted)break;
    }
    check(admitted,"Original rocket target widget never admitted");++widgets;
  }
  check(widgets && wide && split,"Target HUD omitted its widescreen/split camera viewport");
  std::cout<<widgets<<" authored rocket target widgets, "<<draws<<" frozen original absolute projection draws ("<<wide<<" wide, "<<split<<" split) passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
