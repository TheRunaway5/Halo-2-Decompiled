#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
void key(Uint32 window,SDL_Scancode scan) {
  SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=window;e.key.keysym.scancode=scan;e.key.state=SDL_PRESSED;
  check(SDL_PushEvent(&e)==1,"Queue weapon key press");e.type=SDL_KEYUP;e.key.state=SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue weapon key release");
}
void hold_key(Uint32 window,SDL_Scancode scan,bool down) {
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;e.key.keysym.scancode=scan;
  e.key.state=down?SDL_PRESSED:SDL_RELEASED;check(SDL_PushEvent(&e)==1,"Queue held interaction key");
}
void trigger(Uint32 window,Uint8 button,bool down) {
  SDL_Event e{};e.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;e.button.windowID=window;
  e.button.button=button;e.button.state=down?SDL_PRESSED:SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue weapon trigger");
}
}
int main(int argc,char **argv){try {
  check(argc==4 || argc==5,"Supply MAP SHARED XBE [CAPTURE]");
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);
  SDL_SetMainReady();MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.width=800;options.height=600;options.local_players=1;options.starting_weapon=5;options.mute=false;options.input_instances={-1};
  options.frames=argc==5?0u:320u;options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
  options.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
  if(const auto *artwork=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))options.ui_assets=artwork;
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  const auto &world=*options.prepared_world;
  const NetgameEquipment *row=nullptr,*primary_row=nullptr;
  for(const auto &r:world.content.netgame_equipment)
    if(!r.classification && source_netgame_equipment_selector(r,true,world.rules.source_engine)>=0 &&
        r.items.size()==1) {
      if(!row && r.items[0].definition==world.rules.weapon)row=&r;
      const auto w=world.content.weapons.find(r.items[0].definition);
      if(!primary_row && w!=world.content.weapons.end() && w->second.name.find("battle_rifle")!=std::string::npos)
        primary_row=&r;
    }
  check(row && primary_row,"Actual map has no SMG/battle-rifle pickup rows");
  // Only initial diagnostic placement is supplied. Item allocation, transfer,
  // clocks, trigger/reload/grenade outcomes and poses come from the live loop.
  const auto location=row->position;const auto slot=world.local_slots.at(0);
  options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(800,600,false);
  std::cout<<"Native SDL video driver: "<<SDL_GetCurrentVideoDriver()<<'\n';
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  std::optional<unsigned> start,carried_at,dual_at,grenade_at;MultiplayerState current;unsigned step{},draws{};
  std::uint32_t primary_target=null_datum,starting_datum=null_datum;
  bool map_pickup{},switched{},dual_requested{};
  bool raised{},dual{},right_fired{},left_fired{},reloaded{},thrown{},dropped{};
  bool offhand_radar{},primary_radar{},grenade_radar_phase{};
  bool reload_received{},lost_focus{};
  unsigned firing_particles{},firing_sounds{},fired_rounds{};
  std::optional<std::array<int,2>> previous_ammunition;
  const auto firing_sound=world.content.weapons.at(world.rules.weapon).sound;
  std::set<std::uint32_t> posed_weapons;std::set<unsigned> clips;
  options.input_observer=[&](unsigned tick,std::span<const Controls> controls,const MultiplayerState &state){
    step=tick;current=state;const auto &p=state.players[slot];
    reload_received|=controls[0].reload;
    lost_focus|=start.has_value() && SDL_GetKeyboardFocus()!=window;
    const auto primary_item=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &item) {
      const auto d=item.position-primary_row->position;
      return item.definition==primary_row->items[0].definition && d.x*d.x+d.y*d.y+d.z*d.z<1e-8f;
    });
    if(!start && primary_item!=state.world_weapons.end() && SDL_GetKeyboardFocus()==window) {
      start=tick;primary_target=primary_item->handle;starting_datum=p.source_weapon;
      Spawn at;at.position=primary_row->position;at.position.x-=.2f;at.bsp=0;at.facing=0;
      options.prepared_world->match.debug_request({DebugGameRequest::Teleport,slot},&at);hold_key(window_id,SDL_SCANCODE_E,true);
      std::cout<<"Actual map battle rifle available at tick="<<tick<<'\n';
    }
    check(tick<120 || start,"Weapon window never received actual keyboard focus");
    if(start && !map_pickup && p.source_weapon==primary_target) {
      check(p.weapon==primary_row->items[0].definition,"Map pickup changed its authored definition");
      check(std::none_of(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &item){return item.handle==primary_target;}),
          "Held map pickup still has a world owner");
      map_pickup=true;carried_at=tick;hold_key(window_id,SDL_SCANCODE_E,false);
    }
    if(carried_at && tick==*carried_at+10)key(window_id,SDL_SCANCODE_TAB);
    if(carried_at && tick>*carried_at+10 && !dual_requested && p.source_weapon==starting_datum) {
      switched=true;dual_requested=true;
      Spawn at;at.position=location;at.position.x-=.2f;at.bsp=0;at.facing=0;
      options.prepared_world->match.debug_request({DebugGameRequest::Teleport,slot},&at);hold_key(window_id,SDL_SCANCODE_X,true);
    }
    if(p.secondary_weapon.datum!=null_datum) {
      dual=true;if(!dual_at){dual_at=tick;hold_key(window_id,SDL_SCANCODE_X,false);}
      if(previous_ammunition) {
        fired_rounds+=unsigned(std::max(0,(*previous_ammunition)[0]-p.loaded));
        fired_rounds+=unsigned(std::max(0,(*previous_ammunition)[1]-p.secondary_weapon.loaded));
      }
      previous_ammunition=std::array{p.loaded,p.secondary_weapon.loaded};
      right_fired|=p.loaded<60;left_fired|=p.secondary_weapon.loaded<60;
      reloaded|=right_fired && left_fired && p.loaded==60 && p.secondary_weapon.loaded==60;
    }
    if(dual_at) {
      const auto elapsed=tick-*dual_at;
      if(elapsed==20)trigger(window_id,SDL_BUTTON_RIGHT,true);
      if(elapsed>20 && elapsed<26) {
        check(p.loaded==60,"Offhand-only SDL trigger consumed primary ammunition");
        offhand_radar|=p.motion_sensor_control==0x20;
      }
      if(elapsed==26){trigger(window_id,SDL_BUTTON_RIGHT,false);trigger(window_id,SDL_BUTTON_LEFT,true);}
      if(elapsed>26 && elapsed<32)primary_radar|=p.motion_sensor_control==1;
      if(elapsed==32){trigger(window_id,SDL_BUTTON_LEFT,false);trigger(window_id,SDL_BUTTON_RIGHT,false);}
      if(elapsed==40)key(window_id,SDL_SCANCODE_R);
      if(reloaded && !(p.source_action_word&1u) && !grenade_at && argc!=5)grenade_at=tick;
      if(grenade_at && tick==*grenade_at)key(window_id,SDL_SCANCODE_Q);
      if(grenade_at && tick>*grenade_at && p.grenade_action.animation_active)thrown=true;
      if(grenade_at && tick>*grenade_at && (p.grenade_action.phase()==1 || p.grenade_action.phase()==2)) {
        check(!p.motion_sensor_control,"SDL grenade request published a weapon trigger bit");
        grenade_radar_phase=true;
      }
      if(grenade_at && tick>*grenade_at && p.secondary_weapon.datum==null_datum)dropped=true;
      if(grenade_at && tick==*grenade_at+40){SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Exit weapon probe");}
    }
    check(!start || tick<*start+250,"Live dual pickup/animation sequence timed out");
  };
  options.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &sample){
    if(!raised){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);raised=true;}
    if(!sample.first_person)return;
    for(const auto &v:sample.vertices)check(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&std::isfinite(v.position.z),"Nonfinite posed weapon vertex");
    if(current.players[slot].secondary_weapon.datum!=null_datum) {
      posed_weapons.insert(sample.weapon);clips.insert(sample.animation);++draws;
    }
  };
  options.audio_observer=[&](const MultiplayerAudioSubmission &s) {
    if(s.sound==firing_sound){check(s.frames>0,"SMG firing sound submitted empty PCM");++firing_sounds;}
  };
  options.effect_observer=[&](unsigned,unsigned,std::span<const ParticleDraw> particles) {
    if(dual_at)firing_particles+=unsigned(particles.size());
  };
  if(argc==5){options.screenshot=argv[4];options.capture_tick=240;}
  check(run_multiplayer(options)==0,"Weapon runtime failed");
  check(map_pickup && switched,"Actual SDL map pickup and carried-weapon switch failed");
  if(!(dual && right_fired && left_fired && reloaded)) {
    const auto &p=current.players[slot];
    std::cerr<<"Weapon runtime state: dual="<<dual<<" right_fired="<<right_fired
        <<" left_fired="<<left_fired<<" reloaded="<<reloaded<<" dead="<<p.dead
        <<" reload_received="<<reload_received<<" lost_focus="<<lost_focus
        <<" primary_ammo="<<p.loaded<<'/'<<p.reserve<<" secondary_ammo="<<p.secondary_weapon.loaded
        <<'/'<<p.secondary_weapon.reserve<<" reload_seconds="<<p.reload_seconds
        <<'/'<<p.secondary_weapon.reload_seconds<<'\n';
  }
  check(dual && right_fired && left_fired && reloaded,"Live dual pickup, independent triggers or reload failed");
  check(offhand_radar && primary_radar,"Actual independent SDL triggers did not retain their original radar bits");
  check(posed_weapons.size()==2 && clips.size()>=3 && draws>20,"Both real weapon datums did not submit animated poses");
  std::cout<<"SMG firing observations: sounds="<<firing_sounds<<" rounds="<<fired_rounds<<'\n';
  check(firing_sounds==fired_rounds && fired_rounds>1 && firing_particles>0,
      "Live dual SMGs did not submit exactly one authored sound per fired round");
  if(argc!=5) {
    std::cout<<"SMG grenade observations: request_tick="<<grenade_at.value_or(0)<<" thrown="<<thrown
        <<" dropped="<<dropped<<" radar_phase="<<grenade_radar_phase<<'\n';
    check(thrown && dropped && grenade_radar_phase,"Grenade animation/phase did not release the actual offhand");
  }
  std::cout<<"PASS actual SDL map pickup and switch; dual pickup, two independently firing/reloading weapon datums, "<<draws<<" posed draws, "<<clips.size()<<" clips, "<<firing_particles<<" particle draws, "<<firing_sounds<<" firing sounds";
  if(argc!=5)std::cout<<", grenade animation and offhand drop";
  std::cout<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
