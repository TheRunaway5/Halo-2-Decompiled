#include "halo2/frontend.hpp"
#include "halo2/frontend_matchmaking_party.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <type_traits>
using namespace halo2;
using Json=nlohmann::json;
namespace bridge=halo2::online_protocol_bridge;
namespace fixture {
std::string raw(const Json&j,const char*key,std::string fallback={}){if(!j.contains(key))return fallback;if(j[key].is_string())return j[key].get<std::string>();const auto b=j[key].get<std::vector<unsigned char>>();return {reinterpret_cast<const char*>(b.data()),b.size()};}
Json bytes(const std::string&s){Json out=Json::array();for(unsigned char c:s)out.push_back(c);return out;}
struct Frame {unsigned screen{},context{},selected{};std::shared_ptr<int>vista_pcr;std::vector<MenuEntry>entries;};
struct Model;
struct Account {Model*m;void profile(FrontendAppearance,std::string);void connect(std::string);void sign_out();OnlineAccountSnapshot snapshot();};
struct Login {Model*m;DiscordLoginSnapshot snapshot();};
struct Queue {Model*m;void cancel_queue();};
struct Search {Model*m;void stop();};
struct Voice {Model*m;bool party;Voice(std::shared_ptr<Account>,bool);void leave();void join();OnlineVoiceSnapshot snapshot();};
struct NativeFailure:std::exception {const char*what()const noexcept override{return "native account callback failed";}};
std::exception_ptr injected;const NativeFailure*original_exception{};
Model*active_model{};
Json snapshot(const OnlineAccountSnapshot&a){Json friends=Json::array(),clan=Json::array(),invites=Json::array(),members=Json::array();for(const auto&p:a.friends)friends.push_back(bytes(p.id));if(a.clan)for(const auto&p:a.clan->members)clan.push_back(bytes(p.id));for(const auto&p:a.party_invitations)invites.push_back(bytes(p.id));if(a.party)for(const auto&p:a.party->members)members.push_back({bytes(p.handle),p.local_players.size()});return {{"revision",a.revision},{"authenticated",a.authenticated},{"busy",a.busy},{"party",a.party.has_value()},{"members",members},{"invitations",invites},{"friends",friends},{"clan",a.clan?clan:Json(nullptr)},{"search",a.friend_search.has_value()},{"serial",a.party_command_serial},{"successful",a.party_command_successful?Json(*a.party_command_successful):Json(nullptr)},{"message",bytes(a.party_message)}};}
Json snapshot(const OnlineVoiceSnapshot&v){return {{"revision",v.revision},{"joined",v.joined},{"connecting",v.connecting},{"muted",v.microphone_muted},{"message",bytes(v.message)}};}
struct Model {
  std::shared_ptr<Account>online_account_;std::shared_ptr<Login>discord_login_;std::shared_ptr<Queue>online_matchmaking_;std::shared_ptr<Voice>online_voice_;
  Search matchmaking_search_{this};OnlineAccountSnapshot account_,worker;OnlineVoiceSnapshot voice_,worker_voice;DiscordLoginSnapshot identity;
  std::string signed_in_,account_name_;std::map<std::string,std::string>profile_service_tags_;bool voice_party_{},party_voice_wanted_{},native{};
  std::uint64_t friend_join_serial_{};FrontendMatchmakingPartyFlow party_flow_;
  std::vector<FrontendUiSound>pending_ui_sounds_;std::vector<Frame>stack_;std::optional<int>launch;int mode_=3;unsigned revision{};
  int fail=-1;unsigned reads{},reentries{};Json calls=Json::array();std::function<Json()>state_probe;
  std::vector<MenuEntry>population;std::string reenter_on;bool in_reentry{},partial_population{};std::function<void()>reenter;
  FrontendAppearance appearance;
  Json storage()const{Json frames=Json::array(),sounds=Json::array();for(const auto&f:stack_){Json entries=Json::array();for(const auto&e:f.entries)entries.push_back({bytes(e.label),e.destination,e.value});frames.push_back({{"screen",f.screen},{"context",f.context},{"selected",f.selected},{"pcr",bool(f.vista_pcr)},{"entries",entries}});}for(auto cue:pending_ui_sounds_)sounds.push_back(unsigned(cue));return {{"account",fixture::snapshot(account_)},{"voice",fixture::snapshot(voice_)},{"frames",frames},{"sounds",sounds},{"voice_client",bool(online_voice_)},{"device_party",online_voice_?Json(online_voice_->party):Json(nullptr)},{"revision",revision},{"mode",mode_},{"launch",launch.has_value()}};}
  void call(const std::string&name,Json args=nullptr){
    auto state=state_probe?state_probe():Json(nullptr);(void)party_flow_.state();++reentries;
    const auto at=reads++;calls.push_back({{"name",name},{"arguments",args},{"storage",storage()},{"state",state}});
    if(int(at)==fail)std::rethrow_exception(injected);
    if(!in_reentry&&reenter&&name==reenter_on){in_reentry=true;try{reenter();in_reentry=false;}catch(...){in_reentry=false;throw;}}
  }
  const Frame&frame()const{return stack_.back();}
  bool online_clan_screen(const Frame&f)const{return (f.screen==26&&f.context==1)||f.screen==217||f.screen==236||((f.screen==31||f.screen==211||f.screen==33)&&f.context==41);}
  bool online_party_screen(const Frame&f)const{return frontend_party_flow_screen(mode_,f.screen,f.context);}
  bool online_friends_screen(const Frame&f)const{return (f.screen==26&&f.context==0)||((f.screen==216||f.screen==31)&&f.context==43)||(f.screen==31&&(f.context==44||f.context==46));}
  bool native_friends(){call("native-friends");return native;}
  FrontendAppearance appearance_for_profile(const std::string&name){call("appearance",bytes(name));return appearance;}
  void return_online(){call("return-online");stack_.clear();launch.reset();mode_=3;stack_.push_back({6});stack_.push_back({186});}
  void open(unsigned s,unsigned c=0){call("open",{s,c});stack_.push_back({s,c});}
  void unavailable(std::string text){call("unavailable",bytes(text));open(8);}
  void populate(Frame&f,const char*kind){call("populate",{kind,f.screen,f.context});for(const auto&e:population){f.entries.push_back(e);if(partial_population)throw NativeFailure{};}}
  void populate_online_friends(Frame&f){populate(f,"friends");}void populate_online_party(Frame&f){populate(f,"party");}void populate_online_clans(Frame&f){populate(f,"clan");}
};
void Account::profile(FrontendAppearance a,std::string tag){const auto*p=reinterpret_cast<const unsigned char*>(&a);m->call("profile",{std::vector<unsigned char>(p,p+sizeof a),bytes(tag)});}
void Account::connect(std::string n){m->call("connect",bytes(n));}void Account::sign_out(){m->call("signout");}OnlineAccountSnapshot Account::snapshot(){m->call("account-snapshot");return m->worker;}
DiscordLoginSnapshot Login::snapshot(){m->call("identity");return m->identity;}
void Queue::cancel_queue(){m->call("cancel");}void Search::stop(){m->call("stop-search");}
Voice::Voice(std::shared_ptr<Account>a,bool p):m(a?a->m:active_model),party(p){if(m&&m->state_probe)m->call("voice-create",p);}
void Voice::leave(){m->call("voice-leave");m->worker_voice.joined=false;++m->worker_voice.revision;}
void Voice::join(){m->call("voice-join");m->worker_voice.joined=true;++m->worker_voice.revision;}
OnlineVoiceSnapshot Voice::snapshot(){m->call("voice-snapshot");return m->worker_voice;}
}
namespace before {
using namespace fixture;using OnlineVoice=fixture::Voice;
constexpr unsigned friend_context=43;
class MenuState:public Model {public:void update_online_account();void update_online_friend_join();};
#include "frontend_account_flow_before.inc"
}
namespace after {
using namespace fixture;using OnlineVoice=fixture::Voice;
class MenuState:public Model {public:void update_online_account();void update_online_friend_join();void account_flow_drive(bool);};
#include "frontend_account_flow_after.inc"
}
namespace {
using namespace fixture;
OnlineAccountSnapshot account(const Json&j){OnlineAccountSnapshot a;a.revision=j.value("revision",std::uint64_t{});a.authenticated=j.value("authenticated",false);a.busy=j.value("busy",false);a.party_message=raw(j,"message");
 if(j.value("party",false)){a.party.emplace();for(const auto&m:j.value("members",Json::array())){OnlinePartyMember p;p.handle=raw(m,"id");p.local_players.resize(m.value("locals",0u));a.party->members.push_back(std::move(p));}}
 for(const auto&id:j.value("invitations",Json::array()))a.party_invitations.push_back({raw(Json{{"id",id}},"id"),"invite",0});
 for(const auto&id:j.value("friends",Json::array())){OnlineFriend f;f.id=raw(Json{{"id",id}},"id");a.friends.push_back(std::move(f));}
 if(j.contains("clan")&&!j["clan"].is_null()){a.clan.emplace();for(const auto&id:j["clan"]){OnlineClanMember m;m.id=raw(Json{{"id",id}},"id");a.clan->members.push_back(std::move(m));}}
 if(j.value("search",false))a.friend_search.emplace();a.party_command_serial=j.value("serial",std::uint64_t{});if(j.contains("successful")&&!j["successful"].is_null())a.party_command_successful=j["successful"].get<bool>();return a;}
OnlineVoiceSnapshot voice(const Json&j){OnlineVoiceSnapshot v;v.revision=j.value("revision",std::uint64_t{});v.joined=j.value("joined",false);v.connecting=j.value("connecting",false);v.microphone_muted=j.value("muted",false);v.message=raw(j,"message");return v;}
Frame frame(const Json&j){Frame f;f.screen=j.value("screen",186u);f.context=j.value("context",0u);f.selected=j.value("selected",0u);if(j.value("pcr",false))f.vista_pcr=std::make_shared<int>(1);for(const auto&e:j.value("entries",Json::array()))f.entries.push_back({raw(e,"label"),e.value("destination",31),e.value("value",0)});return f;}
void configure(Model&m,const Json&j){active_model=&m;m.mode_=j.value("mode",3);m.account_=account(j.value("before",Json::object()));m.worker=account(j.value("after",Json::object()));m.voice_=voice(j.value("voice",Json::object()));m.worker_voice=voice(j.value("next_voice",Json::object()));
 if(j.value("account_client",true))m.online_account_=std::make_shared<Account>(&m);if(j.value("login_client",true))m.discord_login_=std::make_shared<Login>(&m);if(j.value("queue_client",true))m.online_matchmaking_=std::make_shared<Queue>(&m);
 if(j.value("identity",true)){m.identity.identity.emplace();m.identity.identity->username=raw(j,"username","identity");}
 m.signed_in_=raw(j,"signed_in");m.account_name_=raw(j,"account_name","old name");m.friend_join_serial_=j.value("join_serial",std::uint64_t{});m.voice_party_=j.value("voice_party",false);m.party_voice_wanted_=j.value("voice_wanted",false);
 if(j.value("voice_client",m.voice_party_))m.online_voice_=std::make_shared<Voice>(m.online_account_,m.voice_party_);
 m.party_flow_.voice_party(m.voice_party_);m.party_flow_.voice_wanted(m.party_voice_wanted_);m.party_flow_.account_name(m.account_name_);m.party_flow_.friend_join_serial(m.friend_join_serial_);
 for(const auto&t:j.value("tags",Json::array()))m.profile_service_tags_.emplace(raw(t,"key"),raw(t,"value"));
 for(const auto&f:j.value("frames",Json::array({{{"screen",6}},{{"screen",186}}})))m.stack_.push_back(frame(f));
 if(j.value("launch",false))m.launch=1;for(unsigned i=0;i<j.value("sounds",0u);++i)m.pending_ui_sounds_.push_back(FrontendUiSound::Tab);
 for(const auto&e:j.value("population",Json::array()))m.population.push_back({raw(e,"label"),e.value("destination",31),e.value("value",0)});
 if(j.contains("appearance")){const auto b=j["appearance"].get<std::vector<unsigned char>>();if(b.size()!=sizeof m.appearance)throw std::runtime_error("appearance needs nine bytes");std::memcpy(&m.appearance,b.data(),b.size());}
 m.native=j.value("native",false);m.reenter_on=raw(j,"reenter_on");m.partial_population=j.value("partial_population",false);m.fail=j.value("fail",-1);m.revision=j.value("revision",0u);
}
Json state(const before::MenuState&m){return {{"name",bytes(m.account_name_)},{"serial",m.friend_join_serial_},{"voice_party",m.voice_party_},{"voice_wanted",m.party_voice_wanted_},{"dismissed",m.party_flow_.dismissed()},{"match_voice",m.voice_party_&&m.party_voice_wanted_&&m.voice_.joined}};}
Json state(const after::MenuState&m){return {{"name",bytes(m.party_flow_.account_name())},{"serial",m.party_flow_.friend_join_serial()},{"voice_party",m.party_flow_.voice_party()},{"voice_wanted",m.party_flow_.voice_wanted()},{"dismissed",m.party_flow_.dismissed()},{"match_voice",m.party_flow_.match_voice(false,m.voice_.joined)}};}
template<class M>Json run_one(const Json&j){M m;configure(m,j);m.state_probe=[&]{return state(m);};m.reenter=[&]{m.update_online_friend_join();};Json result;
 try{if(j.value("continuation",false))m.update_online_friend_join();else m.update_online_account();}
 catch(const NativeFailure&e){result["error"]="native";result["same_object"]=&e==original_exception;}
 catch(int n){result["error"]="int";result["integer"]=n;}
 catch(const std::exception&e){result["error"]="unexpected";result["what"]=e.what();}
 result["storage"]=m.storage();result["state"]=state(m);result["calls"]=m.calls;result["reentries"]=m.reentries;return result;}
Json run(const Json&j){try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}catch(const NativeFailure&e){original_exception=&e;injected=std::current_exception();}catch(...){original_exception=nullptr;injected=std::current_exception();}return {{"before",run_one<before::MenuState>(j)},{"rust",run_one<after::MenuState>(j)}};}
}
int main()try{for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
