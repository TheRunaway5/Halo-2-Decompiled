#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_camera.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-death-render-XXXXXX").string();
    const auto p=::mkdtemp(name.data());check(p,"Create render test directory");path=p;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
}
int main(int argc,char **argv)try {
  check(argc==4,"Supply owned TU1.5 XBE, Xbox map and output screenshot");
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  SDL_SetMainReady();Files files;
  const auto original=OriginalData::load_xbe(argv[1]);Cache cache(argv[2],{},original);
  const auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);
  std::array<Spawn,2> selected{};float closest=std::numeric_limits<float>::infinity();
  for(const auto &a:cache.spawns())for(const auto &b:cache.spawns()) {
    const auto distance=length(b.position-a.position);if(distance<2 || distance>8 || distance>=closest)continue;
    const auto from=a.position+Vec3{0,0,content.player.camera_height};
    const auto delta=b.position+Vec3{0,0,content.player.camera_height}-from;
    if(collision.raycast(from,delta/length(delta),length(delta)))continue;
    selected={a,b};closest=distance;
  }
  check(std::isfinite(closest),"No clear pair of authored starts");
  const UiVariant variant{"Slayer",0,{{89,5},{17,5},{2,0},{10,0}}};
  constexpr std::uint32_t seed=0x12345678u;
  auto rules=slayer_rules(variant);rules.weapon=cache.slayer_starting_weapon(5);rules.random_seed=seed;
  const auto script=files.path/"input.txt";std::ofstream input_file(script);input_file<<std::setprecision(9);
  unsigned death_tick{};
  {
    std::filesystem::create_directories(files.path/"producer");
    MultiplayerObjectRuntime source(argv[1],cache,0,files.path/"producer");
    source.configure_session(source_game_variant(variant),1,30,seed);
    MultiplayerMatch match(content,0,selected,rules,{}, {},&source);match.join("Shooter");match.join("Victim");
    std::array<Controls,multiplayer_capacity> input{};
    for(unsigned tick=0;tick<100;++tick)match.step(input);
    for(unsigned tick=100;tick<340 && !match.unit(1).vitality.dead;++tick) {
      const auto &shooter=match.unit(0),&victim=match.unit(1);
      const auto eye=ordinary_unit_eye(shooter.body.position,shooter.definition.camera_height,
          shooter.definition.crouch_camera_height,shooter.crouch);
      const auto delta=victim.body.position+Vec3{0,0,victim.body.height*.5f}-eye;
      input[0].yaw=std::atan2(delta.y,delta.x);input[0].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
      input[0].fire=true;
      input_file<<tick<<" 0 1 0 0 "<<input[0].yaw<<' '<<input[0].pitch<<'\n';
      match.step(input);
    }
    check(match.unit(1).vitality.dead && match.player(0).kills==1,"Control producer did not deliver the real fatal hit");
    death_tick=match.state().tick;
  }
  input_file.close();
  MultiplayerOptions options;options.map=argv[2];options.gameplay_xbe=argv[1];
  options.variant=variant;options.starting_weapon=5;options.session_random_seed=seed;
  options.local_players=2;options.native_spawn_inputs.assign(selected.begin(),selected.end());
  options.width=960;options.height=720;options.mute=true;options.input_script=script;
  // Ordinary TU1.5 launch: no development source-world switch or directory.
  options.capture_tick=death_tick+220;options.screenshot=argv[3];
  options.reference_tick_cadence=true;options.reference_auxiliary_seed=seed;
  options.validation=std::getenv("HALO2_DEATH_VALIDATION");
  MultiplayerState state;std::uint32_t victim=null_datum;bool dead{},corpse_drawn{},camera_moved{},automatic{},submitted{};
  Vec3 ordinary_eye{};bool respawn{},hud_restored{},corpse_after_respawn{};
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState &snapshot) {
    state=snapshot;const auto &player=state.players[1];
    if(!player.dead && !dead)victim=player.source_unit;
    else if(!player.dead) {
      check(player.source_unit!=null_datum && player.source_unit!=victim,"Rendered respawn reused its corpse");respawn=true;
    }
    else {
      dead=true;check(player.source_previous_unit==victim && player.deaths==1 && state.players[0].kills==1,
          "Rendered input playback did not preserve the fatal owner");
      ordinary_eye=ordinary_unit_eye(player.position,content.player.camera_height,content.player.crouch_camera_height,player.crouch);
    }
  };
  options.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &draw) {
    if(dead && !draw.first_person && draw.unit==victim) {
      check(!draw.vertices.empty() && !draw.body_nodes.empty(),"Corpse draw omitted the actual skin or WORLD nodes");
      for(const auto &vertex:draw.vertices)
        check(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y) && std::isfinite(vertex.position.z),"Corpse skin is non-finite");
      corpse_drawn=true;
      corpse_after_respawn|=respawn;
    }
  };
  options.view_observer=[&](unsigned view,const Camera &camera,float,std::int16_t) {
    if(view==1 && dead)camera_moved|=length(camera.position-ordinary_eye)>.01f;
  };
  options.scoreboard_observer=[&](unsigned view,bool normal,const FullScoreboardLayout &layout,const FullScoreboardPaintResult &paint) {
    if(view==1 && dead && !normal) {
      check(layout.rows.size()==2 && !paint.draw.vertices.empty(),"Death scoreboard did not reach actual UI submission");automatic=true;
    }
    if(view==1 && respawn && normal && paint.draw.vertices.empty())hud_restored=true;
  };
  options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
    if(dead && corpse_drawn && camera_moved && automatic && frame.submitted)submitted=true;
  };
  const auto result=run_multiplayer(options);
  std::cout<<"Rendered gates dead="<<dead<<" corpse="<<corpse_drawn<<" camera="<<camera_moved
      <<" scoreboard="<<automatic<<" submitted="<<submitted<<" respawn="<<respawn
      <<" restored="<<hud_restored<<" retained="<<corpse_after_respawn<<'\n';
  check(result==0 && dead && corpse_drawn && camera_moved && automatic && submitted &&
      respawn && hud_restored && corpse_after_respawn,
      "Rendered source corpse/camera/automatic-scoreboard acceptance failed");
  check(std::filesystem::file_size(argv[3])>1000,"Death screenshot was not written");
  std::cout<<"PASS actual authored-spawn fatal shot, source corpse skin/WORLD nodes, death camera, automatic scoreboard, respawn, HUD restoration and retained GPU corpse\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
