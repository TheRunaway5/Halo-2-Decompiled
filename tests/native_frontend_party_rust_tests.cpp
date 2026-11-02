#include "halo2/frontend_matchmaking_party.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <bit>
#include <exception>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
struct NativeFailure:std::exception {std::string text;explicit NativeFailure(std::string value):text(std::move(value)){}const char*what()const noexcept override{return text.c_str();}};
struct Observation {
  std::vector<std::string> calls;int fail=-1;std::exception_ptr exception;
  void read(std::string label){const auto index=int(calls.size());calls.push_back(std::move(label));if(index==fail)std::rethrow_exception(exception);}
};
Observation *observed;
struct Login {
  unsigned controller{};bool authorized{};
  std::optional<int> authorization()const{observed->read("auth:"+std::to_string(controller));return authorized?std::optional<int>(1):std::nullopt;}
};
struct Data {
  OnlineAccountSnapshot account_;
  std::string signed_in_="Pilot";std::vector<std::string> dismissed_party_invites_;
  std::optional<int> launch;OnlineMatchmakingSnapshot matchmaking_;
  OnlineVoiceSnapshot voice_;bool voice_party_{};
  FrontendAppearance primary;std::vector<FrontendLocalPlayer> seats;
  std::array<std::shared_ptr<Login>,4> local_discord_logins_;
  FrontendAppearance appearance_for_profile(const std::string&)const{observed->read("profile");return primary;}
  std::vector<FrontendLocalPlayer> local_players()const{observed->read("locals");return seats;}
};
} // namespace
namespace before {
class MenuState:public Data {
public:
  std::optional<FrontendInviteNotice> invite_notice()const;
  std::vector<OnlinePartyMember> online_party_members()const;
  std::string online_party_column(const MenuFrame&,unsigned,unsigned)const;
};
#include "frontend_party_before.inc"
}
namespace after {
class MenuState:public Data {
public:
  // Projection controls supply the dismissed-ID observation directly. Party
  // controller state transitions have a separate actual CXX comparison.
  struct DismissedView {
    const Data &input;FrontendMatchmakingPartyFlow flow;
    std::vector<std::string> dismissed()const{return input.dismissed_party_invites_;}
    bool voice_party()const{return flow.voice_party();}
  } party_flow_{*this};
  std::optional<FrontendInviteNotice> invite_notice()const;
  std::vector<OnlinePartyMember> online_party_members()const;
  std::string online_party_column(const MenuFrame&,unsigned,unsigned)const;
};
#include "frontend_party_after.inc"
}
namespace {
std::string raw(const Json &j,const char *key,std::string fallback={}) {
  if(!j.contains(key))return fallback;
  if(j[key].is_string())return j[key].get<std::string>();
  const auto value=j[key].get<std::vector<std::uint8_t>>();return {reinterpret_cast<const char*>(value.data()),value.size()};
}
Json bytes(const std::string &value){Json out=Json::array();for(unsigned char c:value)out.push_back(c);return out;}
FrontendAppearance appearance(const Json &value) {
  const auto a=value.get<std::array<std::uint8_t,9>>();return std::bit_cast<FrontendAppearance>(a);
}
Json appearance(const std::optional<FrontendAppearance> &a) {
  return a?Json(std::bit_cast<std::array<std::uint8_t,9>>(*a)):Json(nullptr);
}
OnlinePartyMember member(const Json &j) {
  OnlinePartyMember p;p.handle=raw(j,"handle");p.name=raw(j,"name");p.ready_playlist=raw(j,"ready");
  p.self=j.value("self",false);p.leader=j.value("leader",false);p.online=j.value("online",false);p.guest=j.value("guest",false);p.local_controller=j.value("controller",-1);
  if(j.contains("appearance")&&!j["appearance"].is_null())p.appearance=appearance(j["appearance"]);
  if(j.contains("postgame")&&!j["postgame"].is_null())p.postgame=j["postgame"].get<bool>();
  if(j.contains("locals"))for(const auto &s:j["locals"]) {OnlinePartyMember::Local local;local.name=raw(s,"name");local.guest=s.value("guest",false);
    if(s.contains("appearance")&&!s["appearance"].is_null())local.appearance=appearance(s["appearance"]);p.local_players.push_back(std::move(local));}
  return p;
}
Json member(const OnlinePartyMember &p) {
  Json locals=Json::array();for(const auto &s:p.local_players)locals.push_back({{"name",bytes(s.name)},{"guest",s.guest},{"appearance",appearance(s.appearance)}});
  return {{"handle",bytes(p.handle)},{"name",bytes(p.name)},{"ready",bytes(p.ready_playlist)},{"self",p.self},{"leader",p.leader},{"online",p.online},{"guest",p.guest},{"controller",p.local_controller},{"appearance",appearance(p.appearance)},{"postgame",p.postgame?Json(*p.postgame):Json(nullptr)},{"locals",locals}};
}
OnlineAccountSnapshot account(const Json &j) {
  OnlineAccountSnapshot f;f.authenticated=j.value("authenticated",true);f.busy=j.value("busy",false);f.party_message=raw(j,"message");
  if(j.contains("party")&&!j["party"].is_null()){f.party.emplace();for(const auto &p:j["party"])f.party->members.push_back(member(p));}
  if(j.contains("invitations"))for(const auto &i:j["invitations"])f.party_invitations.push_back({raw(i,"id"),raw(i,"name"),i.value("version",std::uint64_t(0))});
  return f;
}
Data data(const Json &j) {
  Data f;f.account_=account(j);f.signed_in_=raw(j,"signed_in","Pilot");f.voice_party_=j.value("voice_party",false);
  f.voice_.joined=j.value("joined",false);f.voice_.transmitting=j.value("transmitting",false);
  for(const auto key:{"muted","talking","dismissed"})if(j.contains(key))for(const auto &value:j[key]) {
    const auto s=raw(Json{{"value",value}},"value");
    if(std::string_view(key)=="muted")f.voice_.muted.push_back(s);else if(std::string_view(key)=="talking")f.voice_.talking.push_back(s);else f.dismissed_party_invites_.push_back(s);
  }
  if(j.value("launch",false))f.launch=1;
  f.matchmaking_.status=static_cast<OnlineMatchmakingStatus>(j.value("status",0u));
  if(j.contains("primary"))f.primary=appearance(j["primary"]);
  if(j.contains("seats"))for(const auto &s:j["seats"]) {
    const auto controller=s.value("controller",0u);if(controller>=4)throw std::logic_error("Fixture requires admitted controller indices0..3");
    FrontendLocalPlayer p;p.controller=controller;p.name=raw(s,"name");if(s.contains("appearance"))p.appearance=appearance(s["appearance"]);f.seats.push_back(std::move(p));
  }
  if(j.contains("authorizations"))for(unsigned i=0;i<j["authorizations"].size();++i) {
    if(i>=4)throw std::logic_error("Fixture login resource index outside admitted four-controller bank");
    if(!j["authorizations"][i].is_null())f.local_discord_logins_[i]=std::make_shared<Login>(Login{i,j["authorizations"][i].get<bool>()});
  }
  return f;
}
Json notice(const std::optional<FrontendInviteNotice> &value) {
  if(!value)return nullptr;return {{"id",bytes(value->id)},{"name",bytes(value->name)},{"busy",value->accepting},{"can_accept",value->can_accept}};
}
Json execute(const Json &j) {
  before::MenuState a;after::MenuState b;static_cast<Data&>(a)=data(j);static_cast<Data&>(b)=static_cast<Data&>(a);
  b.party_flow_.flow.voice_party(b.voice_party_);
  Observation observation;observation.fail=j.value("fail",-1);observation.exception=j.value("nonstd",false)?std::make_exception_ptr(1234567):std::make_exception_ptr(NativeFailure(std::string("bad\xff\0native",11)));
  const void *expected{};try{std::rethrow_exception(observation.exception);}catch(const NativeFailure&e){expected=&e;}catch(...){}
  observed=&observation;const auto op=j.value("op",std::string("members"));
  const auto evaluate=[&](auto &state,bool rust) {
    Json result;
    try {
      if(op=="members") {Json rows=Json::array();for(const auto &p:state.online_party_members())rows.push_back(member(p));result["members"]=rows;}
      else if(op=="column") {MenuFrame f;f.screen=j.value("screen",26u);result["text"]=bytes(state.online_party_column(f,j.value("row",0u),j.value("column",1u)));}
      else if(op=="notice")result["notice"]=notice(state.invite_notice());
      else if(op=="sounds") {const auto old=account(j.at("before"));const auto next=account(j.at("after"));const auto sounds=rust?halo2::frontend_party_notification_sounds(old,next):before::frontend_party_notification_sounds(old,next);result["sounds"]=Json::array();for(const auto sound:sounds)result["sounds"].push_back(unsigned(sound));}
      else throw std::logic_error("Unknown party comparison operation");
      result["error"]=nullptr;
    }catch(const NativeFailure &e){result["error"]="native";result["same_object"]=&e==expected;result["what"]=bytes(std::string(e.what(),4));}
    catch(int value){result["error"]="int";result["value"]=value;}
    catch(const std::exception &e){result["error"]="other";result["what"]=bytes(e.what());}
    result["calls"]=observation.calls;observation.calls.clear();return result;
  };
  const auto old=evaluate(a,false);const auto current=evaluate(b,true);return {{"before",old},{"rust",current}};
}
}
int main()try {std::string line;while(std::getline(std::cin,line))std::cout<<execute(Json::parse(line)).dump()<<'\n';}
catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
