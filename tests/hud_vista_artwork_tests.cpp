#include "halo2/content.hpp"
#include "halo2/hud.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void check(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)return 2;
    const auto maps=std::filesystem::path(argv[1]),vista=std::filesystem::path(argv[2]);
    if(!std::filesystem::exists(maps/"lockout.map")||!std::filesystem::exists(vista/"hud.tsv"))return 77;
    Cache cache(maps/"lockout.map");const auto level=cache.load_content(0,true);
    unsigned fills=0,outlines=0;
    for(unsigned representation:{2u,3u}) {
      auto content=cache.load_hud(level.player_representations.at(representation).biped.datum,cache.slayer_starting_weapon());
      std::erase_if(content.widgets,[](const auto &w){return w.name!="shield_meter"&&w.name!="shield_mask";});
      check(content.widgets.size()==2,"Missing shield layer pair");
      Mesh old_mesh,hd_mesh;
      MultiplayerHud old_hud(content,maps/"fonts",old_mesh,4),hd_hud(content,maps/"fonts",hd_mesh,4,"Slayer",{},vista);
      MultiplayerState state;auto &player=state.players[0];player.connected=true;player.shield=player.health=1;
      if(representation==3){player.source_configuration=SourcePlayerConfiguration{};(*player.source_configuration)[0x44]=1;}
      for(unsigned view=0;view<4;++view){old_hud.animate(state,0,view,25,14400,0);hd_hud.animate(state,0,view,25,14400,0);}
      for(unsigned width:{640u,1280u})for(unsigned players=1;players<=4;++players)for(unsigned view=0;view<players;++view) {
        const auto old=old_hud.draw(state,0,view,players,width,width==640?480:720,25,14400);
        const auto hd=hd_hud.draw(state,0,view,players,width,width==640?480:720,25,14400);
        check(old.size()==hd.size() && old.size()==12,"Shield layer pair count changed");
        for(unsigned n=0;n<old.size();++n) {
          check(old[n].position.x==hd[n].position.x && old[n].position.y==hd[n].position.y,
                "Vista artwork moves the current shield layout");
          const bool fill=hd_mesh.materials.at(hd[n].material).name.ends_with("shield_test");
          // Independently read 11081 shared.map nhdt records: full Spartan
          // meter-mask=(0,-1), Elite=(-1,-2), versus Xbox=(0,+1)/(0,0).
          // Vista pages have twice the density. Split records have delta zero.
          const float du=fill&&players==1&&representation==3?1.f/256:0;
          const float dv=!fill||players!=1?0:representation==3?1.f/32:1.5f/32;
          check(std::abs(hd[n].uv.x-old[n].uv.x-du)<1e-6f &&
                std::abs(hd[n].uv.y-old[n].uv.y-dv)<1e-6f,
                "Vista shield fill ignores authored placement relative to its outline");
          fill?++fills:++outlines;
        }
      }
    }
    check(fills>100 && outlines>100,"Shield artwork regression did not cover split layouts");
    std::cout<<fills<<" fill and "<<outlines<<" outline vertices preserve current layouts with authored Vista alignment\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
