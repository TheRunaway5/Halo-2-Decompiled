#include "halo2/native_player_profile.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/hud.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char* text){if(!ok)throw FormatError(text);}
template<class F>void rejects(F fn,const char* text) {
  bool failed=false;try{fn();}catch(const FormatError&){failed=true;}check(failed,text);
}
struct Preferences {
  std::filesystem::path path=std::filesystem::temp_directory_path()/
      ("halo2-profile-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".cfg");
  ~Preferences(){std::error_code e;std::filesystem::remove(path,e);std::filesystem::remove(path.string()+".tmp",e);}
};
void choose(MenuState& menu,unsigned index,unsigned screen) {
  menu.select(index);menu.accept();check(menu.frame().screen==screen,"Normal selected-profile menu route changed");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the repository root");const std::filesystem::path root(argv[1]),maps=root/"assets/disc/maps";
  Cache front(maps/"mainmenu.map");const auto frontend=front.load_frontend();
  const std::string name="Zo\xc3\xab";
  Preferences preferences;
  {std::ofstream file(preferences.path);file<<"halo2-native-menu-v1\nprofile "<<std::quoted(name)
      <<"\nactive "<<std::quoted(name)<<'\n';
    for(unsigned i=0;i<4;++i)file<<"option "<<std::quoted(name)<<' '<<256+i<<' '<<3+i<<'\n';}
  MenuState menu(frontend,preferences.path);menu.accept();menu.accept();
  check(menu.signed_in_profile()==name,"Selected saved profile was not signed in");
  choose(menu,2,30);choose(menu,0,14);menu.select(0);menu.accept();
  check(menu.launch && menu.launch->selected_profiles.size()==1 && menu.launch->local_players==1,
      "Normal Start lost the selected complete profile or invented users");
  const auto requested=menu.launch->selected_profiles[0];
  std::array<std::uint8_t,9> original_appearance{10,0,0,10,0,0,0,0,0};
  const auto initialized=native_selected_player_configuration("",original_appearance,0);
  {std::ifstream file(root/"tests/fixtures/local_profile_initialized_default.hex");std::string line;
   while(std::getline(file,line) && (line.empty() || line[0]=='#')){}
   check(line.size()==288,"Original initialized profile fixture unavailable");
   for(unsigned i=0;i<144;++i)check(initialized[i]==std::stoul(line.substr(i*2,2),nullptr,16),
       "Native initial local-profile bytes differ from the complete original owning bridge");}
  for(const auto& text:{std::string("ASCII"),std::string("\xc2\x80"),std::string("\xdf\xbf"),
      std::string("\xe0\xa0\x80"),std::string("\xef\xbf\xbf"),std::string("\xf0\x90\x80\x80"),std::string("\xf4\x8f\xbf\xbf")})
    check(native_player_configuration_name(native_selected_player_configuration(text,original_appearance,0))==text,
        "Native Unicode scalar boundary did not preserve original UTF16 profile data");
  for(const auto& text:{std::string("\xc0\x80"),std::string("\xed\xa0\x80"),std::string("\xf4\x90\x80\x80"),std::string("\xf0\x9f\x92")})
    rejects([&]{native_selected_player_configuration(text,original_appearance,0);},"Malformed native UTF8 profile was accepted");
  const auto truncated=native_selected_player_configuration(std::string(30,'A')+"\xf0\x90\x80\x80",original_appearance,0);
  check(source_player_configuration_name(truncated).size()==31 && truncated[61]==0xd8 && truncated[62]==0,
      "Native provider changed the original31-code-unit name truncation");
  rejects([&]{native_player_configuration_name(truncated);},"Unavailable native encoding of truncated source UTF16 was accepted");
  check(requested[0x7e]==0 && requested[0x7f]==0 && requested[0x84]==0,
      "Native selected profile substituted standalone-constructor sentinels for the initialized input member");
  check(source_player_configuration_name(requested)==u"Zo\u00eb" && requested[0x40]==3 && requested[0x43]==6,
      "Saved Unicode name/appearance did not reach the actual launch record");
  Cache cache(maps/"lockout.map",{},OriginalData::load_xbe(root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"));
  const auto content=cache.load_content(0,true);
  auto rules=slayer_rules(menu.launch->variant,true);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  rules.score_limit=rules.time_ticks=0;
  MultiplayerMatch match(content,0,cache.spawns(),rules);
  std::array<std::uint8_t,9> appearance{3,4,5,6,0,0,0,0,0};
  std::array<SourcePlayerConfiguration,4> profiles;
  for(unsigned i=0;i<4;++i) {
    profiles[i]=i ? native_selected_player_configuration("Local "+std::to_string(i),appearance,-1) : requested;
    profiles[i][0x82]=std::uint8_t(0x90+i);
    check(match.join(native_player_configuration_name(profiles[i]),-1,profiles[i])==i,"Profile local slot allocation changed");
    check(match.player(i).source_profile_request==profiles[i] && match.player(i).source_configuration &&
        (*match.player(i).source_configuration)[0x44]==2 && (*match.player(i).source_configuration)[0x7c]==i &&
        (*match.player(i).source_configuration)[0x82]==0x90+i,"Original request, sanitizer or FFA membership record lost bytes");
  }
  const auto hud=cache.load_hud(content.player.datum,rules.weapon);
  const auto display=multiplayer_hud_score_display(match.state(),0,25,0,hud);
  check(display.status==SourceHudScoreStatus::Ready && display.color==hud.score_appearance_colors[3] &&
      display.color!=hud.score_appearance_colors[10],"Actual HUD still substituted the default profile color");
  LanHost host(match,0,0x192baa);const auto address="127.0.0.1:"+std::to_string(host.port());
  std::array<std::unique_ptr<LanClient>,3> peers;
  for(unsigned peer=0;peer<3;++peer) {
    auto selected=profiles;
    for(unsigned i=0;i<4;++i) {
      selected[i]=native_selected_player_configuration(peer==0 && i==0 ? name : "Peer "+std::to_string(peer)+" "+std::to_string(i),appearance,-1);
      selected[i][0x82]=std::uint8_t(0xa0+peer*4+i);
    }
    peers[peer]=std::make_unique<LanClient>(address,0x192baa,4,std::span<const std::int8_t>{},selected);
  }
  std::array<Controls,16> controls{};
  for(unsigned n=0;n<64 && host.peers()!=3;++n) {
    for(auto& peer:peers)peer->send(std::array<Controls,4>{});
    host.receive(controls);host.broadcast();for(auto& peer:peers)peer->receive();
  }
  check(host.peers()==3 && match.available_player_slots()==0,"Four-view profile hellos did not admit a complete16-player roster");
  for(unsigned n=0;n<90;++n) {
    for(auto& peer:peers)peer->send(std::array<Controls,4>{});
    host.receive(controls);match.step(controls);host.broadcast();for(auto& peer:peers)peer->receive();
  }
  for(const auto& peer:peers) {
    check(peer->ready(),"Fragmented native profile snapshots were not published");
    for(unsigned i=0;i<16;++i)check(peer->state().players[i].source_configuration==match.state().players[i].source_configuration,
        "Complete source configuration changed across actual UDP snapshot fragments");
  }
  check(peers[0]->state().players[4].name==name,"Unicode peer name was replaced by LAN Player");
  Mesh mesh;MultiplayerScoreboard scoreboard(cache,maps/"fonts",mesh,menu.launch->variant);
  check(scoreboard.available(),"Original scoreboard providers unavailable");
  for(unsigned i=0;i<40;++i)scoreboard.step(0,true);
  const std::array<unsigned,4> local{0,1,2,3};
  const auto painted=scoreboard.draw(peers[0]->state(),local,0,1280,720);
  const auto row=std::find_if(painted.layout.rows.begin(),painted.layout.rows.end(),[&](const auto& row){return row.datum==match.player(0).source_datum;});
  check(row!=painted.layout.rows.end() && row->widgets[2].text==u"Zo\u00eb","Actual scoreboard lost the source UTF16 name");
  auto team_rules=rules;team_rules.team_mode=true;
  auto missing_body=content;missing_body.player_representations.at(3)={};
  MultiplayerMatch team(missing_body,0,cache.spawns(),team_rules);team.join("Existing",0);
  LanHost team_host(team,0,0x14e9e0);auto invalid=profiles;
  for(auto& profile:invalid)profile[0x7c]=7;
  invalid[3][0x44]=1;
  const std::array<std::int8_t,4> choices{7,7,7,7};
  LanClient rejected("127.0.0.1:"+std::to_string(team_host.port()),0x14e9e0,4,choices,invalid);
  rejected.send(std::array<Controls,4>{});team_host.receive(controls);rejected.receive();
  check(!rejected.error().empty() && !rejected.ready() && team_host.peers()==0 &&
      team.available_player_slots()==15 && team.state().retained_team_mask==1,
      "Rejected last-view profile allocated players or polluted retained team rankings");
  auto mismatch=profiles[0];mismatch[0x7c]=1;
  rejects([&]{team.join(name,0,mismatch);},"Conflicting profile/session team was accepted");
  auto bad=profiles;bad[0][0]=0;bad[0][1]=0xdc;
  rejects([&]{LanClient malformed(address,0x192baa,4,{},bad);},"Malformed native UTF16 provider was silently accepted");
  std::cout<<"Normal saved-profile Start, original sanitizer,16-player/four-view UDP profiles, HUD/scoreboard and atomic rejection pass\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
