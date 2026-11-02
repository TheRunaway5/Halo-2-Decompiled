#include "halo2/frontend_matchmaking_lobby.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
using Json=nlohmann::json;
namespace bridge=halo2::online_protocol_bridge;
namespace fixture {
struct Countdown {std::uint64_t serial{};std::optional<int> seconds;bool complete{};};
struct Member {std::string handle,ready_playlist;};
struct Party {bool leader{},countdown_supported{};std::optional<Countdown> countdown;std::vector<Member> members;};
struct Account {bool authenticated{},busy{};std::optional<Party> party;std::string party_message;};
struct Playlist {std::string id,revision;bool eligible{};};
struct Entry {int destination{};};
struct Frame {unsigned screen{};std::vector<Entry> entries;unsigned context{};};
struct PartyCommand {std::string action,value,playlist,revision;};
enum class FrontendUiSound {Error=2,PregameCountdown=10};
struct NativeRefusal:std::overflow_error {using std::overflow_error::overflow_error;};
struct Model;
struct Queue {Model *m;void cancel_queue();void sign_out();};
struct Client {Model *m;bool party_command(PartyCommand);};
struct Model {
  int mode_=3;Account account_;
  struct Snapshot {std::vector<Playlist> playlists;} matchmaking_;
  std::string selected_playlist_="ranked";
  bool quickmatch_pending_=true,matchmaking_active_=true,online_lobby_playlist_pending_=true;
  std::optional<bool> launch=true;
  std::vector<Frame> stack_{{6},{186},{14}};
  std::vector<FrontendUiSound> pending_ui_sounds_;
  std::shared_ptr<Queue> online_matchmaking_;
  std::shared_ptr<Client> online_account_;
  Json trace=Json::array();std::uint64_t revision{};
  bool accept_command=true;std::string fault;std::exception_ptr native_exception;const void *native_address{};
  Model(){online_matchmaking_=std::make_shared<Queue>(this);online_account_=std::make_shared<Client>(this);}
  const Frame&frame()const{return stack_.back();}
  void call(std::string name,Json value=Json()) {
    trace.push_back({name,value});
    if(fault==name){try{throw NativeRefusal("actual native lobby refusal: "+name);}catch(const NativeRefusal &e){native_address=&e;native_exception=std::current_exception();throw;}}
  }
  void populate_online_party(Frame&){call("populate");}
  void reset_online_matchmaking_progress(){call("reset_progress");}
  void open(unsigned id){call("open",id);stack_.push_back({id});}
  void dialog(unsigned id,int destination,bool quiet=false){call("dialog",Json::array({id,destination,quiet}));stack_.push_back({7,{{destination}}});}
  void unavailable(std::string message){call("unavailable",message);}
  void update_online_account(){call("update_account");}
  void load(const Json&f) {
    mode_=f.value("mode",3);stack_.clear();for(auto s:f.value("screens",Json::array({6,186,14})))stack_.push_back({s.get<unsigned>()});
    if(stack_.empty())throw std::runtime_error("Fixture requires an actual top frame");
    account_={};account_.authenticated=f.value("authenticated",true);account_.busy=f.value("busy",false);account_.party_message=f.value("party_message",std::string{});
    if(f.value("party",true)) {
      Party p;p.leader=f.value("leader",true);p.countdown_supported=f.value("supported",true);
      for(auto m:f.value("members",Json::array({{{"handle","self"},{"ready","ranked"}},{{"handle","peer"},{"ready","ranked"}}})))p.members.push_back({m.at("handle"),m.at("ready")});
      if(f.contains("countdown")&&!f["countdown"].is_null()) {
        const auto &c=f["countdown"];Countdown v;v.serial=c.value("serial",std::uint64_t(1));v.complete=c.value("complete",false);
        if(c.contains("seconds")&&!c["seconds"].is_null())v.seconds=c["seconds"].get<int>();p.countdown=v;
      }
      account_.party=p;
    }
    selected_playlist_=f.value("selected",std::string("ranked"));matchmaking_.playlists.clear();
    for(auto p:f.value("playlists",Json::array({{{"id","ranked"},{"revision","v7"},{"eligible",true}}})))matchmaking_.playlists.push_back({p.at("id"),p.at("revision"),p.at("eligible")});
    online_matchmaking_=f.value("queue_client",true)?std::make_shared<Queue>(this):nullptr;
    online_account_=f.value("account_client",true)?std::make_shared<Client>(this):nullptr;
    accept_command=f.value("command_accepted",true);fault=f.value("fault",std::string{});
    if(f.contains("sounds"))pending_ui_sounds_.assign(f["sounds"].get<unsigned>(),FrontendUiSound::Error);
    trace=Json::array();native_exception={};native_address=nullptr;
  }
  Json io_view()const {
    Json screens=Json::array(),sounds=Json::array();for(const auto&f:stack_)screens.push_back(f.screen);
    for(auto s:pending_ui_sounds_)sounds.push_back(unsigned(s));
    return {{"revision",revision},{"screens",screens},{"sounds",sounds},{"trace",trace},{"quickmatch",quickmatch_pending_},
      {"active",matchmaking_active_},{"playlist_pending",online_lobby_playlist_pending_},{"selected",selected_playlist_},{"launch",launch.has_value()}};
  }
  bridge::FrontendLobbyFacts facts()const {
    bridge::FrontendLobbyFacts f{};f.mode=mode_;for(const auto&s:stack_)f.screens.push_back(s.screen);
    f.account_client=bool(online_account_);f.authenticated=account_.authenticated;f.busy=account_.busy;f.party_message_present=!account_.party_message.empty();
    f.queue_client=bool(online_matchmaking_);f.selected_playlist=selected_playlist_;
    for(const auto&p:matchmaking_.playlists)f.playlists.push_back({p.id,p.revision,p.eligible});
    f.party_present=account_.party.has_value();if(account_.party){const auto&p=*account_.party;f.leader=p.leader;f.countdown_supported=p.countdown_supported;
      for(const auto&m:p.members)f.members.push_back({m.handle,m.ready_playlist});f.countdown_present=p.countdown.has_value();
      if(p.countdown){const auto&c=*p.countdown;f.serial=c.serial;f.complete=c.complete;f.seconds_present=c.seconds.has_value();f.seconds=c.seconds.value_or(0);}}
    return f;
  }
};
void Queue::cancel_queue(){m->call("cancel_queue");}void Queue::sign_out(){m->call("sign_out");}
bool Client::party_command(PartyCommand c){m->call("command",Json::array({c.action,c.value,c.playlist,c.revision}));return m->accept_command;}
}
// Required reference header/source bytes are included unchanged. All native
// UI/account observations above are declared deterministic fixture IO.
namespace cpp_before {
#include "reference/frontend_lobby_before_rust_20261102/frontend_matchmaking_progress.hpp.reference"
namespace halo2 {
using namespace ::fixture;
using MenuFrame=Frame;
constexpr unsigned party_context=42,invite_context=45;
struct MenuState:Model {
  bool online_lobby_matchmaking_{},online_lobby_leave_pending_{};
  int online_lobby_countdown_=-1,online_lobby_countdown_stage_=-1;
  std::uint64_t online_lobby_countdown_consumed_{};std::vector<std::string> online_lobby_countdown_party_;
  void enter_online_matchmaking_lobby();void cancel_online_lobby_countdown();bool update_online_lobby_countdown();
  void update_online_lobby_leave();std::string online_lobby_start_error()const;
  bool online_party_screen(const Frame&)const;
  bool accept(int destination){const auto f=frame();if(!online_party_screen(f))return false;Entry entry{destination};
#include "reference/frontend_lobby_before_rust_20261102/input-accept.cpp.inc"
    return false;
  }
  void back(){
#include "reference/frontend_lobby_before_rust_20261102/input-back.cpp.inc"
    dialog(0x8b,-11);
  }
  void auxiliary(){
#include "reference/frontend_lobby_before_rust_20261102/input-auxiliary.cpp.inc"
  }
  void leave(){
#include "reference/frontend_lobby_before_rust_20261102/input-leave.cpp.inc"
    while(stack_.size()>1&&frame().screen!=6)stack_.pop_back();++revision;
  }
  Json state()const {return {{"matchmaking",online_lobby_matchmaking_},{"seconds",online_lobby_countdown_},{"stage",online_lobby_countdown_stage_},
      {"consumed",online_lobby_countdown_consumed_},{"counted",online_lobby_countdown_party_},{"leave_pending",online_lobby_leave_pending_}};}
};
#include "reference/frontend_lobby_before_rust_20261102/frontend_lobby_policy.cpp.inc"
#include "reference/frontend_lobby_before_rust_20261102/online_party_screen.cpp.inc"
}}
namespace candidate {
using namespace fixture;
struct MenuState:Model {
  rust::Box<bridge::FrontendLobby> lobby=bridge::new_frontend_lobby();
  void enter(){bridge::frontend_lobby_enter(*lobby,facts());if(frame().screen==14&&mode_==3){stack_.back().entries.clear();populate_online_party(stack_.back());++revision;}}
  bool observe(){const auto e=bridge::frontend_lobby_observe(*lobby,facts());if(e.departure){quickmatch_pending_=false;if(online_matchmaking_)online_matchmaking_->cancel_queue();reset_online_matchmaking_progress();while(stack_.size()>1&&frame().screen!=14)stack_.pop_back();dialog(0xae,-5,true);++revision;return true;}
    if(e.revision)++revision;if(e.beep&&pending_ui_sounds_.size()<16)pending_ui_sounds_.push_back(FrontendUiSound::PregameCountdown);
    if(e.search){while(stack_.size()>1&&frame().screen!=14)stack_.pop_back();open(210);return true;}return false;}
  void leave_observe(){if(!bridge::frontend_lobby_observe_leave(*lobby,facts()))return;if(online_matchmaking_)online_matchmaking_->sign_out();bridge::frontend_lobby_left(*lobby);
    matchmaking_active_=quickmatch_pending_=false;matchmaking_.playlists.clear();selected_playlist_.clear();online_lobby_playlist_pending_=false;
    reset_online_matchmaking_progress();launch.reset();while(stack_.size()>1&&frame().screen!=6)stack_.pop_back();++revision;}
  bool input(unsigned event,int destination){auto p=bridge::frontend_lobby_plan(*lobby,facts(),event,destination);const auto c=bridge::frontend_lobby_plan_command(*p);
    const auto playlist=std::string(bridge::frontend_lobby_plan_playlist(*p)),version=std::string(bridge::frontend_lobby_plan_revision(*p));
    bool accepted=true;if(c){const char*names[]{"","countdown_start","countdown_cancel","countdown_request","leave_lobby"};accepted=online_account_->party_command({names[c],{},playlist,version});}
    const auto e=bridge::frontend_lobby_finish_input(*lobby,std::move(p),accepted);if(!e.handled)return false;
    if(e.sound&&(!e.bounded_sound||pending_ui_sounds_.size()<16))pending_ui_sounds_.push_back(FrontendUiSound::Error);
    if(!e.error.empty())unavailable(std::string(e.error));if(e.refresh_account)update_online_account();if(e.confirm_leave)dialog(0x8b,-11);if(e.friends)open(216);
    if(e.begin_leave){if(online_matchmaking_)online_matchmaking_->cancel_queue();quickmatch_pending_=false;update_online_account();leave_observe();}return true;}
  Json state()const {const auto v=bridge::frontend_lobby_view(*lobby);std::vector<std::string> counted;for(const auto&s:v.counted)counted.push_back(std::string(s));
    return {{"matchmaking",v.matchmaking},{"seconds",v.seconds},{"stage",v.stage},{"consumed",v.consumed},{"counted",counted},{"leave_pending",v.leave_pending}};}
};
}
template<class M,class F>Json invoke(M&m,F&&f){Json result;std::string error;bool identity=true;try{result=f();}catch(const fixture::NativeRefusal&e){error=e.what();identity=m.native_address==&e;}catch(const std::exception&e){error=e.what();identity=false;}
  return {{"value",result},{"error",error},{"native_exception_identity",identity},{"state",m.state()},{"io",m.io_view()}};}
