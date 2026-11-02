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
template<class T>struct Bank{std::string name;std::vector<T>values;const T&at(std::size_t i)const{observe(name,i);return values.at(i);}};
struct Text{unsigned string{};};struct Pane{Bank<Text>texts{"text"};};struct Screen{Bank<Pane>panes{"pane"};};
struct Content{Bank<Screen>screens{"screen"};std::string localized(const Screen&,unsigned id)const{observe("localized",id);return std::string("\xff\0",2)+std::to_string(id);}};
struct Account{Model*m;unsigned id;void sign_out();bool command(OnlineClanCommand);};
struct Voice{Model*m;unsigned account_id{},id{};bool party{};explicit Voice(std::shared_ptr<Account>,bool=false);void leave();void join();void push_to_talk(bool);void mute_microphone(bool);void mute(const std::string&,bool);};
struct Social{Model*m;void bind_account(std::shared_ptr<Account>);};
struct NativeFailure:std::exception{const char*what()const noexcept override{return "native clans callback failed";}};
std::exception_ptr injected;const NativeFailure*original_exception{};
Json encoded(const OnlineClanCommand&c){return {bytes(c.action),bytes(c.value),c.role,c.confirmed};}
Json encoded(const bridge::ClansFlowCommand&c){return {bytes(c.action),bytes(c.value),c.role,c.confirmed};}
Json encoded(const OnlineAccountSnapshot&a){
 Json clan=nullptr,invitations=Json::array(),friends=Json::array(),steam=Json::array(),party_invites=Json::array(),party=nullptr;
 if(a.clan){const auto&c=*a.clan;Json members=Json::array(),outgoing=Json::array(),messages=Json::array();for(const auto&p:c.members)members.push_back({bytes(p.id),bytes(p.name),p.role,p.self,int(p.presence)});for(const auto&p:c.outgoing)outgoing.push_back({bytes(p.id),bytes(p.name)});for(const auto&p:c.messages)messages.push_back({bytes(p.id),bytes(p.name),bytes(p.body)});clan={bytes(c.id),bytes(c.name),c.role,members,outgoing,messages};}
 for(const auto&p:a.invitations)invitations.push_back({bytes(p.id),bytes(p.name)});
 auto friend_json=[](const OnlineFriend&p){return Json{bytes(p.id),bytes(p.name),bytes(p.tag),bytes(p.state),bytes(p.presence),p.party_joinable?Json(*p.party_joinable):Json(nullptr),bytes(p.party_context)};};
 for(const auto&p:a.friends)friends.push_back(friend_json(p));for(const auto&p:a.steam_friends)steam.push_back({bytes(p.id),bytes(p.name),p.registered,p.halo_online});for(const auto&p:a.party_invitations)party_invites.push_back({bytes(p.id),bytes(p.name),p.version});
 if(a.party){const auto&p=*a.party;Json members=Json::array(),outgoing=Json::array();for(const auto&m:p.members){Json locals=Json::array();for(const auto&l:m.local_players)locals.push_back({bytes(l.name),l.guest,bool(l.appearance)});members.push_back({bytes(m.handle),bytes(m.name),bytes(m.ready_playlist),m.self,m.leader,m.online,locals,m.local_controller,m.guest,bool(m.appearance),m.postgame?Json(*m.postgame):Json(nullptr)});}for(const auto&i:p.outgoing)outgoing.push_back({bytes(i.id),bytes(i.name)});party={p.version,p.leader,members,outgoing,p.countdown_supported,p.privacy?Json{p.privacy->mode,p.privacy->revision,bytes(p.privacy->context)}:Json(nullptr),p.countdown?Json{p.countdown->serial,p.countdown->seconds?Json(*p.countdown->seconds):Json(nullptr),p.countdown->mode,p.countdown->complete,bytes(p.countdown->requester)}:Json(nullptr)};}
 return {a.authenticated,a.busy,bytes(a.message),clan,invitations,a.revision,party,party_invites,bytes(a.party_message),a.party_command_serial,a.party_command_successful?Json(*a.party_command_successful):Json(nullptr),bytes(a.profile_message),steam,bytes(a.steam_friends_state),bytes(a.steam_friends_message),bytes(a.friend_tag),bytes(a.friends_message),a.friends_available,friends,a.friend_search?friend_json(*a.friend_search):Json(nullptr)};
}
Json encoded(const OnlineVoiceSnapshot&v){Json talking=Json::array(),muted=Json::array();for(const auto&s:v.talking)talking.push_back(bytes(s));for(const auto&s:v.muted)muted.push_back(bytes(s));return {v.joined,v.connecting,v.microphone_muted,v.transmitting,bytes(v.message),talking,muted,v.revision};}
Json encoded(const MenuFrame&f){Json entries=Json::array();for(const auto&e:f.entries)entries.push_back({bytes(e.label),e.destination,e.value});return {f.screen,f.context,f.selected,bytes(f.title),bytes(f.description),entries};}
struct Model{
 Content content_;std::shared_ptr<Account>online_account_;std::shared_ptr<Voice>online_voice_;std::shared_ptr<Social>discord_social_;std::array<std::shared_ptr<Account>,4>local_profile_accounts_{};
 OnlineAccountSnapshot account_,account_after;OnlineVoiceSnapshot voice_;FrontendMatchmakingPartyFlow party_flow_;std::vector<FrontendUiSound>pending_ui_sounds_;
 std::vector<MenuFrame>stack_,frames_after;MenuFrame detached;std::string keyboard_text;
 unsigned revision{},next_voice_id{1};bool reply{true},refresh_account{},refresh_frames{},in_reentry{};int fail=-1;unsigned reads{},reentries{};std::string reenter_on;Json calls=Json::array();std::function<Json()>state_probe;std::function<void()>reenter;
 Json storage()const{Json frames=Json::array(),locals=Json::array(),sounds=Json::array();for(const auto&f:stack_)frames.push_back(encoded(f));for(const auto&a:local_profile_accounts_)locals.push_back(a?Json(a->id):Json(nullptr));for(auto sound:pending_ui_sounds_)sounds.push_back(unsigned(sound));return {{"account",encoded(account_)},{"voice",encoded(voice_)},{"frames",frames},{"detached",encoded(detached)},{"keyboard",bytes(keyboard_text)},{"revision",revision},{"account_client",online_account_?Json(online_account_->id):Json(nullptr)},{"voice_client",online_voice_?Json{online_voice_->id,online_voice_->account_id,online_voice_->party}:Json(nullptr)},{"locals",locals},{"sounds",sounds},{"next_voice_id",next_voice_id}};}
 void call(const std::string&name,Json args=nullptr){if(!state_probe)return;auto state=state_probe();(void)party_flow_.state();++reentries;const auto at=reads++;calls.push_back({{"name",name},{"arguments",args},{"storage",storage()},{"state",state}});if(int(at)==fail)std::rethrow_exception(injected);if(!in_reentry&&reenter&&name==reenter_on){in_reentry=true;try{reenter();in_reentry=false;}catch(...){in_reentry=false;throw;}}}
 const MenuFrame&frame()const{return stack_.back();}
 void open(unsigned s,unsigned c=0){call("open",{s,c});MenuFrame f;f.screen=s;f.context=c;stack_.push_back(std::move(f));++revision;}
 void back(){call("back");if(stack_.size()>1)stack_.pop_back();++revision;}
 void select(unsigned i){call("select",i);stack_.back().selected=i;++revision;}
 void unavailable(std::string text){call("unavailable",bytes(text));open(8);}
 void party_command(OnlinePartyCommand c){call("wrapped-party",{bytes(c.action),bytes(c.value),bytes(c.playlist),bytes(c.revision),c.version,c.privacy?Json(*c.privacy):Json(nullptr),c.privacy_revision,bytes(c.privacy_context),bytes(c.join_context)});}
 void account_flow_drive(bool c){call("update-account",c);if(refresh_account)account_=account_after;if(refresh_frames)stack_=frames_after;}
};
void observe(const std::string&name,Json args){active->call(name,std::move(args));}
void Account::sign_out(){m->call("signout",id);}bool Account::command(OnlineClanCommand c){m->call("command",encoded(c));return m->reply;}
Voice::Voice(std::shared_ptr<Account>a,bool p):m(a?a->m:active),account_id(a?a->id:0),party(p){m->call("voice-create",{account_id,party});id=m->next_voice_id++;}
void Voice::leave(){m->call("voice-leave",id);}void Voice::join(){m->call("voice-join",id);}void Voice::push_to_talk(bool held){m->call("ptt",{id,held});}void Voice::mute_microphone(bool value){m->call("microphone",{id,value});}void Voice::mute(const std::string&id_,bool value){m->call("mute",{id,bytes(id_),value});}
void Social::bind_account(std::shared_ptr<Account>a){m->call("social-bind",a?Json(a->id):Json(nullptr));}
}
#define CLANS_METHODS void set_online_account(std::shared_ptr<OnlineAccount>);void set_online_voice(std::shared_ptr<OnlineVoice>);void voice_push_to_talk(bool);bool online_clan_screen(const MenuFrame&)const;void update_online_account();void populate_online_clans(MenuFrame&)const;std::string online_clan_column(const MenuFrame&,unsigned,unsigned)const;std::string online_clan_text(const MenuFrame&,unsigned)const;void clan_command(OnlineClanCommand);void confirm_clan(OnlineClanCommand,std::string);bool accept_online_clans();bool clan_message_advance();
namespace before{
using namespace fixture;using OnlineAccount=fixture::Account;using OnlineVoice=fixture::Voice;
class MenuState:public Model{public:std::string clan_target_;unsigned clan_target_kind_{},clan_message_page_{};std::optional<OnlineClanCommand>clan_confirmation_;CLANS_METHODS};
#include "frontend_clans_before.inc"
}
namespace after{
using namespace fixture;using OnlineAccount=fixture::Account;using OnlineVoice=fixture::Voice;
class MenuState:public Model{public:CLANS_METHODS FrontendClansResult clans_flow_drive(unsigned,MenuFrame* =nullptr,unsigned=0,unsigned=0,bool=false,OnlineClanCommand={},std::string={})const;void clans_bind_account(std::shared_ptr<OnlineAccount>);void clans_bind_voice(std::shared_ptr<OnlineVoice>);};
#include "frontend_clans_after.inc"
}
namespace {
using namespace fixture;
OnlineClanCommand clan_command(const Json&j){return {raw(j,"action","role"),raw(j,"value"),j.value("role",0u),j.value("confirmed",false)};}
OnlineAccountSnapshot account(const Json&j){OnlineAccountSnapshot a;a.authenticated=j.value("authenticated",true);a.busy=j.value("busy",false);a.message=raw(j,"message","account message");a.revision=j.value("revision",std::uint64_t(0x100000001));
 if(j.contains("clan")&&!j["clan"].is_null()){const auto&c=j["clan"];a.clan.emplace();a.clan->id=raw(c,"id","clan id");a.clan->name=raw(c,"name","clan name");a.clan->role=c.value("role",3u);for(const auto&p:c.value("members",Json::array()))a.clan->members.push_back({raw(p,"id"),raw(p,"name","same"),p.value("role",1u),p.value("self",false),static_cast<OnlineClanPresence>(p.value("presence",0))});for(const auto&p:c.value("outgoing",Json::array()))a.clan->outgoing.push_back({raw(p,"id"),raw(p,"name","same")});for(const auto&p:c.value("messages",Json::array()))a.clan->messages.push_back({raw(p,"id"),raw(p,"name","same"),raw(p,"body")});}
 for(const auto&p:j.value("invitations",Json::array()))a.invitations.push_back({raw(p,"id"),raw(p,"name","same")});
 if(j.value("party",true)){a.party.emplace();a.party->version=0xffffffffffffffff;a.party->leader=j.value("leader",true);a.party->members.push_back({"opaque","member","playlist",true,true,true,{{"local",true,{}}},3,true,{},true});a.party->outgoing={{"invite","name"}};a.party->countdown_supported=true;a.party->countdown=OnlineParty::Countdown{99,-1,2,true,"requester"};a.party->privacy=OnlineParty::Privacy{3,99,"privacy"};}
 // Opaque unrelated cached data must survive non-reset operations byte for byte.
 a.party_invitations={{"party invite","name",88}};a.party_message="party message";a.party_command_serial=0xffffffffffffffff;a.party_command_successful=false;a.profile_message="profile";a.steam_friends={{"steam","name",true,true}};a.steam_friends_state="steam state";a.steam_friends_message="steam message";a.friend_tag="tag";a.friends_message="friends message";a.friends_available=true;a.friends={{"friend","name","tag","friend","playing",false,"private"}};a.friend_search=a.friends.front();return a;
}
OnlineVoiceSnapshot voice(const Json&j){OnlineVoiceSnapshot v;v.joined=j.value("joined",false);v.connecting=j.value("connecting",false);v.microphone_muted=j.value("microphone",false);v.transmitting=j.value("transmitting",false);v.message=raw(j,"message","voice message");v.revision=j.value("revision",std::uint64_t(0x100000001));for(const auto&id:j.value("muted",Json::array()))v.muted.push_back(raw(Json{{"id",id}},"id"));for(const auto&id:j.value("talking",Json::array()))v.talking.push_back(raw(Json{{"id",id}},"id"));return v;}
MenuFrame frame(const Json&j){MenuFrame f;f.screen=j.value("screen",26u);f.context=j.value("context",1u);f.selected=j.value("selected",0u);f.title=raw(j,"title","old title");f.description=raw(j,"description","old description");for(const auto&e:j.value("entries",Json::array()))f.entries.push_back({raw(e,"label","old"),e.value("destination",31),e.value("value",0)});return f;}
void configure(Model&m,const Json&j){active=&m;m.account_=account(j.value("account",Json::object()));m.voice_=voice(j.value("voice",Json::object()));m.keyboard_text=raw(j,"keyboard","old keyboard");m.reply=j.value("reply",true);
 if(j.value("account_client",true))m.online_account_=std::make_shared<Account>(&m,1);if(j.value("social_client",true))m.discord_social_=std::make_shared<Social>(&m);if(j.value("voice_client",true))m.online_voice_=std::make_shared<Voice>(m.online_account_,j.value("device_party",false));
 for(unsigned i=0;i<4;++i)if(j.value("locals",15u)&(1u<<i))m.local_profile_accounts_[i]=std::make_shared<Account>(&m,10+i);
 for(const auto&f:j.value("frames",Json::array({{{"screen",6}},{{"screen",186}},{{"screen",26},{"context",1}}})))m.stack_.push_back(frame(f));m.detached=frame(j.value("view",Json::object()));m.revision=j.value("revision",0u);for(unsigned i=0;i<j.value("sounds",2u);++i)m.pending_ui_sounds_.push_back(FrontendUiSound::Tab);
 m.party_flow_.account_name(raw(j,"account_name","old name"));m.party_flow_.friend_join_serial(j.value("join_serial",std::uint64_t(0xffffffffffffffff)));m.party_flow_.voice_party(j.value("voice_party",false));m.party_flow_.voice_wanted(j.value("voice_wanted",true));
 m.content_.screens.values.resize(j.value("screen_count",237u));for(unsigned s:{26u,236u})if(m.content_.screens.values.size()>s){auto&screen=m.content_.screens.values[s];screen.panes.values.resize(j.value("pane_count",4u));for(unsigned p=0;p<screen.panes.values.size();++p){auto&pane=screen.panes.values[p];pane.texts.values.resize(j.value("text_count",9u));for(unsigned i=0;i<pane.texts.values.size();++i)pane.texts.values[i].string=s*100+p*10+i;}}
 if(j.contains("account_after")){m.refresh_account=true;m.account_after=account(j["account_after"]);}if(j.contains("frames_after")){m.refresh_frames=true;for(const auto&f:j["frames_after"])m.frames_after.push_back(frame(f));}m.fail=j.value("fail",-1);m.reenter_on=raw(j,"reenter_on");
}
template<class M>void initialize(M&m,const Json&j){auto frames=m.stack_;auto account=m.account_;auto revision=m.revision;const auto target=raw(j,"target");const auto kind=j.value("target_kind",0u);if(!target.empty()){
 m.account_.clan.emplace();m.account_.clan->members={{target,"target",1,false,OnlineClanPresence::Online}};m.account_.clan->outgoing={{target,"target"}};m.account_.invitations={{target,"target"}};m.stack_={frame(Json{{"entries",Json::array({{{"destination",31},{"value",kind==2?2000:kind==1?1000:0}}})}})};m.accept_online_clans();}
 if(j.contains("confirmation"))m.confirm_clan(clan_command(j["confirmation"]),"old confirmation");
 m.stack_={frame(Json{{"screen",211},{"context",41}})};for(unsigned i=0;i<j.value("page",0u);++i)m.clan_message_advance();m.stack_=frames;m.account_=account;m.revision=revision;
}
Json state(const before::MenuState&m){return {{"target",bytes(m.clan_target_)},{"kind",m.clan_target_kind_},{"page",m.clan_message_page_},{"confirmation",m.clan_confirmation_?encoded(*m.clan_confirmation_):Json(nullptr)},{"account_name",bytes(m.party_flow_.account_name())},{"serial",m.party_flow_.friend_join_serial()},{"voice_party",m.party_flow_.voice_party()},{"voice_wanted",m.party_flow_.voice_wanted()},{"dismissed",m.party_flow_.dismissed()}};}
Json state(const after::MenuState&m){auto s=m.party_flow_.state();return {{"target",bytes(s.clan_target)},{"kind",s.clan_target_kind},{"page",s.clan_message_page},{"confirmation",s.clan_confirmation_present?encoded(s.clan_confirmation):Json(nullptr)},{"account_name",bytes(s.account_name)},{"serial",s.friend_join_serial},{"voice_party",s.voice_party},{"voice_wanted",s.voice_wanted},{"dismissed",m.party_flow_.dismissed()}};}
template<class M>Json run_one(const Json&j){M m;configure(m,j);initialize(m,j);m.state_probe=[&]{return state(m);};
 m.reenter=[&]{const auto op=j.value("reenter",std::string("advance"));if(op=="advance")m.clan_message_advance();else if(op=="same-account")m.set_online_account(m.online_account_);else if(op=="drop-local")m.local_profile_accounts_[2].reset();else if(op=="replace-account")m.online_account_=std::make_shared<Account>(&m,3);else if(op=="drop-account")m.online_account_.reset();else if(op=="replace-clan")m.account_=account(j.value("account_after",Json::object()));else if(op=="confirm")m.confirm_clan({"remove","reentry",0,true},"reentered");};Json result;
 try{const auto op=j.value("op",std::string("populate"));auto view=[&]()->MenuFrame&{return j.value("detached",false)?m.detached:m.stack_.back();};
 if(op=="populate")m.populate_online_clans(view());else if(op=="text")result["value"]=bytes(m.online_clan_text(view(),j.value("index",0u)));else if(op=="column")result["value"]=bytes(m.online_clan_column(view(),j.value("row",0u),j.value("column",1u)));else if(op=="screen")result["value"]=m.online_clan_screen(view());else if(op=="accept")result["value"]=m.accept_online_clans();else if(op=="advance")result["value"]=m.clan_message_advance();else if(op=="command")m.clan_command(clan_command(j.value("command",Json::object())));else if(op=="confirm")m.confirm_clan(clan_command(j.value("command",Json::object())),raw(j,"message","confirm message"));else if(op=="ptt")m.voice_push_to_talk(j.value("held",false));else if(op=="bind-account"){const auto mode=j.value("replacement",2);m.set_online_account(mode==0?nullptr:mode==1?m.online_account_:std::make_shared<Account>(&m,2));}else if(op=="bind-voice"){const auto mode=j.value("replacement",2);m.set_online_voice(mode==0?nullptr:mode==1?m.online_voice_:std::make_shared<Voice>(m.online_account_,j.value("replacement_party",true)));}else throw std::logic_error("Invalid clan comparison operation");
 }catch(const NativeFailure&e){result["error"]="native";result["same_object"]=&e==original_exception;}catch(int n){result["error"]="int";result["integer"]=n;}catch(const std::exception&e){result["error"]="std";result["what"]=bytes(e.what());}
 result["storage"]=m.storage();result["state"]=state(m);result["calls"]=m.calls;result["reentries"]=m.reentries;return result;
}
Json run(const Json&j){try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}catch(const NativeFailure&e){injected=std::current_exception();original_exception=&e;}catch(...){injected=std::current_exception();original_exception=nullptr;}return {{"before",run_one<before::MenuState>(j)},{"rust",run_one<after::MenuState>(j)}};}
}
int main()try{for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
