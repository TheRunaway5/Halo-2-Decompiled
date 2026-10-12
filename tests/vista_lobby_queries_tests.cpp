#include "halo2/vista_lobby_queries.hpp"
#include "halo2/cache.hpp"
#include "halo2/vista_session_birth.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void put(std::span<std::byte> b,std::size_t at,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(value>>(8*i));
}
std::vector<std::byte> hex(std::string_view value) {
  if(value.size()%2)throw FormatError("Odd original parameter bank");
  const auto nibble=[](char c)->unsigned {
    if(c>='0' && c<='9')return unsigned(c-'0');
    if(c>='a' && c<='f')return unsigned(c-'a'+10);
    throw FormatError("Invalid original parameter hex");
  };
  std::vector<std::byte> out(value.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::byte(nibble(value[2*i])*16+nibble(value[2*i+1]));
  return out;
}
template<class Function> void rejects(Function&& f) {
  try{f();}catch(const FormatError&){return;}
  throw FormatError("Unowned source query boundary was not rejected");
}
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;
  Json rows;input>>rows;unsigned cases{},initializer_cases{},status_cases{},count_cases{},maximum_cases{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");const auto kind=q.at("kind").get<std::string>();
    const auto u=[&](const char* key){return q.at(key).get<std::uint32_t>();};
    Json events=Json::array();std::uint32_t returned{};
    if(kind=="initialize") {
      const auto bytes=vista_lobby_maximum_parameters_initialize({std::uint16_t(u("cw")),u("mx")});
      const auto expected=hex(q.at("parameters").get<std::string>());
      if(!std::equal(bytes.begin(),bytes.end(),expected.begin(),expected.end()))
        throw FormatError("Original maximum initializer full retained bank mismatch");
      ++initializer_cases;
    } else if(kind=="status") {
      VistaLobbyStatusStringServices s;
      s.selected_map_third_output=[&](auto& third)->std::uint8_t {
        events.push_back({"selected_map",third,q.at("map_return"),q.at("write_third"),q.at("third")});
        if(u("write_third"))third=u("third");
        return std::uint8_t(u("map_return"));
      };
      returned=vista_lobby_status_string(q.at("code").get<std::int32_t>(),s);++status_cases;
    } else {
      std::array<std::byte,vista_session_source_size> bank{};
      put(bank,0x73a0,u("state"));put(bank,0x70,u("generation"));put(bank,0x1254,u("player_count"));
      VistaLobbyQueryServices s;
      s.current_session=[&] {
        events.push_back({"current_session",q.at("enabled"),q.at("state")});
        return vista_lobby_query_current_session(bank,std::uint8_t(u("enabled")));
      };
      s.observed=[&](auto call,auto value) {
        switch(call) {
        case VistaLobbyQueryCall::membership_view:events.push_back({"membership_view",value});break;
        case VistaLobbyQueryCall::membership_generation:events.push_back({"membership_generation",value});break;
        case VistaLobbyQueryCall::maximum_consumer:events.push_back({"maximum_consumer",value});break;
        }
      };
      if(kind=="count") {
        returned=std::bit_cast<std::uint32_t>(vista_lobby_player_count(s));++count_cases;
      } else if(kind=="maximum") {
        put(bank,0x64,u("initial_peer"));put(bank,0x84,u("peer_count"));
        unsigned i{};
        for(const auto& peer:q.at("peers")) {
          const auto a=0x70+i++*0x10c;
          put(bank,a+0xa8,peer.at(0));put(bank,a+0xac,peer.at(1));put(bank,a+0xb4,peer.at(2));
        }
        const auto bytes=hex(q.at("parameters").get<std::string>());
        returned=std::bit_cast<std::uint32_t>(vista_lobby_maximum(s,vista_lobby_maximum_parameters(bytes)));
        ++maximum_cases;
      } else throw FormatError("Unknown original query fixture kind");
    }
    if(returned!=row.at("returned").get<std::uint32_t>() || events!=row.at("events")) {
      std::cerr<<"query fixture "<<cases<<" "<<q<<"\nactual "<<returned<<" "<<events
               <<"\noriginal "<<row.at("returned")<<" "<<row.at("events")<<"\n";
      throw FormatError("Original Vista query output/call-order mismatch");
    }
    ++cases;
  }
  if(vista_lobby_query_current_session({},0))throw FormatError("Disabled query examined absent bank");
  rejects([]{(void)vista_lobby_query_current_session({},1);});
  rejects([]{(void)vista_lobby_maximum_parameters({});});
  rejects([]{(void)vista_lobby_maximum_parameters_initialize({0x037f,0x1f80});});
  rejects([]{(void)vista_lobby_player_count({});});
  rejects([]{(void)vista_lobby_status_string(7,{});});
  if(vista_lobby_status_string(-1,{}) || vista_lobby_status_string(27,{}))
    throw FormatError("Out-of-range status queried a service or returned a string");
  std::array<std::byte,vista_session_source_size> bank{};put(bank,0x73a0,5);put(bank,0x64,17);
  VistaLobbyQueryServices s;s.current_session=[&]{return vista_lobby_query_current_session(bank,1);};
  rejects([&]{(void)vista_lobby_maximum(s,{});});
  put(bank,0x64,0xffffffff);put(bank,0x84,18);
  rejects([&]{(void)vista_lobby_maximum(s,{});});
  std::cout<<"PASS "<<cases<<" original Vista query cases (initializer="<<initializer_cases<<", status="<<status_cases
           <<", count="<<count_cases<<", maximum="<<maximum_cases<<") and explicit unowned boundaries\n";
  return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
