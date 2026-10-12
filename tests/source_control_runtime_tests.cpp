#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/math.hpp"
#include "halo2/source_portal_mask.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <set>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
void hold(Uint32 window,SDL_Scancode scan,bool down){
  SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;event.key.windowID=window;
  event.key.keysym.scancode=scan;event.key.state=down?SDL_PRESSED:SDL_RELEASED;
  check(SDL_PushEvent(&event)==1,"Queue actual interaction/movement key");
}
void look(Uint32 window,const PcKeyboardSettings &settings,const MultiplayerState::Player &player,
          float yaw,float pitch){
  SDL_SetRelativeMouseMode(SDL_TRUE);SDL_Event event{};event.type=SDL_MOUSEMOTION;event.motion.windowID=window;
  const float gain=.0025f*settings.sensitivity/3.f;
  event.motion.xrel=int(std::lround(std::remainder(player.yaw-yaw,6.283185307f)/gain));
  event.motion.yrel=int(std::lround((player.pitch-pitch)/gain*(settings.invert?-1.f:1.f)));
  check(SDL_PushEvent(&event)==1,"Queue actual interaction/movement look");
}
}
int main(int argc,char **argv){try{
  check(argc==4 || argc==5,"Supply ZANZIBAR_MAP SHARED XBE [CAPTURE]");
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  SDL_SetMainReady();MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.width=1280;options.height=720;options.local_players=1;options.starting_weapon=5;options.mute=true;
  const bool test_audio=std::getenv("HALO2_TEST_CONTROL_AUDIO");
  if(test_audio)options.mute=false;
  if(const auto *views=std::getenv("HALO2_TEST_CONTROL_VIEWS")){
    options.local_players=unsigned(std::stoul(views));
    check(options.local_players==1 || options.local_players==4,"Control probe supports one or four actual local views");
  }
  options.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
  if(const auto *artwork=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))options.ui_assets=artwork;
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &world=*options.prepared_world;const auto slot=world.local_slots.at(0);
  const auto &devices=world.match.simulation().devices();const auto &collision=world.match.simulation().collision();
  std::uint32_t control=null_datum,gate=null_datum;int group=-1;Vec3 computer,approach,gate_low,gate_high;
  for(const auto &[handle,owner]:devices.owners()){
    const auto &definition=*world.content.devices[owner.placement];if(!definition.control)continue;
    check(control==null_datum,"Expected one authored Zanzibar computer");control=handle;group=definition.placement.position_group;
    const auto node=compose_transform(owner.root,definition.control_nodes.at(0));
    computer=source_shadow_sphere(definition.control_bounds,node,owner.root.scale).center;
    approach=computer+node.basis[0]*.6f;
  }
  check(control!=null_datum && group>=0,"Authored Zanzibar computer/shared group missing");
  for(const auto &[handle,owner]:devices.owners()){
    const auto &definition=*world.content.devices[owner.placement];
    if(definition.machine && definition.placement.position_group==group){
      check(gate==null_datum,"Expected one gate in Zanzibar computer group");gate=handle;
      gate_low={1e9f,1e9f,1e9f};gate_high={-1e9f,-1e9f,-1e9f};bool found=false;
      for(const auto &triangle:devices.triangles())if(triangle.source.instance==int(owner.placement))
        for(const auto vertex:{triangle.a,triangle.b,triangle.c}){
          found=true;gate_low={std::min(gate_low.x,vertex.x),std::min(gate_low.y,vertex.y),std::min(gate_low.z,vertex.z)};
          gate_high={std::max(gate_high.x,vertex.x),std::max(gate_high.y,vertex.y),std::max(gate_high.z,vertex.z)};
        }
      check(found,"Authored closed gate collision missing");
    }
  }
  check(gate!=null_datum,"Authored computer has no linked gate");
  // Each authored impulse owns one native voice, even if that voice queues
  // several PCM packets. Split-screen must not submit one per viewport.
  std::map<std::pair<std::uint32_t,std::uint32_t>,std::set<std::uint64_t>> device_voices;
  for(const auto &[handle,index]:std::array<std::pair<std::uint32_t,unsigned>,3>{{{control,0},{gate,0},{gate,2}}}){
    const auto *owner=devices.find(handle);
    const auto tag=world.content.devices[owner->placement]->motion.effects[index];
    check(tag!=null_datum,"Authored Zanzibar device impulse missing");
    device_voices[{handle,tag}];
  }
  if(test_audio)options.audio_observer=[&](const MultiplayerAudioSubmission &submission){
    const auto found=device_voices.find({submission.owner,submission.sound});
    if(found==device_voices.end())return;
    check(submission.kind==MultiplayerAudioKind::Impulse && submission.voice && submission.frames,
          "Device impulse did not submit actual SDL PCM");
    if(found->second.insert(submission.voice).second)
      std::cout<<"Authored device PCM owner="<<submission.owner<<" tag="<<submission.sound
               <<" voice="<<submission.voice<<" frames="<<submission.frames<<'\n';
  };
  const auto portal_definition=world.cache->load_source_portal_mask_definition(0);
  // The supplied Zanzibar BSP has NO door-portal associations. Gemini owns
  // the six authored associations in the 21-map Xbox set. Opening this gate
  // must not manufacture a portal bit/acoustic update.
  check(portal_definition.associations.empty() && portal_definition.portal_indices.empty(),
        "Literal Zanzibar door-portal association bank changed");
  SourcePortalMaskState initial_portals;initial_portals.reset_acoustic();initial_portals.initialize(portal_definition,{});
  check(devices.groups()[unsigned(group)].value==0 && devices.groups()[unsigned(group)].flags==1,
        "Expected authored closed one-use group");
  const auto floor=collision.raycast(approach+Vec3{0,0,2},{0,0,-1},5);
  check(floor && floor->normal.z>.9f,"No actual floor in front of Zanzibar computer");approach.z=floor->position.z+.01f;
  const auto gate_center=(gate_low+gate_high)*.5f;
  const Vec3 walk_direction=(gate_high.x-gate_low.x<gate_high.y-gate_low.y)?Vec3{1,0,0}:Vec3{0,1,0};
  Vec3 walk_start=gate_center-walk_direction;walk_start.z=gate_low.z+.7f;
  const auto walk_floor=collision.raycast(walk_start,{0,0,-1},2);
  check(walk_floor && walk_floor->normal.z>.9f,"No actual floor on the gate test route");walk_start.z=walk_floor->position.z+.01f;
  const auto &biped=world.match.unit(slot).definition;
  check(!collision.overlaps(walk_start,biped.height,biped.radius),"Gate test route starts inside collision");
  const auto walk_yaw=std::atan2(walk_direction.y,walk_direction.x);
  const auto ray=computer-ordinary_unit_eye(approach,biped.camera_height,biped.crouch_camera_height,0);
  const auto computer_yaw=std::atan2(ray.y,ray.x),computer_pitch=std::atan2(ray.z,std::hypot(ray.x,ray.y));
  options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(options.width,options.height,false);
  std::cout<<"Computer test SDL driver="<<SDL_GetCurrentVideoDriver()<<'\n';
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);const auto window_id=SDL_GetWindowID(window);
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  enum class Phase{start,closed_walk,computer_place,computer_prompt,computer_hold,motion,repeat_hold,open_walk,done};
  Phase phase=Phase::start;unsigned phase_tick{},changed_tick{},partial_ticks{},group_changes{};
  bool closed_blocked{},prompt_seen{},held_action{},open_traversed{},lost_focus{};
  bool closed_portals{},open_portals{};
  float prior_target=0,closed_progress=-1,open_progress=-1;std::uint16_t held_milliseconds{};
  const auto place=[&](Vec3 position,float yaw,float pitch,const MultiplayerState::Player &player){
    // Diagnostic placement only. Selection, the held action, movement, group
    // mutation, machine clocks and collision all run through normal gameplay.
    Spawn at;at.position=position;at.bsp=0;at.facing=yaw;world.match.debug_request({DebugGameRequest::Teleport,slot},&at);
    look(window_id,options.desktop->keyboard_settings,player,yaw,pitch);
  };
  const auto begin=[&](Phase next,unsigned tick){phase=next;phase_tick=tick;};
  options.input_observer=[&](unsigned tick,std::span<const Controls> controls,const MultiplayerState &state){
    const auto &player=state.players[slot];const auto &bank=world.match.simulation().devices();const auto *machine=bank.find(gate);
    check(machine,"Live gate generation disappeared");const auto &target=bank.groups()[unsigned(group)];
    if(target.value!=prior_target){++group_changes;prior_target=target.value;}
    if(changed_tick){
      check(target.value==1 && target.flags==3,"One-use shared group did not retain its changed state");
      check(machine->motion.position>=0 && machine->motion.position<=1,"Gate left authored channel bounds");
      partial_ticks+=machine->motion.position>0 && machine->motion.position<1;
    }
    lost_focus|=phase!=Phase::start && phase!=Phase::done && SDL_GetKeyboardFocus()!=window;
    check(!player.dead,"Player died during live computer/gate test");
    switch(phase){
    case Phase::start:
      // A newly mapped X11 window can reject the first focus request. Retry
      // the actual window request only before the gameplay sequence starts;
      // retain the later lost-focus check and never synthesize focus events.
      if(SDL_GetKeyboardFocus()!=window && tick%30==0){
        SDL_RaiseWindow(window);const auto result=SDL_SetWindowInputFocus(window);
        std::cout<<"Initial computer window focus retry tick="<<tick<<" result="<<result<<'\n';
      }
      if(tick>=30 && SDL_GetKeyboardFocus()==window){
        place(walk_start,walk_yaw,0,player);hold(window_id,SDL_SCANCODE_W,true);begin(Phase::closed_walk,tick);
        std::cout<<"Closed gate route start="<<walk_start.x<<','<<walk_start.y<<','<<walk_start.z<<'\n';
      }break;
    case Phase::closed_walk:
      closed_progress=dot(player.position-gate_center,walk_direction);
      if(tick>=phase_tick+65){
        hold(window_id,SDL_SCANCODE_W,false);
        check(closed_progress>-.95f && closed_progress<-biped.radius*.8f,"Closed gate did not block actual SDL walking");
        check(target.value==0 && machine->motion.position==0,"Closed-walk probe changed the gate");closed_blocked=true;
        std::cout<<"Closed gate blocked SDL walking: signed distance="<<closed_progress<<'\n';
        begin(Phase::computer_place,tick);
      }break;
    case Phase::computer_place:
      if(controls[0].forward==0){
        place(approach,computer_yaw,computer_pitch,player);begin(Phase::computer_prompt,tick);
      }break;
    case Phase::computer_prompt:
      if(player.action_prompt.target.object==control){
        check(player.action_prompt.target.action==3 && player.action_prompt.message!=0,"Computer selected the wrong action/message");
        prompt_seen=true;hold(window_id,SDL_SCANCODE_E,true);begin(Phase::computer_hold,tick);
        std::cout<<"Actual computer prompt tick="<<tick<<" handle="<<control<<" message="<<player.action_prompt.message<<'\n';
      }break;
    case Phase::computer_hold:
      if((tick-phase_tick)%30==0)std::cout<<"Computer hold tick="<<tick<<" pickup="<<controls[0].pickup<<" ms="<<controls[0].pickup_milliseconds<<" prompt="<<player.action_prompt.target.object<<" focus="<<(SDL_GetKeyboardFocus()==window)<<'\n';
      held_milliseconds=std::max(held_milliseconds,controls[0].pickup_milliseconds);
      if(target.value==1){
        held_action=controls[0].pickup;check(held_action,"Computer changed without the normal held SDL action");
        check(target.flags==3,"Computer did not consume authored shared one-use group");
        check(player.action_prompt.target.object!=control,"Used one-use computer retained its prompt");
        changed_tick=tick;hold(window_id,SDL_SCANCODE_E,false);begin(Phase::motion,tick);
        std::cout<<"Actual held E changed shared group at tick="<<tick<<" hold_ms="<<held_milliseconds<<'\n';
      }break;
    case Phase::motion:
      if(machine->motion.position==1 && machine->motion.position_velocity==0){
        check(partial_ticks>1,"Gate teleported instead of advancing its simulation controller");
        hold(window_id,SDL_SCANCODE_E,true);begin(Phase::repeat_hold,tick);
        std::cout<<"Gate reached authored open endpoint tick="<<tick<<" partial_ticks="<<partial_ticks<<'\n';
      }break;
    case Phase::repeat_hold:
      check(player.action_prompt.target.object!=control,"Repeat hold readmitted a used one-use computer");
      if(tick>=phase_tick+30){
        hold(window_id,SDL_SCANCODE_E,false);place(walk_start,walk_yaw,0,player);
        hold(window_id,SDL_SCANCODE_W,true);begin(Phase::open_walk,tick);
      }break;
    case Phase::open_walk:
      open_progress=dot(player.position-gate_center,walk_direction);
      if(open_progress>biped.radius+.2f){
        hold(window_id,SDL_SCANCODE_W,false);open_traversed=true;begin(Phase::done,tick);
        if(argc==5)look(window_id,options.desktop->keyboard_settings,player,walk_yaw+3.141592654f,0);
        std::cout<<"Opened gate traversed by SDL walking tick="<<tick<<" signed distance="<<open_progress<<'\n';
        if(argc==4){SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Finish computer runtime test");}
      }break;
    case Phase::done:break;
    }
    check(tick<120 || phase!=Phase::start,"Computer test window never received focus");
    check(phase==Phase::done || tick<phase_tick+240,"Live computer/gate phase timed out");
    if(argc==5 && tick==options.capture_tick)check(phase==Phase::done,"Capture arrived before gate traversal completed");
  };
  options.world_portal_observer=[&](unsigned,const SourcePortalMaskState &mask){
    if(phase==Phase::start)return;
    const auto position=world.match.simulation().devices().find(gate)->motion.position;
    check(mask.words==initial_portals.words && mask.acoustic==initial_portals.acoustic,
          "Zanzibar gate manufactured a portal/acoustic update without authored associations");
    if(!changed_tick)closed_portals=true;
    else if(position>0)open_portals=true;
  };
  if(argc==5){options.screenshot=argv[4];options.capture_tick=450;}
  check(run_multiplayer(options)==0,"Computer runtime failed");
  check(closed_blocked && prompt_seen && held_action && open_traversed,"Live computer/gate acceptance sequence incomplete");
  check(group_changes==1 && !lost_focus,"Live computer lost one-use state or input focus");
  check(closed_portals && open_portals,"Live portal mask was not observed before/after gate activation");
  if(test_audio)for(const auto &[identity,voices]:device_voices)
    check(voices.size()==1,"Authored computer/gate sound absent or duplicated in native SDL playback");
  std::cout<<"PASS live Zanzibar: actual SDL closed obstruction, computer prompt/hold, one-use repeat rejection, "
           <<partial_ticks<<" intermediate machine ticks, authored unchanged portal mask and open collision traversal; closed="<<closed_progress
           <<" open="<<open_progress<<" hold_ms="<<held_milliseconds<<'\n';
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
