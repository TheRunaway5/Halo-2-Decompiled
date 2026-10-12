#include "halo2/vista_lobby_browser.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool v,const std::string& reason){++checks;if(!v)throw std::runtime_error(reason);}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v){for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));}
std::uint32_t get(std::span<const std::byte> b,std::size_t at){std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;}
template<class F>void rejects(F&& f,const std::string& why){bool rejected{};try{f();}catch(const std::exception&){rejected=true;}check(rejected,why);}
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);check(bool(input),"original fixture exists");const auto fixture=Json::parse(input);
  check(fixture.at("schema")=="H2V_PC14_BROWSER_GATE_ORIGINAL_V1","fixture schema");check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","original EXE pin");
  check(fixture.at("source_spans").at("queries").at("sha256")=="9bd37c1f15660951890cac4e3a38e46b1d8d5b1e013d1b22a84298d531e17d47","original browser query span");
  check(fixture.at("cases").size()==4080,"original source case coverage");
  unsigned ordinal{};for(const auto& row:fixture.at("cases")) {
    const auto& q=row.at("input");std::array<std::byte,0x14> bank{};put(bank,8,std::uint32_t(q.at("pending").get<std::int64_t>()));put(bank,4,q.at("state").get<std::uint32_t>());put(bank,0x10,q.at("total").get<std::uint32_t>());Json events=Json::array();VistaLobbyBrowserServices services;
    services.dialog=[&](VistaLobbyBrowserDialog dialog){events.push_back(Json::array({"dialog",std::uint32_t(dialog)}));if(!q.at("mutate").is_null())put(bank,4,q.at("mutate").get<std::uint32_t>());};
    services.progress_text=[&](std::uint32_t percent,bool visible){events.push_back(Json::array({"progress",percent,unsigned(visible)}));};
    vista_lobby_browser_update(bank,q.at("received").get<std::uint32_t>(),q.at("texts").get<unsigned>()!=0,services);
    const auto why="original browser case "+std::to_string(ordinal++);check(get(bank,8)==row.at("pending_after").get<std::uint32_t>(),why+" pending acknowledgement");++checks;if(events!=row.at("events"))throw std::runtime_error(why+": "+Json::diff(row.at("events"),events).dump());
  }
  std::array<std::byte,0x14> bank{};VistaLobbyBrowserServices services;check(vista_lobby_browser_percent(bank,0)==100,"actual quiescent browser percent");vista_lobby_browser_update(bank,0,false,{});check(get(bank,8)==0,"absent percent child consumes no sink");
  rejects([&]{vista_lobby_browser_update(bank,0,true,{});},"missing consumed progress widget accepted");put(bank,8,1);rejects([&]{vista_lobby_browser_update(bank,0,false,{});},"missing consumed dialog accepted");
  rejects([&]{vista_lobby_browser_percent(std::span(bank).first(0x13),0);},"truncated retained source query fields accepted");
  std::cout<<"PASS "<<checks<<" checks,"<<ordinal<<" original browser cases; no visible/native browser integration claim\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
