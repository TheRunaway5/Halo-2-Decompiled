#include "halo2/vista_widget_grid_identifiers.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t root=0x1a00000,grid=root+0x1000;
constexpr std::array<std::uint32_t,7> identities{root,root+0x100,grid,grid+0x100,grid+0x200,grid+0x300,grid+0x400};
unsigned checks{};
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
std::array<std::byte,0x80> bytes(const std::string& text) {
  if(text.size()!=0x100)throw std::runtime_error("fixture bank extent");
  std::array<std::byte,0x80> b{};
  for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::stoul(text.substr(i*2,2),nullptr,16));
  return b;
}
std::string hex(std::span<const std::byte> b) {
  constexpr char d[]="0123456789abcdef";std::string text;
  for(auto byte:b){const auto n=std::to_integer<unsigned>(byte);text+=d[n>>4];text+=d[n&15];}return text;
}
std::uint32_t read(std::span<const std::byte> b,std::size_t at) {
  if(at+4>b.size())throw std::runtime_error("fixture read extent");
  std::uint32_t value{};
  for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned char>(b[at+i]))<<(8*i);
  return value;
}
void write(std::span<std::byte> b,std::size_t at,std::uint32_t value) {
  if(at+4>b.size())throw std::runtime_error("fixture write extent");
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(value>>(8*i));
}
struct Run {
  const Json& input;
  std::map<std::uint32_t,std::array<std::byte,0x80>> banks;
  Json events=Json::array();
  VistaLobbyIdentifierServices services;
  explicit Run(const Json& row):input(row.at("input")) {
    for(const auto& r:row.at("initial"))banks.emplace(r.at("identity").get<std::uint32_t>(),bytes(r.at("bank")));
    services.bank=[this](auto id)->std::span<std::byte>{return banks.at(id.value);};
    services.observed=[this](auto id){event("identifier",id);};
    services.child_method=[this](auto id){dispatch(id);};
  }
  void event(const char* name,VistaWidgetIdentity id) {
    auto visible=Json::array();for(auto a:identities)visible.push_back(std::to_integer<unsigned>(banks.at(a)[0x6e]));
    events.push_back(Json::array({name,id.value,read(banks.at(root),0x7c),visible}));
  }
  void dispatch(VistaWidgetIdentity id) {
    const auto vtable=read(banks.at(id.value),0);
    if(vtable==0x7d0614) {
      event("grid",id);vista_widget_grid_assign_identifiers(id,services);
    } else if(vtable==0x7cf29c)vista_lobby_assign_widget_identifiers(id,services);
    else if(vtable==root+0x7000) {
      event("external",id);const auto shape=input.at("shape").get<unsigned>();
      if(shape==3){write(banks.at(id.value),0x18,grid+0x200);banks.at(grid)[0x6e]=std::byte{0xdf};}
      if(shape==4)write(banks.at(id.value),0x18,0);
      if(shape==5){banks.at(grid)[0x6e]=std::byte{0xee};write(banks.at(root),0x7c,0xffffffffu);}
    } else throw FormatError("Unowned fixture child virtual method");
  }
  void execute(const Json& row) {
    dispatch({grid});check(events==row.at("events"),"ordered grid/identifier/service trace differs");
    for(const auto& r:row.at("final"))check(hex(banks.at(r.at("identity").get<std::uint32_t>()))==r.at("bank").get<std::string>(),"retained source bank differs");
    check(banks.at(grid)[0x6e]==std::byte{},"grid visibility must be cleared after recursion");
  }
};
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Usage: vista_widget_grid_identifiers_tests fixture.json");
    std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","original source pin");
    unsigned cases{};
    for(const auto& row:fixture.at("cases")){Run run(row);run.execute(row);++cases;}
    Run short_bank(fixture.at("cases")[3]);
    short_bank.services.bank=[&](auto id)->std::span<std::byte>{auto b=std::span<std::byte>(short_bank.banks.at(id.value));return id.value==grid?b.first(0x6e):b;};
    bool rejected{};
    try{vista_widget_grid_assign_identifiers({grid},short_bank.services);}catch(const FormatError&){rejected=true;}
    check(rejected,"truncated grid visibility owner must fail closed");
    check(short_bank.banks.at(grid)[0x6e]==std::byte{0xff},"unowned write must not alter retained visibility");
    std::cout<<"Vista grid identifiers: "<<cases<<" original cases, "<<checks<<" checks PASS\n";return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
