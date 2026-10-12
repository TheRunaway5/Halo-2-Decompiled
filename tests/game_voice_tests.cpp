#include "halo2/game_voice.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace halo2;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
int main()try {
  check(game_voice_active(true,true,true,2.f),"Current unit eligibility");
  check(game_voice_active(true,false,false,{}),"No previous unit eligibility");
  check(game_voice_active(true,false,true,1.f),"Inclusive one-second death grace");
  check(!game_voice_active(true,false,true,1.001f) && !game_voice_active(false,true,false,{}),"Dead/invalid eligibility");
  std::array<GameVoicePlayer,16> p{};
  for(unsigned i=0;i<3;++i){p[i].connected=p[i].active=true;p[i].team=i==2?1:0;}
  p[1].position={100,0,0};p[2].position={4,0,0};
  check(!game_voice_route(0,1,p,false,true,false,8),"Unarmed teammate heard across map");
  check(game_voice_route(0,1,p,true,true,false,8).has_value(),"Team radio did not reach distant teammate");
  check(game_voice_route(0,2,p,true,true,false,8).has_value(),"Nearby enemy cannot hear a radio talker");
  p[2].position={9.5f,0,0};check(game_voice_route(0,2,p,false,true,false,8).has_value(),"Inclusive proximity cutoff");
  p[2].position={9.5001f,0,0};check(!game_voice_route(0,2,p,false,true,false,8),"Proximity cutoff exceeded");
  check(game_voice_route(0,1,p,true,false,false,8).has_value(),"FFA radio audience");
  p[0].active=false;check(!game_voice_route(0,1,p,true,true,false,8),"Dead talker reached active listener");
  p[1].active=false;check(game_voice_route(0,1,p,false,true,false,8).has_value(),"Dead teammate audience missing");
  p[1].team=2;check(!game_voice_route(0,1,p,true,true,false,8),"Dead enemy received team speech");
  p[0].active=p[1].active=true;p[1].team=0;
  p[1].muted=1;check(!game_voice_route(0,1,p,true,true,false,8),"Listener mute ignored");
  p[1].muted=0;p[0].muted=2;check(!game_voice_route(0,1,p,true,true,false,8),"Reverse mute ignored");
  p[0].muted=0;p[1].disabled=true;check(!game_voice_route(0,1,p,true,true,false,8),"Disabled voice ignored");
  p[1].disabled=false;p[0].juggernaut=true;check(!game_voice_route(0,1,p,true,false,true,8),"Juggernaut received all-map radio audience");
  p[0].juggernaut=false;check(game_voice_route(0,1,p,true,false,true,8).has_value(),"Non-Juggernaut audience missing");
  p[1].position={0,0,0};const auto near=game_voice_route(0,1,p,false,false,false,8);
  p[1].position={9.5f,0,0};const auto far=game_voice_route(0,1,p,false,false,false,8);
  check(near && far && near->left>far->left && near->right>far->right,"Source distance attenuation absent");
  GameVoiceSpeaker speaker;std::array<bool,16> talking{};talking[0]=talking[2]=true;
  for(unsigned i=0;i<3;++i)check(!speaker.update(talking,0),"Speaker appeared before four audible updates");
  check(speaker.update(talking,0)==2,"Local speech selected or remote speaker missing");
  talking[1]=true;for(unsigned i=0;i<3;++i)check(speaker.update(talking,0)==2,"New talker selected before threshold");
  check(speaker.update(talking,0)==1,"Smallest eligible talking counter was not selected");
  talking[1]=talking[2]=false;check(!speaker.update(talking,0),"Inaudible remote speaker retained");
  GameVoiceRadio radio;
  check(radio.step(0,true,false,true),"Tap did not arm radio");
  check(radio.step(0,false,true,true) && radio.step(.749,false,false,true),"Radio ended before silence threshold");
  check(!radio.step(.75,false,false,true),"Radio silence timeout missing");
  check(radio.step(1,true,true,true) && radio.step(10.999,true,true,true),"Continuous speech radio duration");
  check(!radio.step(11,true,true,true) && !radio.step(11.1,true,true,true),"Ten-second cap/release gate missing");
  radio.step(11.2,false,false,true);check(radio.step(11.3,true,false,true),"Release failed to rearm");
  check(!radio.step(11.4,false,true,false),"Ineligible player retained radio");
  std::cout<<"PASS original voice audience, inclusive death/proximity boundaries, same-team/FFA/Juggernaut, reverse mute, disabled voice, source distance attenuation and tap/silence/max radio timing\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
