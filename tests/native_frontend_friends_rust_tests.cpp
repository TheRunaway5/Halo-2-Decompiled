#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
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
Json bytes(const rust::Vec<std::uint8_t>&s){return Json(std::vector<std::uint8_t>(s.begin(),s.end()));}
struct Model;Model*active{};
void observe(const std::string&,Json=nullptr);
template<class T>struct Bank {std::string name;std::vector<T>values;const T&at(std::size_t index)const{observe(name,index);return values.at(index);}};
struct Text{unsigned string{};};struct Pane{Bank<Text>texts{"text"};};struct Screen{Bank<Pane>panes{"pane"};};
struct Content{Bank<Screen>screens{"screen"};std::string localized(const Screen&,unsigned id)const{observe("localized",id);return std::string("\xff\0",2)+std::to_string(id);}};
struct Account{Model*m;bool party_command(OnlinePartyCommand);bool friend_command(std::string,std::string);OnlineAccountSnapshot snapshot();};
struct Login{Model*m;bool uses_steam()const;};
struct Social{Model*m;unsigned generation{};void enable();void disconnect();void pump();void presence(const std::string&,bool);DiscordSocialSnapshot snapshot()const;bool command(const std::string&,const std::string&);void bind_account(std::shared_ptr<Account>);bool invite_to_party(const std::string&);bool accept_activity(const std::string&);bool dismiss_activity(const std::string&);};
struct NativeFailure:std::exception{const char*what()const noexcept override{return "native friends callback failed";}};
std::exception_ptr injected;const NativeFailure*original_exception{};
Json encoded(const OnlineFriend&p){return {{"id",bytes(p.id)},{"name",bytes(p.name)},{"tag",bytes(p.tag)},{"state",bytes(p.state)},{"presence",bytes(p.presence)},{"joinable",p.party_joinable?Json(*p.party_joinable):Json(nullptr)},{"context",bytes(p.party_context)}};}
Json encoded(const OnlineAccountSnapshot&a){Json friends=Json::array();for(const auto&p:a.friends)friends.push_back(encoded(p));return {{"authenticated",a.authenticated},{"busy",a.busy},{"party",bool(a.party)},{"leader",a.party&&a.party->leader},{"available",a.friends_available},{"tag",bytes(a.friend_tag)},{"message",bytes(a.friends_message)},{"friends",friends},{"search",a.friend_search?encoded(*a.friend_search):Json(nullptr)},{"serial",a.party_command_serial}};}
Json encoded(const DiscordSocialSnapshot&s){Json friends=Json::array(),invites=Json::array();for(const auto&p:s.friends)friends.push_back({bytes(p.handle),bytes(p.name),bytes(p.username),int(p.kind),int(p.presence)});for(const auto&p:s.invitations)invites.push_back({bytes(p.handle),bytes(p.name),bytes(p.status)});return {{"available",s.available},{"enabled",s.enabled},{"connecting",s.connecting},{"ready",s.ready},{"busy",s.busy},{"message",bytes(s.message)},{"revision",s.revision},{"friends",friends},{"invitations",invites}};}
Json encoded(const MenuFrame&f){Json entries=Json::array();for(const auto&e:f.entries)entries.push_back({bytes(e.label),e.destination,e.value});return {{"screen",f.screen},{"context",f.context},{"selected",f.selected},{"title",bytes(f.title)},{"description",bytes(f.description)},{"entries",entries}};}
Json encoded(const OnlinePartyCommand&c){return {{"action",bytes(c.action)},{"value",bytes(c.value)},{"playlist",bytes(c.playlist)},{"revision",bytes(c.revision)},{"version",c.version},{"privacy",c.privacy?Json(*c.privacy):Json(nullptr)},{"privacy_revision",c.privacy_revision},{"privacy_context",bytes(c.privacy_context)},{"join_context",bytes(c.join_context)}};}
struct Model{
 Content content_;std::shared_ptr<Login>discord_login_;std::shared_ptr<Social>discord_social_;std::shared_ptr<Account>online_account_;
 DiscordSocialSnapshot friends_,worker;OnlineAccountSnapshot account_,account_after;bool refresh_account{};
 FrontendMatchmakingPartyFlow party_flow_;std::string friend_target_,activity_target_,keyboard_text;std::vector<MenuFrame>stack_;MenuFrame detached;
 OnlineMatchmakingSnapshot matchmaking_;std::optional<int>launch;unsigned revision{};std::uint64_t serial{};
 bool steam{},identity{},reply{},refresh_frames{},in_reentry{};std::vector<MenuFrame>frames_after;Json replies=Json::object();
 int fail=-1;unsigned reads{},reentries{};Json calls=Json::array();std::string reenter_on;std::function<Json()>state_probe;std::function<void()>reenter;
 Json storage()const{Json frames=Json::array();for(const auto&f:stack_)frames.push_back(encoded(f));return {{"account",encoded(account_)},{"social",encoded(friends_)},{"frames",frames},{"detached",encoded(detached)},{"keyboard",bytes(keyboard_text)},{"revision",revision},{"launch",bool(launch)},{"status",unsigned(matchmaking_.status)},{"social_client",bool(discord_social_)},{"social_generation",discord_social_?Json(discord_social_->generation):Json(nullptr)}};}
 void call(const std::string&name,Json args=nullptr){if(!state_probe)return;auto state=state_probe();(void)party_flow_.state();++reentries;const auto at=reads++;calls.push_back({{"name",name},{"arguments",args},{"storage",storage()},{"state",state}});if(int(at)==fail)std::rethrow_exception(injected);if(!in_reentry&&reenter&&name==reenter_on){in_reentry=true;try{reenter();in_reentry=false;}catch(...){in_reentry=false;throw;}}}
 bool response(const std::string&name)const{return replies.value(name,reply);}
 const MenuFrame&frame()const{return stack_.back();}
 std::optional<int>online_identity()const{const_cast<Model*>(this)->call("identity");return identity?std::optional<int>(1):std::nullopt;}
 std::string profile_name()const{const_cast<Model*>(this)->call("profile-name");return std::string("\xff\0",2);}
 void open(unsigned s,unsigned c=0){call("open",{s,c});MenuFrame f;f.screen=s;f.context=c;stack_.push_back(std::move(f));++revision;}
 void back(){call("back");if(stack_.size()>1)stack_.pop_back();++revision;}
 void unavailable(std::string text){call("unavailable",bytes(text));open(8);}
 void party_command(OnlinePartyCommand c){call("wrapped-party",encoded(c));}
 void update_online_account(){call("update-account");if(refresh_account)account_=account_after;if(refresh_frames)stack_=frames_after;}
 void account_flow_drive(bool c){call("account-continuation",c);}
};
void observe(const std::string&name,Json args){active->call(name,std::move(args));}
bool Account::party_command(OnlinePartyCommand c){m->call("party-command",encoded(c));return m->response("party-command");}
bool Account::friend_command(std::string a,std::string t){m->call("friend-command",{bytes(a),bytes(t)});return m->response("friend-command");}
OnlineAccountSnapshot Account::snapshot(){m->call("account-snapshot");auto out=m->account_;out.party_command_serial=m->serial;return out;}
bool Login::uses_steam()const{m->call("steam");return m->steam;}
void Social::enable(){m->call("social-enable");}void Social::disconnect(){m->call("social-disconnect");}void Social::pump(){m->call("social-pump");}
void Social::presence(const std::string&name,bool playing){m->call("social-presence",{bytes(name),playing});}
DiscordSocialSnapshot Social::snapshot()const{m->call("social-snapshot");return m->worker;}
bool Social::command(const std::string&target,const std::string&action){m->call("social-command",{bytes(target),bytes(action)});return m->response("social-command");}
void Social::bind_account(std::shared_ptr<Account>a){m->call("social-bind",bool(a));}
bool Social::invite_to_party(const std::string&t){m->call("social-invite",bytes(t));return m->response("social-invite");}
bool Social::accept_activity(const std::string&t){m->call("social-accept",bytes(t));return m->response("social-accept");}
bool Social::dismiss_activity(const std::string&t){m->call("social-dismiss",bytes(t));return m->response("social-dismiss");}
}
#define FRIENDS_METHODS void set_discord_social(std::shared_ptr<DiscordFriends>);bool online_friends_screen(const MenuFrame&)const;bool native_friends()const;bool online_friends_available()const;void update_online_friend_join();void populate_native_friends(MenuFrame&)const;void populate_online_friends(MenuFrame&)const;bool accept_native_friends();bool accept_online_friends();void update_online_friends();std::string online_friends_text(const MenuFrame&,unsigned)const;std::string online_friends_column(const MenuFrame&,unsigned,unsigned)const;
namespace before{
using namespace fixture;using DiscordFriends=fixture::Social;
class MenuState:public Model{public:FRIENDS_METHODS};
#include "frontend_friends_before.inc"
}
namespace after{
using namespace fixture;using DiscordFriends=fixture::Social;
class MenuState:public Model{public:FRIENDS_METHODS FrontendFriendsResult friends_flow_drive(unsigned,MenuFrame* =nullptr,unsigned=0,unsigned=0)const;};
#include "frontend_friends_after.inc"
}
namespace{
using namespace fixture;
OnlineFriend account_friend(const Json&j){OnlineFriend p;p.id=raw(j,"id");p.name=raw(j,"name","same");p.tag=raw(j,"tag");p.state=raw(j,"state","friend");p.presence=raw(j,"presence","online");if(j.contains("joinable")&&!j["joinable"].is_null())p.party_joinable=j["joinable"].get<bool>();p.party_context=raw(j,"context");return p;}
OnlineAccountSnapshot account(const Json&j){OnlineAccountSnapshot a;a.authenticated=j.value("authenticated",false);a.busy=j.value("busy",false);a.friends_available=j.value("available",false);a.friend_tag=raw(j,"tag");a.friends_message=raw(j,"message");a.party_command_serial=j.value("serial",std::uint64_t{});if(j.value("party",false)){a.party.emplace();a.party->leader=j.value("leader",false);}for(const auto&p:j.value("friends",Json::array()))a.friends.push_back(account_friend(p));if(j.contains("search")&&!j["search"].is_null())a.friend_search=account_friend(j["search"]);return a;}
DiscordSocialSnapshot social(const Json&j){DiscordSocialSnapshot s;s.available=j.value("available",false);s.enabled=j.value("enabled",false);s.connecting=j.value("connecting",false);s.ready=j.value("ready",false);s.busy=j.value("busy",false);s.message=raw(j,"message");s.revision=j.value("revision",std::uint64_t{});for(const auto&p:j.value("friends",Json::array()))s.friends.push_back({raw(p,"id"),raw(p,"name","same"),raw(p,"username"),static_cast<DiscordFriendKind>(p.value("kind",0)),static_cast<DiscordFriendPresence>(p.value("presence",0))});for(const auto&p:j.value("invitations",Json::array()))s.invitations.push_back({raw(p,"id"),raw(p,"name","same"),raw(p,"status","Game invitation")});return s;}
MenuFrame frame(const Json&j){MenuFrame f;f.screen=j.value("screen",26u);f.context=j.value("context",0u);f.selected=j.value("selected",0u);f.title=raw(j,"title","old title");f.description=raw(j,"description","old description");for(const auto&e:j.value("entries",Json::array()))f.entries.push_back({raw(e,"label","old"),e.value("destination",31),e.value("value",0)});return f;}
void configure(Model&m,const Json&j){active=&m;m.account_=account(j.value("account",Json::object()));m.friends_=social(j.value("social",Json::object()));m.worker=social(j.value("next",Json::object()));m.steam=j.value("steam",false);m.identity=j.value("identity",true);m.reply=j.value("reply",true);m.replies=j.value("replies",Json::object());m.serial=j.value("serial",std::uint64_t(0xffffffffffffffff));m.friend_target_=raw(j,"friend_target");m.activity_target_=raw(j,"activity_target");m.keyboard_text=raw(j,"keyboard","old keyboard");
 if(j.value("account_client",true))m.online_account_=std::make_shared<Account>(&m);if(j.value("social_client",true))m.discord_social_=std::make_shared<Social>(&m,1);if(j.value("login_client",true))m.discord_login_=std::make_shared<Login>(&m);
 for(const auto&f:j.value("frames",Json::array({{{"screen",6}},{{"screen",186}},{{"screen",26},{"context",0}}})))m.stack_.push_back(frame(f));m.detached=frame(j.value("view",Json::object()));
 if(j.value("launch",false))m.launch=1;m.matchmaking_.status=static_cast<OnlineMatchmakingStatus>(j.value("status",0u));m.revision=j.value("revision",0u);
 m.content_.screens.values.resize(j.value("screen_count",27u));if(m.content_.screens.values.size()>26){auto&s=m.content_.screens.values[26];s.panes.values.resize(j.value("pane_count",1u));if(!s.panes.values.empty()){auto&p=s.panes.values[0];p.texts.values.resize(j.value("text_count",4u));for(unsigned i=0;i<p.texts.values.size();++i)p.texts.values[i].string=100+i;}}
 if(j.contains("account_after")){m.refresh_account=true;m.account_after=account(j["account_after"]);}if(j.contains("frames_after")){m.refresh_frames=true;for(const auto&f:j["frames_after"])m.frames_after.push_back(frame(f));}
 m.fail=j.value("fail",-1);m.reenter_on=raw(j,"reenter_on");
}
template<class M>void initialize_targets(M&m){
 const auto stack=m.stack_;const auto account=m.account_;const auto friends=m.friends_;const auto login=m.discord_login_;const auto revision=m.revision;
 if(!m.friend_target_.empty()){m.account_.friends={account_friend(Json{{"id",m.friend_target_}})};m.stack_={frame(Json{{"entries",Json::array({{{"destination",31},{"value",0}}})}})};m.accept_native_friends();}
 if(!m.activity_target_.empty()){m.account_.friends_available=false;m.discord_login_.reset();m.friends_.invitations={{m.activity_target_,"target","status"}};m.stack_={frame(Json{{"entries",Json::array({{{"destination",31},{"value",-1}}})}})};m.accept_online_friends();}
 m.stack_=stack;m.account_=account;m.friends_=friends;m.discord_login_=login;m.revision=revision;
}
Json state(const before::MenuState&m){return {{"friend_target",bytes(m.friend_target_)},{"activity_target",bytes(m.activity_target_)},{"join_serial",m.party_flow_.friend_join_serial()}};}
Json state(const after::MenuState&m){const auto f=m.party_flow_.state();return {{"friend_target",bytes(f.friend_target)},{"activity_target",bytes(f.activity_target)},{"join_serial",f.friend_join_serial}};}
template<class M>Json run_one(const Json&j){M m;configure(m,j);initialize_targets(m);m.party_flow_.friend_join_serial(j.value("join_serial",std::uint64_t{}));m.state_probe=[&]{return state(m);};m.reenter=[&]{m.set_discord_social(m.discord_social_);};Json result;
 try{auto&view=j.value("detached",false)?m.detached:m.stack_.back();const auto op=j.value("op",std::string("populate"));
  if(op=="populate")m.populate_online_friends(view);else if(op=="native-populate")m.populate_native_friends(view);else if(op=="accept")result["value"]=m.accept_online_friends();else if(op=="native-accept")result["value"]=m.accept_native_friends();else if(op=="update")m.update_online_friends();else if(op=="bind")m.set_discord_social(j.value("replacement",true)?std::make_shared<Social>(&m,2):nullptr);else if(op=="native")result["value"]=m.native_friends();else if(op=="available")result["value"]=m.online_friends_available();else if(op=="screen")result["value"]=m.online_friends_screen(view);else if(op=="text")result["value"]=bytes(m.online_friends_text(view,j.value("index",0u)));else if(op=="column")result["value"]=bytes(m.online_friends_column(view,j.value("row",0u),j.value("column",1u)));else throw std::logic_error("Invalid friends comparison operation");
 }catch(const NativeFailure&e){result["error"]="native";result["same_object"]=&e==original_exception;}catch(int n){result["error"]="int";result["integer"]=n;}catch(const std::exception&e){result["error"]="std";result["what"]=bytes(e.what());}
 result["storage"]=m.storage();result["state"]=state(m);result["calls"]=m.calls;result["reentries"]=m.reentries;return result;
}
Json run(const Json&j){try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}catch(const NativeFailure&e){injected=std::current_exception();original_exception=&e;}catch(...){injected=std::current_exception();original_exception=nullptr;}return {{"before",run_one<before::MenuState>(j)},{"rust",run_one<after::MenuState>(j)}};}
}
int main()try{for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
