#define SDL_MAIN_HANDLED
#include "halo2/frontend.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::vector<unsigned char> read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary);std::string type;unsigned w,h,max;
  file>>type>>w>>h>>max;file.get();check(type=="P6"&&w==640&&h==480&&max==255,"Invalid native capture");
  std::vector<unsigned char> pixels(w*h*3);file.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
  check(bool(file),"Incomplete native capture");return pixels;
}
int main(int argc,char **argv)try {
  check(argc==3 || (argc==4 && std::string(argv[3])=="--mcc"),"Supply repository and output directory [--mcc]");
  const std::filesystem::path root=argv[1],out=argv[2],maps=root/"assets/disc/maps";
  if(!std::getenv("DISPLAY") || !std::filesystem::is_regular_file(maps/"mainmenu.map") ||
      !std::filesystem::is_regular_file(root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"))return 77;
  const auto menu_assets=argc==4?root/"assets/mcc-classic-menus":std::filesystem::path{};
  const auto hud_assets=argc==4?root/"assets/mcc-classic-hud":std::filesystem::path{};
  if(argc==4 && (!std::filesystem::is_regular_file(menu_assets/"menu-source.tsv") ||
      !std::filesystem::is_regular_file(hud_assets/"scoreboard.tsv")))return 77;
  std::filesystem::create_directories(out);SDL_SetMainReady();
  Cache menu_cache(maps/"mainmenu.map");auto content=menu_cache.load_frontend();
  Mesh mesh;FrontendPainter painter(content,maps/"fonts",mesh,{},
      load_frontend_ui_assets(content,menu_assets),menu_assets.empty()?menu_assets:menu_assets/"fonts");
  Cache world(maps/"lockout.map",maps,OriginalData::load_xbe(root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"));
  MultiplayerScoreboard board(world,maps/"fonts",mesh,UiVariant{},hud_assets);
  const auto preferences=out/"selected-profiles.cfg";
  std::filesystem::remove(preferences);MenuState state(content,preferences);
  const std::array<const char*,3> names{"First Emblem","Second Emblem","Third Emblem"};
  auto choose=[&](unsigned row,unsigned screen){state.select(row);state.accept();check(state.frame().screen==screen,"Actual profile/lobby route changed");};
  for(unsigned i=0;i<3;++i) {
    state.accept();choose(0,167);state.text(names[i]);state.commit_text();
    choose(1,47);choose(1,50);state.select(2+i);state.accept();
    choose(5,49);state.select(7+i);if(i&1)state.auxiliary();state.accept();
    choose(6,233);state.select(11+i);state.accept();state.back();state.back();
    state.back();choose(0,9);
  }
  state.accept();choose(1,6);check(state.controller_slot(100,true)==0,"Host controller lost");choose(2,30);
  for(unsigned i=1;i<3;++i) {
    check(state.controller_slot(100+i,true)==i,"Local controller lost");
    const auto &entries=state.frame().entries;
    const auto row=std::find_if(entries.begin(),entries.end(),[&](const auto &e){return e.label==names[i];});
    check(row!=entries.end(),"Saved selected profile missing");choose(unsigned(row-entries.begin()),30);
  }
  choose(0,14);
  DesktopSession desktop;Renderer renderer(desktop.window(640,480,true),mesh,true);
  auto capture=[&](const std::vector<Vertex> &vertices,const std::filesystem::path &path) {
    renderer.update_material_constants(mesh.materials);
    for(unsigned attempt=0;attempt<4;++attempt)
      if(renderer.draw(Camera{},path,{},false,{},{},vertices))return read(path);
    throw FormatError("Native GPU capture did not complete");
  };
  auto inspect=[&](const std::string &name,const std::vector<Vertex> &vertices,bool scoreboard) {
    std::vector<Vertex> suppressed;std::vector<unsigned> icons;
    for(unsigned first=0;first<vertices.size();first+=6) {
      const auto &m=mesh.materials.at(vertices[first].material);
      const bool icon=scoreboard?(m.name.starts_with("scoreboard/original/") ||
          m.name.starts_with("scoreboard/vista/")):m.ui_emblem_pass;
      if(icon)icons.push_back(first);
      else suppressed.insert(suppressed.end(),vertices.begin()+first,vertices.begin()+first+6);
    }
    check(icons.size()==3,"Actual UI route did not submit three player icons");
    const auto actual=capture(vertices,out/(name+".ppm")),absent=capture(suppressed,out/(name+"-suppressed.ppm"));
    std::set<unsigned> materials;
    for(auto first:icons) {
      const auto &a=vertices[first],&b=vertices[first+4];
      check(materials.insert(a.material).second,"GPU UI rows share an emblem material");
      unsigned changed=0;
      for(int y=std::max(0,int(a.position.y));y<std::min(480,int(b.position.y)+1);++y)
        for(int x=std::max(0,int(a.position.x));x<std::min(640,int(b.position.x)+1);++x) {
          const auto p=(y*640+x)*3;
          changed+=!std::equal(actual.begin()+p,actual.begin()+p+3,absent.begin()+p);
        }
      check(changed>0,"Submitted player icon produced no framebuffer pixels");
      std::cout<<name<<" material="<<a.material<<" bounds="<<a.position.x<<","<<a.position.y<<","<<b.position.x<<","<<b.position.y<<" changed_pixels="<<changed<<'\n';
    }
  };
  const auto lobby=painter.draw(state,5);inspect("lobby",lobby.vertices,false);
  MultiplayerState match;const auto players=state.local_players();
  for(unsigned i=0;i<3;++i) {
    auto &p=match.players[i];p.connected=true;p.name=players[i].name;p.source_player=0x10000+i;
    p.source_unit=i==1?null_datum:0x20000+i;p.source_flags=1;p.source_membership=i;
    const auto a=players[i].appearance;
    const std::array<std::uint8_t,9> bytes{std::uint8_t(a.colors[0]),std::uint8_t(a.colors[1]),std::uint8_t(a.colors[2]),std::uint8_t(a.colors[3]),std::uint8_t(a.species),a.foreground,a.background,a.flags,a.reserved};
    p.source_configuration=native_selected_player_configuration(p.name,bytes,-1);
  }
  const std::array<unsigned,3> ids{0,1,2};for(unsigned tick=0;tick<8;++tick)board.step(0,true);
  const auto held=board.draw(match,ids,0,640,480);check(!held.paint.emblems_unavailable,"Held icons unavailable");
  inspect("scoreboard-held",held.paint.draw.vertices,true);
  match.finished=match.results_finalized=true;
  const auto final=board.draw(match,ids,0,640,480);check(!final.paint.emblems_unavailable,"Final icons unavailable");
  inspect("scoreboard-final",final.paint.draw.vertices,true);
  inspect("lobby-after-scoreboard",painter.draw(state,5).vertices,false);
  check(read(out/"lobby.ppm")==read(out/"lobby-after-scoreboard.ppm"),"Scoreboard changed the lobby's retained emblem banks");
  std::cout<<"PASS native Vulkan pixels for three distinct saved profiles, lobby, held/death/final scoreboard, and return to lobby\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
