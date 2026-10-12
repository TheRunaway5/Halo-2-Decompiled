#include "halo2/vista_pcr_owner_state.hpp"
#include "halo2/vista_session_birth.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
void check(bool value,const char* why) {if(!value)throw std::runtime_error(why);}
void put(std::span<std::byte> b,unsigned offset,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)b[offset+i]=std::byte(value>>(i*8));
}
std::array<std::byte,8> identity(const std::string& hex) {
  check(hex.size()==16,"Invalid original identity fixture");std::array<std::byte,8> out;
  for(unsigned i=0;i<8;++i)out[i]=std::byte(std::stoul(hex.substr(i*2,2),nullptr,16));
  return out;
}
template<class F>void rejects(F f) {
  try {f();}catch(const std::exception&){return;}
  throw std::runtime_error("Missing PCR owner admission rejection");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Provide original PCR owner-state fixture");std::ifstream file(argv[1]);
  check(bool(file),"Cannot open original PCR owner-state fixture");nlohmann::json rows;file>>rows;
  unsigned focus_count{},voice_count{},initialization_count{};
  for(const auto& row:rows) {
    const auto kind=row.at("kind").get<std::string>();
    if(kind=="focus") {
      VistaPcrResults bank;bank.player_count=row.at("players");bank.team_count=row.at("teams");
      const auto before=bank;const auto actual=vista_pcr_initial_focus(bank);
      check(actual.pane==row.at("pane").get<unsigned>() && actual.selected_row==row.at("row").get<std::uint32_t>(),
        "Native PCR focus differs from original protected pane and populated list owners");
      check(bank==before,"Native PCR focus mutated its retained results");++focus_count;
    } else if(kind=="voice") {
      std::vector<std::byte> session(vista_session_source_size+32,std::byte{0x6d});
      std::fill_n(session.begin(),vista_session_source_size,std::byte{});
      put(session,0x73a0,row.at("state"));put(session,0x1258,row.at("mask"));
      const auto query=identity(row.at("identity"));
      for(const auto& value:row.at("matches")) {
        const auto slot=value.get<unsigned>();std::copy(query.begin(),query.end(),session.begin()+0x125c+slot*0x128);
      }
      const auto before=session;const auto enabled=row.at("enabled").get<unsigned>()!=0;
      const auto manager=row.at("manager").get<std::uint32_t>();
      check(vista_pcr_session_identity_slot(session,enabled,manager,query)==row.at("slot").get<std::uint32_t>(),
        "Native PCR identity mapping differs from actual live session slot lookup");
      VistaPcrResults bank;bank.player_count=1;std::copy(query.begin(),query.end(),bank.players[0].begin()+0x100);
      const auto actual=vista_pcr_offline_voice(bank,0,row.at("lobby").get<std::uint8_t>(),session,enabled,manager);
      check(actual.visible==bool(row.at("visible").get<unsigned>()) && actual.frame==row.at("frame").get<unsigned>(),
        "Native PCR offline voice differs from original source and real supplied DLL");
      check(session==before,"Native PCR voice modified its source bank or suffix");++voice_count;
    } else if(kind=="voice_initialization") {
      check(row.at("engine_available").get<unsigned>()==0 && row.at("hresult").get<std::uint32_t>()==0x88780078u,
        "Supplied DLL voice owner contract changed");++initialization_count;
    } else throw std::runtime_error("Unknown original PCR owner-state fixture");
  }
  VistaPcrResults bad;bad.player_count=17;rejects([&]{vista_pcr_initial_focus(bad);});
  bad.player_count=0;bad.team_count=17;rejects([&]{vista_pcr_initial_focus(bad);});
  rejects([&]{vista_pcr_offline_voice({},0,0,{},false,0);});
  check(vista_pcr_session_identity_slot({},false,4,{})==0xffffffffu &&
    vista_pcr_session_identity_slot({},true,0,{})==0xffffffffu,"Unconsumed session owner was required");
  rejects([&]{vista_pcr_session_identity_slot({},true,4,{});});
  VistaPcrResults lobby;lobby.player_count=1;
  check(!vista_pcr_offline_voice(lobby,0,1,{},true,4).visible,"PC185 consumed a live session voice owner");
  std::cout<<focus_count<<" original populated-focus cases, "<<voice_count<<" original live identity/voice cases and "
    <<initialization_count<<" actual supplied-DLL voice initializations passed\n";
  return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
