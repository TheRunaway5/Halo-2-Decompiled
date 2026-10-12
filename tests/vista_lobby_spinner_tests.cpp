#include "halo2/vista_lobby_spinner.hpp"
#include <nlohmann/json.hpp>
#include <array>
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
void check(bool value,const std::string& reason) {
  ++checks;if(!value)throw std::runtime_error(reason);
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(value>>(8*i));
}
std::uint32_t bits(std::int32_t value){return std::bit_cast<std::uint32_t>(value);}
void compare(const Json& actual,const Json& expected,const std::string& reason) {
  ++checks;if(actual!=expected)throw std::runtime_error(reason+": "+Json::diff(expected,actual).dump().substr(0,1800));
}
void original_case(const Json& row,const Json& assets,unsigned ordinal) {
  const auto& q=row.at("input");std::vector<std::byte> bank(0x8000);
  put(bank,0x73a0,bits(q.at("state").get<std::int32_t>()));
  put(bank,0x1258,q.at("occupied_mask").get<std::uint32_t>());
  for(unsigned i=0;i<16;++i)put(bank,0x125c+i*0x128+8,i%17);
  for(unsigned i=0;i<17;++i)put(bank,0x88+i*0x10c+0x88,bits(q.at("peer_state").get<std::int32_t>()));
  std::array<VistaLobbySpinnerBitmap,16> widgets{};
  std::array<bool,16> present{};
  std::array<std::uint8_t,16> observed{};
  const auto previous=q.at("previous").get<std::uint16_t>();
  for(unsigned i=0;i<16;++i) {
    const auto& a=assets.at(i);const auto missing=q.at("missing").get<int>();
    present[i]=a.at("present").get<bool>() && (missing<0 || int(i+42)<missing);
    widgets[i]={0xa5,previous,present[i]?a.at("pages").get<std::int32_t>():0,1};observed[i]=0xa5;
  }
  Json events=Json::array();
  auto flush=[&] {
    for(unsigned i=0;i<16;++i)if(present[i] && widgets[i].visible!=observed[i]) {
      events.push_back(Json::array({"visible",i+42,widgets[i].visible}));observed[i]=widgets[i].visible;
    }
  };
  VistaLobbySpinnerSessionQuery query=[&]()->std::optional<std::span<const std::byte>> {
    flush();events.push_back(Json::array({"current_session"}));return bank;
  };
  VistaLobbySpinnerServices services;
  services.bitmap=[&](std::int16_t i)->VistaLobbySpinnerBitmap* {
    flush();events.push_back(Json::array({"lookup_bitmap",i}));
    return i>=42 && i<58 && present[unsigned(i-42)]?&widgets[unsigned(i-42)]:nullptr;
  };
  services.occupied=[&](std::uint32_t id) {
    flush();events.push_back(Json::array({"occupied",id}));return vista_lobby_spinner_player_present(id,query);
  };
  services.peer_loading=[&](std::uint32_t id) {
    flush();events.push_back(Json::array({"peer_loading",id}));
    // Actual615B4F invokes6156E1 before its second membership query.
    events.push_back(Json::array({"occupied",id}));return vista_lobby_spinner_peer_loading(id,query);
  };
  services.frame_requested=[&](std::int16_t i,std::int32_t frame) {
    flush();events.push_back(Json::array({"frame",i,bits(frame)}));
  };
  std::array<std::uint32_t,16> ids{};for(unsigned i=0;i<16;++i)ids[i]=i;
  VistaLobbySpinnerState state{q.at("counter").get<std::int32_t>(),q.at("frame").get<std::int32_t>()};
  vista_lobby_spinner(state,ids,q.at("count").get<std::int32_t>(),services);flush();
  const auto context="case "+std::to_string(ordinal);
  check(state.counter==row.at("counter").get<std::int32_t>() && state.frame==row.at("frame").get<std::int32_t>(),context+" retained counters");
  Json children=Json::object();for(unsigned i=0;i<16;++i)if(present[i])
    children[std::to_string(i+42)]={{"visible",widgets[i].visible},{"frame",widgets[i].frame}};
  compare(children,row.at("children"),context+" source bitmap states");
  compare(events,row.at("events"),context+" exact owner service order");
}
template<class F> void rejects(F&& f,const std::string& reason) {
  bool rejected{};try{f();}catch(const std::exception&){rejected=true;}check(rejected,reason);
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)return 77;
    std::ifstream source(argv[1]);check(bool(source),"missing original fixture");const auto fixture=Json::parse(source);
    check(fixture.at("schema")=="H2V_PC14_SPINNER_ORIGINAL_V1","fixture schema");
    check(fixture.at("source").at("sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","original executable pin");
    check(fixture.at("map").at("sha256")=="eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44","original map pin");
    check(fixture.at("source_spans").at("spinner").at("sha256")=="2222d934cec2892fc96dc0aa60d533b7a2026015a9a98dfa4768b2d494dcc0cc","original spinner span pin");
    const auto& rows=fixture.at("cases");check(rows.size()==339,"original case coverage");
    unsigned i{};for(const auto& row:rows)original_case(row,fixture.at("spinner_assets").at(std::to_string(row.at("input").at("pane").get<unsigned>())),i++);
    std::vector<std::byte> bank(0x8000);put(bank,0x73a0,5);put(bank,0x1258,1);put(bank,0x110,5);
    unsigned calls{};VistaLobbySpinnerSessionQuery query=[&]()->std::optional<std::span<const std::byte>>{++calls;return bank;};
    check(vista_lobby_spinner_player_present(0xffffffffu,{})==0,"NONE player consumed no membership service");
    check(vista_lobby_spinner_player_present(0x10000u,query)==1,"occupied source low16 ID semantics");
    check(vista_lobby_spinner_peer_loading(0,query)==1 && calls==3,"peer predicate made both actual membership reads");
    VistaLobbySpinnerSessionQuery lost=[&]()->std::optional<std::span<const std::byte>>{if(++calls%2)return bank;return {};};calls=0;
    check(vista_lobby_spinner_peer_loading(0,lost)==0 && calls==2,"second original membership failure retained");
    rejects([&]{vista_lobby_spinner_peer_loading(0x10000u,query);},"unowned full-ID peer record accepted");
    rejects([&]{vista_lobby_spinner_player_present(0,{});},"missing consumed session service accepted");
    VistaLobbySpinnerSessionQuery short_bank=[]()->std::optional<std::span<const std::byte>>{return std::span<const std::byte>{};};
    rejects([&]{vista_lobby_spinner_player_present(0,short_bank);},"truncated bank accepted");
    VistaLobbySpinnerState state;check(state.counter==0 && state.frame==2,"source initializer values");
    rejects([&]{vista_lobby_spinner(state,{},1,{});},"missing source player bank accepted");
    rejects([&]{vista_lobby_spinner(state,{},0,{});},"missing consumed bitmap lookup accepted");
    std::cout<<"PASS "<<checks<<" checks,339 original PC14 spinner cases, exact state/frame/service sequence; no visible integration claim\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
