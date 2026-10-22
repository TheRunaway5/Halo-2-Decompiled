#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/source_teleporter.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
float distance2(Vec3 a,Vec3 b){return (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y);}
void key(SDL_Window *window,bool pressed) {
  SDL_Event event{};event.type=pressed?SDL_KEYDOWN:SDL_KEYUP;
  event.key.windowID=SDL_GetWindowID(window);event.key.state=pressed?SDL_PRESSED:SDL_RELEASED;
  event.key.keysym.scancode=SDL_SCANCODE_W;event.key.keysym.sym=SDLK_w;
  check(SDL_PushEvent(&event)==1,"Could not deliver ordinary SDL movement input");
}
}
int main(int argc,char **argv)try {
  if(argc!=3)return 2;
  if(!std::getenv("DISPLAY") || !std::filesystem::is_regular_file(argv[1]))return 77;
  const Cache cache(argv[1]);check(cache.is_mcc(),"Rendered teleporter acceptance requires MCC assets");
  const auto content=cache.load_content(0,true);
  SourceTeleporterBank pairs;source_teleporter_initialize(pairs,content.netgame_flags);
  check(pairs.count>0 && !cache.spawns().empty(),"Map has no authored teleporter/start");
  const auto &entrance=content.netgame_flags.at(pairs.pairs[0].entrance);
  const auto &exit=content.netgame_flags.at(pairs.pairs[0].exit);
  const bool blocked=std::getenv("HALO2_TELEPORTER_BLOCKED_RENDERED")!=nullptr;
  SDL_SetMainReady();MultiplayerOptions options;options.map=argv[1];options.starting_weapon=5;
  options.width=640;options.height=480;options.local_players=blocked?2:1;options.validation=true;
  options.capture_tick=40;options.reference_tick_cadence=true;
  const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Run through tools/run_test.py");
  options.screenshot=std::filesystem::path(scratch)/"teleporter-runtime.png";
  Spawn start=cache.spawns().front();start.position=entrance.position;start.facing=entrance.facing;
  if(!blocked){start.position.x-=1.25f*std::cos(entrance.facing);start.position.y-=1.25f*std::sin(entrance.facing);}
  options.native_spawn_inputs={start};
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &world=*options.prepared_world;const auto slot=world.local_slots.at(0);
  check(world.source_objects && world.source_objects->owns_physics(),"Rendered match lost original source physics");
  unsigned original_requests{},native_packets{},submitted{},moving_ticks{},teleport_tick{},cut_views{};
  std::uint32_t serial{};bool holding{};MultiplayerState::Player latest;
  unsigned waiting_frames{};std::vector<std::uint16_t> waiting_text;
  world.source_objects->observe_sound_submissions([&](bool before,std::uint32_t sound,auto,const auto&,const auto&,auto,auto) {
    if(before)if(const auto *tag=cache.find_tag(TagHandle{sound});tag && tag->name=="sound\\game_sfx\\multiplayer\\teleporter_activate")++original_requests;
  });
  options.audio_observer=[&](const MultiplayerAudioSubmission &packet) {
    if(const auto *tag=cache.find_tag(TagHandle{packet.sound});tag && tag->name=="sound\\game_sfx\\multiplayer\\teleporter_activate")++native_packets;
  };
  options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(640,480,false);
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  options.input_observer=[&](auto tick,std::span<const Controls> input,const MultiplayerState &state) {
    latest=state.players[slot];if(!input.empty() && input[0].forward>.5f)++moving_ticks;
    if(blocked) {
      const auto &waiting=state.players[world.local_slots.at(1)];
      check(!waiting.source_teleport_serial,"Rendered blocked player traversed its occupied exit");
      const auto messages=source_hud_message_lines(waiting.source_messages,state.tick,tick_seconds,1000,1,4);
      if(!messages.empty()) {
        ++waiting_frames;waiting_text.assign(messages.front().text.begin(),messages.front().text.end());
        check(latest.source_messages==SourceHudMessages{},"Blocked caption reached another local viewport");
      }
    } else if(!holding && !serial){key(window,true);holding=true;}
    if(latest.source_teleport_serial!=serial) {
      check(latest.source_teleport_serial==1 && !serial,"Rendered player bounced through its exit");
      serial=latest.source_teleport_serial;teleport_tick=tick;
      check(distance2(latest.position,exit.position)<distance2(latest.position,entrance.position),"Rendered traversal lost the source exit pose");
      key(window,false);holding=false;
    }
  };
  options.view_observer=[&](unsigned view,const Camera &camera,float,std::int16_t) {
    if(view==0 && serial) {
      check(distance2(camera.position,exit.position)<distance2(camera.position,entrance.position),
          "Actual rendered camera retained the entrance or blended through the world");
      ++cut_views;
    }
  };
  options.frame_observer=[&](const MultiplayerFrameTiming &frame){submitted+=frame.submitted;};
  const auto status=run_multiplayer(options);if(holding)key(window,false);
  nlohmann::json report{{"runtime_status",status},{"map",cache.name()},{"moving_ticks",moving_ticks},
      {"teleport_tick",teleport_tick},{"serial",serial},{"submitted_frames",submitted},
      {"exit_camera_views",cut_views},{"original_activation_requests",original_requests},
      {"native_activation_packets",native_packets},{"screenshot_bytes",std::filesystem::is_regular_file(options.screenshot)?std::filesystem::file_size(options.screenshot):0},
      {"blocked",blocked},{"waiting_frames",waiting_frames},{"waiting_utf16",waiting_text},
      {"boundary",blocked?
          "Declared births at the authored entrance; actual occupied-exit query, two local views, original text producer, native renderer/camera and audio. No supplied blocked result or message text; no physical Xbox comparison.":
          "Declared birth behind entrance; ordinary SDL movement, native source world, actual renderer/camera and audio observations. No physical Xbox framebuffer or audio parity claim."}};
  std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
  check(status==0 && serial==1 && (blocked?waiting_frames>5:moving_ticks>1) && submitted>0 && cut_views>0 && original_requests>0,
      "Rendered ordinary teleporter traversal did not complete");
  check(native_packets==1,"Original activation sound did not reach the native SDL queue exactly once");
  check(std::filesystem::is_regular_file(options.screenshot) && std::filesystem::file_size(options.screenshot)>1024,
      "Actual rendered teleporter capture is missing");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
