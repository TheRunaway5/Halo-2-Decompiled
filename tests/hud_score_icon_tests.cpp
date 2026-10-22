#define SDL_MAIN_HANDLED
#include "halo2/hud.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/original_data.hpp"
#if defined(HALO2_SCORE_ICONS_GPU)
#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#endif
#include "halo2/native_player_profile.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
MultiplayerState roster() {
  MultiplayerState state;state.source_engine=2;
  for(unsigned slot:{0u,3u,15u}) {
    auto &p=state.players[slot];p.connected=true;p.health=p.shield=1;
    p.source_player=(0x7000u+slot)<<16|slot;p.source_flags=1;
    p.source_membership=slot==0?0:1;p.score=slot==0?7:slot==3?9:4;
    const std::array<std::uint8_t,9> appearance{
        std::uint8_t(slot==0?1:4),2,3,5,0,std::uint8_t(slot==0?63:slot==3?17:8),
        std::uint8_t(slot==0?31:slot==3?12:6),std::uint8_t(slot==0?0:3),0};
    p.source_configuration=native_selected_player_configuration("Score icon",appearance,-1);
  }
  state.team_scores[0]=7;state.team_scores[1]=9;return state;
}
#if defined(HALO2_SCORE_ICONS_GPU)
std::vector<unsigned char> pixels(const std::filesystem::path &path,unsigned width,unsigned height) {
  std::ifstream file(path,std::ios::binary);std::string magic;unsigned w{},h{},max{};
  file>>magic>>w>>h>>max;file.get();
  check(magic=="P6" && w==width && h==height && max==255,"Invalid score icon framebuffer");
  std::vector<unsigned char> out(w*h*3);file.read(reinterpret_cast<char*>(out.data()),out.size());
  check(bool(file),"Incomplete score icon framebuffer");return out;
}
#endif
}
int main(int argc,char **argv)try {
  check(argc>=2 && argc<=4,"Supply maps, optional UI assets, optional GPU output directory");
  const std::filesystem::path maps=argv[1],assets=argc>=3?argv[2]:"";
  if(!std::filesystem::is_regular_file(maps/"lockout.map"))return 77;
  Cache cache(maps/"lockout.map",maps,OriginalData::native_tu15());const auto level=cache.load_content(0,true);
  auto content=cache.load_hud(level.player.datum,cache.slayer_starting_weapon());
  std::erase_if(content.widgets,[](const auto &w){return w.is_text || (w.special!=1 && w.special!=2);});
  check(content.widgets.size()==4,"Original score icon widget bank changed");
  Mesh mesh;MultiplayerHud hud(content,gameplay_font_directory(cache),mesh,4,"Slayer",{},assets);
  auto state=roster();
  auto draw=[&](unsigned slot,unsigned view,unsigned players,unsigned width=640,unsigned height=480) {
    hud.animate(state,slot,view,25,14400,state.tick/30.0);
    return hud.draw(state,slot,view,players,width,height,25,14400);
  };
  auto inspect=[&](const std::vector<Vertex> &v,bool teams,unsigned own,unsigned rival) {
    check(v.size()==12,"Bottom-right HUD must submit both own and rival score icons");
    const auto &a=mesh.materials.at(v[0].material),&b=mesh.materials.at(v[6].material);
    check(a.ui_emblem_pass==!teams && b.ui_emblem_pass==!teams,
          "FFA emblems and team score symbols use the wrong authored pass");
    check(v[0].material!=v[6].material,"Own and rival score icons share mutable material state");
    if(!teams) {
      for(const auto pair:{std::pair{&a,own},std::pair{&b,rival}}) {
        const auto &profile=*state.players[pair.second].source_configuration;
        if(pair.first->texture_frames[0]!=profile[0x46] || pair.first->texture_frames[1]!=profile[0x45])
          throw FormatError("Score icon selected background/foreground "+std::to_string(pair.first->texture_frames[0])+"/"+
              std::to_string(pair.first->texture_frames[1])+", expected "+std::to_string(profile[0x46])+"/"+
              std::to_string(profile[0x45])+" for player "+std::to_string(pair.second));
      }
    }
  };
  unsigned cases=0;
  for(bool teams:{false,true}) {
    state.team_mode=teams;
    for(const auto size:{std::array{640u,480u},std::array{1280u,720u}})
      for(unsigned players=1;players<=4;++players) {
        std::set<unsigned> materials;
        for(unsigned view=0;view<players;++view) {
          const auto slot=view%2?3u:0u;
          const auto vertices=draw(slot,view,players,size[0],size[1]);
          inspect(vertices,teams,slot,slot==0?3:0);
          check(materials.insert(vertices[0].material).second,"Split-screen score icons share appearance state");
          ++cases;
        }
      }
  }
  state.team_mode=false;
  const auto first=draw(0,0,1);const auto other_view_before=mesh.materials.at(draw(3,1,2)[0].material).forward_program.words;
  state.players[15].score=12;
  inspect(draw(0,0,1),false,0,15);
  check(mesh.materials.at(draw(3,1,2)[0].material).forward_program.words==other_view_before,
        "Changing the other viewport's rival changed this player's emblem");
  state.players[15].source_flags|=2;inspect(draw(0,0,1),false,0,3);
  state.players[15].connected=false;state.players[3].connected=false;
  check(draw(0,0,1).size()==6,"A departed FFA rival left a stale score emblem");
  state.team_mode=true;check(draw(0,0,1).size()==6,"An absent rival team left a stale score symbol");
  state.players[0].dead=true;check(draw(0,0,1).empty(),"Dead local player retained live score icons");
  state=roster();state.players[0].source_configuration.reset();
  check(draw(0,0,1).size()==12,"Default source appearance removed the FFA icon");
  ++cases;
  std::ifstream fixture(HALO2_SCORE_ICON_GEOMETRY_FIXTURE);
  check(bool(fixture),"Missing original score-icon geometry fixture");
  unsigned geometry_cases=0;
  for(std::string line;std::getline(fixture,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);unsigned winning{},wide{},players{},view{};
    std::string name;std::array<float,4> expected;
    row>>winning>>wide>>players>>view>>name;
    for(auto &value:expected)row>>value;
    check(bool(row),"Truncated original score-icon geometry");
    state=roster();state.team_mode=name.ends_with("_teams");
    state.players[0].score=state.team_scores[0]=winning?12:7;
    hud.reset_lifetimes();state.tick=30;
    const unsigned width=wide?1280:640,height=wide?720:480;
    draw(0,view,players,width,height);state.tick+=30;
    const auto vertices=draw(0,view,players,width,height);
    const auto first=name.starts_with("other_")?6u:0u;
    std::array<float,4> bounds{640,480,0,0};
    for(unsigned k=first;k<first+6;++k) {
      const auto p=vertices.at(k).position;
      bounds[0]=std::min(bounds[0],p.x);bounds[1]=std::min(bounds[1],p.y);
      bounds[2]=std::max(bounds[2],p.x);bounds[3]=std::max(bounds[3],p.y);
    }
    if(wide) {
      const auto v=hud_viewport(players,view,true);
      const auto shifted=v.outer.right*2-(v.outer.right-v.safe.right)*1.5f;
      for(unsigned k:{0u,2u})expected[k]=(shifted+(expected[k]-v.safe.right)*1.5f)*.5f;
    }
    for(unsigned k=0;k<4;++k)check(std::abs(bounds[k]-expected[k])<.001f,
        "Score icon position differs from original22AFF1 geometry");
    ++geometry_cases;
  }
  check(geometry_cases==160,"Original score-icon layout coverage changed");
#if defined(HALO2_SCORE_ICONS_GPU)
  if(argc==4) {
    if(!std::getenv("DISPLAY"))return 77;
    SDL_SetMainReady();mesh.vertices.resize(3);
    // Keep the same UI composition path and opaque background in every
    // capture, so a frame-wide background change cannot count as icon ink.
    Texture background;background.width=background.height=1;background.format=11;
    background.pixels={std::byte{32},std::byte{32},std::byte{32},std::byte{255}};
    const auto background_material=unsigned(mesh.materials.size());
    Material backing{"score-icon-test-background",background};backing.ui_full_surface=true;
    mesh.materials.push_back(std::move(backing));
    const Vertex a{{0,0,1},{1,1,1},{0,0},background_material},
        b{{640,0,1},{1,1,1},{1,0},background_material},
        c{{0,480,1},{1,1,1},{0,1},background_material},
        d{{640,480,1},{1,1,1},{1,1},background_material};
    const std::array backdrop{a,b,c,b,d,c};
    const std::filesystem::path output=argv[3];std::filesystem::create_directories(output);
    for(const auto size:{std::array{640u,480u},std::array{1280u,720u},std::array{2560u,1080u}}) {
      DesktopSession desktop;const auto width=size[0],height=size[1];
      Renderer renderer(desktop.window(width,height,true),mesh,true);
      const auto prefix=std::to_string(width)+"x"+std::to_string(height)+"-";
      auto capture=[&](const std::vector<Vertex> &v,const std::string &name) {
        renderer.update_material_constants(mesh.materials);
        const auto path=output/(prefix+name+".ppm");
        std::vector<Vertex> submitted(backdrop.begin(),backdrop.end());
        submitted.insert(submitted.end(),v.begin(),v.end());
        for(unsigned retry=0;retry<4;++retry)
          if(renderer.draw(Camera{},path,{},false,{},{},submitted))return pixels(path,width,height);
        throw FormatError("Score icon GPU capture did not complete");
      };
      const auto empty=capture({},"empty");state=roster();
      for(unsigned players=1;players<=4;++players)for(bool teams:{false,true}) {
        state.team_mode=teams;
        auto draw_views=[&] {
          std::vector<Vertex> vertices;
          for(unsigned view=0;view<players;++view) {
            const auto part=draw(0,view,players,width,height);
            vertices.insert(vertices.end(),part.begin(),part.end());
          }
          return vertices;
        };
        const auto vertices=draw_views();
        const auto name=std::to_string(players)+(teams?"-teams":"-ffa");
        const auto actual=capture(vertices,name);
        for(unsigned first=0;first<vertices.size();first+=6) {
          float l=float(width),t=float(height),r=0,b=0;
          for(unsigned k=first;k<first+6;++k) {
            const auto p=vertices[k].position;
            l=std::min(l,p.x*width/640);t=std::min(t,p.y*height/480);
            r=std::max(r,p.x*width/640);b=std::max(b,p.y*height/480);
          }
          unsigned changed=0;
          unsigned red=0,blue=0;
          for(int y=std::max(0,int(t));y<std::min(int(height),int(b)+1);++y)
            for(int x=std::max(0,int(l));x<std::min(int(width),int(r)+1);++x) {
              const auto p=(y*width+x)*3;
              changed+=!std::equal(actual.begin()+p,actual.begin()+p+3,empty.begin()+p);
              red+=actual[p]>actual[p+2]+20;
              blue+=actual[p+2]>actual[p]+20;
            }
          if(!changed) {
            unsigned actual_left=width,actual_right=0;
            for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
              const auto p=(y*width+x)*3;
              if(!std::equal(actual.begin()+p,actual.begin()+p+3,empty.begin()+p)) {
                actual_left=std::min(actual_left,x);actual_right=std::max(actual_right,x);
              }
            }
            throw FormatError(prefix+name+" icon="+std::to_string(first/6)+
                " has no pixels in its submitted HUD rectangle; expected x="+
                std::to_string(l)+".."+std::to_string(r)+", all icon pixels x="+
                std::to_string(actual_left)+".."+std::to_string(actual_right));
          }
          if(teams)check(first%12==0?red>0 && blue==0:blue>0 && red==0,
              "Team score symbol has the wrong team's authored color");
          std::cout<<prefix<<name<<" icon="<<first/6<<" changed_pixels="<<changed<<'\n';
        }
        // Profile customization changes FFA pixels; team symbols retain the
        // exact same shape and team color across profile changes.
        auto &a=*state.players[0].source_configuration;a[0x45]=8;a[0x46]=6;a[0x47]=1;
        const auto customized=capture(draw_views(),name+"-customized");
        check((actual==customized)==teams,"FFA/team icon rendering ignored its correct appearance owner");
        state=roster();
      }
    }
  }
#endif
  std::cout<<"PASS "<<cases<<" score icon layout/view cases, "<<geometry_cases
      <<" original quads, saved/default appearances, rival changes and team/FFA handoff\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
