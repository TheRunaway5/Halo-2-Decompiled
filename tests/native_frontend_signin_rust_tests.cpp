#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
using namespace halo2;
using Json=nlohmann::json;
namespace bridge=halo2::online_protocol_bridge;
namespace fixture {
std::string raw(const Json&j,const char*k,std::string fallback={}){if(!j.contains(k))return fallback;if(j[k].is_string())return j[k].get<std::string>();const auto b=j[k].get<std::vector<unsigned char>>();return {reinterpret_cast<const char*>(b.data()),b.size()};}
Json bytes(const std::string&s){Json out=Json::array();for(unsigned char c:s)out.push_back(c);return out;}
Json bytes(const rust::Vec<std::uint8_t>&s){return Json(std::vector<std::uint8_t>(s.begin(),s.end()));}
Json encoded(const MenuFrame&f){Json rows=Json::array();for(const auto&e:f.entries)rows.push_back({bytes(e.label),e.destination,e.value});return {f.screen,f.context,f.selected,f.instance,f.pane_created_clock,bytes(f.title),bytes(f.description),rows};}
struct Model;
struct Login{Model*m;unsigned id{};DiscordLoginSnapshot data;bool steam{},authorized{};bool uses_steam()const;std::optional<std::string>authorization()const;DiscordLoginSnapshot snapshot()const;void bind_profile(std::string,std::string);void restore();void start();void cancel();void sign_out();};
struct Account{Model*m;unsigned id{};void sign_out();void connect(std::string);};
struct Worker{Model*m;std::string kind;unsigned id{};void sign_out();void cancel_queue();void leave();void disconnect();void cancel();};
struct Search{Model*m{};bool running=true,is_quick=true;void stop();void quick(bool);};
struct NativeFailure:std::exception{const char*what()const noexcept override{return "native sign-in callback failed";}};
std::exception_ptr injected;const NativeFailure*original_exception{};
struct NativeData {
 std::shared_ptr<Login>discord_login_;std::shared_ptr<Account>online_account_;std::shared_ptr<Worker>online_matchmaking_,online_voice_,discord_social_,online_directory_;
 std::shared_ptr<int>online_prior_report_=std::make_shared<int>(7);OnlineAccountSnapshot account_;Search matchmaking_search_;
 std::vector<MenuFrame>stack_;MenuFrame next_frame;std::filesystem::path preferences_;int mode_=3;unsigned revision{};std::string signed_in_="host",keyboard_text="old keyboard",profile_value="profile",tag_value="AB12",online_detail_id_="detail";
 bool online_directory_active_=true,quit=true,identity{},route_open{},entropy_success=true;std::vector<int>online_servers_{1,2,3};std::optional<int>launch=7;std::vector<unsigned char>entropy;std::vector<std::shared_ptr<Login>>kept;bool keep_alive{};
};
struct Model:NativeData {
 FrontendMatchmakingPartyFlow party_flow_;std::function<Json()>state_probe;std::function<void()>reenter;std::function<std::pair<unsigned,unsigned>(unsigned,unsigned)>prepare_branch;
 std::map<unsigned,std::weak_ptr<Login>>issued;Json calls=Json::array();int fail=-1;unsigned reads{},reentries{};bool in_reentry{};std::string reenter_on;
 Model(){matchmaking_search_.m=this;}
 Json storage()const{Json frames=Json::array(),live=Json::array();for(const auto&f:stack_)frames.push_back(encoded(f));for(const auto&[i,w]:issued)live.push_back({i,w.use_count()});return {{"frames",frames},{"mode",mode_},{"revision",revision},{"signed_in",bytes(signed_in_)},{"keyboard",bytes(keyboard_text)},{"prior",bool(online_prior_report_)},{"authenticated",account_.authenticated},{"busy",account_.busy},{"launch",bool(launch)},{"quit",quit},{"directory_active",online_directory_active_},{"servers",online_servers_},{"detail",bytes(online_detail_id_)},{"search_running",matchmaking_search_.running},{"quick",matchmaking_search_.is_quick},{"login",discord_login_?Json(discord_login_->id):Json(nullptr)},{"account",online_account_?Json(online_account_->id):Json(nullptr)},{"live",live}};}
 void call(const std::string&name,Json args=nullptr){if(!state_probe)return;auto state=state_probe();(void)party_flow_.state();++reentries;const auto at=reads++;calls.push_back({{"name",name},{"arguments",args},{"storage",storage()},{"state",state}});if(int(at)==fail)std::rethrow_exception(injected);if(!in_reentry&&reenter&&name==reenter_on){in_reentry=true;try{reenter();in_reentry=false;}catch(...){in_reentry=false;throw;}}}
 const MenuFrame&frame()const{return stack_.back();}
 std::optional<DiscordIdentity>online_identity()const{const_cast<Model*>(this)->call("identity");return identity?std::optional<DiscordIdentity>(DiscordIdentity{"opaque","name","display"}):std::nullopt;}
 std::string profile_name()const{const_cast<Model*>(this)->call("profile-name");return profile_value;}
 std::string profile_service_tag()const{const_cast<Model*>(this)->call("profile-tag");return tag_value;}
 void save()const{const_cast<Model*>(this)->call("save");}
 MenuFrame build(unsigned s,unsigned c)const{const_cast<Model*>(this)->call("build",{s,c});auto f=next_frame;f.screen=s;f.context=c;return f;}
 void open(unsigned s,unsigned c=0){call("open",{s,c});if(route_open&&prepare_branch){auto p=prepare_branch(s,c);s=p.first;c=p.second;}MenuFrame f;f.screen=s;f.context=c;f.instance=0x100000001;f.entries.push_back({"opened",0,0});stack_.push_back(std::move(f));++revision;}
 void unavailable(std::string s){call("unavailable",bytes(s));open(8);}
 void update_online_couch(){call("update-couch");}void update_online_friends(){call("update-friends");}void update_online_account(){call("update-account");}void update_online_privacy(){call("update-privacy");}void update_online_lobby_leave(){call("update-leave");}void update_online_postgame(){call("update-postgame");}void clear_online_couch(){call("clear-couch");}void reset_online_matchmaking_progress(){call("reset-progress");}void enter_online_matchmaking_lobby(){call("enter-lobby");}
 std::shared_ptr<Login>make_login(unsigned id){auto p=std::make_shared<Login>();p->m=this;p->id=id;issued[id]=p;if(keep_alive)kept.push_back(p);return p;}
};
thread_local Model*entropy_owner{};
int RAND_bytes(unsigned char*b,int n){auto&m=*entropy_owner;m.call("random",n);for(int i=0;i<n;++i)b[i]=m.entropy.at(unsigned(i));return m.entropy_success?1:0;}
bool Login::uses_steam()const{m->call("steam",id);return steam;}std::optional<std::string>Login::authorization()const{m->call("authorized",id);return authorized?std::optional<std::string>("private authorization"):std::nullopt;}DiscordLoginSnapshot Login::snapshot()const{m->call("snapshot",id);return data;}
void Login::bind_profile(std::string ns,std::string key){m->call("bind",{id,bytes(ns),bytes(key)});}void Login::restore(){m->call("restore",id);}void Login::start(){m->call("start",id);}void Login::cancel(){m->call("cancel",id);}void Login::sign_out(){m->call("login-signout",id);}
void Account::sign_out(){m->call("account-signout",id);}void Account::connect(std::string n){m->call("account-connect",{id,bytes(n)});}
void Worker::sign_out(){m->call(kind+"-signout",id);}void Worker::cancel_queue(){m->call("cancel-queue",id);}void Worker::leave(){m->call("voice-leave",id);}void Worker::disconnect(){m->call("social-disconnect",id);}void Worker::cancel(){m->call("directory-cancel",id);}
void Search::stop(){m->call("search-stop");running=false;}void Search::quick(bool value){m->call("quick",value);is_quick=value;}
}
#define SIGNIN_METHODS std::string profile_login_key(const std::string&);void bind_primary_login();void update_online_login();void return_online(std::string={});void return_online_lobby(std::string={});std::pair<unsigned,unsigned>signin_prepare_open(unsigned,unsigned);bool signin_accept();void signin_back();void signin_load_key(const std::string&,const std::string&);std::optional<std::string>signin_take_key(const std::string&);void signin_restore_key(const std::optional<std::string>&);std::string signin_saved_keys()const;
namespace before {
using namespace fixture;
class MenuState:public Model {public:unsigned pending_online_screen_=186,pending_online_context_{};bool awaiting_discord_login_{},saved_login_suppressed_{};std::string login_profile_;std::map<std::string,std::string>profile_login_keys_;SIGNIN_METHODS};
#include "frontend_signin_before.inc"
}
namespace after {
using namespace fixture;
class MenuState:public Model {public:SIGNIN_METHODS FrontendSigninResult signin_flow_drive(unsigned,unsigned=0,unsigned=0,std::string={},bool=false)const;};
#include "frontend_signin_after.inc"
}
namespace {
using namespace fixture;
MenuFrame frame(const Json&j){MenuFrame f;f.screen=j.value("screen",20u);f.context=j.value("context",0u);f.selected=j.value("selected",0u);f.instance=j.value("instance",std::uint64_t(-1));f.pane_created_clock=j.value("pane_clock",0xffffffffu);f.title=raw(j,"title","old title");f.description=raw(j,"description","old description");for(const auto&e:j.value("entries",Json::array({Json{{"label","old label"}}})))f.entries.push_back({raw(e,"label","old label"),e.value("destination",0),e.value("value",0)});return f;}
std::shared_ptr<Login>login(Model&m,const Json&j,unsigned id){if(j.is_null())return {};auto p=m.make_login(j.value("generation",id));p->steam=j.value("steam",false);p->authorized=j.value("authorized",true);p->data.status=static_cast<DiscordLoginStatus>(j.value("status",3));if(j.value("identity",false))p->data.identity=DiscordIdentity{"opaque","name","display"};return p;}
void configure(Model&m,const Json&j){m.keep_alive=j.value("keep_alive",false);if(j.value("login",true))m.discord_login_=login(m,j.value("provider",Json::object()),1);if(j.value("account",true))m.online_account_=std::make_shared<Account>(&m,2);auto worker=[&](const char*k,unsigned id){return j.value(k,true)?std::make_shared<Worker>(&m,k,id):std::shared_ptr<Worker>{};};m.online_matchmaking_=worker("queue",3);m.online_voice_=worker("voice",4);m.discord_social_=worker("social",5);m.online_directory_=worker("directory",6);m.mode_=j.value("mode",3);m.signed_in_=raw(j,"signed_in","host");m.profile_value=raw(j,"profile_name","profile");m.tag_value=raw(j,"tag","AB12");m.preferences_=raw(j,"preferences");m.keyboard_text=raw(j,"keyboard","old keyboard");m.identity=j.value("identity",false);m.route_open=j.value("route_open",false);m.entropy_success=j.value("entropy_success",true);m.account_.authenticated=j.value("authenticated",true);m.account_.busy=j.value("busy",true);m.revision=j.value("revision",0u);if(!j.value("prior",true))m.online_prior_report_.reset();m.entropy=j.value("entropy",std::vector<unsigned char>{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15});for(const auto&f:j.value("frames",Json::array({Json{{"screen",6}},Json{{"screen",20}}})))m.stack_.push_back(frame(f));m.next_frame=frame(j.value("next_frame",Json{{"description","rebuilt"},{"entries",Json::array({Json{{"label","new label"}}})}}));m.fail=j.value("fail",-1);m.reenter_on=raw(j,"reenter_on");}
template<class M>void initialize(M&m,const Json&j){const NativeData saved=m;m.prepare_branch=[&](unsigned s,unsigned c){return m.signin_prepare_open(s,c);};
 for(const auto&k:j.value("keys",Json::array()))m.signin_load_key(raw(k,"name"),raw(k,"key"));
 m.route_open=false;m.discord_login_=m.make_login(91);m.discord_login_->steam=true;m.discord_login_->authorized=false;m.identity=false;m.signed_in_.clear();m.stack_={frame(Json{{"screen",6}}),frame(Json{{"screen",20}})};
 if(j.contains("bound_name")){m.signed_in_=raw(j,"bound_name");m.bind_primary_login();}
 if(j.value("awaiting",false))m.signin_prepare_open(20,0);
 if(j.value("suppressed",false)){m.identity=true;m.stack_.back().selected=2;m.stack_.back().entries.resize(3);m.signin_accept();m.identity=false;}
 if(j.contains("pending_screen"))m.signin_prepare_open(j.value("pending_screen",14u),j.value("pending_context",0u));
 static_cast<NativeData&>(m)=saved;m.matchmaking_search_.m=&m;
}
Json state(const before::MenuState&m){Json keys=Json::array();for(const auto&[n,k]:m.profile_login_keys_)keys.push_back({bytes(n),bytes(k)});return {{"keys",keys},{"bound",bytes(m.login_profile_)},{"suppressed",m.saved_login_suppressed_},{"awaiting",m.awaiting_discord_login_},{"pending",{m.pending_online_screen_,m.pending_online_context_}},{"voice_wanted",m.party_flow_.voice_wanted()},{"dismissed",m.party_flow_.dismissed()}};}
Json state(const after::MenuState&m){const auto s=m.party_flow_.state().signin;Json keys=Json::array();for(const auto&k:s.keys)keys.push_back({bytes(k.name),bytes(k.key)});return {{"keys",keys},{"bound",bytes(s.bound)},{"suppressed",s.suppressed},{"awaiting",s.awaiting},{"pending",{s.pending_screen,s.pending_context}},{"voice_wanted",m.party_flow_.voice_wanted()},{"dismissed",m.party_flow_.dismissed()}};}
template<class M>Json run_one(const Json&j){M m;configure(m,j);entropy_owner=&m;initialize(m,j);m.state_probe=[&]{return state(m);};m.reenter=[&]{const auto op=j.value("reenter",std::string("pending"));if(op=="pending")m.signin_prepare_open(14,73);else if(op=="identity")m.identity=j.value("replacement_identity",true);else if(op=="replace-login")m.discord_login_=login(m,j.value("replacement",Json{{"identity",false},{"steam",false}}),72);else if(op=="replace-account")m.online_account_=std::make_shared<Account>(&m,73);else if(op=="drop-queue")m.online_matchmaking_.reset();else if(op=="drop-voice")m.online_voice_.reset();else if(op=="change-frame"){m.stack_.back().description="changed by callback";m.stack_.back().selected=99;m.stack_.back().instance=17;}else if(op=="snapshot")m.discord_login_->data.status=DiscordLoginStatus::Failed;else if(op=="cancel-wait")m.signin_back();else if(op=="key")m.profile_login_key("nested");};Json result;
 try{const auto op=j.value("op",std::string("update"));if(op=="key")result["value"]=bytes(m.profile_login_key(raw(j,"name","host")));else if(op=="bind")m.bind_primary_login();else if(op=="update")m.update_online_login();else if(op=="prepare"){const auto p=m.signin_prepare_open(j.value("screen",186u),j.value("context",0u));result["value"]={p.first,p.second};}else if(op=="accept")result["value"]=m.signin_accept();else if(op=="back")m.signin_back();else if(op=="return")m.return_online(raw(j,"message"));else if(op=="return-lobby")m.return_online_lobby(raw(j,"message"));else if(op=="storage"){for(const auto&k:j.value("load",Json::array()))m.signin_load_key(raw(k,"name"),raw(k,"key"));const auto k=m.signin_take_key(raw(j,"take","missing"));result["taken"]=k?bytes(*k):Json(nullptr);if(j.value("restore",false))m.signin_restore_key(k);result["serialized"]=bytes(m.signin_saved_keys());}else throw std::logic_error("Invalid sign-in comparison operation");
 }catch(const NativeFailure&e){result["error"]="native";result["same_object"]=&e==original_exception;}catch(int n){result["error"]="int";result["integer"]=n;}catch(const std::exception&e){result["error"]="std";result["what"]=bytes(e.what());}
 result["storage"]=m.storage();result["state"]=state(m);result["calls"]=m.calls;result["reentries"]=m.reentries;return result;
}
Json run(const Json&j){try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}catch(const NativeFailure&e){injected=std::current_exception();original_exception=&e;}catch(...){injected=std::current_exception();original_exception=nullptr;}return {{"before",run_one<before::MenuState>(j)},{"rust",run_one<after::MenuState>(j)}};}
}
int main()try{for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
