#include "halo2/frontend_matchmaking_presentation.hpp"
#include "halo2/frontend_matchmaking_progress.hpp"
#include "halo2/original_matchmaking_widgets.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
using Json=nlohmann::json;
using namespace halo2;
namespace before {
using namespace halo2;
struct Failure:std::runtime_error {using std::runtime_error::runtime_error;};
struct Observation {
  std::vector<std::string> calls;
  int fail{-1};std::exception_ptr exception;
  bool variant_available{true};std::array<std::optional<std::string>,6> options;
  std::string original,pc,authored,line,name;
  std::uint64_t party_count{};
  void read(std::string label) {
    const auto i=int(calls.size());calls.push_back(std::move(label));
    if(i==fail)std::rethrow_exception(exception);
  }
};
Observation *observed{};
struct Map {std::string scenario_path,name;};
struct Maps {
  std::vector<Map> rows;
  auto begin()const{observed->read("map");return rows.begin();}
  auto end()const{return rows.end();}
};
struct Text {std::uint32_t string{};};
struct Pane {std::vector<Text> texts;};
struct Screen {unsigned id{};std::vector<Pane> panes;};
struct Bank {
  std::map<unsigned,Screen> screens;
  const Screen &screen(unsigned id)const{return screens.at(id);}
  std::string localized(const Screen &,std::uint32_t id)const{observed->read("authored:"+std::to_string(id));return observed->authored;}
};
struct FrontendContent {std::optional<Bank> vista;std::map<unsigned,Screen> screens;Maps multiplayer_maps;};
enum class OnlineMatchmakingStatus {Unconfigured,Idle,Connecting,Ready,Joining,Searching,CheckingConnections,AwaitingTransport,Loading,Failed};
struct Count {std::uint64_t value{};std::uint64_t size()const{return value;}};
struct OnlinePlaylist {std::string id,name,description;bool eligible{},ranked{},clan_match{};unsigned minimum_players{},maximum_players{};std::optional<unsigned> level;};
struct Match {Count players;std::string variant,map;};
using OnlineMatch=Match;
struct Gathering {Count players;unsigned minimum_players{};};
struct OnlineMatchmakingSnapshot {OnlineMatchmakingStatus status{};std::vector<OnlinePlaylist> playlists;std::optional<Match> match;std::optional<Gathering> gathering;std::string message;unsigned queued_players{};};
struct UiVariant {int engine{};};
std::optional<UiVariant> frontend_matchmaking_variant(const FrontendContent &,const Match &) {
  observed->read("variant");return observed->variant_available?std::optional<UiVariant>(UiVariant{3}):std::nullopt;
}
using Settings=std::array<int,112>;
Settings source_game_variant(const UiVariant &){return {};}
int source_game_variant_get(const Settings &,int id){observed->read("get:"+std::to_string(id));return id;}
std::array<std::optional<std::string>,6> frontend_matchmaking_quick_options(const FrontendContent &,int,const std::map<unsigned,int> &queries,const std::string &) {
  if(queries.size()!=112)throw std::logic_error("source getter census lost");
  for(unsigned i=0;i<112;++i)if(queries.at(i)!=int(i))throw std::logic_error("source getter input changed");
  observed->read("quick-options");return observed->options;
}
namespace detail {
std::string frontend_compiled_localized(const FrontendContent &,const Screen &,std::uint32_t id){observed->read("compiled14:"+std::to_string(id));return "default option";}
std::string frontend_pc_compiled_localized(const Bank &,const Screen &,std::uint32_t id){observed->read("pc:"+std::to_string(id));return observed->pc;}
namespace og_matchmaking_layout {
std::string original_text(unsigned s,unsigned p,unsigned i){observed->read("text:"+std::to_string(s)+":"+std::to_string(p)+":"+std::to_string(i));return observed->original;}
std::string original_string(unsigned s,std::uint32_t id){observed->read("string:"+std::to_string(s)+":"+std::to_string(id));return observed->name;}
}}
struct MenuFrame {unsigned screen{},selected{};};
struct Party {bool leader{};};
struct Account {std::optional<Party> party;std::string party_message;};
struct Search {
  std::string id;int seconds{-1},state{};
  std::string selected()const{return id;}int countdown()const{return seconds;}unsigned title()const{return unsigned(state);}
};
class MenuState {
public:
  FrontendContent content_;OnlineMatchmakingSnapshot matchmaking_;Account account_;Search matchmaking_search_;
  unsigned pane_{};int ui_{};
  unsigned online_matchmaking_progress_pane()const{return pane_;}
  MatchmakingProgressUi online_matchmaking_ui()const{return static_cast<MatchmakingProgressUi>(ui_);}
  unsigned online_matchmaking_pane()const{return matchmaking_progress_pc_pane(online_matchmaking_ui(),matchmaking_search_.seconds);}
  Count online_party_members()const{observed->read("party");return {observed->party_count};}
  std::string expand(const std::string &v)const{observed->read("expand");return "expanded["+v+"]";}
  std::string online_matchmaking_original_text(unsigned,unsigned)const;
  std::string online_matchmaking_text(const MenuFrame &,unsigned)const;
};
#include "frontend_presentation_reference.inc"
} // namespace before

