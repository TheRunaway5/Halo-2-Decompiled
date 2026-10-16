#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_camera.hpp"
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <future>
#include <iostream>
#include <limits>
#include <map>
#include <thread>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-remote-death-XXXXXX").string();
    const auto p=::mkdtemp(name.data());check(p,"Create remote death test directory");path=p;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
}
int main(int argc,char **argv)try {
  check(argc==3,"Supply owned TU1.5 XBE and Xbox map");
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  SDL_SetMainReady();Files files;
  const bool seated_component=std::getenv("HALO2_DEATH_SEATED_COMPONENT");
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
  const UiVariant variant{"Slayer",0,{{89,5},{17,1},{2,0},{10,0}}};
  auto rules=slayer_rules(variant);rules.weapon=cache.slayer_starting_weapon(5);rules.random_seed=0x12345678u;
  const auto fingerprint=multiplayer_fingerprint(argv[2],cache.shared_directory(),0,rules.weapon,argv[1]);
  std::promise<unsigned> port;auto pending=port.get_future();std::atomic<bool> advance{},server_failed{};
  std::exception_ptr server_failure;
  std::jthread server([&](std::stop_token stop) {
    bool published{};
    try {
      // Construct and run every source physics/observer owner on this thread.
      Cache host_cache(argv[2],{},original);const auto host_content=host_cache.load_content(0,true);
      MultiplayerObjectRuntime source(argv[1],host_cache,0,files.path);
      source.configure_session(source_game_variant(variant),1,30,rules.random_seed);
      MultiplayerMatch match(host_content,0,selected,rules,{}, {},&source);match.join("Shooter");
      LanHost host(match,0,fingerprint);port.set_value(host.port());published=true;
      std::array<Controls,multiplayer_capacity> input{};unsigned warmup{};bool killed{},seated{};
      auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        host.receive(input);
        if(advance && match.player(1).connected) {
          ++warmup;
          if(seated_component && warmup==20) {
            const auto snapshot=match.state();std::uint32_t vehicle=null_datum;
            for(const auto &v:snapshot.world_vehicles) {
              const auto definition=host_content.objects.at(v.placement).definition;
              const auto tag=std::find_if(host_cache.tags().begin(),host_cache.tags().end(),[&](const auto &t){return t.datum==definition;});
              if(tag!=host_cache.tags().end() && tag->name=="objects\\vehicles\\ghost\\ghost"){vehicle=v.handle;break;}
            }
            if(vehicle==null_datum)for(unsigned i=0;i<host_content.objects.size();++i) {
              const auto &placement=host_content.objects[i];if(placement.type!=1 || placement.model_attachment)continue;
              const auto tag=std::find_if(host_cache.tags().begin(),host_cache.tags().end(),[&](const auto &t){return t.datum==placement.definition;});
              if(tag!=host_cache.tags().end() && tag->name=="objects\\vehicles\\ghost\\ghost") {
                vehicle=match.create_world_vehicle(i);break;
              }
            }
            check(vehicle!=null_datum,"Remote seated camera component has no actual Ghost");
            auto a=source.tu15_world().prepare_source_call(std::array{vehicle,0u,1u,0x1fu,0u,1u});
            a.gp[0]=match.player(1).source_current_object;source.tu15_world().run_actual(0xec780u,a);seated=true;
          }
          if(seated_component && warmup>=100 && !killed) {
            check(seated && match.state().players[1].source_parent!=null_datum,"Remote fatal cleanup lost its seated source unit");
            // Deliver the original fatal stimulus before the next world pass.
            // Its world owner completesCCF40; eager cleanup here would detach
            // the source player before Match has sampled the completed frame.
            const auto unit=match.player(1).source_current_object;
            source.tu15_world().call_actual(0xd8c20u,std::array{unit,0x60040000u});
            source.tu15_world().call_actual(0xd8d50u,std::array{unit,0x60040000u,1u,0u});killed=true;
          }
          if(!seated_component && warmup>=100 && !killed) {
            const auto &shooter=match.unit(0),&victim=match.unit(1);
            const auto eye=ordinary_unit_eye(shooter.body.position,shooter.definition.camera_height,shooter.definition.crouch_camera_height,shooter.crouch);
            const auto delta=victim.body.position+Vec3{0,0,victim.body.height*.5f}-eye;
            input[0].yaw=std::atan2(delta.y,delta.x);input[0].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));input[0].fire=true;
          }
          match.step(input);killed|=match.player(1).deaths!=0;if(killed)input[0].fire=false;
        }
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){server_failure=std::current_exception();if(!published)port.set_exception(server_failure);server_failed=true;}
  });
  MultiplayerOptions options;options.map=argv[2];options.gameplay_xbe=argv[1];options.variant=variant;
  options.starting_weapon=5;options.width=640;options.height=480;options.mute=true;
  options.join="127.0.0.1:"+std::to_string(pending.get());
  options.validation=std::getenv("HALO2_DEATH_VALIDATION");
  MultiplayerState state;std::uint32_t victim=null_datum;
  bool dead{},corpse{},camera{},board{},respawn{},restored{},retained{},submitted{},quitting{};
  bool seated_received{},seated_rendered{},on_foot_restored{};
  bool look_injected{};
  bool mouse_raw_observed{};
  unsigned respawn_input_checks{};
  std::optional<float> initial_raw_yaw,seated_raw_yaw;
  std::map<std::uint32_t,unsigned> body_draws;
  options.input_observer=[&](unsigned,std::span<const Controls> inputs,const MultiplayerState &snapshot) {
    if(server_failed)std::rethrow_exception(server_failure);
    state=snapshot;advance=true;
    if(state.tick>=450) {
      std::cerr<<"Remote stages: dead="<<dead<<" corpse="<<corpse<<" camera="<<camera<<" board="<<board
        <<" respawn="<<respawn<<" restored="<<restored<<" retained="<<retained<<" submitted="<<submitted
        <<" seated_received="<<seated_received<<" seated_rendered="<<seated_rendered
        <<" on_foot_restored="<<on_foot_restored<<'\n';
      throw FormatError("Remote rendered cycle timed out");
    }
    const auto &p=state.players[1];if(!p.connected)return;
    if(seated_component && !initial_raw_yaw && !inputs.empty())initial_raw_yaw=inputs.front().yaw;
    if(seated_component && look_injected && initial_raw_yaw && !inputs.empty())
      mouse_raw_observed|=inputs.front().yaw!=*initial_raw_yaw;
    if(seated_component && look_injected && !dead && !p.dead && p.source_parent!=null_datum && !inputs.empty())
      seated_raw_yaw=inputs.front().yaw;
    if(seated_component && respawn && seated_raw_yaw && !inputs.empty()) {
      check(inputs.front().yaw==*seated_raw_yaw,"Respawn rebased the native relative-look accumulator");
      ++respawn_input_checks;
    }
    if(seated_component && p.source_parent!=null_datum && !p.dead) {
      check(p.source_vehicle_camera.has_value(),"UDP seated original director was omitted");
      seated_received|=p.source_vehicle_camera->camera_mode==1;
    }
    if(!dead && !p.dead)victim=p.source_unit;
    if(p.dead){dead=true;check(p.source_death_camera.has_value(),"UDP death camera was omitted");}
    if(dead && !p.dead){respawn=true;check(p.source_unit!=null_datum && p.source_unit!=victim && !p.source_death_camera,"Remote respawn retained the old camera or body");}
  };
  options.view_state_observer=[&](unsigned,const MultiplayerState &rendered,const Camera &view) {
    const auto &own=rendered.players[1];
    on_foot_restored|=respawn && !own.dead && !own.source_death_camera && !own.source_vehicle_camera;
    if(seated_component && own.source_vehicle_camera && !own.dead) {
      const auto &expected=*own.source_vehicle_camera;
      check(length(view.position-expected.position)<.0001f && length(view.forward-expected.forward)<.0001f,
        "Remote seated render discarded the original authority camera");seated_rendered=true;
    }
    for(const auto &[unit,count]:body_draws) {
      check(count==1,"Remote death/respawn submitted a duplicate player model");
      const bool living=std::any_of(rendered.players.begin(),rendered.players.end(),[&](const auto &p){
        return p.connected && !p.dead && p.source_unit==unit;
      });
      const bool retained_corpse=std::any_of(rendered.corpses.begin(),rendered.corpses.end(),[&](const auto &p){return p.object==unit;});
      check(living!=retained_corpse,"Remote death/respawn submitted an unowned or both live and corpse model");
    }
    body_draws.clear();
    if(rendered.players[1].source_death_camera) {
      const auto &expected=*rendered.players[1].source_death_camera;
      check(length(view.position-expected.position)<.0001f && length(view.forward-expected.forward)<.0001f,
          "Remote renderer did not submit the host's completed observer view");camera=true;
    }
  };
  options.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &draw) {
    if(!draw.first_person) {
      check(++body_draws[draw.unit]==1,"Remote death/respawn submitted a duplicate player model");
    }
    if(dead && !draw.first_person && draw.unit==victim){check(!draw.vertices.empty() && !draw.body_nodes.empty(),"Remote corpse geometry unavailable");corpse=true;retained|=respawn;}
  };
  options.scoreboard_observer=[&](unsigned,bool normal,const FullScoreboardLayout &layout,const FullScoreboardPaintResult &paint) {
    if(dead && !normal){check(layout.rows.size()==2 && !paint.draw.vertices.empty(),"Remote automatic scoreboard missing");board=true;}
    if(respawn && normal && paint.draw.vertices.empty())restored=true;
  };
  options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
    if(seated_component && !look_injected && state.players[1].source_unit!=null_datum && !state.players[1].dead) {
      auto *window=SDL_GetKeyboardFocus();if(!window)window=SDL_GetMouseFocus();
      if(window) {
        check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Capture native camera mouse input");
        SDL_Event motion{};motion.type=SDL_MOUSEMOTION;motion.motion.windowID=SDL_GetWindowID(window);motion.motion.xrel=137;
        check(SDL_PushEvent(&motion)==1,"Queue native camera mouse input");look_injected=true;
      }
    }
    // The declared fatal component does not publish a credited weapon kill.
    // Automatic scoreboard admission remains with the weapon-hit test.
    submitted|=frame.submitted && corpse && camera && (board || seated_component);
    if(submitted && restored && retained && (!seated_component || (on_foot_restored && respawn_input_checks>=3)) && !quitting){SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Queue remote test completion");quitting=true;}
  };
  const auto result=run_multiplayer(options);server.request_stop();server.join();if(server_failure)std::rethrow_exception(server_failure);
  check(result==0 && dead && corpse && camera && (board || seated_component) && respawn && restored && retained && submitted,
      "Remote source death/camera/scoreboard/respawn GPU acceptance failed");
  if(seated_component)check(seated_received && seated_rendered && on_foot_restored && seated_raw_yaw.has_value() && mouse_raw_observed,"Remote seated camera handoff or native mouse input was not rendered");
  if(seated_component){std::cout<<"PASS UDP seated original director, source fatal cleanup, corpse skin, death camera and salted respawn on GPU\n";return 0;}
  std::cout<<"PASS UDP source fatal hit, replicated corpse WORLD skin, authoritative obstructed camera, automatic scoreboard, salted respawn and HUD restoration on GPU\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
