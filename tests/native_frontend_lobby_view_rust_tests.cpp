#include "halo2/frontend_lobby.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include <nlohmann/json.hpp>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
struct NativeFailure:std::exception {const char*what()const noexcept override{return "native lobby read failed";}};
struct Observation {
  std::vector<std::string> calls;int fail=-1;std::exception_ptr error;const NativeFailure *original{};
  bool default_override{};std::optional<int> default_result;
  void read(std::string call){const auto i=int(calls.size());calls.push_back(std::move(call));if(i==fail)std::rethrow_exception(error);}
  std::optional<int> default_value(unsigned engine,unsigned parameter)const {
    return default_override?default_result:source_game_variant_get(source_game_variant_default(engine),int(parameter));
  }
};
Observation *observed;
}
#include "frontend_lobby_view_before.inc"
namespace {
namespace before=halo2::lobby_view_before;
template<class T>Json optional(const std::optional<T>&v){return v?Json(*v):Json(nullptr);}
Json presentation(const FrontendLobbyPresentation &p){
  return { {"pane",p.pane},{"buttons",optional(p.buttons)},{"leader_text",optional(p.leader_text)},
    {"leader_visible",optional(p.leader_visible)},{"start",optional(p.start_eligible)},
    {"status",optional(p.status_string)},{"warning",p.status_warning},{"subtitle",p.subtitle_string},
    {"privacy_text",optional(p.privacy_text)},{"privacy_visible",optional(p.privacy_visible)},
    {"privacy",optional(p.privacy_string)},{"stats_text",optional(p.prior_stats_text)},
    {"stats_visible",optional(p.prior_stats_visible)} };
}
template<class T>Json help(const T &h){return {{"text",optional(h.text_index)},{"visible",h.visible},{"value",optional(h.value_index)},{"string",h.string_id}};}
std::string raw(const Json&j,const char*key,std::string fallback={}){
  if(!j.contains(key))return fallback;if(j[key].is_string())return j[key].get<std::string>();
  const auto b=j[key].get<std::vector<unsigned char>>();return {reinterpret_cast<const char*>(b.data()),b.size()};
}
Json byte_rows(const std::vector<std::optional<std::string>>&rows){
  Json out=Json::array();for(const auto &row:rows){if(!row){out.push_back(nullptr);continue;}
    std::vector<unsigned char>b(row->begin(),row->end());out.push_back(b);}return out;
}
template<std::size_t N>Json byte_rows(const std::array<std::optional<std::string>,N>&rows){return byte_rows(std::vector<std::optional<std::string>>{rows.begin(),rows.end()});}
struct Io final:FrontendLobbyViewIo {
  const FrontendContent &content;const std::map<unsigned,int> &values;Observation &probe;
  Io(const FrontendContent&c,const std::map<unsigned,int>&v,Observation&p):content(c),values(v),probe(p){}
  std::optional<std::vector<std::uint32_t>>category(unsigned id)const override{
    probe.read("category:"+std::to_string(id));auto i=content.categories.find(id);return i==content.categories.end()?std::nullopt:std::optional{i->second};}
  std::optional<UiParameter>parameter(std::uint32_t tag)const override{
    probe.read("parameter:"+std::to_string(tag));auto i=content.parameters.find(tag);return i==content.parameters.end()?std::nullopt:std::optional{i->second};}
  std::optional<int>value(unsigned id)const override{
    probe.read("value:"+std::to_string(id));auto i=values.find(id);return i==values.end()?std::nullopt:std::optional{i->second};}
  std::optional<int>default_value(unsigned engine,unsigned id)const override{
    probe.read("default:"+std::to_string(engine)+":"+std::to_string(id));return probe.default_value(engine,id);}
};
template<class F>Json capture(Observation &p,F&&f){
  Json out;
  try{out["value"]=f();}catch(const NativeFailure&e){out["error"]="native";out["same_object"]=(&e==p.original);}
  catch(int value){out["error"]="int";out["integer"]=value;}
  catch(const std::exception&e){out["error"]="unexpected";out["what"]=e.what();}
  out["calls"]=p.calls;return out;
}
Observation probe(const Json&j){
  Observation p;p.fail=j.value("fail",-1);p.default_override=j.contains("default");
  if(p.default_override&&!j["default"].is_null())p.default_result=j["default"].get<int>();
  try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}
  catch(const NativeFailure&e){p.original=&e;p.error=std::current_exception();}
  catch(...){p.error=std::current_exception();}return p;
}
Json run(const Json&j){
  const auto kind=j.value("kind",std::string{"presentation"});
  if(kind=="session"){
    FrontendLobbySessionInput f;f.session_class=j.value("class",-1);f.live_subtype=j.value("subtype",0);
    f.campaign_id=j.value("campaign",-1);f.settings_valid=j.value("valid",false);
    if(j.contains("variant")&&!j["variant"].is_null())f.variant_state=j["variant"].get<int>();
    return {{"before",before::frontend_lobby_session_enum(f)},{"rust",frontend_lobby_session_enum(f)}};
  }
  if(kind=="scalars"){
    const int a=j.value("a",-1),b=j.value("b",0),c=j.value("c",0),d=j.value("d",0),e=j.value("e",0);
    const unsigned u=j.value("u",0u);const auto difficulty=std::int16_t(j.value("difficulty",0));
    auto out=[&](bool old){return Json{{"pane",old?before::frontend_lobby_pane(a,b):frontend_lobby_pane(a,b)},
      {"protocol",old?before::frontend_lobby_protocol_string(a):frontend_lobby_protocol_string(a)},
      {"type",old?optional(before::frontend_lobby_variant_type(u)):optional(frontend_lobby_variant_type(u))},
      {"icon",old?before::frontend_lobby_game_icon(a):frontend_lobby_game_icon(a)},
      {"category",old?optional(before::frontend_lobby_quick_category(a)):optional(frontend_lobby_quick_category(a))},
      {"difficulty",old?before::frontend_lobby_difficulty_string(difficulty):frontend_lobby_difficulty_string(difficulty)},
      {"help",old?help(before::frontend_lobby_help(a,b,c,d,e)):help(frontend_lobby_help(a,b,c,d,e))},
      {"count",old?before::frontend_lobby_player_count(a,b,c):frontend_lobby_player_count(a,b,c)},
      {"count_string",old?optional(before::frontend_lobby_player_count_string(a)):optional(frontend_lobby_player_count_string(a))}};};
    return {{"before",out(true)},{"rust",out(false)}};
  }
  if(kind=="quick"||kind=="default"){
    FrontendContent content;std::map<unsigned,int>values;
    if(j.contains("categories"))for(const auto&x:j["categories"])content.categories[x.at("id").get<unsigned>()]=x.at("rows").get<std::vector<std::uint32_t>>();
    if(j.contains("parameters"))for(const auto&x:j["parameters"]){UiParameter p;p.parameter=x.value("parameter",0u);p.title=raw(x,"title");
      if(x.contains("choices"))for(const auto&choice:x["choices"])p.choices.push_back({raw(choice,"label"),choice.value("value",0)});
      content.parameters[x.at("tag").get<std::uint32_t>()]=std::move(p);}
    if(j.contains("values"))for(const auto&x:j["values"])values[x.at("id").get<unsigned>()]=x.at("value").get<int>();
    const auto engine=j.value("engine",0u),parameter=j.value("parameter",0u),count=j.value("count",6u);
    const auto separator=raw(j,"separator",": ");auto p=probe(j),q=probe(j);Io io{content,values,q};observed=&p;
    Json out;
    out["before"]=capture(p,[&]()->Json{
      if(kind=="default")return optional(before::frontend_lobby_parameter_default(engine,parameter));
      if(count==0)return byte_rows(before::frontend_variant_quick_options<0>(content,engine,values,separator));
      if(count==5)return byte_rows(before::frontend_variant_quick_options<5>(content,engine,values,separator));
      return byte_rows(before::frontend_variant_quick_options<6>(content,engine,values,separator));});
    out["rust"]=capture(q,[&]()->Json{
      if(kind=="default")return optional(frontend_lobby_parameter_default_rust(engine,parameter,io));
      return byte_rows(frontend_variant_quick_options_rust(engine,separator,count,io));});
    if(!j.contains("default")&&p.fail<0){
      if(kind=="default")out["production"]=optional(frontend_lobby_parameter_default(engine,parameter));
      else out["production"]=byte_rows(frontend_variant_quick_options_rows(content,engine,values,separator,count));
    }
    return out;
  }
  FrontendLobbyInput f;
  f.session_enum=j.value("session",-1);f.previous_pane=j.value("previous",0);f.host=j.value("host",false);
  f.team_mode=j.value("teams",false);f.local_team_selection=j.value("local_team",false);f.countdown=j.value("countdown",false);
  f.start_permission=j.value("permission",false);f.update_guard=j.value("guard",false);f.status=j.value("status",-1);
  f.delayed_player=j.value("delayed",-1);f.countdown_cancelled=j.value("cancelled",false);f.loading_map=j.value("loading",false);
  f.editing_options=j.value("editing",false);f.team_count=j.value("team_count",1);f.player_count=j.value("players",0);
  f.max_players=j.value("maximum",0);f.overflow_status_string=j.value("overflow",0u);f.party_privacy=j.value("privacy",0);f.prior_stats_count=j.value("stats",0);
  return {{"before",presentation(before::frontend_lobby_presentation(f))},{"rust",presentation(frontend_lobby_presentation(f))}};
}
}
int main()try{
  for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
