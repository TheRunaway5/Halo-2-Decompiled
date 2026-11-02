#include "halo2/frontend.hpp"
#include "halo2/frontend_matchmaking_party.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <bit>
#include <exception>
#include <iostream>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace bridge=halo2::online_protocol_bridge;
namespace fixture {
Json bytes(const std::string&s){Json out=Json::array();for(unsigned char c:s)out.push_back(c);return out;}
std::string raw(const Json&j,const char*key,std::string fallback={}){if(!j.contains(key))return fallback;if(j[key].is_string())return j[key].get<std::string>();const auto b=j[key].get<std::vector<std::uint8_t>>();return {reinterpret_cast<const char*>(b.data()),b.size()};}
struct NativeFailure:std::exception{std::string message;explicit NativeFailure(std::string value):message(std::move(value)){}const char*what()const noexcept override{return message.c_str();}};
std::exception_ptr injected;const void *injected_address{};
struct Model;
Model *active_model{};
struct Account{Model*m;bool party_command(OnlinePartyCommand);OnlineAccountSnapshot snapshot();};
struct Queue{Model*m;void cancel_queue();};
struct Voice{Model*m;Voice(std::shared_ptr<Account>,bool);void leave();void join();void mute_microphone(bool);void mute(std::string,bool);OnlineVoiceSnapshot snapshot();};
Json command(const OnlinePartyCommand &c){return {{"action",bytes(c.action)},{"value",bytes(c.value)},{"playlist",bytes(c.playlist)},{"revision",bytes(c.revision)},{"version",c.version},{"privacy",c.privacy?Json(*c.privacy):Json(nullptr)},{"privacy_revision",c.privacy_revision},{"privacy_context",bytes(c.privacy_context)},{"join_context",bytes(c.join_context)}};}
OnlinePartyCommand command(const Json&j){OnlinePartyCommand c;c.action=raw(j,"action");c.value=raw(j,"value");c.playlist=raw(j,"playlist");c.revision=raw(j,"revision");c.version=j.value("version",std::uint64_t{});if(j.contains("privacy")&&!j["privacy"].is_null())c.privacy=j["privacy"].get<unsigned>();c.privacy_revision=j.value("privacy_revision",std::uint64_t{});c.privacy_context=raw(j,"privacy_context");c.join_context=raw(j,"join_context");return c;}
MenuFrame frame(const Json&j){MenuFrame f;f.screen=j.value("screen",26u);f.context=j.value("context",2u);f.instance=j.value("instance",std::uint64_t(0x10000000042));f.selected=j.value("selected",0u);for(const auto&e:j.value("entries",Json::array({{{"destination",31},{"value",0}}})))f.entries.push_back({raw(e,"label"),e.value("destination",31),e.value("value",0)});return f;}
OnlinePartyMember member(const Json&j){OnlinePartyMember p;p.handle=raw(j,"handle","member");p.name=raw(j,"name","same name");p.self=j.value("self",false);p.local_controller=j.value("controller",-1);return p;}
struct Model {
  int mode_=3;std::vector<MenuFrame> stack_;
  OnlineAccountSnapshot account_,worker;OnlineMatchmakingSnapshot matchmaking_;
  std::shared_ptr<Account> online_account_;std::shared_ptr<Queue> online_matchmaking_;std::shared_ptr<Voice> online_voice_;
  OnlineVoiceSnapshot voice_,worker_voice;bool voice_party_{},party_voice_wanted_{};
  std::string signed_in_="Pilot",keyboard_text="keyboard";std::optional<int> launch;
  unsigned revision{};std::uint64_t next_instance=0x10000001000,worker_serial=0xfffffffffffffff0ULL;
  std::vector<OnlinePartyMember> rows;
  std::string party_target_;unsigned party_target_kind_{},party_invite_tab_{};std::uint64_t party_target_version_{};
  std::vector<std::string> dismissed_party_invites_;
  struct OnlinePrivacyPending{std::uint64_t frame_instance{},command_serial{},revision{};unsigned mode{};std::string context;};
  std::optional<OnlinePrivacyPending> online_privacy_pending_;
  bool accepted=true,lobby_handled{},withdraw_queue{},publish_worker{},reenter_update{},command_busy{};
  int fail=-1;mutable unsigned call_index{};mutable Json calls=Json::array();
  std::function<void()> opened_hook,update_hook;
  void call(const std::string&name,Json args=nullptr)const{calls.push_back({name,args});const auto index=call_index++;if(int(index)==fail)std::rethrow_exception(injected);}
  const MenuFrame&frame()const{return stack_.back();}
  void open(unsigned screen,unsigned context=0){call("open",{screen,context});MenuFrame f;f.screen=screen;f.context=context;f.instance=next_instance++;stack_.push_back(std::move(f));if(opened_hook)opened_hook();}
  void back(){call("back");if(stack_.size()>1)stack_.pop_back();}
  void unavailable(std::string text){call("unavailable",bytes(text));open(8);}
  void populate_online_party(MenuFrame&f)const{call("populate",{f.screen,f.context});f.entries.push_back({"synthetic row",-5,0});}
  bool online_lobby_input(unsigned event,int destination){call("lobby",{event,destination});return lobby_handled;}
  std::vector<OnlinePartyMember> online_party_members()const{call("members");return rows;}
  void open_online_local_player(unsigned controller){call("local",controller);}
  void update_online_account(){call("update");if(publish_worker)account_=worker;if(reenter_update&&update_hook){call("reenter_privacy");update_hook();}}
  void configure(const Json&j){
    if(j.contains("mode"))mode_=j["mode"].get<int>();if(j.contains("signed_in"))signed_in_=raw(j,"signed_in");
    if(j.contains("frames")){stack_.clear();for(const auto&f:j["frames"])stack_.push_back(fixture::frame(f));}
    if(stack_.empty()){stack_.push_back(fixture::frame({{"screen",6},{"context",0}}));stack_.push_back(fixture::frame({{"screen",186},{"context",0}}));stack_.push_back(fixture::frame(Json::object()));}
    if(j.contains("frame"))stack_.back()=fixture::frame(j["frame"]);
    account_.authenticated=j.value("authenticated",account_.authenticated);account_.busy=j.value("busy",account_.busy);
    account_.party_message=raw(j,"message",account_.party_message);
    if(j.contains("party")){
      account_.party.reset();const auto&p=j["party"];if(!p.is_null()&&p!=false){account_.party.emplace();auto&party=*account_.party;party.leader=p.value("leader",true);party.version=p.value("version",std::uint64_t(41));
        if(!p.contains("privacy")||!p["privacy"].is_null()){const auto privacy=p.value("privacy",Json::object());party.privacy=OnlineParty::Privacy{privacy.value("mode",0u),privacy.value("revision",std::uint64_t(7)),raw(privacy,"context","context")};}
        for(const auto&i:p.value("outgoing",Json::array()))party.outgoing.push_back({raw(i,"id"),raw(i,"name","same name")});
        for(const auto&m:p.value("members",Json::array()))party.members.push_back(member(m));
      }
    }
    if(j.contains("invitations")){account_.party_invitations.clear();for(const auto&i:j["invitations"])account_.party_invitations.push_back({raw(i,"id"),raw(i,"name","same name"),i.value("version",std::uint64_t(0))});}
    if(j.contains("friends")){account_.friends.clear();for(const auto&i:j["friends"]){OnlineFriend f;f.id=raw(i,"id");f.state=raw(i,"state","friend");account_.friends.push_back(std::move(f));}}
    if(j.contains("clan")){account_.clan.reset();if(!j["clan"].is_null()){account_.clan.emplace();for(const auto&i:j["clan"]){OnlineClanMember m;m.id=raw(i,"id");account_.clan->members.push_back(std::move(m));}}}
    if(j.contains("rows")){rows.clear();for(const auto&m:j["rows"])rows.push_back(member(m));}
    account_.party_command_serial=j.value("serial",account_.party_command_serial);
    if(j.contains("successful")){account_.party_command_successful.reset();if(!j["successful"].is_null())account_.party_command_successful=j["successful"].get<bool>();}
    matchmaking_.status=static_cast<OnlineMatchmakingStatus>(j.value("status",unsigned(matchmaking_.status)));
    if(j.contains("launch")){launch.reset();if(j["launch"].get<bool>())launch=1;}
    if(j.contains("account_client")){online_account_.reset();if(j["account_client"].get<bool>())online_account_=std::make_shared<Account>(this);}
    if(j.contains("queue_client")){online_matchmaking_.reset();if(j["queue_client"].get<bool>())online_matchmaking_=std::make_shared<Queue>(this);}
    if(j.contains("voice_client")){online_voice_.reset();if(j["voice_client"].get<bool>())online_voice_=std::make_shared<Voice>(online_account_,true);}
    voice_party_=j.value("voice_party",voice_party_);party_voice_wanted_=j.value("voice_wanted",party_voice_wanted_);
    voice_.joined=j.value("joined",voice_.joined);voice_.connecting=j.value("connecting",voice_.connecting);voice_.microphone_muted=j.value("microphone_muted",voice_.microphone_muted);
    if(j.contains("muted")){voice_.muted.clear();for(const auto&id:j["muted"])voice_.muted.push_back(raw(Json{{"value",id}},"value"));}
    worker_voice=voice_;worker_serial=j.value("worker_serial",worker_serial);accepted=j.value("accept",accepted);lobby_handled=j.value("lobby_handled",lobby_handled);
    withdraw_queue=j.value("withdraw_queue",withdraw_queue);reenter_update=j.value("reenter_update",reenter_update);
    command_busy=j.value("command_busy",command_busy);
    if(j.contains("fail"))fail=j["fail"].get<int>();
  }
  void initial(const Json&j){active_model=this;account_.authenticated=true;online_account_=std::make_shared<Account>(this);online_matchmaking_=std::make_shared<Queue>(this);
    configure(Json{{"party",Json::object()},{"rows",Json::array({{{"handle","member"}}})}});configure(j);calls=Json::array();call_index=0;
  }
  Json snapshot()const{Json frames=Json::array();for(const auto&f:stack_){Json entries=Json::array();for(const auto&e:f.entries)entries.push_back({bytes(e.label),e.destination,e.value});frames.push_back({{"screen",f.screen},{"context",f.context},{"instance",f.instance},{"selected",f.selected},{"entries",entries}});}
    return {{"frames",frames},{"revision",revision},{"keyboard",bytes(keyboard_text)},{"voice_party",voice_party_},{"voice_wanted",party_voice_wanted_},{"queue_client",bool(online_matchmaking_)},{"voice_client",bool(online_voice_)},{"calls",calls}};
  }
};
bool Account::party_command(OnlinePartyCommand c){m->call("command",fixture::command(c));if(m->withdraw_queue)m->online_matchmaking_.reset();if(m->accepted&&m->command_busy)m->account_.busy=true;return m->accepted;}
OnlineAccountSnapshot Account::snapshot(){m->call("serial");auto out=m->account_;out.party_command_serial=m->worker_serial;return out;}
void Queue::cancel_queue(){m->call("cancel");}
Voice::Voice(std::shared_ptr<Account>a,bool party):m(a?a->m:active_model){m->call("voice_create",party);}
void Voice::leave(){m->call("voice_leave");}void Voice::join(){m->call("voice_join");}
void Voice::mute_microphone(bool value){m->call("voice_microphone",value);m->worker_voice.microphone_muted=value;}
void Voice::mute(std::string target,bool value){m->call("voice_mute",{bytes(target),value});}
OnlineVoiceSnapshot Voice::snapshot(){m->call("voice_snapshot");return m->worker_voice;}
}
#define PARTY_METHODS \
 std::optional<FrontendInviteNotice> invite_notice()const;bool act_invite_notice(bool); \
 bool online_party_screen(const MenuFrame&)const;bool party_invite_picker()const;bool switch_invite_tab(int); \
 void update_online_privacy();void party_command(OnlinePartyCommand);bool accept_online_party(); \
 void opened_privacy();void select_privacy(int);void prune_dismissed();void clear_dismissed();
