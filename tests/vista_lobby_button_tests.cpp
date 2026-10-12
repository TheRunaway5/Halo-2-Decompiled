#include "halo2/vista_lobby_button.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
template<class F> void rejects(F&& f) {
  try{f();}catch(const FormatError&){return;}
  throw FormatError("Unowned original button service was not rejected");
}
std::uint32_t bits(std::int32_t value){return std::bit_cast<std::uint32_t>(value);}
Json units(std::span<const char16_t> value) {
  auto out=Json::array();for(auto c:value)out.push_back(std::uint16_t(c));return out;
}
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;
  Json rows;input>>rows;unsigned count{},chat_cases{},factories{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");Json events=Json::array();
    VistaLobbyButtonEvent event{q.at("controller").get<std::uint32_t>()};
    auto ordinal=q.at("ordinal").get<std::uint16_t>();
    VistaLobbyButtonState state{q.at("chat").get<std::uint8_t>(),q.at("teams").get<std::array<std::uint8_t,4>>()};
    VistaLobbyButtonChat buffer{};for(unsigned i=0;i<buffer.size();++i)buffer[i]=char16_t(0x200+i);
    std::u16string text;if(!q.at("text").is_null())for(const auto& ch:q.at("text"))text.push_back(char16_t(ch.get<std::uint16_t>()));
    const auto& mutations=q.at("mutations");std::vector<bool> applied(mutations.size());
    const auto mutate=[&](std::string_view name) {
      for(unsigned i=0;i<mutations.size();++i) {
        const auto& m=mutations[i];if(applied[i] || m.at("at")!=name)continue;applied[i]=true;
        if(m.contains("controller"))event.controller=m.at("controller");
        if(m.contains("ordinal"))ordinal=m.at("ordinal");
        if(m.contains("chat"))state.chat_enabled=m.at("chat");
        if(m.contains("teams"))state.team_edit=m.at("teams").get<std::array<std::uint8_t,4>>();
      }
    };
    VistaLobbyButtonServices s;unsigned language{};
    s.chat_owner=[&]{events.push_back({"chat_owner"});mutate("chat_owner");++chat_cases;return std::uintptr_t{1};};
    s.chat_characters=[&](auto owner)->std::optional<std::u16string_view> {
      events.push_back({"chat_characters",owner});mutate("chat_characters");
      if(q.at("text").is_null())return {};
      return text;
    };
    s.set_errno=[&](auto value){events.push_back({"set_errno",value});mutate("set_errno");};
    s.invalid_parameter=[&]{events.push_back({"invalid_parameter"});mutate("invalid_parameter");};
    s.language=[&]{auto value=q.at("languages").at(language++).get<std::uint32_t>();events.push_back({"language",value});mutate("language"+std::to_string(language));return std::bit_cast<std::int32_t>(value);};
    s.keyboard=[&](auto controller,auto kind,auto data,auto capacity){events.push_back({"keyboard",bits(controller),kind,units(data),capacity});mutate("keyboard");};
    s.game_details=[&]{events.push_back({"game_details"});mutate("game_details");};
    s.signed_in=[&](auto controller)->std::uint8_t{events.push_back({"signed_in",bits(controller),q.at("signed_in")});mutate("signed_in");return std::uint8_t(q.at("signed_in").get<std::uint32_t>());};
    s.host=[&]()->std::uint8_t{events.push_back({"host",q.at("host")});mutate("host");return std::uint8_t(q.at("host").get<std::uint32_t>());};
    s.finish_team_edit=[&](auto controller,auto value){events.push_back({"finish_team_edit",bits(controller),value});mutate("finish_team_edit");};
    s.setup_ready=[&]()->std::uint8_t{events.push_back({"setup_ready",q.at("ready")});mutate("setup_ready");return std::uint8_t(q.at("ready").get<std::uint32_t>());};
    s.settings_change=[&](auto value){events.push_back({"settings_change",value});mutate("settings_change");};
    s.cancel_action=[&](auto controller){events.push_back({"cancel_action",bits(controller)});mutate("cancel_action");};
    s.readiness=[&]{auto value=q.at("readiness").get<std::uint32_t>();events.push_back({"readiness",0,value});mutate("readiness");return std::bit_cast<std::int32_t>(value);};
    s.start_action=[&](auto controller){events.push_back({"start_action",bits(controller)});mutate("start_action");};
    s.task_factory=[&](auto factory,const auto& descriptor){events.push_back({"task_factory",factory,descriptor});mutate("task_factory");++factories;};
    vista_lobby_button(event,ordinal,state,buffer,s);
    const Json final{{"controller",event.controller},{"ordinal",ordinal},{"chat",state.chat_enabled},{"teams",state.team_edit},{"buffer",units(buffer)}};
    if(events!=row.at("events") || final!=row.at("final")) {
      std::cerr<<"button fixture "<<count<<" "<<q<<"\nactual "<<events<<"\noriginal "<<row.at("events")<<"\nfinal "<<final<<"\noriginal final "<<row.at("final")<<'\n';
      throw FormatError("Original Vista button output/call-order mismatch");
    }
    ++count;
  }
  VistaLobbyButtonEvent event{0xffffffff};std::uint16_t ordinal=3;
  VistaLobbyButtonState state{};VistaLobbyButtonChat chat{};VistaLobbyButtonServices s;
  bool details{};s.game_details=[&]{details=true;};vista_lobby_button(event,ordinal,state,chat,s);
  if(!details)throw FormatError("Game Details incorrectly depended on sign-in or controller storage");
  ordinal=0;rejects([&]{vista_lobby_button(event,ordinal,state,chat,s);});
  s.signed_in=[](auto){return std::uint8_t{1};};s.host=[]{return std::uint8_t{1};};
  rejects([&]{vista_lobby_button(event,ordinal,state,chat,s);});
  event.controller=0;ordinal=4;state.chat_enabled=1;s.chat_owner=[]{return std::uintptr_t{1};};
  s.chat_characters=[](auto)->std::optional<std::u16string_view>{return {};};
  bool errno_written{};s.set_errno=[&](auto value){if(value!=22)throw FormatError("Incorrect null source errno");errno_written=true;};
  s.invalid_parameter=[]{throw FormatError("Declared terminating invalid-parameter boundary");};
  rejects([&]{vista_lobby_button(event,ordinal,state,chat,s);});
  if(!errno_written || chat[0])throw FormatError("Invalid-parameter effects did not precede handler");
  std::cout<<"PASS "<<count<<" original Vista button cases (chat="<<chat_cases<<", factories="<<factories<<") and explicit missing/invalid owner boundaries\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