std::string raw(const Json &j,const char *key,std::string fallback={}) {
  if(!j.contains(key))return fallback;
  if(j[key].is_string())return j[key].get<std::string>();
  const auto v=j[key].get<std::vector<std::uint8_t>>();return {reinterpret_cast<const char *>(v.data()),v.size()};
}
Json bytes(const std::string &v){Json out=Json::array();for(unsigned char c:v)out.push_back(c);return out;}
FrontendTextFacts facts(const Json &j) {
  FrontendTextFacts f;f.screen=j.value("screen",210u);f.selected_row=j.value("row",0u);f.selected_id=raw(j,"selected");
  f.status=j.value("status",5u);f.ui=j.value("ui",3);f.pane=j.value("pane",0u);f.title=j.value("title",3);f.countdown=j.value("countdown",-1);f.pc_progress=j.value("pc",false);
  if(j.contains("matched")&&!j["matched"].is_null())f.matched=j["matched"].get<std::uint64_t>();
  if(j.contains("gathering")&&!j["gathering"].is_null())f.gathering=j["gathering"].get<std::uint64_t>();
  f.minimum=j.value("minimum",0u);f.queued=j.value("queued",0u);f.variant=raw(j,"variant","variant");f.map=raw(j,"map","map");f.message=raw(j,"message");
  f.party=j.value("party",false);f.leader=j.value("leader",false);f.party_message=raw(j,"party_message");
  if(j.contains("playlists"))for(const auto &p:j["playlists"]) {
    FrontendTextPlaylist q;q.id=raw(p,"id");q.name=raw(p,"name");q.description=raw(p,"description");q.eligible=p.value("eligible",false);q.ranked=p.value("ranked",false);q.clan=p.value("clan",false);
    q.minimum=p.value("minimum",0u);q.maximum=p.value("maximum",0u);if(p.contains("level")&&!p["level"].is_null())q.level=p["level"].get<unsigned>();f.playlists.push_back(std::move(q));
  }return f;
}
before::MenuState state(const FrontendTextFacts &f) {
  before::MenuState s;s.pane_=f.pane;s.ui_=f.ui;s.matchmaking_search_={f.selected_id,f.countdown,f.title};s.matchmaking_.status=static_cast<before::OnlineMatchmakingStatus>(f.status);
  s.matchmaking_.message=f.message;s.matchmaking_.queued_players=f.queued;
  for(const auto &p:f.playlists)s.matchmaking_.playlists.push_back({p.id,p.name,p.description,p.eligible,p.ranked,p.clan,p.minimum,p.maximum,p.level});
  if(f.matched)s.matchmaking_.match=before::Match{{*f.matched},f.variant,f.map};
  if(f.gathering)s.matchmaking_.gathering=before::Gathering{{*f.gathering},f.minimum};
  if(f.party)s.account_.party=before::Party{f.leader};s.account_.party_message=f.party_message;
  before::Screen screen;screen.id=210;for(unsigned p=0;p<3;++p)screen.panes.push_back({{{100+p}}});
  if(f.pc_progress){s.content_.vista.emplace();s.content_.vista->screens.emplace(210,screen);}
  s.content_.screens.emplace(14,screen);s.content_.multiplayer_maps.rows={{"levels/shared/map", "display map"},{"levels/shared/other","other map"}};
  return s;
}
FrontendTextIo readers(before::MenuState &s) {
  FrontendTextIo io;io.original_text=before::detail::og_matchmaking_layout::original_text;io.original_string=before::detail::og_matchmaking_layout::original_string;
  io.pc_string=[&](std::uint32_t id){return before::detail::frontend_pc_compiled_localized(*s.content_.vista,s.content_.vista->screen(210),id);};
  io.pc_authored=[&](unsigned p){const auto &screen=s.content_.vista->screen(210);return s.content_.vista->localized(screen,screen.panes.at(p).texts.at(0).string);};
  io.map_name=[&]{return before::matched_map_name(s.content_,s.matchmaking_.match->map);};
  io.pc_match_line=[&]{
    const auto &screen=s.content_.vista->screen(210);
    const auto localized=[&](std::uint32_t id){return before::detail::frontend_pc_compiled_localized(*s.content_.vista,screen,id);};
    return before::replace_token(localized(matchmaking_variant_pc),"\xee\x90\x8a",s.matchmaking_.match->variant)+" "+
        before::replace_token(localized(matchmaking_on_map_pc),"\xee\x90\x88",before::matched_map_name(s.content_,s.matchmaking_.match->map));
  };
  io.quick_option=[&](unsigned i){
    const auto variant=before::matched_variant(s.content_,*s.matchmaking_.match);if(!variant)return std::string{};
    std::map<unsigned,int> queries;const auto settings=before::source_game_variant(*variant);
    for(unsigned id=0;id<112;++id)queries.emplace(id,before::source_game_variant_get(settings,int(id)));
    const auto options=before::frontend_matchmaking_quick_options(s.content_,variant->engine,queries,
        before::detail::frontend_compiled_localized(s.content_,s.content_.screens.at(14),0x1a001820));
    return options.at(i).value_or("");
  };
  io.party_count=[&]{return s.online_party_members().size();};io.expand=[&](const std::string &v){return s.expand(v);};return io;
}
Json execute(const Json &j) {
  const auto f=facts(j);auto s=state(f);auto io=readers(s);const bool original=j.value("original",false);const unsigned pane=j.value("query_pane",f.pane),index=j.value("index",0u);
  before::Observation o;o.fail=j.value("fail",-1);o.variant_available=j.value("variant_available",true);o.party_count=j.value("party_count",std::uint64_t(1));
  o.original=raw(j,"original_template","raw \xee\x90\xa5 / \xee\x90\xa5 / \xee\x91\x9b / \xee\x90\x8a / \xee\x90\x88");
  o.pc=raw(j,"pc_template","pc \xee\x91\xb8 \xee\x90\x8a \xee\x90\x88");o.authored=raw(j,"authored_template","authored");o.name=raw(j,"source_string","needed %d");
  for(unsigned i=0;i<6;++i)o.options[i]=i%2?std::nullopt:std::optional<std::string>("option "+std::to_string(i));
  const std::string failure_bytes("bad\xff\0native",11);
  o.exception=j.value("nonstd",false)?std::make_exception_ptr(1234567):std::make_exception_ptr(before::Failure(failure_bytes));
  const void *expected{};try{std::rethrow_exception(o.exception);}catch(const before::Failure &e){expected=&e;}catch(...){}
  before::observed=&o;
  auto run=[&](bool rust){
    Json result;
    try{result["bytes"]=bytes(rust?frontend_matchmaking_text_rust(f,original,pane,index,io):
      original?s.online_matchmaking_original_text(pane,index):s.online_matchmaking_text({f.screen,f.selected_row},index));result["error"]=nullptr;}
    catch(const before::Failure &e){result["error"]="native";result["same_object"]=&e==expected;result["what"]=bytes(std::string(e.what(),4));}
    catch(int e){result["error"]="int";result["value"]=e;}
    catch(const std::exception &e){result["error"]="other";result["what"]=bytes(e.what());}
    result["calls"]=o.calls;o.calls.clear();return result;
  };
  const auto old=run(false);const auto actual=run(true);return {{"before",old},{"rust",actual}};
}
int main()try{
  std::string line;while(std::getline(std::cin,line))std::cout<<execute(Json::parse(line)).dump()<<"\n";
}catch(const std::exception &e){std::cerr<<e.what()<<"\n";return 1;}
