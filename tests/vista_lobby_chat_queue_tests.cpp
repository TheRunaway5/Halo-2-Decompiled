#include "halo2/vista_lobby_chat_queue.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool v,const std::string& reason){++checks;if(!v)throw std::runtime_error(reason);}
std::vector<std::byte> bytes(const std::string& hex) {
  auto digit=[](char c)->unsigned {if(c>='0' && c<='9')return unsigned(c-'0');if(c>='a' && c<='f')return unsigned(c-'a'+10);throw std::runtime_error("fixture hex");};
  check(hex.size()%2==0,"fixture even hex");std::vector<std::byte> b(hex.size()/2);
  for(std::size_t i=0;i<b.size();++i)b[i]=std::byte((digit(hex[2*i])<<4)|digit(hex[2*i+1]));
  return b;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v){for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));}
void compare(const Json& a,const Json& e,const std::string& why){++checks;if(a!=e)throw std::runtime_error(why+": "+Json::diff(e,a).dump());}
template<class F>void rejects(F&& f,const std::string& why){bool rejected{};try{f();}catch(const std::exception&){rejected=true;}check(rejected,why);}
void original(const Json& row,unsigned ordinal) {
  const auto q=row.at("input");auto owner=bytes(row.at("before_hex"));const auto expected_owner=bytes(row.at("after_hex"));const auto before_output=bytes(row.at("output_before_hex"));const auto expected_output=bytes(row.at("output_after_hex"));
  check(before_output.size()==vista_lobby_chat_record_size && owner.size()==vista_lobby_chat_queue_source_size,"fixture owner size");VistaLobbyChatRecord output;std::copy(before_output.begin(),before_output.end(),output.begin());
  const auto op=q.at("op").get<std::string>();Json events=Json::array();bool returned{};
  if(op=="enable")vista_lobby_chat_queue_enable(owner);
  else if(op=="enqueue") {
    const auto seed=q.at("seed").get<unsigned>();VistaLobbyChatRecord message;
    for(std::size_t i=0;i<message.size();++i)message[i]=std::byte((i*37+seed*13)&255);
    const std::string identity="KEY_ID!!";std::transform(identity.begin(),identity.end(),message.begin(),[](char c){return std::byte(c);});put(message,8,seed);vista_lobby_chat_queue_enqueue(owner,message);
  }else if(op=="pop")returned=vista_lobby_chat_queue_pop(owner,output);
  else if(op=="receive") {
    std::vector<std::byte> bank(0x7b88);put(bank,0x10,0x9035f8);put(bank,0x73a0,std::uint32_t(q.at("state").get<std::int64_t>()));put(bank,0x1c,q.at("class").get<std::uint32_t>());put(bank,0x1258,q.at("mask").get<std::uint32_t>());bank[0x48]=std::byte{1};
    const std::string identity="KEY_ID!!";std::transform(identity.begin(),identity.end(),bank.begin()+0x20,[](char c){return std::byte(c);});
    for(unsigned player=0;player<16;++player){put(bank,0x125c+player*0x128,0x89abcdefu+player);put(bank,0x1260+player*0x128,0x12345678);put(bank,0x1264+player*0x128,q.at("peer").get<std::uint32_t>());}
    VistaLobbyChatQueueServices services;
    services.current_session=[&]()->std::optional<VistaLobbyChatQueueSession>{events.push_back(Json::array({"current_session"}));if(!q.at("present").get<unsigned>())return {};return VistaLobbyChatQueueSession{bank,owner};};
    services.mute_list_query=[&](std::uint32_t controller,std::uint64_t xuid,std::uint32_t& muted){check(muted==0,"mute BOOL initialized");const auto hr=q.at("hr").get<std::uint32_t>();muted=q.at("muted").get<std::uint32_t>();events.push_back(Json::array({"mute_query",controller,std::uint32_t(xuid),std::uint32_t(xuid>>32),hr,muted}));return hr;};
    returned=vista_lobby_chat_receive(output,services);
  }else throw std::runtime_error("fixture operation");
  const auto why="original queue case "+std::to_string(ordinal);
  check(owner==expected_owner,why+" complete retained owner bytes");check(std::equal(output.begin(),output.end(),expected_output.begin()),why+" exact output bytes");compare(events,row.at("events"),why+" service order/arguments");
  if(op=="pop" || op=="receive")check(returned==bool(row.at("returned").get<unsigned>()),why+" AL result");
}
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);check(bool(input),"original fixture exists");const auto fixture=Json::parse(input);
  check(fixture.at("schema")=="H2V_PC14_CHAT_QUEUE_ORIGINAL_V1","fixture schema");check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","original EXE pin");
  check(fixture.at("source_spans").at("receive").at("sha256")=="e1eb0753b15150676be7633aec9688109f1c78eeeafab30506626b8eca00d0ac","original receive span");check(fixture.at("queue_static_initial_sha256")=="f7641593a814bd2f84e2fddd1e26c55bc8bcccb1609364721a1297c7cfcd5163","source fresh owner image");
  check(fixture.at("mute_import").at("supplied_export").at("name")=="XUserMuteListQuery","original mute import identity");
  const auto& rows=fixture.at("cases");check(rows.size()==343,"original source case coverage");unsigned i{};for(const auto& row:rows)original(row,i++);
  std::array<std::byte,vista_lobby_chat_queue_source_size> owner{};VistaLobbyChatRecord output;output.fill(std::byte{0x55});vista_lobby_chat_queue_enable(owner);check(owner[0]==std::byte{1},"source enabled byte");check(!vista_lobby_chat_queue_pop(owner,output) && output[0]==std::byte{0x55},"empty raw pop leaves output untouched");
  rejects([&]{vista_lobby_chat_receive(output,{});},"missing consumed session service accepted");
  rejects([&]{vista_lobby_chat_queue_enable(std::span(owner).first(owner.size()-1));},"truncated source owner accepted");put(owner,0x108c,17);rejects([&]{vista_lobby_chat_queue_pop(owner,output);},"unowned source ring count accepted");put(owner,0x108c,0);vista_lobby_chat_queue_enqueue(owner,output);
  std::vector<std::byte> bank(0x7b88);put(bank,0x73a0,5);bank[0x48]=std::byte{1};unsigned calls{};VistaLobbyChatQueueServices services;
  services.current_session=[&]()->std::optional<VistaLobbyChatQueueSession>{if(++calls==1)return VistaLobbyChatQueueSession{bank,owner};return {};};
  rejects([&]{vista_lobby_chat_receive(output,services);},"lost nonempty source session silently returned false");check(calls==2,"lost session source queries retained");
  std::cout<<"PASS "<<checks<<" checks,"<<rows.size()<<" original source chat queue cases; no visible/live chat integration claim\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
