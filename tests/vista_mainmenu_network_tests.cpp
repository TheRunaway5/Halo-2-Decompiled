#include "halo2/vista_mainmenu_network.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using nlohmann::json;
namespace {
void require(bool value,const char* message) {
  if(!value)throw std::runtime_error(message);
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto byte:bytes) {
    const auto v=std::to_integer<unsigned>(byte);out+=digits[v>>4];out+=digits[v&15];
  }
  return out;
}
}
int main(int argc,char** argv)try {
  require(argc==2,"Expected original Network fixture path");
  std::ifstream input(argv[1]);json cases;input>>cases;
  std::size_t count{};
  for(const auto& row:cases) {
    const auto& q=row.at("input");json events=json::array();std::size_t special_index{};
    std::array<std::byte,0xac> root{};root[0]=std::byte{0x83};root[1]=std::byte{0x84};
    root[2]=std::byte{0x85};root[3]=std::byte{0x86};
    std::array<std::array<std::byte,8>,41> banks{};
    std::array<halo2::VistaGameStateResetChild,41> children;
    for(std::size_t i=0;i<children.size();++i) {
      banks[i][4]=std::byte(0x80+i);banks[i][5]=std::byte(0xf0-i);
      banks[i][6]=std::byte{0x71};banks[i][7]=std::byte{0x72};
      children[i]={banks[i],[&,i](std::uint32_t value){events.push_back({"child_reset",i,value});}};
    }
    halo2::VistaMainmenuNetworkServices s;
    s.pbuffer_get_byte=[&](std::uint8_t& byte) {
      events.push_back({"pbuffer",0,byte});
      if(q.at("pbuffer")=="valid")byte=q.at("byte").get<std::uint8_t>();
      events.push_back({"pbuffer_return",q.at("pbuffer")=="valid"?0u:0xffffffffu,byte});
    };
    s.reset_game_state=[&] {
      events.push_back({"game_state"});events.push_back({"reset"});
      halo2::vista_mainmenu_reset_game_state(root,children);
    };
    s.network_ready=[&] {
      const auto raw=q.at("ready").get<std::uint32_t>();events.push_back({"network_ready",raw});return raw!=0;
    };
    s.special_ui=[&] {
      const auto raw=q.at("special").at(special_index++).get<std::uint32_t>();
      events.push_back({"special",raw});return raw!=0;
    };
    s.controller_mask=[&] {
      events.push_back({"controller_mask"});std::uint32_t mask{};
      for(unsigned i=0;i<4;++i)if(q.at("flags").at(i).get<std::uint32_t>()&8)mask|=1u<<i;
      return mask;
    };
    s.dialog=[&](const auto& values) {
      json event=json::array({"dialog"});for(auto v:values)event.push_back(v);events.push_back(event);
    };
    s.ui_signin_context=[&] {
      const auto context=q.at("signin_context").get<std::uint32_t>();
      events.push_back({"ui_context",context});return context;
    };
    s.create_screen=[&](const auto& values) {
      json event=json::array({"factory"});for(auto v:values)event.push_back(v);events.push_back(event);
    };
    s.request_profile_admission=[&](std::uint32_t zero,std::uint32_t callback) {
      events.push_back({"admission",0xd712c8,zero,callback});
    };
    (void)halo2::vista_mainmenu_open_network(q.at("controller").get<std::uint32_t>(),s);
    if(events!=row.at("events")) {
      std::cerr<<"Case "<<q<<"\nExpected "<<row.at("events")<<"\nActual "<<events<<'\n';
      throw std::runtime_error("Original Network ordered service sequence differs");
    }
    require(hex(std::span(root).first(4))==row.at("root").get<std::string>(),"Original game-state root clear differs");
    for(std::size_t i=0;i<banks.size();++i)
      require(hex(std::span(banks[i]).subspan(4))==row.at("children").at(i).get<std::string>(),"Original child flags differ");
    ++count;
  }
  require(count==2160,"Original Network corpus count differs");
  bool refused{};
  try{(void)halo2::vista_mainmenu_open_network(0,{});}catch(const halo2::FormatError&){refused=true;}
  require(refused,"Missing Network owners must reject");
  std::array<std::byte,0xac> root{};std::array<halo2::VistaGameStateResetChild,41> missing{};
  refused=false;try{halo2::vista_mainmenu_reset_game_state(root,missing);}catch(const halo2::FormatError&){refused=true;}
  require(refused,"Missing child reset owners must reject");
  std::cout<<"PASS original PC6 Network cases="<<count<<" complete 41-child reset and required owners\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
