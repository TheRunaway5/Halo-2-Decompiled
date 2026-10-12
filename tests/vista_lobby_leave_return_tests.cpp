#include "halo2/vista_lobby_leave_return.hpp"
#include "halo2/vista_lobby_input.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream f(argv[1]);Json rows;f>>rows;unsigned count{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");const auto op=q.at("op").get<std::string>();Json events=Json::array();
    VistaLobbyLeaveReturnState state{q.at("cached")};std::uint32_t returned{};
    if(op=="suffix") {
      VistaLobbyLeaveReturnServices s;
      s.protocol=[&](){auto v=q.at("protocol").get<std::uint32_t>();events.push_back({"protocol",v});return v;};
      s.request_pending=[&](){auto v=q.at("pending").get<std::uint8_t>();events.push_back({"pending",v});return v;};
      s.controller_mask=[&](){auto v=q.at("mask").get<std::uint32_t>();events.push_back({"mask",v});return v;};
      s.screen_present=[&](std::uint32_t c,std::uint32_t w,std::uint32_t screen){auto v=q.at(screen==13?"present13":"present6").get<std::uint8_t>();events.push_back({"present",c,w,screen,v});return v;};
      s.create_screen=[&](const auto& descriptor){Json e=Json::array({"factory"});for(auto word:descriptor)e.push_back(word);events.push_back(e);};
      const auto requested=vista_lobby_leave_observe(state,q.at("screen"),q.at("old"),q.at("context"),s);
      if(requested!=(!events.empty() && events.back()[0]=="factory"))throw FormatError("Factory request projection mismatch");
    } else {
      std::array<std::byte,4*0x127c> controllers{};
      for(unsigned i=0;i<4;++i){auto v=q.at("flags")[i].get<std::uint32_t>();for(unsigned n=0;n<4;++n)controllers[i*0x127c+n]=std::byte(v>>(8*n));}
      returned=op=="count"?vista_lobby_controller_count(controllers):vista_lobby_controller_mask(controllers);
    }
    if(events!=row.at("events") || state.cached_protocol!=row.at("cached").get<std::uint32_t>() ||
       (op!="suffix" && returned!=row.at("returned").get<std::uint32_t>())) {
      std::cerr<<"case="<<count<<" expected="<<row.dump()<<"\nactual="<<events.dump()<<" cache="<<state.cached_protocol<<" returned="<<returned<<'\n';
      throw FormatError("Original lobby Leave suffix/controller mismatch");
    }++count;
  }
  if(count!=730)throw FormatError("Incomplete original Leave return corpus");
  VistaLobbyLeaveReturnState state{3};VistaLobbyLeaveReturnServices s;
  s.request_pending=[](){return 0;};s.controller_mask=[](){return 1;};bool rejected{};
  try{vista_lobby_leave_observe(state,6,2,0,s);}catch(const FormatError&){rejected=true;}
  if(!rejected || state.cached_protocol!=3)throw FormatError("Unknown Leave return fabricated a screen");
  std::cout<<"PASS original Vista Leave suffix and selected-controller leaves cases="<<count<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
