// Actual native room socket clients. The supplied credentials come from Rust
// HTTP; datagrams/PCR are explicitly synthetic component controls, not a world.
#include "halo2/online_match_session.hpp"
#include "halo2/frontend.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void require(bool value,const char *text){if(!value)throw std::runtime_error(text);}
template<class F> void until(F check,const char *message,int seconds=8){
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
  while(std::chrono::steady_clock::now()<deadline){
    if(check())return;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error(message);
}
std::vector<std::byte> packet(unsigned kind,std::size_t size){
  std::vector<std::byte> bytes(size,std::byte{0x33});
  bytes[0]=std::byte{'H'};bytes[1]=std::byte{'2'};bytes[2]=std::byte{'N'};bytes[3]=std::byte{'2'};
  bytes[4]=std::byte(native_multiplayer_protocol&255);bytes[5]=std::byte(native_multiplayer_protocol>>8);bytes[6]=std::byte(kind);
  return bytes;
}
}
int main(){try{
  const auto *endpoint=std::getenv("H2_TEST_ROOM_URL"),*raw=std::getenv("H2_TEST_ROOM_DESCRIPTORS");
  require(endpoint && raw,"Private Rust room fixture inputs required");
  const auto descriptors=Json::parse(raw);require(descriptors.is_array() && descriptors.size()==2,"Two actual room descriptors required");
  std::vector<OnlineAdmission> roster;
  for(const auto &row:descriptors[0].at("players")){
    OnlineAdmission player;player.slot=roster.size();player.name=row.at("name");player.team=row.at("team");
    player.peer=row.at("peer");player.guest=row.at("guest");
    const std::string identity=row.at("identity");require(identity.size()==16,"Native room identity extent differs");
    for(unsigned i=0;i<8;++i)player.identity[i]=std::byte(std::stoul(identity.substr(2*i,2),nullptr,16));
    roster.push_back(std::move(player));
  }
  std::vector<std::unique_ptr<OnlineMatchSession>> sessions;
  for(const auto &row:descriptors)sessions.push_back(std::make_unique<OnlineMatchSession>(endpoint,row.at("id").get<std::string>(),row.at("token").get<std::string>(),row.at("slot").get<unsigned>(),roster,true,false));
  auto healthy=[&]{for(const auto &s:sessions)require(s->phase()!=OnlineMatchSession::Phase::Failed,"Actual native room client failed");};
  until([&]{healthy();return std::all_of(sessions.begin(),sessions.end(),[](const auto &s){return s->phase()==OnlineMatchSession::Phase::Loading;});},"Native clients did not reach Rust loading");
  const UiVariant variant{"Slayer",0,{}};
  const auto started=std::chrono::steady_clock::now();
  for(auto &s:sessions)s->loaded(0x0123456789abcdefull,variant);
  until([&]{healthy();return std::all_of(sessions.begin(),sessions.end(),[](const auto &s){return s->phase()==OnlineMatchSession::Phase::Running;});},"Native clients did not pass the real Rust start timer",15);
  require(std::chrono::steady_clock::now()-started>std::chrono::seconds(9),"Native start timer was shortened");
  require(sessions[0]->host()==sessions[1]->host(),"Native host publications differ");
  const unsigned h=sessions[0]->self()==sessions[0]->host()?0:1,c=1-h;
  auto input=packet(1,27);sessions[c]->send(input,sessions[h]->self());
  std::vector<std::byte> received;unsigned peer=16;
  until([&]{healthy();return sessions[h]->receive(received,peer);},"Native input did not cross the Rust relay");
  require(received==input && peer==sessions[c]->self(),"Native input route or contents changed");
  auto snapshot=packet(3,1400);sessions[h]->send(snapshot,sessions[c]->self());
  until([&]{healthy();return sessions[c]->receive(received,peer);},"Native snapshot did not cross the Rust relay");
  require(received==snapshot && peer==sessions[h]->self(),"Native snapshot route or contents changed");
  std::vector<std::byte> report(0xdc78);report[0]=report[1]=report[0x10]=report[0x11]=std::byte{1};report[0x64]=std::byte{2};
  for(const auto &player:roster){
    const auto at=0x370+player.slot*0x94;report[at]=std::byte{1};report[at+0x88]=std::byte{255};report[at+0x90]=std::byte(player.slot?1:0);
    std::copy(player.identity.begin(),player.identity.end(),report.begin()+at+2);
    require(player.name.size()<=31,"Synthetic ASCII PCR name exceeds extent");
    for(unsigned i=0;i<player.name.size();++i){require(static_cast<unsigned char>(player.name[i])<128,"Synthetic PCR requires declared ASCII fixture names");report[at+12+2*i]=std::byte(player.name[i]);}
  }
  sessions[h]->complete(report);
  until([&]{healthy();return std::all_of(sessions.begin(),sessions.end(),[](const auto &s){return s->phase()==OnlineMatchSession::Phase::Reviewing;});},"Native report review did not cross Rust sockets");
  for(auto &s:sessions){require(s->postgame()==report,"Native report bytes changed");s->confirm_result();}
  until([&]{healthy();return std::all_of(sessions.begin(),sessions.end(),[](const auto &s){return s->phase()==OnlineMatchSession::Phase::Postgame && s->result_recorded()==true;});},"Native result completion did not commit through Rust");
  sessions.clear();
  std::cout<<"PASS native Rust room clients "<<Json{{"protocol",native_multiplayer_protocol},{"players",roster.size()},{"input_bytes",input.size()},{"snapshot_bytes",snapshot.size()},{"result_recorded",true},{"real_start_timer",true}}.dump()<<'\n';
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
