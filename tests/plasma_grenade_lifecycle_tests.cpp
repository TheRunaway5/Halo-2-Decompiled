#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/multiplayer.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char**argv){try{
  if(argc!=4&&argc!=5)throw FormatError("Supply MAP SHARED_MAPS UPDATE_XBE [baseline]");
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
  const bool baseline=argc==5&&std::string_view(argv[4])=="baseline";
  SDL_SetMainReady();SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER,"dummy",SDL_HINT_OVERRIDE);
  if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());
  auto*window=SDL_CreateWindow("plasma-contact",0,0,32,32,SDL_WINDOW_HIDDEN);
  if(!window)throw FormatError(SDL_GetError());
  const auto id=SDL_GetWindowID(window);MultiplayerKbm kbm(id);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  const auto cache=std::make_shared<Cache>(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  auto content=cache->load_content(0,true);UiVariant variant{"Slayer",0,{{89,5}}};
  auto rules=slayer_rules(variant);
  rules.weapon=cache->slayer_starting_weapon(rules.starting_weapon);
  MultiplayerMatch match(content,0,cache->spawns(),rules);
  match.configure_grenades(cache,*cache->original_data()->grenade_constants);
  const auto player=match.join("Plasma natural inventory");
  std::array<Controls,16> controls{};controls[player].yaw=match.unit(player).yaw;
  const auto initial=match.unit(player).grenades;
  if(initial!=std::array<std::uint8_t,2>{0,2})throw FormatError("Midship source spawn must supply two plasma grenades and no frag grenades");
  const auto matg=std::find_if(cache->tags().begin(),cache->tags().end(),[](const auto&t){return t.group=="matg";});
  const auto globals=cache->load_source_tag_metadata(matg->datum);
  const Bytes gb(globals.bytes_at(globals.source_address(),globals.size()));
  const Bytes grenade_rows(globals.bytes_at(gb.u32(0x104),gb.u32(0x100)*0x2c));
  const auto plasma=grenade_rows.u32(0x2c+0x28);
  unsigned began=0,prepared=0,released=0,contacts=0,detonated=0;
  bool plasma_released=false,plasma_contacted=false,plasma_detonated=false;
  auto pulse=[&]{for(const auto type:{SDL_KEYDOWN,SDL_KEYUP}){SDL_Event e{};e.type=type;e.key.windowID=id;
    e.key.state=type==SDL_KEYDOWN?SDL_PRESSED:SDL_RELEASED;e.key.keysym.scancode=SDL_SCANCODE_Q;e.key.keysym.sym=SDLK_q;
    if(SDL_PushEvent(&e)!=1)throw FormatError("Could not queue actual SDL Q event");}
    SDL_Event e;while(SDL_PollEvent(&e))kbm.handle_event(e);
    controls[player]=kbm.consume(controls[player].yaw,controls[player].pitch).controls;
    if(!controls[player].grenade)throw FormatError("SDL Q event was not delivered");};
  std::cout<<std::unitbuf<<"initial inventory "<<unsigned(initial[0])<<' '<<unsigned(initial[1])<<" plasma "<<std::hex<<plasma<<std::dec<<'\n';
  try{
    for(unsigned tick=0;tick<660;++tick){
      // Real ordinary inventory is consumed in order. No vitality, pose,
      // grounded state, projectile, timer, selector or contact is assigned.
      if(tick==0){controls[player].pitch=-1.2f;pulse();}
      match.step(controls);controls[player].grenade=false;
      for(const auto&e:match.take_grenade_events()){
        std::cout<<"event "<<unsigned(e.kind)<<" tick "<<tick<<" definition "<<std::hex<<e.definition<<" projectile "<<e.projectile<<std::dec
          <<" position "<<e.position.x<<','<<e.position.y<<','<<e.position.z<<" velocity "<<e.velocity.x<<','<<e.velocity.y<<','<<e.velocity.z<<'\n';
        began+=e.kind==NativeGrenadeEvent::Begin;prepared+=e.kind==NativeGrenadeEvent::Prepared;released+=e.kind==NativeGrenadeEvent::Released;
        contacts+=e.kind==NativeGrenadeEvent::Contact;detonated+=e.kind==NativeGrenadeEvent::Detonated;
        if(e.definition==plasma){plasma_released|=e.kind==NativeGrenadeEvent::Released;plasma_contacted|=e.kind==NativeGrenadeEvent::Contact;plasma_detonated|=e.kind==NativeGrenadeEvent::Detonated;}
      }
      if(prepared && match.unit(player).grenades!=std::array<std::uint8_t,2>{0,1})
        throw FormatError("Plasma action did not consume exactly one authored spawn grenade");
      if(plasma_detonated)break;
    }
  }catch(const FormatError&e){
    if(!baseline)throw;
    std::cout<<"BASELINE_STOP "<<e.what()<<" plasma_released "<<plasma_released<<" counts "<<unsigned(match.unit(player).grenades[0])<<' '<<unsigned(match.unit(player).grenades[1])<<'\n';
    if(!plasma_released||std::string_view(e.what())!="Projectile response requires original stochastic/contact selector")throw;
    SDL_DestroyWindow(window);SDL_Quit();return 0;
  }
  if(baseline)throw FormatError("Baseline did not reproduce plasma contact rejection");
  const auto remaining=match.unit(player).grenades;
  if(began!=1||prepared!=1||released!=1||!plasma_released||!plasma_contacted||!plasma_detonated||remaining[0]!=0||remaining[1]!=1)
    throw FormatError("Natural Midship plasma lifecycle did not complete");
  std::cout<<"PASS actual SDL Q Midship plasma: "<<began<<" begins "<<released<<" releases "<<contacts<<" terrain contacts "<<detonated
    <<" detonations; remaining "<<unsigned(remaining[0])<<' '<<unsigned(remaining[1])<<'\n';
  std::cout<<"LIMIT native triangle terrain and area-damage backend; no unit sticking, water, stochastic response, Havok or GPU parity\n";
  SDL_DestroyWindow(window);SDL_Quit();return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