int main()try {
  std::string line;while(std::getline(std::cin,line)){
    const auto request=Json::parse(line);cpp_before::halo2::MenuState old;candidate::MenuState next;Json results=Json::array();
    for(const auto&cmd:request.at("commands")){
      const auto f=cmd.value("facts",Json::object());old.load(f);next.load(f);const auto op=cmd.at("op").get<std::string>();const int destination=cmd.value("destination",-17);
      auto before=invoke(old,[&]()->Json{if(op=="enter"){old.enter_online_matchmaking_lobby();return nullptr;}if(op=="cancel"){old.cancel_online_lobby_countdown();return nullptr;}
        if(op=="exit"){old.online_lobby_matchmaking_=false;old.cancel_online_lobby_countdown();return nullptr;}
        if(op=="observe")return old.update_online_lobby_countdown();if(op=="start_error")return old.online_lobby_start_error();if(op=="leave_observe"){old.update_online_lobby_leave();return nullptr;}
        if(op=="accept")return old.accept(destination);if(op=="back"){old.back();return nullptr;}if(op=="auxiliary"){old.auxiliary();return nullptr;}if(op=="leave"){old.leave();return nullptr;}throw std::runtime_error("Unknown fixture operation");});
      auto after=invoke(next,[&]()->Json{if(op=="enter"){next.enter();return nullptr;}if(op=="cancel"){bridge::frontend_lobby_cancel(*next.lobby);return nullptr;}if(op=="exit"){bridge::frontend_lobby_exit(*next.lobby);return nullptr;}
        if(op=="observe")return next.observe();if(op=="start_error")return std::string(bridge::frontend_lobby_start_error(*next.lobby,next.facts()));if(op=="leave_observe"){next.leave_observe();return nullptr;}
        if(op=="accept")return next.input(0,destination);if(op=="back"){if(!next.input(1,destination))next.dialog(0x8b,-11);return nullptr;}if(op=="auxiliary"){next.input(2,destination);return nullptr;}
        if(op=="leave"){if(!next.input(3,destination)){while(next.stack_.size()>1&&next.frame().screen!=6)next.stack_.pop_back();++next.revision;}return nullptr;}throw std::runtime_error("Unknown fixture operation");});
      results.push_back({{"cpp",before},{"rust",after}});
    }
    // Exercise the actual native wrapper independently of the modeled UI IO.
    fixture::Model valid;valid.load(Json::object());valid.account_.party->countdown=fixture::Countdown{1,2,false};
    halo2::FrontendMatchmakingLobby native;auto f=valid.facts();native.enter(f);native.observe(f);halo2::FrontendMatchmakingLobby copy=native;
    const auto plan_before=native.seconds();bool actual_identity=false,callback_completed=false;const void* address{};std::exception_ptr actual;
    try{native.input(f,0,-17,[&](unsigned command,const auto&playlist,const auto&version)->bool{
      if(command!=1||playlist!="ranked"||version!="v7")throw std::runtime_error("Actual wrapper changed its typed native command");
      callback_completed=true;native.cancel();
      try{throw fixture::NativeRefusal("actual wrapper command refusal");}catch(const fixture::NativeRefusal&e){address=&e;actual=std::current_exception();throw;}
    });}catch(const fixture::NativeRefusal&e){actual_identity=address==&e;}
    bool retained_identity=false;try{if(actual)std::rethrow_exception(actual);}catch(const fixture::NativeRefusal&e){retained_identity=address==&e;}
    std::cout<<Json{{"results",results},{"wrapper_callback_completed",callback_completed},{"wrapper_native_exception",actual_identity},
      {"wrapper_retained_identity",retained_identity},{"wrapper_reentrant_seconds",native.seconds()},{"wrapper_copy_seconds",copy.seconds()},{"wrapper_initial_seconds",plan_before}}.dump()<<'\n';
  }
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