namespace before {
using namespace fixture;using OnlineVoice=fixture::Voice;
class MenuState:public Model{public:PARTY_METHODS};
#include "frontend_party_flow_before.inc"
Json state(const MenuState&m){Json pending=nullptr;if(m.online_privacy_pending_){const auto&p=*m.online_privacy_pending_;pending={{"frame",p.frame_instance},{"serial",p.command_serial},{"revision",p.revision},{"mode",p.mode},{"context",bytes(p.context)}};}Json ids=Json::array();for(const auto&id:m.dismissed_party_invites_)ids.push_back(bytes(id));return {{"target",bytes(m.party_target_)},{"kind",m.party_target_kind_},{"version",m.party_target_version_},{"tab",m.party_invite_tab_},{"dismissed",ids},{"pending",pending}};}
}
namespace after {
using namespace fixture;using OnlineVoice=fixture::Voice;
class MenuState:public Model{public:FrontendMatchmakingPartyFlow party_flow_;bool party_flow_drive(FrontendPartyFlowAction,int value=0);PARTY_METHODS
 void initial(const Json&j){Model::initial(j);party_flow_.voice_party(voice_party_);party_flow_.voice_wanted(party_voice_wanted_);}
 void configure(const Json&j){Model::configure(j);if(j.contains("voice_party"))party_flow_.voice_party(voice_party_);if(j.contains("voice_wanted"))party_flow_.voice_wanted(party_voice_wanted_);}
 Json snapshot()const{auto out=Model::snapshot();out["voice_party"]=party_flow_.voice_party();out["voice_wanted"]=party_flow_.voice_wanted();return out;}
};
#include "frontend_party_flow_after.inc"
Json state(const MenuState&m){const auto s=m.party_flow_.state();const auto raw=[](const rust::Vec<std::uint8_t>&v){return Json(std::vector<std::uint8_t>(v.begin(),v.end()));};Json ids=Json::array();for(const auto&id:s.dismissed)ids.push_back(raw(id.value));Json pending=nullptr;if(s.pending)pending={{"frame",s.frame_instance},{"serial",s.command_serial},{"revision",s.revision},{"mode",s.mode},{"context",raw(s.context)}};return {{"target",raw(s.target)},{"kind",s.kind},{"version",s.version},{"tab",s.tab},{"dismissed",ids},{"pending",pending}};}
}
#undef PARTY_METHODS
namespace {
template<class Menu>Json evaluate(Menu&m,const Json&j){using namespace fixture;active_model=&m;m.initial(j.value("initial",Json::object()));m.opened_hook=[&]{m.opened_privacy();};m.update_hook=[&]{m.update_online_privacy();};Json results=Json::array();
  for(const auto&op:j.at("operations")){Json result;try{
    const auto name=op.at("op").template get<std::string>();
    if(name=="set")m.configure(op.at("value"));else if(name=="accept")result["return"]=m.accept_online_party();
    else if(name=="notice")result["return"]=m.act_invite_notice(op.value("accept",true));
    else if(name=="tab")result["return"]=m.switch_invite_tab(op.value("direction",1));
    else if(name=="privacy_open")m.opened_privacy();else if(name=="privacy_select")m.select_privacy(op.value("value",0));
    else if(name=="privacy_observe")m.update_online_privacy();else if(name=="prune")m.prune_dismissed();else if(name=="clear")m.clear_dismissed();
    else if(name=="command")m.party_command(fixture::command(op.at("value")));
    else if(name=="copy"){
      Menu copied=m;result["copied_state"]=state(copied);Menu assigned;assigned=copied;
      Menu moved=std::move(assigned);moved.clear_dismissed();result["moved_state_after_clear"]=state(moved);
      result["original_after_clear"]=state(m);
    }else throw std::logic_error("Unknown native party flow comparison operation");
    result["error"]=nullptr;
  }catch(const NativeFailure&e){result["error"]="native";result["same_object"]=&e==injected_address;result["message"]=bytes(std::string(e.what(),4));}
   catch(int value){result["error"]="int";result["value"]=value;}
   catch(const FormatError&e){result["error"]="format";result["message"]=bytes(e.what());}
   catch(const std::exception&e){result["error"]="other";result["message"]=bytes(e.what());}
   result["native"]=m.snapshot();result["state"]=state(m);results.push_back(std::move(result));
  }return results;
}
Json execute(const Json&j){using namespace fixture;injected=j.value("nonstd",false)?std::make_exception_ptr(7654321):std::make_exception_ptr(NativeFailure(std::string("bad\xff\0native",11)));injected_address=nullptr;try{std::rethrow_exception(injected);}catch(const NativeFailure&e){injected_address=&e;}catch(...){}
  before::MenuState old;after::MenuState current;return {{"before",evaluate(old,j)},{"rust",evaluate(current,j)}};
}
}
int main()try{std::string line;while(std::getline(std::cin,line))std::cout<<execute(Json::parse(line)).dump()<<'\n';}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
