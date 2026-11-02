#include "halo2/frontend.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/frontend_multiplayer_maps.hpp"
#include "halo2/frontend_lobby_ready.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <type_traits>
using namespace halo2;
using Json=nlohmann::json;
namespace bridge=halo2::online_protocol_bridge;
namespace {
std::string raw(const Json&j,const char*key,std::string fallback={}){if(!j.contains(key))return fallback;if(j[key].is_string())return j[key].get<std::string>();const auto b=j[key].get<std::vector<unsigned char>>();return {reinterpret_cast<const char*>(b.data()),b.size()};}
Json bytes(const std::string&s){Json j=Json::array();for(unsigned char c:s)j.push_back(c);return j;}
Json appearance_bytes(const FrontendAppearance&a){const auto*p=reinterpret_cast<const unsigned char*>(&a);return std::vector<unsigned char>(p,p+sizeof a);}
FrontendAppearance appearance(const Json&j){FrontendAppearance a;if(j.is_array()){const auto b=j.get<std::vector<unsigned char>>();if(b.size()!=sizeof a)throw std::runtime_error("appearance fixture requires all nine bytes");std::memcpy(&a,b.data(),sizeof a);}return a;}
struct Noop final:bridge::NativeFrontendLobbyQueryIo {
 bridge::LobbyQueryFacts lobby_query_facts()const override{return {};}
 rust::Vec<std::uint8_t> lobby_query_start_error()const override{throw std::logic_error("reentry query escaped absent mode");}
 std::int32_t lobby_query_seconds()const override{throw std::logic_error("reentry seconds");}
 rust::Vec<bridge::LobbyQueryMember> lobby_query_members()const override{throw std::logic_error("reentry members");}
 rust::Vec<std::int32_t> lobby_query_locals()const override{throw std::logic_error("reentry locals");}
 bool lobby_query_has_stats()const override{throw std::logic_error("reentry stats");}
 std::int32_t lobby_query_prior_count()const override{throw std::logic_error("reentry prior");}
 rust::Vec<std::uint8_t> lobby_query_campaign_file(std::uint32_t)const override{throw std::logic_error("reentry campaign");}
 std::int32_t lobby_query_profile_option(std::uint32_t,std::uint32_t)const override{throw std::logic_error("reentry profile");}
 void lobby_query_variant()const override{throw std::logic_error("reentry variant");}
 bool lobby_query_defusal()const override{throw std::logic_error("reentry defusal");}
 std::int32_t lobby_query_value(std::uint32_t)const override{throw std::logic_error("reentry value");}
 void lobby_query_appearance()const override{throw std::logic_error("reentry appearance");}
 bool lobby_query_custom()const override{throw std::logic_error("reentry custom");}
 std::int32_t lobby_query_capacity(std::uint32_t)const override{throw std::logic_error("reentry capacity");}
 void lobby_query_quick(std::uint32_t,rust::Slice<const bridge::LobbyQueryValue>,std::uint32_t,std::uint32_t)const override{throw std::logic_error("reentry quick");}
};
struct NativeFailure:std::exception {const char*what()const noexcept override{return "native lobby query read failed";}};
struct Observation {Json calls=Json::array();int fail=-1;std::exception_ptr exception;const NativeFailure*original{};unsigned reentries{};
 void read(const std::string&kind,Json args=Json::array()){
  const auto n=int(calls.size());calls.push_back({{"kind",kind},{"arguments",args}});
  Noop io;if(bridge::frontend_lobby_query(io).present)throw std::logic_error("reentry manufactured a view");++reentries;
  if(n==fail)std::rethrow_exception(exception);
 }
};
Observation*observed{};const Json*configuration{};
struct CampaignFiles {std::array<const char*,15> values{{"00a_introduction","01a_tutorial","01b_spacestation","03a_oldmombasa","03b_newmombasa","04a_gasgiant","04b_floodlab","05a_deltaapproach","05b_deltatowers","06a_sentinelwalls","06b_floodzone","07a_highcharity","08a_deltacliffs","07b_forerunnership","08b_deltacontrol"}};
 const char*at(unsigned n)const{observed->read("campaign",{n});return values.at(n);}
}campaign_files;
struct Screens {std::map<unsigned,UiScreen> values;const UiScreen&at(unsigned n)const{observed->read("screen",{n});return values.at(n);}};
struct Content {Screens screens;std::vector<UiLobbyMap> campaign_maps,multiplayer_maps;};
struct Variant {std::map<unsigned,int> values;bool defusal{},custom{};int capacity{};};
struct Settings {std::map<unsigned,int> values;};
Settings source_game_variant(const Variant&v){observed->read("settings");return {v.values};}
int source_game_variant_get(const Settings&s,int n){observed->read("value",{n});const auto i=s.values.find(unsigned(n));return i==s.values.end()?0:i->second;}
bool bomb_defusal_variant(const Variant&v){observed->read("defusal");return v.defusal;}
std::array<std::optional<std::string>,5> frontend_lobby_quick_options(const Content&,unsigned e,const std::map<unsigned,int>&q,std::string_view separator){
 Json values=Json::array();for(const auto&[id,n]:q)values.push_back({id,n});observed->read("quick",{e,values,bytes(std::string(separator))});
 std::array<std::optional<std::string>,5>out;for(unsigned i=0;i<5;++i)if(i!=2)out[i]=std::string(separator)+std::to_string(q.at(i*10));return out;
}
struct Lobby {int value=-1;int seconds()const{observed->read("seconds");return value;}};
struct Results {unsigned player_count{};};struct Report {std::shared_ptr<Results>results;};
struct Data {
 FrontendMatchmakingPartyFlow party_flow_;
 Content content_;int mode_{};OnlineAccountSnapshot account_;bool voice_party_{};OnlineVoiceSnapshot voice_;
 unsigned engine_{},difficulty_{},level_{},map_{};int lobby_protocol_{},lobby_team_{};
 std::string signed_in_;FrontendLobbyTeamEdit lobby_team_edit_;std::array<FrontendLobbyTeamEdit,4>local_team_edits_;
 std::array<int,4>system_link_local_slots_{0,1,2,3};std::optional<int>session_countdown_;std::optional<bool>system_link_host_;unsigned system_link_machines_{};
 std::vector<FrontendLobbyRosterPlayer>system_link_players_;Lobby online_lobby_;std::shared_ptr<Report>online_prior_report_;
 std::vector<std::vector<OnlinePartyMember>>member_reads;mutable unsigned member_at{};
 std::vector<std::vector<FrontendLocalPlayer>>local_reads;mutable unsigned local_at{};
 Variant saved;FrontendAppearance profile_appearance;std::string start_error;bool stats{};int privacy{};
 std::string online_lobby_start_error()const{observed->read("start-error");return start_error;}
 std::vector<OnlinePartyMember>online_party_members()const{observed->read("members",{member_at});const auto n=member_at++;return n<member_reads.size()?member_reads[n]:std::vector<OnlinePartyMember>{};}
 std::vector<FrontendLocalPlayer>local_players()const{observed->read("locals",{local_at});const auto n=local_at++;return n<local_reads.size()?local_reads[n]:std::vector<FrontendLocalPlayer>{};}
 bool has_online_prior_stats()const{observed->read("stats");return stats;}
 int profile_option(unsigned s,unsigned p)const{observed->read("option",{s,p});return privacy;}
 Variant current_variant()const{observed->read("variant");return saved;}
 FrontendAppearance appearance_for_profile(const std::string&id)const{observed->read("appearance",{bytes(id)});return profile_appearance;}
};
namespace controlled_detail {
 bool frontend_custom_variant(const Variant&v){observed->read("custom");return v.custom;}
 int frontend_variant_parameter_value(const Variant&v,const UiParameter&p){observed->read("capacity",{p.parameter});return v.capacity;}
 std::string frontend_compiled_localized(const Content&,const UiScreen&s,std::uint32_t id){observed->read("localized",{s.id,id});return raw(*configuration,"separator",std::string(1,'/'));}
}
}
namespace before {
using namespace halo2;namespace detail=controlled_detail;
class MenuState:public Data {public:std::optional<FrontendLobbyView>lobby_view()const;};
#include "frontend_lobby_query_before.inc"
}
namespace after {
using namespace halo2;namespace detail=controlled_detail;
using UiVariant=::Variant;using SourceGameVariant=::Settings;
class MenuState:public Data {public:std::optional<FrontendLobbyView>lobby_view()const;std::string lobby_query_campaign_file(unsigned)const;};
#include "frontend_lobby_query_after.inc"
}
namespace {
OnlinePartyMember member(const Json&j){OnlinePartyMember p;p.handle=raw(j,"handle");p.name=raw(j,"name");p.self=j.value("self",false);p.leader=j.value("leader",false);if(j.contains("appearance"))p.appearance=appearance(j["appearance"]);return p;}
UiLobbyMap map(const Json&j){UiLobbyMap m;m.scenario_path=raw(j,"scenario");m.campaign_id=j.value("campaign_id",-1);m.name=raw(j,"name");if(j.contains("counts")){const auto c=j["counts"].get<std::vector<unsigned char>>();std::copy_n(c.begin(),std::min(c.size(),m.team_counts.size()),m.team_counts.begin());}return m;}
FrontendLobbyTeamEdit edit(const Json&j){return {j.value("valid",false),j.value("team",std::int16_t(-1))};}
Data data(const Json&j){Data d;d.mode_=j.value("mode",3);d.engine_=j.value("engine",0u);d.difficulty_=j.value("difficulty",0u);d.level_=j.value("level",0u);d.map_=j.value("map",0u);d.lobby_protocol_=j.value("protocol",0);d.lobby_team_=j.value("team",0);d.signed_in_=raw(j,"profile");d.privacy=j.value("privacy",0);
 if(j.value("party",false)){d.account_.party.emplace();auto&p=*d.account_.party;p.leader=j.value("leader",false);if(j.contains("physical"))for(const auto&v:j["physical"])p.members.push_back(member(v));
  if(j.contains("requester")){p.countdown.emplace();p.countdown->requester=raw(j,"requester");}if(j.contains("party_privacy")){p.privacy.emplace();p.privacy->mode=j["party_privacy"].get<unsigned>();}}
 d.voice_party_=j.value("voice_party",false);d.voice_.joined=j.value("joined",false);d.voice_.transmitting=j.value("transmitting",false);d.voice_.microphone_muted=j.value("microphone_muted",false);
 for(const auto&key:{"talking","muted"})if(j.contains(key))for(const auto&v:j[key]){Json item={{"value",v}};(std::string_view(key)=="talking"?d.voice_.talking:d.voice_.muted).push_back(raw(item,"value"));}
 d.online_lobby_.value=j.value("seconds",-1);d.start_error=raw(j,"start_error");d.stats=j.value("stats",false);d.online_prior_report_=std::make_shared<Report>();d.online_prior_report_->results=std::make_shared<Results>();d.online_prior_report_->results->player_count=j.value("prior_count",0u);
 if(j.contains("member_reads"))for(const auto&batch:j["member_reads"]){auto&out=d.member_reads.emplace_back();for(const auto&m:batch)out.push_back(member(m));}
 if(j.contains("local_reads"))for(const auto&batch:j["local_reads"]){auto&out=d.local_reads.emplace_back();for(const auto&team:batch){FrontendLocalPlayer p;p.team=team.get<int>();out.push_back(p);}}
 if(j.contains("countdown")&&!j["countdown"].is_null())d.session_countdown_=j["countdown"].get<int>();if(j.contains("system_host")&&!j["system_host"].is_null())d.system_link_host_=j["system_host"].get<bool>();d.system_link_machines_=j.value("machines",0u);
 if(j.contains("indices"))d.system_link_local_slots_=j["indices"].get<std::array<int,4>>();if(j.contains("edit"))d.lobby_team_edit_=edit(j["edit"]);if(j.contains("edits"))for(unsigned n=0;n<4;++n)d.local_team_edits_[n]=edit(j["edits"].at(n));
 if(j.contains("system_players"))for(const auto&v:j["system_players"]){FrontendLobbyRosterPlayer p;p.name=raw(v,"name");p.session_index=v.value("session_index",0);p.team=v.value("team",-1);p.rank_value=v.value("rank",-1);p.member_count=v.value("member_count",-1);p.voice_port_flag0_only=v.value("flag0",false);p.voice_query_53750=v.value("query",false);p.appearance_available=v.value("appearance_available",true);if(v.contains("appearance"))p.appearance=appearance(v["appearance"]);d.system_link_players_.push_back(p);}
 if(j.contains("campaign_maps"))for(const auto&v:j["campaign_maps"])d.content_.campaign_maps.push_back(map(v));if(j.contains("maps"))for(const auto&v:j["maps"])d.content_.multiplayer_maps.push_back(map(v));
 if(!j.value("missing_screen",false)){UiScreen s;s.id=14;d.content_.screens.values.emplace(14,std::move(s));}
 d.saved.custom=j.value("custom",false);d.saved.defusal=j.value("defusal",false);d.saved.capacity=j.value("capacity",0);if(j.contains("values"))for(const auto&v:j["values"])d.saved.values[v[0].get<unsigned>()]=v[1].get<int>();if(j.contains("profile_appearance"))d.profile_appearance=appearance(j["profile_appearance"]);d.party_flow_.voice_party(d.voice_party_);return d;}
Json input(const FrontendLobbyInput&i){return Json::array({i.session_enum,i.previous_pane,i.host,i.team_mode,i.local_team_selection,i.countdown,i.start_permission,i.update_guard,i.status,i.delayed_player,i.countdown_cancelled,i.loading_map,i.editing_options,i.team_count,i.player_count,i.max_players,i.overflow_status_string,i.party_privacy,i.prior_stats_count});}
Json team_edit(const FrontendLobbyTeamEdit&e){return {e.valid,e.team};}
Json view(const std::optional<FrontendLobbyView>&value,const Data&m){if(!value)return nullptr;const auto&v=*value;Json result={{"input",input(v.input)},{"protocol",v.protocol},{"machines",v.machines},{"engine",v.engine},{"difficulty",v.difficulty},{"team",v.player_team},{"team_count",v.team_count},{"allow_none",v.allow_no_team},{"edit",team_edit(v.team_edit)},{"indices",v.local_team_session_indices},{"speakers",v.party_speakers}};
 result["map"]=nullptr;for(unsigned k=0;k<2;++k){const auto&maps=k?m.content_.multiplayer_maps:m.content_.campaign_maps;for(unsigned n=0;n<maps.size();++n)if(v.map==&maps[n])result["map"]={k+1,n};}if(v.map&&result["map"].is_null())throw std::runtime_error("view escaped native map owner");
 result["appearance"]=v.player_appearance?appearance_bytes(*v.player_appearance):Json(nullptr);result["edits"]=Json::array();for(const auto&e:v.local_team_edits)result["edits"].push_back(team_edit(e));
 result["quick"]=Json::array();for(const auto&q:v.quick_options)result["quick"].push_back(q?bytes(*q):Json(nullptr));result["names"]=nullptr;if(v.party_names){result["names"]=Json::array();for(const auto&s:*v.party_names)result["names"].push_back(bytes(s));}
 result["rows"]=Json::array();for(const auto&p:v.party_players)result["rows"].push_back({{"name",bytes(p.name)},{"index",p.session_index},{"appearance",appearance_bytes(p.appearance)},{"team",p.team},{"rank",p.rank_value},{"member_count",p.member_count},{"flag0",p.voice_port_flag0_only},{"query",p.voice_query_53750},{"appearance_available",p.appearance_available}});return result;}
Observation observation(const Json&j){Observation p;p.fail=j.value("fail",-1);try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}catch(const NativeFailure&e){p.original=&e;p.exception=std::current_exception();}catch(...){p.exception=std::current_exception();}return p;}
template<class M>Json run_one(const Json&j){M m;static_cast<Data&>(m)=data(j);auto p=observation(j);observed=&p;Json result;
 try{result["view"]=view(m.lobby_view(),m);}
 catch(int n){result["error"]="int";result["integer"]=n;}
 catch(const NativeFailure&e){result["error"]="native";result["what"]=e.what();result["same_object"]=&e==p.original;}
 catch(const std::out_of_range&e){result["error"]="out_of_range";result["what"]=e.what();}
 catch(const FormatError&e){result["error"]="format";result["what"]=e.what();}
 catch(const std::exception&e){result["error"]="unexpected";result["what"]=e.what();}
 result["calls"]=p.calls;result["reentries"]=p.reentries;return result;}
Json run(const Json&j){configuration=&j;if(j.value("rust_only",false))return {{"rust",run_one<after::MenuState>(j)}};return {{"before",run_one<before::MenuState>(j)},{"rust",run_one<after::MenuState>(j)}};}
}
int main()try{for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
