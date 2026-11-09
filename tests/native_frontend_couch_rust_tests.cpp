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
struct Model;
struct Login{Model*m;unsigned id{};DiscordLoginSnapshot data;bool authorized{},steam{};DiscordLoginSnapshot snapshot()const;std::optional<std::string>authorization()const;bool uses_steam()const;void cancel();void bind_profile(std::string,std::string);void start();};
struct Account{Model*m;unsigned id{};void sign_out();std::shared_ptr<Account>profile_worker(std::shared_ptr<Login>);void profile(FrontendAppearance,std::string);void connect(std::string);};
struct LocalResult{std::string name;std::shared_ptr<Login>login;FrontendAppearance appearance;};
struct NativeFailure:std::exception{const char*what()const noexcept override{return "native couch callback failed";}};
std::exception_ptr injected;const NativeFailure*original_exception{};
Json appearance(const FrontendAppearance&a){const auto*p=reinterpret_cast<const unsigned char*>(&a);return std::vector<unsigned char>(p,p+sizeof a);}
Json encoded(const MenuFrame&f){Json entries=Json::array();for(const auto&e:f.entries)entries.push_back({bytes(e.label),e.destination,e.value});return {f.screen,f.context,f.selected,f.instance,f.pane_created_clock,bytes(f.title),bytes(f.description),entries};}
Json encoded(const DiscordIdentity&i){return {bytes(i.id),bytes(i.username),bytes(i.display_name)};}
Json encoded(const DiscordLoginSnapshot&s){return {int(s.status),s.identity?encoded(*s.identity):Json(nullptr),bytes(s.message)};}
struct NativeData{
 std::shared_ptr<Login>discord_login_;std::shared_ptr<Account>online_account_;std::array<std::shared_ptr<Login>,4>local_discord_logins_{};std::array<std::shared_ptr<Account>,4>local_profile_accounts_{};std::function<std::shared_ptr<Login>()>local_discord_factory_;
 OnlineAccountSnapshot account_;OnlineMatchmakingSnapshot matchmaking_;std::vector<MenuFrame>stack_;MenuFrame detached,next_frame;std::array<std::string,4>local_profiles_{};std::array<std::int32_t,4>local_inputs_{5,-1,-1,-1};std::array<std::int8_t,4>local_teams_{1,2,3,4};std::array<std::array<int,2>,4>local_team_edits_{{{1,2},{3,4},{5,6},{7,8}}};
 unsigned profile_{2},revision{},next_actor{100};int mode_{3};std::string signed_in_="host",description="new description",profile_name_value="selected profile";std::optional<int>system_link_host_,session_countdown_;std::filesystem::path preferences_;std::map<std::string,std::string>profile_service_tags_;
 std::vector<FrontendLocalPlayer>roster;std::vector<std::shared_ptr<Login>>kept;bool keep_alive{};bool workshop{},leave_pending{},overlay{},resume{},populate_built{},null_factory{},null_worker{};
};
struct Model:NativeData{
 FrontendMatchmakingPartyFlow party_flow_;std::map<unsigned,std::weak_ptr<Login>>issued;std::function<Json()>state_probe;std::function<void()>reenter;std::function<bool(bool)>back_branch;std::function<void(MenuFrame&)>populate_branch;
 int fail=-1;unsigned reads{},reentries{};bool in_reentry{};std::string reenter_on;Json calls=Json::array();
 Json storage()const{Json frames=Json::array(),profiles=Json::array(),inputs=Json::array(),teams=Json::array(),edits=Json::array(),logins=Json::array(),accounts=Json::array(),live=Json::array();for(const auto&f:stack_)frames.push_back(encoded(f));for(unsigned i=0;i<4;++i){profiles.push_back(bytes(local_profiles_[i]));inputs.push_back(local_inputs_[i]);teams.push_back(int(local_teams_[i]));edits.push_back(local_team_edits_[i]);logins.push_back(local_discord_logins_[i]?Json(local_discord_logins_[i]->id):Json(nullptr));accounts.push_back(local_profile_accounts_[i]?Json(local_profile_accounts_[i]->id):Json(nullptr));}for(const auto&[id,w]:issued)live.push_back({id,w.use_count()});return {{"frames",frames},{"detached",encoded(detached)},{"profile",profile_},{"profiles",profiles},{"inputs",inputs},{"teams",teams},{"edits",edits},{"logins",logins},{"accounts",accounts},{"primary",discord_login_?Json(discord_login_->id):Json(nullptr)},{"account",online_account_?Json(online_account_->id):Json(nullptr)},{"authenticated",account_.authenticated},{"status",int(matchmaking_.status)},{"signed_in",bytes(signed_in_)},{"revision",revision},{"mode",mode_},{"next_actor",next_actor},{"live_logins",live}};}
 void call(const std::string&name,Json args=nullptr){if(!state_probe)return;auto state=state_probe();(void)party_flow_.state();++reentries;const auto at=reads++;calls.push_back({{"name",name},{"arguments",args},{"storage",storage()},{"state",state}});if(int(at)==fail)std::rethrow_exception(injected);if(!in_reentry&&reenter&&name==reenter_on){in_reentry=true;try{reenter();in_reentry=false;}catch(...){in_reentry=false;throw;}}}
 const MenuFrame&frame()const{return stack_.back();}
 void open(unsigned s,unsigned c=0){call("open",{s,c});MenuFrame f;f.screen=s;f.context=c;f.instance=0x100000001;stack_.push_back(std::move(f));++revision;}
 void back(){call("back");if(workshop||leave_pending)return;if(back_branch&&back_branch(false))return;if(overlay&&stack_.size()==1&&resume){call("resume");return;}if(back_branch&&back_branch(true))return;if(stack_.size()>1)stack_.pop_back();++revision;}
 void unavailable(std::string text){call("unavailable",bytes(text));open(8);}
 MenuFrame build(unsigned s,unsigned c)const{auto*self=const_cast<Model*>(this);self->call("build",{s,c});if(s==186){MenuFrame f;f.screen=s;f.description=description;return f;}auto f=next_frame;f.screen=s;f.context=c;if(populate_built&&populate_branch)populate_branch(f);return f;}
 std::string profile_name()const{const_cast<Model*>(this)->call("profile-name");return profile_name_value;}
 std::string profile_login_key(const std::string&n){call("profile-key",bytes(n));return std::string("key\0",4)+n;}
 FrontendAppearance appearance_for_profile(const std::string&n)const{const_cast<Model*>(this)->call("appearance",bytes(n));FrontendAppearance a;const auto size=n.size();for(unsigned i=0;i<a.colors.size();++i)a.colors[i]=std::int8_t(size+i);a.species=std::int8_t(size);a.foreground=0xff;a.background=0x80;a.flags=3;a.reserved=0x99;return a;}
 std::vector<FrontendLocalPlayer>local_players()const{const_cast<Model*>(this)->call("local-players");return roster;}
 std::shared_ptr<Login>make_login(unsigned id){auto p=std::make_shared<Login>();p->m=this;p->id=id;issued[id]=p;if(keep_alive)kept.push_back(p);return p;}
};
DiscordLoginSnapshot Login::snapshot()const{m->call("snapshot",id);return data;}std::optional<std::string>Login::authorization()const{m->call("authorized",id);return authorized?std::optional<std::string>("fixture private authorization"):std::nullopt;}bool Login::uses_steam()const{m->call("steam",id);return steam;}
void Login::cancel(){m->call("cancel",id);}void Login::bind_profile(std::string ns,std::string key){m->call("bind",{id,bytes(ns),bytes(key)});}void Login::start(){m->call("start",id);}
void Account::sign_out(){m->call("signout",id);}std::shared_ptr<Account>Account::profile_worker(std::shared_ptr<Login>login){m->call("profile-worker",{id,login?Json(login->id):Json(nullptr)});if(m->null_worker)return {};return std::make_shared<Account>(m,m->next_actor++);}
void Account::profile(FrontendAppearance a,std::string tag){m->call("profile",{id,appearance(a),bytes(tag)});}void Account::connect(std::string name){m->call("connect",{id,bytes(name)});}
}
#define COUCH_METHODS void set_local_discord_login_factory(std::function<std::shared_ptr<DiscordLogin>()>);bool online_couch_screen(const MenuFrame&)const;bool online_couch_mutable()const;void open_online_local_player(unsigned);void populate_online_couch(MenuFrame&)const;std::string online_couch_text(const MenuFrame&,unsigned)const;bool accept_online_couch();void update_online_couch();void clear_online_couch();std::vector<OnlineLocalPlayer>online_local_players()const;std::optional<unsigned>controller_slot(std::int32_t,bool);void finish_local_signin();void leave_local_controller(unsigned);std::optional<DiscordIdentity>online_identity()const;bool couch_back(bool=false);bool couch_profile_entry(unsigned,const std::string&)const;bool guest_entry(unsigned)const;
namespace before{
using namespace fixture;using DiscordLogin=fixture::Login;using OnlineAccount=fixture::Account;using OnlineLocalPlayer=fixture::LocalResult;
class MenuState:public Model{public:std::array<std::string,4>local_discord_subjects_{};std::optional<unsigned>online_local_options_,joining_controller_;bool joining_online_{};std::uint64_t couch_revision_{};unsigned host_profile_{};COUCH_METHODS std::optional<unsigned>joining_controller()const{return joining_controller_?joining_controller_:online_local_options_;}};
#include "frontend_couch_before.inc"
}
namespace after{
using namespace fixture;using DiscordLogin=fixture::Login;using OnlineAccount=fixture::Account;using OnlineLocalPlayer=fixture::LocalResult;
class MenuState:public Model{public:COUCH_METHODS FrontendCouchResult couch_flow_drive(unsigned,MenuFrame* =nullptr,unsigned=0,int=0,bool=false,std::string={})const;std::optional<DiscordIdentity>couch_identity()const;std::vector<OnlineLocalPlayer>couch_players()const;std::optional<unsigned>joining_controller()const{return party_flow_.active_controller();}};
#include "frontend_couch_after.inc"
}
namespace {
using namespace fixture;
MenuFrame frame(const Json&j){MenuFrame f;f.screen=j.value("screen",31u);f.context=j.value("context",0x434f5543u);f.selected=j.value("selected",0u);f.instance=j.value("instance",std::uint64_t(0xffffffffffffffff));f.pane_created_clock=j.value("pane_clock",99u);f.title=raw(j,"title","old title");f.description=raw(j,"description","old description");for(const auto&e:j.value("entries",Json::array()))f.entries.push_back({raw(e,"label","old"),e.value("destination",-80),e.value("value",0)});return f;}
std::shared_ptr<Login>login(Model&m,const Json&j,unsigned default_id){if(j.is_null())return {};auto p=m.make_login(j.value("generation",default_id));p->data.status=static_cast<DiscordLoginStatus>(j.value("status",3));p->data.message=raw(j,"message","opaque message");if(j.contains("identity")&&!j["identity"].is_null())p->data.identity=DiscordIdentity{raw(j,"identity"),raw(j,"username","opaque username"),raw(j,"display","opaque display")};p->authorized=j.value("authorized",true);p->steam=j.value("steam",false);return p;}
void configure(Model&m,const Json&j){m.keep_alive=j.value("keep_alive",false);m.account_.authenticated=j.value("authenticated",true);m.matchmaking_.status=static_cast<OnlineMatchmakingStatus>(j.value("status",3));if(j.value("account_client",true))m.online_account_=std::make_shared<Account>(&m,1);if(j.value("primary_login",true))m.discord_login_=login(m,j.value("primary",Json{{"identity","primary"},{"steam",true}}),2);
 m.mode_=j.value("mode",3);m.profile_=j.value("profile",2u);m.revision=j.value("revision",0u);m.signed_in_=raw(j,"signed_in","host");m.description=raw(j,"description","new description");m.profile_name_value=raw(j,"profile_name","selected profile");m.preferences_=raw(j,"preferences");
 for(unsigned i=0;i<4;++i){m.local_profiles_[i]=raw(j,("local"+std::to_string(i)).c_str());}
 if(j.contains("profiles"))for(unsigned i=0;i<4;++i)m.local_profiles_[i]=raw(Json{{"name",j["profiles"].at(i)}},"name");
 if(j.contains("inputs"))m.local_inputs_=j["inputs"].get<std::array<std::int32_t,4>>();
 for(unsigned i=0;i<4;++i){const auto key="login"+std::to_string(i);if(j.contains(key))m.local_discord_logins_[i]=login(m,j[key],10+i);if(j.value("accounts",0u)&(1u<<i))m.local_profile_accounts_[i]=std::make_shared<Account>(&m,20+i);}
 m.null_factory=j.value("null_factory",false);m.null_worker=j.value("null_worker",false);if(j.value("factory",true))m.local_discord_factory_=[&m]{m.call("factory");if(m.null_factory)return std::shared_ptr<Login>{};return m.make_login(m.next_actor++);};
 Json defaults=Json::array();defaults.push_back(Json{{"screen",6},{"context",0}});Json last{{"screen",31},{"context",0x434f5543}};last["entries"]=Json::array({Json{{"value",1}}});defaults.push_back(last);
 for(const auto&f:j.value("frames",defaults))m.stack_.push_back(frame(f));m.detached=frame(j.value("view",Json::object()));m.next_frame=frame(j.value("next_frame",Json{{"entries",Json::array({{{"label","new"},{"value",1}}})}}));m.populate_built=j.value("populate_built",false);if(j.contains("countdown")&&!j["countdown"].is_null())m.session_countdown_=j["countdown"].get<int>();if(j.value("system_link_host",false))m.system_link_host_=1;
 m.workshop=j.value("workshop",false);m.leave_pending=j.value("leave_pending",false);m.overlay=j.value("overlay",false);m.resume=j.value("resume",false);m.fail=j.value("fail",-1);m.reenter_on=raw(j,"reenter_on");
 for(const auto&p:j.value("players",Json::array())){FrontendLocalPlayer f;f.controller=p.value("controller",0u);f.name=raw(p,"name","same");f.appearance=m.appearance_for_profile(f.name);m.roster.push_back(f);}for(const auto&p:j.value("tags",Json::array()))m.profile_service_tags_[raw(p,"name")]=raw(p,"value");
}
template<class M>void initialize(M&m,const Json&j){const NativeData saved=m;
 m.account_.authenticated=true;m.discord_login_=m.make_login(90);m.discord_login_->data.identity=DiscordIdentity{"setup","setup","setup"};m.online_account_=std::make_shared<Account>(&m,91);m.matchmaking_.status=OnlineMatchmakingStatus::Ready;m.mode_=3;m.signed_in_="host";m.stack_={frame(Json{{"screen",186}})};
 if(j.contains("subjects")){for(unsigned i=1;i<4;++i){const auto&value=j["subjects"].at(i);if(!value.is_null()){m.local_discord_logins_[i]=login(m,Json{{"identity",value}},80+i);}}m.update_online_couch();}
 if(j.contains("joining")&&!j["joining"].is_null()){const auto index=j["joining"].get<unsigned>();if(index<1||index>3)throw std::logic_error("comparison joining producer requires actual controller 1..3");m.local_inputs_={5,-1,-1,-1};for(unsigned i=1;i<4;++i)m.local_profiles_[i]=i==index?"":"occupied";m.profile_=j.value("host_profile",7u);m.mode_=j.value("joining_online",true)?3:1;m.stack_={frame(Json{{"screen",m.mode_==3?186:30}})};m.controller_slot(42,true);}
 if(j.contains("options")&&!j["options"].is_null()){const auto index=j["options"].get<unsigned>();if(index<1||index>3)throw std::logic_error("comparison options producer requires actual controller 1..3");m.local_profiles_[index]="option";m.open_online_local_player(index);}
 static_cast<NativeData&>(m)=saved;
}
Json state(const before::MenuState&m){Json subjects=Json::array();for(const auto&s:m.local_discord_subjects_)subjects.push_back(bytes(s));return {{"subjects",subjects},{"options",m.online_local_options_?Json(*m.online_local_options_):Json(nullptr)},{"joining",m.joining_controller_?Json(*m.joining_controller_):Json(nullptr)},{"joining_online",m.joining_online_},{"host_profile",m.host_profile_},{"couch_revision",m.couch_revision_}};}
Json state(const after::MenuState&m){const auto s=m.party_flow_.state().couch;Json subjects=Json::array();for(const auto&p:s.subjects)subjects.push_back(bytes(p.value));return {{"subjects",subjects},{"options",s.options_present?Json(s.options):Json(nullptr)},{"joining",s.joining_present?Json(s.joining):Json(nullptr)},{"joining_online",s.joining_online},{"host_profile",s.host_profile},{"couch_revision",s.revision}};}
template<class M>Json run_one(const Json&j){M m;configure(m,j);m.back_branch=[&](bool joining){return m.couch_back(joining);};m.populate_branch=[&](MenuFrame&f){m.populate_online_couch(f);};initialize(m,j);m.state_probe=[&]{return state(m);};
 m.reenter=[&]{const auto op=j.value("reenter",std::string("replace-local"));const auto index=j.value("reenter_slot",1u);if(op=="replace-local")m.local_discord_logins_[index]=login(m,j.value("replacement_login",Json{{"identity","replacement"},{"authorized",false}}),70+index);else if(op=="reset-local")m.local_discord_logins_[index].reset();else if(op=="replace-account")m.local_profile_accounts_[index]=std::make_shared<Account>(&m,72);else if(op=="reset-account")m.local_profile_accounts_[index].reset();else if(op=="change-profile")m.local_profiles_[index]=raw(j,"replacement_name","changed profile");else if(op=="open-options")m.open_online_local_player(index);else if(op=="change-snapshot"){auto p=m.local_discord_logins_[index];if(p)p->data.identity=DiscordIdentity{"changed","changed username","changed display"};}else if(op=="clear")m.clear_online_couch();};Json result;
 try{const auto op=j.value("op",std::string("populate"));auto view=[&]()->MenuFrame&{return j.value("detached",false)?m.detached:m.stack_.back();};
 if(op=="populate")m.populate_online_couch(view());else if(op=="text")result["value"]=bytes(m.online_couch_text(view(),j.value("index",0u)));else if(op=="accept")result["value"]=m.accept_online_couch();else if(op=="update")m.update_online_couch();else if(op=="clear")m.clear_online_couch();else if(op=="mutable")result["value"]=m.online_couch_mutable();else if(op=="screen")result["value"]=m.online_couch_screen(view());else if(op=="identity"){auto value=m.online_identity();result["value"]=value?encoded(*value):Json(nullptr);}else if(op=="open")m.open_online_local_player(j.value("controller",1u));else if(op=="controller"){auto value=m.controller_slot(j.value("input",9),j.value("join",true));result["value"]=value?Json(*value):Json(nullptr);}else if(op=="finish")m.finish_local_signin();else if(op=="leave")m.leave_local_controller(j.value("controller",1u));else if(op=="back")m.back();else if(op=="back-branch")result["value"]=m.couch_back(j.value("joining_branch",false));else if(op=="guest-entry")result["value"]=m.guest_entry(j.value("screen",10u));else if(op=="profile-entry")result["value"]=m.couch_profile_entry(j.value("screen",10u),raw(j,"name","host"));else if(op=="selected-controller"){auto value=m.joining_controller();result["value"]=value?Json(*value):Json(nullptr);}else if(op=="players"){Json out=Json::array();for(const auto&p:m.online_local_players())out.push_back({bytes(p.name),p.login?Json(p.login->id):Json(nullptr),appearance(p.appearance)});result["value"]=out;}else throw std::logic_error("Invalid couch comparison operation");
 }catch(const NativeFailure&e){result["error"]="native";result["same_object"]=&e==original_exception;}catch(int n){result["error"]="int";result["integer"]=n;}catch(const std::exception&e){result["error"]="std";result["what"]=bytes(e.what());}
 result["storage"]=m.storage();result["state"]=state(m);result["calls"]=m.calls;result["reentries"]=m.reentries;return result;
}
Json run(const Json&j){try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}catch(const NativeFailure&e){injected=std::current_exception();original_exception=&e;}catch(...){injected=std::current_exception();original_exception=nullptr;}return {{"before",run_one<before::MenuState>(j)},{"rust",run_one<after::MenuState>(j)}};}
}
int main()try{for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
