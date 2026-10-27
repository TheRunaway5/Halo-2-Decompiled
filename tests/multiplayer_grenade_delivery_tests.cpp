#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/multiplayer_network.hpp"
#include <iostream>
using namespace halo2;
namespace {
unsigned checks{};
void check(bool value,const char *message){++checks;if(!value)throw std::runtime_error(message);}
struct Events {
  SDL_Window *window{}; Uint32 id{};
  Events(){SDL_SetMainReady();SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER,"dummy",SDL_HINT_OVERRIDE);
    if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
    window=SDL_CreateWindow("grenade-delivery",0,0,32,32,SDL_WINDOW_HIDDEN);
    if(!window)throw std::runtime_error(SDL_GetError());id=SDL_GetWindowID(window);
    SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);}
  ~Events(){SDL_DestroyWindow(window);SDL_Quit();}
  void key(bool down,bool repeat=false){SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;
    event.key.windowID=id;event.key.state=down?SDL_PRESSED:SDL_RELEASED;event.key.repeat=repeat;
    event.key.keysym.scancode=SDL_SCANCODE_Q;event.key.keysym.sym=SDLK_q;
    check(SDL_PushEvent(&event)==1,"SDL could not queue Q");}
  void focus(bool gain){SDL_Event event{};event.type=SDL_WINDOWEVENT;event.window.windowID=id;
    event.window.event=gain?SDL_WINDOWEVENT_FOCUS_GAINED:SDL_WINDOWEVENT_FOCUS_LOST;
    check(SDL_PushEvent(&event)==1,"SDL could not queue focus");}
  void drain(MultiplayerKbm &kbm){SDL_Event event;while(SDL_PollEvent(&event))kbm.handle_event(event);}
};
}
int main(int argc,char **argv){try{
  if(argc!=4)throw std::runtime_error("Supply actual MAP SHARED_MAPS UPDATE_XBE");
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
  std::cout<<std::unitbuf;std::cerr<<"stage SDL\n";Events events;MultiplayerKbm kbm(events.id);
  const auto cache_owner=std::make_shared<Cache>(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto& cache=*cache_owner;std::cerr<<"stage Cache\n";
  const auto content=cache.load_content(0,true);UiVariant variant{"Slayer",0,{}};variant.values.set(0x59,5);
  auto rules=slayer_rules(variant);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  MultiplayerMatch match(content,0,cache.spawns(),rules);match.join("Host");LanHost host(match,0,0x51554c5345ull);
  LanClient client(host.join_address(),0x51554c5345ull,1);
  std::array<Controls,16> received{};std::array<Controls,1> outgoing{};
  for(unsigned i=0;i<20&&!client.ready();++i){client.send(outgoing);host.receive(received);host.broadcast();client.receive();}
  check(client.ready(),"Actual UDP client failed to join");const auto remote=client.slots()[0];
  check(match.unit(remote).grenades[0]==2,"Actual authored ordinary spawn grenade inventory was not available");
  events.focus(true);events.drain(kbm);
  events.key(true);events.key(false);events.drain(kbm);
  for(unsigned frame=0;frame<5;++frame)check(kbm.snapshot().controls.grenade,"Render observation lost queued Q tap");
  outgoing[0]=kbm.consume().controls;check(outgoing[0].grenade,"Consumed Q tap absent from Controls");
  client.send(outgoing);host.receive(received);check(received[remote].grenade,"UDP lost real SDL short tap");
  host.receive(received);check(!received[remote].grenade,"Cached packet retriggered consumed grenade pulse");
  check(!kbm.consume().controls.grenade,"SDL consumed Q tap twice");
  // Two consecutive consumed ticks can both contain a new Q press. These are
  // pulses, unlike the retained fire/jump/reload edges in the native protocol.
  for(unsigned i=0;i<2;++i){events.key(true);events.key(false);events.drain(kbm);
    outgoing[0]=kbm.consume().controls;client.send(outgoing);host.receive(received);
    check(received[remote].grenade,"Consecutive Q pulses were coalesced as held input");}
  // Latest continuous input can replace a packet; its serial still retains
  // each consumed pulse. Host drains exactly one queued pulse per receive.
  for(unsigned i=0;i<2;++i){events.key(true);events.key(false);events.drain(kbm);
    outgoing[0]=kbm.consume().controls;client.send(outgoing);}
  outgoing[0]=kbm.consume().controls;client.send(outgoing);
  host.receive(received);check(received[remote].grenade,"First coalesced serial was dropped");
  host.receive(received);check(received[remote].grenade,"Second coalesced serial was dropped");
  host.receive(received);check(!received[remote].grenade,"Serial queue delivered an extra grenade pulse");
  events.key(true);events.drain(kbm);check(kbm.consume().controls.grenade,"New held key lacks first pulse");
  events.key(true,true);events.drain(kbm);check(!kbm.consume().controls.grenade,"Key repeat created grenade pulse");
  events.key(false);events.key(true);events.focus(false);events.drain(kbm);
  check(!kbm.consume().controls.grenade,"Focus loss retained dangerous pending Q pulse");
  events.focus(true);events.drain(kbm);check(!kbm.consume().controls.grenade,"Focus gain recreated old Q pulse");
  // Actual authored start, native movement, source inventory, decoded throw
  // animation/current marker, genuine shared allocation, terrain and blast.
  // Only Q is queued through SDL; yaw/pitch are explicit native look inputs.
  check(cache.original_data()->grenade_constants.has_value(),"Selected executable grenade operands absent");
  MultiplayerMatch play(content,0,cache.spawns(),rules);
  std::cerr<<"stage configure\n";play.configure_grenades(cache_owner,*cache.original_data()->grenade_constants);std::cerr<<"stage configured\n";
  const auto local=play.join("Q local");std::cerr<<"stage joined\n";
  std::array<Controls,16> controls{};
  // A steeper actual look input keeps the bounced grenade within the authored
  // 1.75-unit radius after BSP material indices are resolved to global rows.
  controls[local].yaw=play.unit(local).yaw;controls[local].pitch=-1.5f;
  events.key(true);events.key(false);events.drain(kbm);
  controls[local]=kbm.consume(controls[local].yaw,controls[local].pitch).controls;
  check(controls[local].grenade,"Live SDL Q pulse absent");
  const auto initial_health=play.unit(local).vitality.body,initial_shield=play.unit(local).vitality.shield;
  bool began=false,prepared=false,released=false,contacted=false,detonated=false,damaged=false;
  std::uint32_t projectile=null_datum;
  for(unsigned tick=0;tick<150;++tick){
    if(tick%10==0)std::cerr<<"stage tick "<<tick<<"\n";play.step(controls);controls[local].grenade=false;
    const auto&u=play.unit(local);
    if(tick==0)check(u.grenades[0]==2&&u.grenade_action.counter()==1,"Begin tick debited inventory before source prepare");
    if(tick==1)check(u.grenades[0]==1&&u.grenade_action.phase()==2,"Source prepare tick did not debit/attach exactly once");
    damaged|=u.vitality.body<initial_health||u.vitality.shield<initial_shield;
    for(const auto&e:play.take_grenade_events()){
      std::cout<<"event "<<unsigned(e.kind)<<" tick "<<tick<<" counter "<<unsigned(e.counter)<<" phase "<<unsigned(e.phase)
        <<" datum "<<std::hex<<e.projectile<<std::dec<<" position "<<e.position.x<<","<<e.position.y<<","<<e.position.z<<'\n';
      if(e.kind==NativeGrenadeEvent::Begin){began=true;check(e.graph!=null_datum&&e.clip!=null_datum,"Throw has no actual graph/clip");}
      if(e.kind==NativeGrenadeEvent::Prepared){prepared=true;projectile=e.projectile;check((projectile>>16)!=0,"Projectile lacks real allocator salt");}
      if(e.kind==NativeGrenadeEvent::Released){released=true;check(e.projectile==projectile&&e.counter==std::to_integer<std::uint8_t>(u.grenade_action.source[0xf]),"Release ignored retained authored counter");}
      contacted|=e.kind==NativeGrenadeEvent::Contact;detonated|=e.kind==NativeGrenadeEvent::Detonated;
    }
  }
  check(began&&prepared&&released,"Actual SDL Q failed begin/prepare/release");
  check(contacted&&detonated,"Actual Lockout throw failed native terrain/fuse/detonation");
  check(damaged,"Actual nearby grenade blast never damaged the throwing player");
  check(play.grenade_projectiles().empty(),"Detonated grenade retained its object allocation");
  const auto live_state=play.state();
  check(live_state.grenade_detonations.size()==1&&live_state.grenade_detonations.front().serial==1,
      "Actual detonation callback was not retained for native presentation");
  check(live_state.grenade_detonations.front().projectile==projectile&&
      live_state.grenade_detonations.front().effect!=null_datum,
      "Detonation presentation lost original projectile/effect identity");
  // Genuine remote Q follows the same authoritative action. The client only
  // receives snapshots and decodes the selected clip; it never runs selection.
  match.configure_grenades(cache_owner,*cache.original_data()->grenade_constants);
  MultiplayerMatch replica(content,0,cache.spawns(),rules);
  replica.configure_grenades(cache_owner,*cache.original_data()->grenade_constants);
  received[0].yaw=match.unit(0).yaw;
  outgoing[0].yaw=match.unit(remote).yaw;outgoing[0].pitch=-1.2f;
  events.key(true);events.key(false);events.drain(kbm);
  outgoing[0]=kbm.consume(outgoing[0].yaw,outgoing[0].pitch).controls;
  bool remote_prepared=false,remote_released=false,remote_detonated=false,remote_clip=false;
  for(unsigned tick=0;tick<100;++tick){
    client.send(outgoing);host.receive(received);match.step(received);outgoing[0].grenade=false;
    host.broadcast();client.receive();
    const auto authoritative=match.state();
    check(client.state().tick==authoritative.tick,"Remote grenade snapshot tick lagged on actual loopback UDP");
    const auto& actual=match.unit(remote).grenade_action;
    const auto& copy=client.state().players[remote].grenade_action;
    check(actual.source==copy.source&&actual.graph==copy.graph&&actual.clip==copy.clip&&actual.frame==copy.frame,
        "Remote grenade action bytes/selected animation differ from authority");
    check(client.state().grenade_projectiles.size()==match.grenade_projectiles().size(),
        "Remote grenade projectile lifecycle differs from authority");
    if(copy.animation_active){const auto* decoded=replica.grenade_clip(copy.graph,copy.clip);
      check(decoded&&decoded->source_graph==copy.graph&&decoded->source_index==copy.clip,
          "Client could not retain the actual selected grenade clip");remote_clip=true;}
    for(const auto&e:match.take_grenade_events()){
      if(e.unit!=match.unit(remote).source_datum)continue;
      remote_prepared|=e.kind==NativeGrenadeEvent::Prepared;
      remote_released|=e.kind==NativeGrenadeEvent::Released;
      remote_detonated|=e.kind==NativeGrenadeEvent::Detonated;
    }
    if(!client.state().grenade_detonations.empty()){
      const auto& a=authoritative.grenade_detonations.back();
      const auto& b=client.state().grenade_detonations.back();
      check(a.serial==b.serial&&a.projectile==b.projectile&&a.effect==b.effect&&a.tick==b.tick,
          "Native UDP changed actual detonation request identity");
    }
  }
  check(remote_prepared&&remote_released&&remote_detonated&&remote_clip,
      "Actual remote SDL Q failed authoritative throw or selected-clip presentation");
  check(match.unit(remote).grenades[0]==1,"Remote Q debited grenade inventory more than once");
  std::cout<<"PASS "<<checks<<" checks: actual SDL Q event queue and native UDP pulse delivery on actual Lockout\n";
  std::cout<<"LIMIT native decoded-animation residency and triangle/capsule collision adapter; no Xbox Havok/platform/effect playback parity\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
