#include "halo2/vista_lobby_layout.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::string utf8(std::u16string_view s) {
  std::string out;
  for(const auto c:s) {
    const auto v=unsigned(c);
    if(v>=0xd800 && v<=0xdfff)throw FormatError("Unexpected surrogate in original lobby captions");
    if(v<128)out.push_back(char(v));
    else if(v<2048) {out.push_back(char(0xc0|(v>>6)));out.push_back(char(0x80|(v&63)));}
    else {out.push_back(char(0xe0|(v>>12)));out.push_back(char(0x80|((v>>6)&63)));out.push_back(char(0x80|(v&63)));}
  }
  return out;
}
}
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream input(argv[1]);Json rows;input>>rows;unsigned count{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");const auto op=q.at("op").get<std::string>();
    VistaLobbyLayoutState state;
    state.flags=q.at("flags");state.pane=q.at("pane");state.chat_enabled=q.at("chat");
    state.team_edit=q.at("edits").get<decltype(state.team_edit)>();state.buttons_allowed=q.at("before_allowed");
    state.remembered_focus=q.at("remembered").is_null()?std::nullopt:
        std::optional{VistaLobbyLayoutWidget(q.at("remembered").get<unsigned>())};
    state.normal_bounds=q.at("normal_bounds").get<VistaLobbyLayoutRect>();state.host_bounds=q.at("host_bounds").get<VistaLobbyLayoutRect>();
    for(unsigned i=0;i<5;++i) {
      state.widgets[i].visible=q.at("visible").at(i);state.widgets[i].enabled=q.at("enabled").at(i);
      state.widgets[i].bounds=q.at("widget_bounds").at(i).get<VistaLobbyLayoutRect>();
    }
    VistaLobbyLayoutText text{q.at("text_visible"),q.at("text_bounds").get<VistaLobbyLayoutRect>()};
    VistaLobbyLayoutBitmap bitmap{q.at("favorite_visible")};
    std::optional<unsigned> focus=q.at("focus").is_null()?std::nullopt:std::optional{q.at("focus").get<unsigned>()};
    Json events=Json::array(),registration,caption;
    const auto byte=[&](const char* key){return q.at(key).get<std::uint8_t>();};
    VistaLobbyLayoutServices s;
    s.host=[&]{events.push_back({"host",q.at("host")});return byte("host");};
    s.countdown_active=[&]{events.push_back({"active",q.at("active")});return byte("active");};
    s.factory_flag=[&]{events.push_back({"factory",q.at("factory")});return byte("factory");};
    s.player_count=[&]{events.push_back({"count",q.at("count")});return q.at("count").get<std::int32_t>();};
    s.text=[&](auto i){events.push_back({"text",i,q.at("text_exists")});return byte("text_exists")?&text:nullptr;};
    s.focused=[&](auto id){const auto i=unsigned(id);const bool found=focus==i;events.push_back({"focused",i,unsigned(found)});return std::uint8_t(found);};
    s.focus_change=[&](auto id,auto controller,auto select){const auto i=unsigned(id);events.push_back({"focus_change",i,controller,select});
      if(select)focus=i;else if(focus==i)focus.reset();};
    s.activate_focus=[&](auto id){const auto i=unsigned(id);events.push_back({"activate",i});focus=i;};
    s.session_enum=[&]{events.push_back({"enum",q.at("enum")});return q.at("enum").get<std::int32_t>();};
    s.select_pane=[&](auto pane){events.push_back({"pane",pane});};
    s.bitmap=[&](auto i){events.push_back({"bitmap",i,q.at("favorite_exists")});return byte("favorite_exists")?&bitmap:nullptr;};
    s.favorite=[&]{events.push_back({"favorite",q.at("favorite")});return byte("favorite");};
    if(op=="registration") {
      events.push_back({"factory",q.at("factory")});
      const auto r=vista_lobby_layout_registration(byte("factory"),byte("chat"));registration=Json::array();
      for(unsigned i=0;i<r.count;++i)registration.push_back(unsigned(r.buttons[i]));
    } else if(op=="caption") {
      events.push_back({"factory",q.at("factory")});
      if(byte("factory"))events.push_back({"language",q.at("language")});
      if(const auto value=vista_lobby_layout_players_caption(byte("factory"),q.at("language"))) {
        caption=utf8(*value);events.push_back({"caption",3,caption});
      }
    } else if(op=="pane")vista_lobby_layout_pane(state,s);
    else vista_lobby_layout_visibility(state,s);
    Json actual={{"visible",Json::array()},{"enabled",Json::array()},{"widget_bounds",Json::array()},
      {"text_visible",text.visible},{"text_bounds",text.bounds},{"allowed",state.buttons_allowed},
      {"remembered",state.remembered_focus?Json(unsigned(*state.remembered_focus)):Json()},
      {"pane",state.pane},{"favorite_visible",bitmap.visible}};
    for(const auto& w:state.widgets) {actual["visible"].push_back(w.visible);actual["enabled"].push_back(w.enabled);actual["widget_bounds"].push_back(w.bounds);}
    //6116D2 executes in the original corpus. It is an internal exact rectangle
    //write in the port, so verify the resulting bytes rather than invent a
    //callback for it. Every actual external service/focus call stays ordered.
    Json expected_events=Json::array();
    for(const auto& e:row.at("events"))if(e.at(0)!="bounds")expected_events.push_back(e);
    if(events!=expected_events || actual!=row.at("state") ||
        registration!=row.at("registration") || caption!=row.at("caption") ||
        (focus?Json(*focus):Json())!=row.at("focus")) {
      std::cerr<<"case="<<count<<" input="<<q.dump()<<"\nexpected="<<row.dump()<<"\nactual="<<actual.dump()
          <<" events="<<events.dump()<<" registration="<<registration.dump()<<" caption="<<caption.dump()<<" focus="<<(focus?Json(*focus):Json()).dump()<<'\n';
      throw FormatError("Original Vista lobby layout mismatch");
    }
    ++count;
  }
  if(count!=787)throw FormatError("Incomplete original Vista lobby layout corpus");
  VistaLobbyLayoutState bypass;bypass.flags=2;vista_lobby_layout_visibility(bypass,{});
  bool missing{};try{VistaLobbyLayoutState reached;vista_lobby_layout_visibility(reached,{});}catch(const FormatError&){missing=true;}
  if(!missing)throw FormatError("Missing consumed layout service accepted");
  if(vista_lobby_layout_players_caption(0,99))throw FormatError("Unconsumed caption language consulted");
  std::cout<<"PASS original Vista lobby layout cases="<<count<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
