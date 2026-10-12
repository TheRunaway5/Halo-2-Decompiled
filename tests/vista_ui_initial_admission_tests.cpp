#include "halo2/vista_ui_initial_admission.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using nlohmann::json;
namespace {
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto byte:bytes) {const auto v=std::to_integer<unsigned>(byte);out+=digits[v>>4];out+=digits[v&15];}
  return out;
}
void assign(std::span<std::byte> dst,const std::string& value) {
  require(value.size()==dst.size()*2,"Fixture storage length differs");
  for(std::size_t i=0;i<dst.size();++i)dst[i]=std::byte(std::stoul(value.substr(i*2,2),nullptr,16));
}
std::uint32_t get(std::span<const std::byte> b,std::size_t at) {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));
}
bool compared_event(const json& e) {
  const auto kind=e.at(0).get<std::string>();
  return kind=="listener" || kind=="resource_context" || kind=="signin_query" ||
    kind=="apply_signin" || kind=="special_ui" || kind=="cancel_overlapped";
}
}
int main(int argc,char** argv)try {
  require(argc==2,"Expected original initial UI admission fixture path");
  std::ifstream input(argv[1]);json fixture;input>>fixture;std::size_t count{};
  for(const auto& row:fixture.at("rows")) {
    std::array<std::byte,0x610> ui;assign(ui,row.at("before").get<std::string>());
    std::array<std::byte,0x1b44> manager{};
    assign(std::span(manager).subspan(0x1b40),row.at("manager_before").get<std::string>());
    std::array<std::byte,0xbc> game{};assign(std::span(game).first(8),row.at("game_before").get<std::string>());
    std::array<std::byte,0x1c0> profile;assign(profile,row.at("profile_before").get<std::string>());
    std::array<std::byte,0x28> user{};put(user,0xc,row.at("identity_state").get<std::uint32_t>());
    json events=json::array();halo2::VistaUiContextConstructionServices services;
    services.create_listener=[&](std::uint64_t mask) {
      events.push_back({"listener",std::uint32_t(mask),std::uint32_t(mask>>32)});
      return row.at("event_handle").get<std::uint32_t>();
    };
    services.resource_context_available=[&] {events.push_back({"resource_context"});return false;};
    services.initialize_localized_messages=[](auto) {throw std::runtime_error("Null source resource context reached localization");};
    halo2::vista_ui_context_initialize(ui,row.at("listener_enabled").get<bool>(),services);
    require(hex(ui)==row.at("constructor").get<std::string>(),"Complete source UI constructor bytes differ");
    const auto query=[&](std::uint32_t controller) {
      events.push_back({"signin_query",controller});return halo2::vista_ui_xuser_signin_state(user);
    };
    const auto apply=[&](std::uint32_t state) {
      events.push_back({"apply_signin",state});require(state==1,"Fixture must use actual offline sign-in state");
      halo2::vista_ui_apply_fresh_offline_signin(ui,manager,game,profile,
        [&] {events.push_back({"special_ui",get(ui,8)});return false;},
        [&](std::span<std::byte> overlapped) {events.push_back({"cancel_overlapped",hex(overlapped)});});
    };
    const auto polled=halo2::vista_ui_initial_signin_poll(ui,query,apply);
    require(polled==(row.at("listener_enabled").get<bool>() && row.at("event_handle").get<std::uint32_t>()!=0),
      "Initial poll's real listener guard differs");
    require(hex(ui)==row.at("after").get<std::string>(),"Original complete first-poll UI bytes differ");
    require(hex(std::span(manager).subspan(0x1b40))==row.at("manager").get<std::string>(),"Original manager context bytes differ");
    require(hex(std::span(game).first(8))==row.at("game_root").get<std::string>(),"Original game context bytes differ");
    require(hex(profile)==row.at("game_profile").get<std::string>(),"Original complete profile-service bytes differ");
    json original=json::array();for(const auto& e:row.at("events"))if(compared_event(e))original.push_back(e);
    if(original!=events) {std::cerr<<"Expected "<<original<<"\nActual "<<events<<'\n';throw std::runtime_error("Original initial admission service order differs");}
    const auto previous=events.size();
    require(!halo2::vista_ui_initial_signin_poll(ui,query,apply),"Initial identity must not be reapplied on a second frame");
    require(events.size()==previous,"Second initial poll unexpectedly called platform/source owners");
    ++count;
  }
  for(const auto& row:fixture.at("queries")) {
    std::array<std::byte,0x28> user{};put(user,0xc,row.at("state").get<std::uint32_t>());
    require(halo2::vista_ui_xuser_signin_state(user)==row.at("result").get<std::uint32_t>(),"Actual supplied-DLL XUser sign-in query differs");
  }
  require(count==24,"Original initial admission fixture count differs");
  std::cout<<"PASS original UI constructor and initial offline poll cases="<<count<<" complete UI/manager/game/profile bytes and cancel order\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
