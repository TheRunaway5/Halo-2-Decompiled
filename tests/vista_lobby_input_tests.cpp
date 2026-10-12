#include "halo2/vista_lobby_input.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
const char* name(VistaLobbyInputRoute r) {
  switch(r) {
  case VistaLobbyInputRoute::consumed:return "consumed";
  case VistaLobbyInputRoute::base_input:return "base_input";
  case VistaLobbyInputRoute::start_and_base_input:return "start_and_base_input";
  case VistaLobbyInputRoute::setup:return "setup";
  case VistaLobbyInputRoute::leave:return "leave";
  case VistaLobbyInputRoute::team_edit:return "team_edit";
  case VistaLobbyInputRoute::unsigned_controller:return "unsigned_controller";
  }throw FormatError("Unknown route");
}
}
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream file(argv[1]);Json rows;file>>rows;unsigned count{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");Json events=Json::array(),returned;std::string route;
    const auto byte=[&](const char* key){return q.at(key).get<std::uint8_t>();};
    VistaLobbyCountdownServices c;
    c.duration_context=[&]{events.push_back({"duration"});};
    c.controller_user=[&](auto controller){events.push_back({"user",controller,q.at("user")});return q.at("user").get<std::int32_t>();};
    c.session_stage=[&]{events.push_back({"stage",q.at("stage")});return q.at("stage").get<std::int32_t>();};
    c.host_permission=[&]{events.push_back({"host",q.at("host")});return byte("host");};
    c.ui_state=[&]{events.push_back({"ui",q.at("ui")});return q.at("ui").get<std::int32_t>();};
    c.write=[&](auto active,auto stage,auto reason,auto user){events.push_back({"write",active,stage,reason,user});return byte("write");};
    VistaLobbyInputServices s;
    s.pregame=[&]{events.push_back({"pregame",q.at("pregame")});return byte("pregame");};
    s.channel_in_use=[&](auto channel){events.push_back({"channel",channel,q.at("channel")});return byte("channel");};
    s.signed_in=[&](auto controller){events.push_back({"signed",controller,q.at("signed")});return byte("signed");};
    s.team_edit_active=[&](auto){events.push_back({"team",q.at("edit")});return byte("edit");};
    s.host_permission=[&]{events.push_back({"host",q.at("host")});return byte("host");};
    s.ready=[&]{events.push_back({"ready",q.at("ready")});return byte("ready");};
    s.countdown_active=[&]{events.push_back({"active",q.at("active")});return byte("active");};
    s.start_action=[&](auto controller){events.push_back({"start",controller});};
    s.cancel_action=[&](auto controller){events.push_back({"cancel",controller,q.at("cancel")});return byte("cancel");};
    s.settings_change=[&](auto value){events.push_back({"settings",value});};
    s.finish_team_edit=[&](auto controller){events.push_back({"finish",controller});};
    const auto op=q.at("op").get<std::string>();
    const auto controller=q.at("controller").get<std::int32_t>();
    if(op=="request" || op=="cancel") {
      returned=unsigned(op=="cancel"?vista_lobby_cancel_action(controller,c):
          vista_lobby_cancel_request(controller,q.at("requested"),c));route="cancel_result";
    } else {
      const auto action=op=="start"?VistaLobbyInputAction::start:op=="back"?VistaLobbyInputAction::back:
          op=="setup"?VistaLobbyInputAction::setup:VistaLobbyInputAction::auxiliary;
      route=name(vista_lobby_input(action,controller,s));
    }
    if(events!=row.at("events") || route!=row.at("route").get<std::string>() || returned!=row.at("returned")) {
      std::cerr<<"case="<<count<<" input="<<q.dump()<<"\nexpected="<<row.dump()<<"\nactual="<<events.dump()<<" route="<<route<<" returned="<<returned.dump()<<'\n';
      throw FormatError("Original Vista lobby input mismatch");
    }++count;
  }
  if(count!=6016)throw FormatError("Incomplete original corpus");
  bool rejected{};try{vista_lobby_cancel_action(0,{});}catch(const FormatError&){rejected=true;}
  if(!rejected)throw FormatError("Missing consumed cancel service accepted");
  VistaLobbyCountdownServices none;none.controller_user=[](auto){return -1;};
  if(vista_lobby_cancel_request(0,10,none))throw FormatError("Unbound user accepted");
  VistaLobbyInputServices blocked;blocked.pregame=[] {return std::uint8_t{};};
  if(vista_lobby_input(VistaLobbyInputAction::back,0,blocked)!=VistaLobbyInputRoute::consumed)
    throw FormatError("Inactive event consumed unreachable services");
  std::cout<<"PASS original Vista lobby input/cancel cases="<<count<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
