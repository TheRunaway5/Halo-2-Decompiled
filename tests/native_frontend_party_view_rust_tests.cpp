#include "halo2/frontend.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <type_traits>
using namespace halo2;
using Json=nlohmann::json;
namespace bridge=halo2::online_protocol_bridge;
namespace {
std::string raw(const Json&j,const char*key,std::string fallback={}){
  if(!j.contains(key))return fallback;if(j[key].is_string())return j[key].get<std::string>();
  const auto bytes=j[key].get<std::vector<unsigned char>>();return {reinterpret_cast<const char*>(bytes.data()),bytes.size()};
}
Json bytes(const std::string&s){Json out=Json::array();for(unsigned char c:s)out.push_back(c);return out;}
rust::Vec<std::uint8_t> encoded(const std::string&s){rust::Vec<std::uint8_t> out;for(unsigned char c:s)out.push_back(c);return out;}
Json frame(const MenuFrame&f){Json entries=Json::array();for(const auto&e:f.entries)entries.push_back({{"label",bytes(e.label)},{"destination",e.destination},{"value",e.value}});
  return {{"title",bytes(f.title)},{"description",bytes(f.description)},{"entries",entries},{"screen",f.screen},{"context",f.context}};}
struct NativeFailure:std::exception {const char*what()const noexcept override{return "native party view read failed";}};
struct Observation {
  Json calls=Json::array();int fail=-1;std::exception_ptr exception;const NativeFailure*original{};
  const MenuFrame *frame{};const FrontendMatchmakingPartyFlow *flow{};unsigned reentries{};
  void read(std::string kind,Json arguments=Json::array()){
    const auto at=int(calls.size());calls.push_back({{"kind",kind},{"arguments",arguments},{"frame",::frame(*frame)}});
    if(flow){(void)flow->state();++reentries;}
    if(at==fail)std::rethrow_exception(exception);
  }
};
Observation *observed{};const Json *configuration{};
std::string resource(const std::string&key){
  observed->read(key);
  const auto found=configuration->find("resources");
  if(found!=configuration->end()&&found->contains(key))return raw(*found,key.c_str());
  return key;
}
struct Screens {
  std::map<unsigned,UiScreen> values;
  const UiScreen&at(unsigned id)const{observed->read("direct-screen",{id});return values.at(id);}
};
struct Content {
  Screens screens;bool original{};
  const UiScreen&screen(unsigned id)const{observed->read("screen",{id});return screens.values.at(id);}
  std::string localized(const UiScreen&s,std::uint32_t id)const{return resource("locale:"+std::to_string(s.id)+":"+std::to_string(id));}
};
struct Lobby {
  int value=-1;bool matching{};
  int seconds()const{observed->read("seconds");return value;}
  bool matchmaking()const{observed->read("matchmaking");return matching;}
};
struct Search {std::string value;std::string selected()const{observed->read("selected");return value;}};
struct Data {
  Content content_;OnlineAccountSnapshot account_;OnlineVoiceSnapshot voice_;bool voice_party_{},online_voice_{};
  Lobby online_lobby_;Search matchmaking_search_;FrontendMatchmakingPartyFlow party_flow_;
  std::vector<OnlinePartyMember> projected;std::optional<FrontendLobbyView> view;bool stats{};
  std::string start_error;
  std::vector<OnlinePartyMember>online_party_members()const{observed->read("members");return projected;}
  std::optional<FrontendLobbyView>lobby_view()const{observed->read("view");return view;}
  bool has_online_prior_stats()const{observed->read("stats");return stats;}
  std::string online_lobby_start_error()const{observed->read("start-error");return start_error;}
  std::string expand(const std::string&s)const{observed->read("expand",bytes(s));return "<"+s+">";}
};
Json original_layout;
}
namespace controlled_detail {
bool mcc_frontend_content(const Content&c){observed->read("theme");return c.original;}
std::string frontend_compiled_localized(const Content&,const UiScreen&s,std::uint32_t id){return resource("compiled:"+std::to_string(s.id)+":"+std::to_string(id));}
namespace og_matchmaking_layout {
const Json&data(){observed->read("original-data");return original_layout;}
std::string original_string(unsigned screen,std::uint32_t id){return resource("original-string:"+std::to_string(screen)+":"+std::to_string(id));}
std::string original_text(unsigned screen,unsigned pane,unsigned index){return resource("original-text:"+std::to_string(screen)+":"+std::to_string(pane)+":"+std::to_string(index));}
}
}
namespace before {
using namespace halo2;namespace detail=controlled_detail;
constexpr unsigned invite_context=45;
class MenuState:public Data {
public:void populate_online_party(MenuFrame&)const;std::string online_party_text(const MenuFrame&,unsigned)const;
};
#include "frontend_party_view_before.inc"
}
namespace after {
using namespace halo2;namespace detail=controlled_detail;
class MenuState:public Data {
public:void populate_online_party(MenuFrame&)const;std::string online_party_text(const MenuFrame&,unsigned)const;
  void party_view_populate(MenuFrame&)const;std::string party_view_text(const MenuFrame&,unsigned)const;
};
#include "frontend_party_view_after.inc"
#include "frontend_party_view_adapter.inc"
}
namespace {
// Establish production-valid targets through the actual Rust party controller,
// rather than adding a test-only state setter to the public native API.
struct Setup final:bridge::NativeFrontendPartyFlowIo {
  std::string target;unsigned kind{};bool picker{};
  bridge::PartyFlowFacts party_flow_facts()const override{
    bridge::PartyFlowFacts f{};f.mode=3;f.party=true;
    bridge::PartyFlowFrame frame{};frame.screen=picker?31:26;frame.context=picker?45:2;frame.instance=1;
    frame.entries.push_back({31,kind==1?1000:kind==2?2000:0});f.frames.push_back(std::move(frame));
    f.invitations.push_back({encoded(target),encoded("target"),1});f.outgoing.push_back({encoded(target)});return f;
  }
  rust::Vec<bridge::PartyMemberData> party_flow_members()const override{rust::Vec<bridge::PartyMemberData>out;bridge::PartyMemberData p{};p.handle=encoded(target);p.controller=-1;out.push_back(std::move(p));return out;}
  bool party_flow_command(const bridge::PartyFlowCommand&)const override{return false;}
  std::uint64_t party_flow_serial()const override{return 0;}
  void party_flow_update()const override{}
  void party_flow_cancel()const override{}
  bool party_flow_lobby(std::int32_t)const override{return false;}
  void party_flow_ui(const bridge::PartyFlowUi&)const override{}
};
OnlinePartyMember member(const Json&j){OnlinePartyMember p;p.handle=raw(j,"handle");p.name=raw(j,"name");p.self=j.value("self",false);p.leader=j.value("leader",false);p.local_controller=j.value("controller",-1);return p;}
Data data(const Json&j){
  Data d;d.content_.original=j.value("original",false);
  for(unsigned id:{14u,26u})if(!j.value(id==14?"missing14":"missing26",false)){
    UiScreen s;s.id=id;s.header=0x3000+id;s.panes.resize(j.value("panes",3u));
    if(s.panes.size()>2){auto&p=s.panes[2];p.buttons.resize(j.value("buttons",2u));p.texts.resize(j.value("texts",49u));
      for(unsigned i=0;i<p.buttons.size();++i)p.buttons[i].text.string=0x50000000+i;
      for(unsigned i=0;i<p.texts.size();++i)p.texts[i].string=0x60000000+i;}
    d.content_.screens.values.emplace(id,std::move(s));}
  d.account_.friends_message=raw(j,"friends_message");d.account_.party_message=raw(j,"party_message");d.account_.message=raw(j,"account_message");
  if(j.value("party",false)){d.account_.party.emplace();auto&p=*d.account_.party;p.leader=j.value("leader",false);p.countdown_supported=j.value("countdown_supported",false);
    if(j.contains("members"))for(const auto&v:j["members"])p.members.push_back(member(v));
    if(j.contains("outgoing"))for(const auto&v:j["outgoing"])p.outgoing.push_back({raw(v,"id"),raw(v,"name")});}
  if(j.contains("invitations"))for(const auto&v:j["invitations"])d.account_.party_invitations.push_back({raw(v,"id"),raw(v,"name"),0});
  if(j.contains("friends"))for(const auto&v:j["friends"]){OnlineFriend p;p.name=raw(v,"name");p.state=raw(v,"state");p.presence=raw(v,"presence");d.account_.friends.push_back(std::move(p));}
  if(j.value("clan",false)){d.account_.clan.emplace();d.account_.clan->role=j.value("role",0u);
    if(j.contains("clan_members"))for(const auto&v:j["clan_members"]){OnlineClanMember p;p.name=raw(v,"name");p.self=v.value("self",false);p.presence=static_cast<OnlineClanPresence>(v.value("presence",0u));d.account_.clan->members.push_back(std::move(p));}}
  if(j.contains("projected"))for(const auto&v:j["projected"])d.projected.push_back(member(v));
  d.voice_party_=j.value("voice_party",false);d.online_voice_=j.value("voice_client",false);d.voice_.joined=j.value("joined",false);d.voice_.connecting=j.value("connecting",false);d.voice_.microphone_muted=j.value("microphone_muted",false);
  if(j.contains("muted"))for(const auto&id:j["muted"]){Json v={{"id",id}};d.voice_.muted.push_back(raw(v,"id"));}
  d.online_lobby_.value=j.value("seconds",-1);d.online_lobby_.matching=j.value("matchmaking",false);d.matchmaking_search_.value=raw(j,"selected");d.stats=j.value("stats",false);d.start_error=raw(j,"start_error","start error");
  if(j.value("view",false)){d.view.emplace();auto&v=*d.view;v.protocol=j.value("protocol",2);v.machines=j.value("machines",1);auto&i=v.input;
    i.session_enum=j.value("session",6);i.previous_pane=j.value("previous",0);i.host=j.value("host",false);i.team_mode=j.value("teams",false);i.local_team_selection=j.value("local_team",false);i.countdown=j.value("countdown",false);i.start_permission=j.value("permission",false);i.update_guard=j.value("guard",false);i.status=j.value("status",-1);i.delayed_player=j.value("delayed",-1);i.countdown_cancelled=j.value("cancelled",false);i.loading_map=j.value("loading",false);i.editing_options=j.value("editing",false);i.team_count=j.value("team_count",1);i.player_count=j.value("players",0);i.max_players=j.value("maximum",0);i.overflow_status_string=j.value("overflow",0u);i.party_privacy=j.value("privacy",0);i.prior_stats_count=j.value("prior_stats",0);}
  d.party_flow_.voice_party(d.voice_party_);
  Setup setup;setup.target=raw(j,"target");setup.kind=j.value("kind",0u);d.party_flow_.run(FrontendPartyFlowAction::Accept,setup);
  if(j.value("tab",0u)){setup.picker=true;d.party_flow_.run(FrontendPartyFlowAction::SwitchTab,setup,1);}
  return d;
}
MenuFrame menu_frame(const Json&j){MenuFrame f;f.screen=j.value("screen",14u);f.context=j.value("context",0u);f.title=raw(j,"title","old title");f.description=raw(j,"description","old description");
  if(j.contains("entries"))for(const auto&e:j["entries"])f.entries.push_back({raw(e,"label"),e.value("destination",-1),e.value("value",0)});return f;}
Observation observation(const Json&j){Observation p;p.fail=j.value("fail",-1);try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}catch(const NativeFailure&e){p.original=&e;p.exception=std::current_exception();}catch(...){p.exception=std::current_exception();}return p;}
template<class M>Json run_one(const Json&j){M m;static_cast<Data&>(m)=data(j);auto f=menu_frame(j);auto p=observation(j);p.frame=&f;p.flow=&m.party_flow_;observed=&p;Json result;
  try{if(j.value("operation",std::string{"populate"})=="text")result["text"]=bytes(m.online_party_text(f,j.value("index",0u)));else m.populate_online_party(f);}
  catch(const NativeFailure&e){result["error"]="native";result["same_object"]=(&e==p.original);}
  catch(int value){result["error"]="int";result["integer"]=value;}
  catch(const std::out_of_range&e){result["error"]="out_of_range";result["what"]=e.what();}
  catch(const Json::exception&e){result["error"]="json";result["what"]=e.what();}
  catch(const FormatError&e){result["error"]="format";result["what"]=e.what();}
  catch(const std::exception&e){result["error"]="unexpected";result["what"]=e.what();}
  result["frame"]=frame(f);result["calls"]=p.calls;result["reentries"]=p.reentries;result["flow"]={{"target",bytes(m.party_flow_.target())},{"kind",m.party_flow_.kind()},{"tab",m.party_flow_.tab()}};return result;}
Json run(const Json&j){
  configuration=&j;original_layout=Json::object();auto &screen=original_layout["original"]["screens"]["14"];
  screen["header"]=0x10000014u;screen["panes"]=Json::array({Json::object(),Json::object(),Json::object()});
  screen["panes"][2]["buttons"]=Json::array();
  for(unsigned id:{0x20000010u,0x20000011u})screen["panes"][2]["buttons"].push_back({{"text",{{"string",id}}}});
  if(j.value("missing_original",false))original_layout=Json::object();
  return {{"before",run_one<before::MenuState>(j)},{"rust",run_one<after::MenuState>(j)}};}
}
int main()try{for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
