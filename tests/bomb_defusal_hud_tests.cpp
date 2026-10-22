#include "halo2/hud.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/math.hpp"
#include "halo2/renderer.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool condition,const char *message){if(!condition)throw FormatError(message);}
std::filesystem::path maps(const std::filesystem::path &root) {
  std::ifstream file(root/"assets/mcc-source.tsv");std::string schema,line;
  std::getline(file,schema);std::getline(file,line);
  if(schema!="H2_MCC_ASSET_SOURCE_V1" || !line.starts_with("cache\t"))return {};
  return root/std::filesystem::path(line.substr(6))/"halo2/h2_maps_win64_dx11";
}
struct Frame {
  unsigned width{},height{};std::vector<unsigned char> pixels;
  unsigned red()const {
    unsigned count=0;for(std::size_t i=0;i<pixels.size();i+=3)
      if(pixels[i]>100 && pixels[i]>pixels[i+1]*2 && pixels[i]>pixels[i+2]*2)++count;
    return count;
  }
};
Frame read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary);std::string magic;unsigned maximum;Frame frame;
  file>>magic>>frame.width>>frame.height>>maximum;file.get();
  check(magic=="P6" && maximum==255,"Invalid HUD framebuffer");
  frame.pixels.resize(frame.width*frame.height*3);
  file.read(reinterpret_cast<char*>(frame.pixels.data()),frame.pixels.size());
  check(bool(file),"Truncated HUD framebuffer");return frame;
}
}
int main(int argc,char **argv)try {
  check(argc>=2 && argc<=4,"Supply root and optional --render / capture directory");
  const auto directory=maps(argv[1]);if(directory.empty() || !std::filesystem::is_regular_file(directory/"lockout.map"))return 77;
  const Cache cache(directory/"lockout.map");const auto fonts=gameplay_font_directory(cache);
  Mesh mesh;HudContent content;
  MultiplayerHud hud(content,fonts,mesh,4,"Bomb Defusal",{}, {},0,{},true);
  Mesh weapon_mesh;MultiplayerHud weapon(content,fonts,weapon_mesh,4,"Bomb Defusal");
  MultiplayerState state;state.source_engine=9;state.team_mode=true;
  for(unsigned i=0;i<state.players.size();++i) {
    auto &p=state.players[i];p.connected=true;p.source_player=0x10000+i;p.source_membership=int(i%2);p.health=p.shield=1;
  }
  state.defusal_rules=BombDefusalRules{};state.defusal.emplace();
  auto &bomb=*state.defusal;bomb.phase=BombDefusalPhase::planted;bomb.round=1;bomb.site=0;
  bomb.sites={{0,{10,-3,0}},{1,{10,3,0}}};bomb.bomb_position=bomb.sites[0].position;
  const SourceHudAimCamera camera{{0,0,1},{1,0,0},{0,0,1},1.f,false};
  const auto draw=[&](unsigned remaining,unsigned players=1,unsigned view=0,unsigned width=640,unsigned height=480) {
    bomb.remaining=remaining;
    hud.animate(state,view,view,13,0,state.tick/30.0);
    return hud.draw(state,view,view,players,width,height,13,0,{}, {},camera);
  };
  const auto fill_height=[&](const std::vector<Vertex> &vertices) {
    float height=0;
    for(std::size_t i=0;i<vertices.size();i+=6)if(mesh.materials.at(vertices[i].material).name=="bomb-defusal/marker" &&
        vertices[i].color.y==.025f && vertices[i].color.z==.015f)height+=vertices[i+2].position.y-vertices[i].position.y;
    return height;
  };
  check(fill_height(draw(1350))==0,"Fresh HUD marker contains red fill");
  check(std::abs(fill_height(draw(675))-12)<.001f,"Half fuse did not produce half red fill");
  check(fill_height(draw(1))>23.9f,"Nearly expired HUD marker did not fill red");
  check(weapon.action_text(state,0,0).empty() && weapon.draw(state,0,0,1,640,480,13,0,{}, {},camera).empty(),
      "Weapon HUD duplicated unit-owned objective text/markers");
  for(const auto dimensions:{std::array{640u,480u},std::array{1280u,720u}})
    for(unsigned players:{1u,2u,4u})for(unsigned view=0;view<players;++view) {
      const auto vertices=draw(300,players,view,dimensions[0],dimensions[1]);
      check(!vertices.empty(),"Split-screen bomb markers vanished");
      for(const auto &v:vertices)check(std::isfinite(v.position.x) && std::isfinite(v.position.y),"Invalid split-screen marker vertex");
    }
  state.players[0].dead=true;check(!draw(300).empty(),"Eliminated player lost bomb urgency");state.players[0].dead=false;
  bomb.phase=BombDefusalPhase::ended;
  check(fill_height(draw(300))==0,"Defused/ended round retained red fuse fill");bomb.phase=BombDefusalPhase::planted;
  if(argc>=3) {
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
#endif
    check(std::string_view(argv[2])=="--render","Unknown HUD test option");
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Render capture needs owned test scratch");
    const auto output=argc==4?std::filesystem::path(argv[3]):std::filesystem::path(scratch)/"bomb-hud";
    std::filesystem::create_directories(output);mesh.vertices.resize(3);
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("Bomb Defusal HUD",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    check(window,SDL_GetError());
    {
      Renderer renderer(window,mesh,true);renderer.scene_clear_color({.12f,.16f,.2f});
      const auto capture=[&](unsigned remaining) {
        const auto vertices=draw(remaining);const auto path=output/(std::to_string(remaining)+".ppm");bool drawn=false;
        renderer.update_material_constants(mesh.materials);
        for(unsigned retry=0;retry<4 && !drawn;++retry)drawn=renderer.draw(Camera{},path,{},false,{}, {},vertices);
        check(drawn,"Bomb HUD framebuffer did not render");return read(path);
      };
      const auto fresh=capture(1350),half=capture(675),late=capture(90),bright=capture(83);
      check(fresh.red()<half.red() && half.red()<late.red(),"Actual framebuffer red area did not grow with fuse progress");
      unsigned changes=0;for(std::size_t i=0;i<late.pixels.size();++i)changes+=late.pixels[i]!=bright.pixels[i];
      check(changes>100,"Actual framebuffer bomb pulse did not change brightness");
      std::cout<<"PASS Vulkan framebuffer: growing red fill and final-fuse pulse\n";
    }
    SDL_DestroyWindow(window);SDL_Quit();
  }
  std::cout<<"PASS Bomb Defusal unit HUD, split views, dead player and round reset\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
