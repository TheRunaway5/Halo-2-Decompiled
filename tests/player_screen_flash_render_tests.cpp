#include "halo2/renderer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/render_view.hpp"
#include "scalar_combiners.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
struct Image {unsigned width{},height{};std::vector<unsigned char> bytes;};
Image read(const std::filesystem::path &path) {
  std::ifstream in(path,std::ios::binary);Image out;std::string magic;unsigned maximum;
  in>>magic>>out.width>>out.height>>maximum;in.get();
  check(magic=="P6" && maximum==255,"Invalid flash test capture");
  out.bytes.resize(out.width*out.height*3);in.read(reinterpret_cast<char *>(out.bytes.data()),out.bytes.size());
  check(bool(in),"Truncated flash test capture");return out;
}
void transport() {
  // Explicit native arena/seat inputs exercise the real response producer and
  // UDP codec. This is not an Xbox collision or vehicle-entry parity claim.
  LevelContent content;auto &b=content.player;b.datum=1;b.height=1;b.crouch_height=.6f;b.radius=.2f;
  b.camera_height=.8f;b.crouch_camera_height=.5f;b.crouch_velocity=2;b.jump_velocity=3;b.vitality.health=100;
  content.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},{{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
  content.movement={2,2,2,10,1,1,1,5,2,1};WeaponDefinition w;w.datum=3;w.instantaneous=true;
  w.capacity=20;w.initial_rounds=120;w.reserve=100;w.reload_rounds=20;w.reload_time=1;w.rounds_per_shot=1;
  w.minimum_rounds=1;w.rounds_per_second=30;w.burst=1;w.range=50;w.damage_min=4000;w.projectiles_per_shot=1;
  w.damage=78;
  w.firing_effects.push_back({1,1,{null_datum,null_datum,null_datum},{77,null_datum,null_datum}});
  content.weapons.emplace(3,w);content.multiplayer_starting_equipment_flags=2;
  std::array<Spawn,2> spawns;spawns[0].player_type=spawns[1].player_type=0;spawns[1].position={4,0,0};
  auto rules=slayer_rules({"Slayer",0,{}});rules.weapon=3;MultiplayerMatch match(content,0,spawns,rules);
  LanHost host(match,0,123);LanClient client(host.join_address(),123,1);
  std::array<Controls,1> input{};std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned i=0;i<100 && !client.ready();++i) {
    client.send(input);host.receive(controls);host.broadcast();client.receive();std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  check(client.ready(),"Player response UDP handshake failed");
  const auto player=client.slots().front(),victim=match.join("Flash victim");
  const auto delta=match.unit(victim).body.position+Vec3{0,0,.5f}-match.unit(player).body.position-Vec3{0,0,.8f};
  input[0].yaw=std::atan2(delta.y,delta.x);input[0].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
  input[0].fire=true;client.send(input);host.receive(controls);
  const_cast<Unit &>(match.unit(player)).source_seat=3;
  const_cast<Unit &>(match.unit(victim)).source_seat=2;match.step(controls);
  const_cast<Unit &>(match.unit(player)).source_seat=-1;
  const_cast<Unit &>(match.unit(victim)).source_seat=-1;host.broadcast();client.receive();
  check(!client.state().rumble_events.empty(),"Real firing response did not reach UDP client");
  const auto authority=match.state();bool fired=false,damaged=false;
  for(const auto &row:client.state().rumble_events) {
    const auto expected=std::find_if(authority.rumble_events.begin(),authority.rumble_events.end(),
        [&](const auto &e){return e.serial==row.serial;});
    check(expected!=authority.rumble_events.end() && row.event.flash_scale==expected->event.flash_scale &&
        row.event.seated==expected->event.seated,"UDP response scale/seat differs from its authoritative event");
    if(row.event.definition==77) {
      check(row.event.seated && row.event.flash_scale==1 && row.event.owner==int(player)-1,
          "Firing response lost its event-time seat/scale/owner in UDP transport");fired=true;
    } else if(row.event.definition==78) {
      check(row.event.owner==int(victim)-1 && row.event.seated && !row.event.shielded,
          "Admitted damage response lost its actual victim/shield/seat selection");damaged=true;
    }
  }
  check(fired && damaged,"Real native firing/damage did not produce both screen response journal records");
}
}
int main(int argc,char **argv)try {
  if(argc!=2)return 2;
  transport();
  const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
  check(scratch && *scratch,"Player flash rendering requires tools/run_test.py scratch ownership");
  const auto output=std::filesystem::path(scratch)/"player-screen-flash";std::filesystem::create_directories(output);
  SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
  auto *window=SDL_CreateWindow("Player screen flash",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN);
  check(window,SDL_GetError());
  std::ostringstream diagnostics;auto *previous=std::cerr.rdbuf(diagnostics.rdbuf());
  try {
    Mesh mesh;mesh.materials.emplace_back();mesh.materials.back().diffuse={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};Camera camera;
    Renderer renderer(window,mesh,true);renderer.scene_clear_color({.2f,.3f,.4f});
    const auto capture=[&](const char *name,ScreenEffects effects,std::span<const ModelView> views={},std::span<const Vertex> ui={}) {
      const auto path=output/(std::string(name)+".ppm");
      for(unsigned attempt=0;attempt<4;++attempt)
        if(renderer.draw(camera,path,{},false,{},effects,ui,views))return read(path);
      throw FormatError("Flash test did not submit its frame");
    };
    ModelView root;root.camera=camera;root.full_surface=true;root.width=640;root.height=480;
    const auto baseline=capture("baseline",{},{&root,1});
    ScreenEffects effect;effect.flash={1,.3f,{1,1,0,0}};
    const auto red=capture("red",effect,{&root,1});
    const auto pixel=[](const Image &im,unsigned x,unsigned y,unsigned c){return im.bytes[(y*im.width+x)*3+c];};
    check(pixel(red,320,240,0)>pixel(baseline,320,240,0)+20 &&
        pixel(red,320,240,2)<pixel(baseline,320,240,2)-10,
        "TU1.5 full-screen body-hit flash was not rendered");
    const std::array<Vertex,6> hud{{{{80,80,1},{1,1,1},{0,0},0},{{200,80,1},{1,1,1},{1,0},0},{{200,200,1},{1,1,1},{1,1},0},
        {{80,80,1},{1,1,1},{0,0},0},{{200,200,1},{1,1,1},{1,1},0},{{80,200,1},{1,1,1},{0,1},0}}};
    const auto hud_baseline=capture("hud-baseline",{},{&root,1},hud);
    const auto hud_hit=capture("hud-hit",effect,{&root,1},hud);
    check(pixel(hud_hit,120,120,1)<pixel(hud_baseline,120,120,1)-40,
        "Player flash was drawn before the HUD instead of over it");
    auto pause=hud;
    for(auto &v:pause)v.position.x+=200;
    std::vector<Vertex> ordered(hud.begin(),hud.end());ordered.insert(ordered.end(),pause.begin(),pause.end());
    effect.flash_after_ui_vertex=hud.size();
    const auto pause_baseline=capture("pause-baseline",{},{&root,1},ordered);
    const auto pause_hit=capture("pause-hit",effect,{&root,1},ordered);
    check(pixel(pause_hit,120,120,1)<pixel(pause_baseline,120,120,1)-40 &&
        pixel(pause_hit,320,120,1)==pixel(pause_baseline,320,120,1),
        "Player flash did not stay between the game HUD and external pause UI");
    effect.flash_after_ui_vertex.reset();
    std::array<ModelView,2> views;
    for(unsigned i=0;i<2;++i){auto &v=views[i];v.camera=camera;v.full_surface=true;v.left=i*320;v.width=320;v.height=480;}
    const auto split_baseline=capture("split-baseline",{},views);
    views[0].screen_flash={1,.3f,{1,0,.6f,.8f}};
    const auto split=capture("split",{},views);
    check(pixel(split,160,240,2)>pixel(split_baseline,160,240,2)+10,
        "TU1.5 local shield-hit flash did not reach its view");
    for(unsigned y=0;y<480;++y)for(unsigned x=320;x<640;++x)for(unsigned c=0;c<3;++c)
      check(pixel(split,x,y,c)==pixel(split_baseline,x,y,c),"A local flash leaked into another split-screen view");
    for(unsigned count:{3,4}) {
      std::vector<ModelView> grid(count);
      for(unsigned i=0;i<count;++i) {
        auto &v=grid[i];v.camera=camera;v.full_surface=true;v.left=(i%2)*320;v.top=(i/2)*240;v.width=320;v.height=240;
      }
      const auto before=capture(count==3?"three-before":"four-before",{},grid);
      grid.front().screen_flash={1,.3f,{1,0,.6f,.8f}};grid.back().screen_flash={1,.3f,{1,1,0,0}};
      const auto after=capture(count==3?"three-hit":"four-hit",{},grid);
      check(pixel(after,160,120,2)>pixel(before,160,120,2)+10 &&
          pixel(after,unsigned(grid.back().left)+160,360,0)>pixel(before,unsigned(grid.back().left)+160,360,0)+20,
          "A three/four-player flash did not reach its own viewport");
      for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x) {
        const unsigned cell=(y/240)*2+x/320;
        if(cell==0 || cell==count-1)continue;
        for(unsigned c=0;c<3;++c)check(pixel(after,x,y,c)==pixel(before,x,y,c),
            "A three/four-player flash leaked outside its own viewport");
      }
    }
    effect.fade_color={};effect.fade=1;
    const auto faded=capture("fade",effect);
    check(pixel(faded,320,240,0)==0 && pixel(faded,320,240,1)==0 && pixel(faded,320,240,2)==0,
        "Damage flash escaped the campaign's opaque fade");
    unsigned compositions{};std::uint64_t compared{};
    for(const auto extent:std::array{std::array{640u,480u},std::array{1280u,720u},
        std::array{1920u,1080u},std::array{3840u,2160u},std::array{1279u,719u}}) {
      SDL_SetWindowSize(window,int(extent[0]),int(extent[1]));SDL_PumpEvents();renderer.resized();
      for(unsigned count:{1u,2u,4u})for(bool inset:{false,true}) {
        std::vector<ModelView> local(count);
        const unsigned columns=count==1?1:2,rows=count==4?2:1;
        for(unsigned i=0;i<count;++i) {
          auto& v=local[i];v.camera=camera;v.full_surface=true;
          v.left=float(i%columns)*640/columns;v.top=float(i/columns)*480/rows;
          v.width=640.f/columns;v.height=480.f/rows;
          if(inset) {
            const auto r=full_surface_viewport({v.left,v.top,v.width,v.height},extent[0],extent[1]);
            const auto l=int(r.left)+int(r.width*3/8),t=int(r.top)+int(r.height/4);
            v.source_ui_viewport=std::array<std::int32_t,4>{t,l,t+int(r.height/2),l+int(r.width/2)};
          }
        }
        const auto before=capture("resolution-before",{},local);
        check(before.width==extent[0] && before.height==extent[1],"Flash retained a stale drawable resolution");
        const auto& v=local.back();auto r=full_surface_viewport({v.left,v.top,v.width,v.height},extent[0],extent[1]);
        if(v.source_ui_viewport) {
          const auto& p=*v.source_ui_viewport;r={float(p[1]),float(p[0]),float(p[3]-p[1]),float(p[2]-p[0])};
        }
        for(int type:{1,2,5}) {
          // TU1.5 409A2 uses ONE/INVSRCALPHA/ADD.409DE instead selects
          // ONE/ONE/REVERSE_SUBTRACT: DARKEN is DST-C in this executable.
          // Retain these source states rather than the later jpt! XML equation.
          const PlayerScreenFlash flash{type,.35f,{1,.25f,.5f,.75f}};
          local.back().screen_flash=flash;const auto after=capture("resolution-flash",{},local);
          const auto packed=player_screen_flash_argb(flash);const double alpha=double(packed>>24)/255;
          // Complete literal40880 case5 bank, evaluated independently of the
          // production shader. General-stage C0 and fixed final R0 are source words.
          std::array<std::uint32_t,60> program{};
          program[8]=0xcu;program[9]=0x1c00u;program[10]=packed;program[26]=0xc00u;
          program[34]=0x01411120u;program[45]=0xc00u;program[53]=1;
          const auto invert=halo2_test::scalar_combiners(program,{});
          for(unsigned y=0;y<before.height;++y)for(unsigned x=0;x<before.width;++x) {
            const bool inside=x>=unsigned(r.left) && x<unsigned(r.left+r.width) &&
                y>=unsigned(r.top) && y<unsigned(r.top+r.height);
            for(unsigned c=0;c<3;++c) {
              const auto n=(std::size_t(y)*before.width+x)*3+c;
              const double color=double((packed>>(16-c*8))&255);
              const double result=type==5?before.bytes[n]*color/255.0+(255-before.bytes[n])*invert[c]:
                  before.bytes[n]*(type==1?1-alpha:1)+(type==1?color:-color);
              const auto expected=inside?int(std::lround(std::clamp(result,0.0,255.0))):int(before.bytes[n]);
              check(std::abs(int(after.bytes[n])-expected)<=(inside?1:0),
                  "Source flash blend, last pixel edge or explicit local viewport differs");++compared;
            }
          }
          ++compositions;
        }
        local.back().screen_flash={};const auto reset=capture("resolution-reset",{},local);
        check(reset.bytes==before.bytes,"Flash retained another frame's local state after reset");
      }
    }
    check(diagnostics.str().find("Validation Error")==std::string::npos,"Vulkan flash validation failed");
    std::cerr.rdbuf(previous);std::cout<<"PASS "<<compositions<<" LIGHTEN/DARKEN/INVERT resolution/viewport compositions and "<<compared
      <<" channels; full-screen red / local blue / HUD-pause ordering / two-three-four-view isolation / opaque campaign fade on "<<renderer.device_name()<<'\n';
  }catch(...){std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();throw;}
  SDL_DestroyWindow(window);SDL_Quit();
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
