#include "halo2/frontend_matchmaking_search.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
using Json=nlohmann::json;
namespace bridge=halo2::online_protocol_bridge;
namespace fixture {
struct FormatError:std::runtime_error{using std::runtime_error::runtime_error;};
struct NativeRefusal:std::overflow_error{using std::overflow_error::overflow_error;};
struct Member{std::string handle;bool self{};std::string ready_playlist;};
struct Party{bool leader{};std::vector<Member> members;};
struct OnlineAccountSnapshot{std::optional<Party> party;bool busy{};std::string party_message;};
struct Playlist{std::string id,revision;bool eligible{},quickmatch{},clan_match{};};
struct Appearance{std::uint8_t raw[9]{};};
struct Player{std::string name;bool self{};int team{};Appearance appearance;int input_instance{};};
struct SourceSettings{std::array<std::uint8_t,304> bytes{};};
struct UiVariant{std::string name;unsigned engine{};std::vector<int> unused;std::optional<SourceSettings> source_settings;};
struct FrontendContent{std::vector<UiVariant> variants{{"Slayer",1,{}}},bundled_variants;};
struct OnlineMatch{std::string map="lockout",variant="Slayer";std::optional<SourceSettings> settings;std::vector<Player> players{{"self",true,1}};};
enum class OnlineMatchmakingStatus{Unconfigured,Idle,Connecting,Ready,Joining,Searching,CheckingConnections,AwaitingTransport,Loading,Failed};
struct Snapshot{OnlineMatchmakingStatus status=OnlineMatchmakingStatus::Unconfigured;std::vector<Playlist> playlists;std::optional<OnlineMatch> match;std::optional<bool> gathering;std::uint64_t revision{};std::string message;bool start_countdown_supported{};std::optional<unsigned> start_countdown_ms;std::optional<int> failure_state;};
struct Entry{std::string label;int destination{};};
struct Frame{unsigned screen{},context{},selected{},pane_generation{},pane_created_clock{};std::vector<Entry> entries{{"row",0}};};
enum class FrontendUiSound{Error=2};
struct Model;
struct Session{Model*m;std::optional<bool> team_mode()const;std::vector<unsigned> local_slots()const;const std::vector<Player>&roster()const;};
struct FrontendLaunch{std::string map;bool multiplayer{},match_voice{};UiVariant variant;std::shared_ptr<Session> online_match;unsigned local_players{};std::vector<std::string> selected_profiles;std::vector<int> input_instances;std::vector<std::int8_t> selected_teams;};
struct PartyCommand{std::string action,value,playlist,revision;};
struct Queue{Model*m;void sign_out();void refresh(std::string,std::vector<Player>,Appearance);void cancel_queue();void join(std::string);std::optional<Snapshot> snapshot_after(std::uint64_t);Snapshot snapshot();std::shared_ptr<Session> take_launch();void reject_content();};
struct Account{Model*m;OnlineAccountSnapshot snapshot();bool party_command(PartyCommand);};
rust::Vec<std::uint8_t> bytes(std::string_view s){rust::Vec<std::uint8_t> out;for(unsigned char c:s)out.push_back(c);return out;}
std::string text(const rust::Vec<std::uint8_t>&v){return {reinterpret_cast<const char*>(v.data()),v.size()};}
std::string text(rust::Slice<const std::uint8_t>v){return {reinterpret_cast<const char*>(v.data()),v.size()};}
template<class T>std::vector<T> vec(const rust::Vec<T>&v){return {v.begin(),v.end()};}
std::optional<Party> party(const Json&j){if(j.is_null()||j==false)return {};Party p;p.leader=j.value("leader",true);for(const auto&m:j.value("members",Json::array({{{"handle","self"},{"self",true},{"ready","ranked"}},{{"handle","peer"},{"ready","ranked"}}})))p.members.push_back({m.at("handle"),m.value("self",false),m.value("ready",std::string{})});return p;}
Snapshot snapshot(const Json&j){Snapshot s;s.status=OnlineMatchmakingStatus(j.value("status",3));s.revision=j.value("revision",std::uint64_t(1));s.message=j.value("message",std::string{});s.start_countdown_supported=j.value("supported",true);if(j.contains("countdown")&&!j["countdown"].is_null())s.start_countdown_ms=j["countdown"].get<unsigned>();if(j.contains("failure")&&!j["failure"].is_null())s.failure_state=j["failure"].get<int>();if(j.value("matched",false))s.match=OnlineMatch{};if(j.value("gathering",false))s.gathering=true;
 for(const auto&p:j.value("playlists",Json::array({{{"id","ranked"},{"revision","v7"},{"eligible",true},{"quick",true}}})))s.playlists.push_back({p.at("id"),p.value("revision",std::string("v7")),p.value("eligible",false),p.value("quick",false),p.value("clan",false)});return s;}
bridge::SearchSnapshot projection(const Snapshot&s){bridge::SearchSnapshot v{};v.status=unsigned(s.status);v.revision=s.revision;v.message_empty=s.message.empty();v.matched=s.match.has_value();v.gathering=s.gathering.has_value();v.countdown_supported=s.start_countdown_supported;v.countdown_present=s.start_countdown_ms.has_value();v.countdown_ms=s.start_countdown_ms.value_or(0);v.failure_present=s.failure_state.has_value();v.failure=s.failure_state.value_or(0);for(const auto&p:s.playlists)v.playlists.push_back({p.id,p.revision,p.eligible,p.quickmatch,p.clan_match});return v;}
rust::Vec<bridge::SearchMember> projection(const Party&p){rust::Vec<bridge::SearchMember>v;for(const auto&m:p.members)v.push_back({m.handle,m.self,m.ready_playlist});return v;}
struct Model {
 int mode_=3;unsigned ui_clock_{};std::uint64_t couch_revision_{},revision{};OnlineAccountSnapshot account_,worker;Snapshot matchmaking_,published;
 std::shared_ptr<Queue> online_matchmaking_;std::shared_ptr<Account> online_account_;
 std::vector<Frame> stack_{{6},{186},{14}};std::vector<FrontendUiSound> pending_ui_sounds_;
 std::optional<FrontendLaunch> launch;FrontendContent content_;bool match_voice_{},party_voice_wanted_{},voice_party_{};struct{bool joined{};}voice_;
 std::function<void(const FrontendLaunch&)> multiplayer_map_validator_;std::string signed_in_="self",profile="self";
 std::vector<Player> profiles{{"self",true,1}},roster{{"self",true,1}};std::vector<unsigned> slots{0};std::optional<bool> teams=true;
 bool identity=true,changed=true,accept_command=true,countdown_handled{},transfer=true,reenter_open=true;
 Json trace=Json::array(),observations=Json::array();std::string fault;bool format_fault{};mutable unsigned depth{};std::exception_ptr native_exception;const void*native_address{};
 std::function<Json()> inspect;
 virtual ~Model()=default;
 Model(){online_matchmaking_=std::make_shared<Queue>(this);online_account_=std::make_shared<Account>(this);}
 void call(const std::string&n,Json value=Json()){
  trace.push_back({n,value});if(inspect)observations.push_back({n,inspect()});
  if(fault==n){try{if(format_fault)throw FormatError("actual native search format refusal: "+n);throw NativeRefusal("actual native search refusal: "+n);}catch(const std::exception&e){native_address=&e;native_exception=std::current_exception();throw;}}
 }
 Frame&frame(){return stack_.back();}const Frame&frame()const{return stack_.back();}
 virtual void opened(unsigned){};
 void open(unsigned id){call("open",id);stack_.push_back({id});if(reenter_open){if(++depth>8)throw std::runtime_error("Fixture open recursion");opened(id);--depth;}}
 void dialog(unsigned id,int destination,bool quiet=false){call("dialog",Json::array({id,destination,quiet}));stack_.push_back({7,0,0,0,0,{{"dialog",destination}}});}
 void unavailable(std::string v){call("unavailable",bytes_json(v));}
 void enter_online_matchmaking_lobby(){call("enter_lobby");}
 bool update_online_lobby_countdown(){call("lobby_countdown");return countdown_handled;}
 bool online_identity(){call("identity");return identity;}
 std::string profile_name(){call("profile");return profile;}
 std::vector<Player> local_players(){call("local_players");return profiles;}
 std::vector<Player> online_local_players(){call("online_local_players");return profiles;}
 Appearance appearance_for_profile(const std::string&){call("appearance");return {};}
 bool online_party_screen(const Frame&f)const{return mode_==3&&(f.screen==14||(f.screen==219&&f.context==42));}
 void populate_online_party(Frame&f){call("populate_lobby",f.screen);f.entries={{"party",0}};}
 void populate_online_matchmaking(Frame&f){call("populate_search",f.screen);f.entries.clear();for(const auto&p:matchmaking_.playlists)f.entries.push_back({p.id,0});if(f.entries.empty())f.entries.push_back({"refresh",0});}
 static Json bytes_json(std::string_view s){return std::vector<unsigned char>(s.begin(),s.end());}
 void load(const Json&j){
  mode_=j.value("mode",3);ui_clock_=j.value("clock",0u);couch_revision_=j.value("couch",std::uint64_t{});identity=j.value("identity",true);profile=j.value("profile",std::string("self"));if(j.contains("profile_bytes")){profile.clear();for(auto b:j["profile_bytes"])profile.push_back(char(b.get<unsigned>()));}
  account_={};account_.party=party(j.value("party",Json()));worker={};worker.party=party(j.value("worker_party",j.value("party",Json())));worker.busy=j.value("busy",false);worker.party_message=j.value("party_message",std::string{});
  matchmaking_=fixture::snapshot(j.value("snapshot",Json::object()));published=fixture::snapshot(j.value("published",j.value("snapshot",Json::object())));changed=j.value("changed",true);accept_command=j.value("accepted",true);countdown_handled=j.value("lobby_handled",false);transfer=j.value("transfer",true);reenter_open=j.value("reenter_open",true);
  stack_.clear();for(const auto&s:j.value("screens",Json::array({6,186,14}))){if(s.is_number())stack_.push_back({s.get<unsigned>()});else{Frame f;f.screen=s.at("screen");f.context=s.value("context",0u);f.selected=s.value("selected",0u);if(s.contains("destination"))f.entries={{"dialog",s["destination"].get<int>()}};stack_.push_back(f);}}if(stack_.empty())throw std::runtime_error("Empty fixture stack");
  online_matchmaking_=j.value("queue",true)?std::make_shared<Queue>(this):nullptr;online_account_=j.value("account",true)?std::make_shared<Account>(this):nullptr;
  profiles.assign(j.value("locals",1u),Player{"self",true,1});slots.clear();roster.clear();for(unsigned i=0;i<j.value("slots",j.value("locals",1u));++i){slots.push_back(i);roster.push_back({j.value("admitted",std::string("self")),true,1});}
  teams=j.contains("teams")&&j["teams"].is_null()?std::nullopt:std::optional<bool>{j.value("teams",true)};content_.variants=j.value("variant_available",true)?std::vector<UiVariant>{{"Slayer",1,{}}}:std::vector<UiVariant>{};
  if(matchmaking_.match){matchmaking_.match->map=j.value("map",std::string("lockout"));if(!j.value("self",true))matchmaking_.match->players.clear();}
  multiplayer_map_validator_=[this,&j](const FrontendLaunch&){call("validate_map");if(j.value("reject_content",false))throw FormatError("map rejected");};
  fault=j.value("fault",std::string{});format_fault=j.value("format_fault",false);pending_ui_sounds_.assign(j.value("sounds",0u),FrontendUiSound::Error);trace=Json::array();observations=Json::array();native_exception={};native_address=nullptr;
 }
 Json io_view()const{Json frames=Json::array(),sounds=Json::array();for(const auto&f:stack_){Json entries=Json::array();for(const auto&e:f.entries)entries.push_back({e.label,e.destination});frames.push_back({f.screen,f.context,f.selected,f.pane_generation,f.pane_created_clock,entries});}for(auto s:pending_ui_sounds_)sounds.push_back(unsigned(s));
  Json result={{"mode",mode_},{"revision",revision},{"frames",frames},{"sounds",sounds},{"status",unsigned(matchmaking_.status)},{"message",bytes_json(matchmaking_.message)},{"failure",matchmaking_.failure_state?Json(*matchmaking_.failure_state):Json()},{"snapshot_revision",matchmaking_.revision},{"trace",trace},{"observations",observations},{"launch",launch.has_value()}};
  if(launch)result["launch_data"]={{"map",launch->map},{"profiles",launch->selected_profiles},{"inputs",launch->input_instances},{"teams",launch->selected_teams},{"count",launch->local_players},{"variant",launch->variant.name}};return result;}
};
std::optional<bool>Session::team_mode()const{m->call("team_mode");return m->teams;}
std::vector<unsigned>Session::local_slots()const{m->call("local_slots");return m->slots;}
const std::vector<Player>&Session::roster()const{m->call("roster");return m->roster;}
void Queue::sign_out(){m->call("sign_out");}void Queue::refresh(std::string n,std::vector<Player>,Appearance){m->call("refresh",Model::bytes_json(n));}
void Queue::cancel_queue(){m->call("cancel");}void Queue::join(std::string p){m->call("join",p);}std::optional<Snapshot>Queue::snapshot_after(std::uint64_t r){m->call("pull_after",r);return m->changed?std::optional<Snapshot>{m->published}:std::nullopt;}
Snapshot Queue::snapshot(){m->call("pull");return m->published;}std::shared_ptr<Session>Queue::take_launch(){m->call("take");return m->transfer?std::make_shared<Session>(m):nullptr;}void Queue::reject_content(){m->call("reject_content");}
OnlineAccountSnapshot Account::snapshot(){m->call("account");return m->worker;}bool Account::party_command(PartyCommand c){m->call("command",Json::array({c.action,c.playlist,c.revision}));return m->accept_command;}
}
// The controller bodies are the captured, unchanged C++ implementation.
// The above observations are declared native IO, not a replacement service.
namespace cpp_before {
#include "reference/frontend_lobby_before_rust_20261102/frontend_matchmaking_progress.hpp.reference"
namespace halo2 {
using namespace ::fixture;using MenuFrame=Frame;using OnlineMatchmaking=Queue;
bool matchmaking_screen(const Frame&f){return f.screen==210||((f.screen==219||f.screen==237)&&f.context==0);}
const Playlist*selected(const Snapshot&s,const std::string&id){auto i=std::find_if(s.playlists.begin(),s.playlists.end(),[&](const auto&p){return p.id==id;});return i==s.playlists.end()?nullptr:&*i;}
std::optional<unsigned>original_matchmaking_error_dialog(int s){const auto v=bridge::matchmaking_error_dialog(s,false);return v.present?std::optional<unsigned>{v.id}:std::nullopt;}
int source_game_variant_type(const SourceSettings&s){std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::uint32_t(s.bytes[0x44+i])<<(8*i);return std::bit_cast<std::int32_t>(v);}
bool matchmaking_variant_valid(const std::array<std::uint8_t,304>&b){return bridge::matchmaking_variant_valid({b.data(),b.size()});}
#include "reference/frontend_search_before_rust_20261102/matched_variant.cpp.inc"
namespace detail{struct Rules{bool team_mode=true;};Rules frontend_multiplayer_rules(const UiVariant&){return {};}}
std::string native_selected_player_configuration(const std::string&n,std::span<const std::uint8_t,9>,int){return n;}
struct MenuState:Model{
 bool matchmaking_active_{},quickmatch_pending_{},online_lobby_playlist_pending_{},online_lobby_create_submitted_{},matchmaking_consent_pending_{},matchmaking_have_fun_{};
 std::string selected_playlist_,matchmaking_name_;std::uint64_t matchmaking_couch_revision_{};std::optional<unsigned>matchmaking_start_clock_;
 int matchmaking_countdown_=-1,matchmaking_progress_title_state_{};unsigned matchmaking_progress_pane_{},matchmaking_pane_clock_{};std::vector<std::string>matchmaking_search_party_;
 void set_online_matchmaking(std::shared_ptr<Queue>);void prepare_online_matchmaking_lobby(bool);void reset_online_matchmaking_progress();bool update_online_party_search();void refresh_online_matchmaking();void update_online_matchmaking();bool launch_online_match();MatchmakingProgressUi online_matchmaking_ui()const;unsigned online_matchmaking_progress_pane()const;
 void cancel_and_return() {
  quickmatch_pending_ = false;
  if (online_matchmaking_) online_matchmaking_->cancel_queue();
  reset_online_matchmaking_progress();
  while (stack_.size() > 1 && frame().screen != 186) stack_.pop_back();
  if (mode_ == 3) { open(14); enter_online_matchmaking_lobby(); }
  else ++revision; // A direct preview of the service menu has no party lobby.
 }
 void opened(unsigned s)override{if(s==14&&mode_==3){if(online_account_&&!account_.party&&!online_account_->snapshot().busy)online_account_->party_command({"create"});if(!matchmaking_active_)refresh_online_matchmaking();}if(s==210){quickmatch_pending_=true;if(!matchmaking_active_)refresh_online_matchmaking();update_online_matchmaking();}}
 Json state()const{return {{"active",matchmaking_active_},{"quick",quickmatch_pending_},{"consent",matchmaking_consent_pending_},{"selected",selected_playlist_},{"name",bytes_json(matchmaking_name_)},{"couch",matchmaking_couch_revision_},{"pending",online_lobby_playlist_pending_},{"create",online_lobby_create_submitted_},{"countdown",matchmaking_countdown_},{"fun",matchmaking_have_fun_},{"pane",matchmaking_progress_pane_},{"title",matchmaking_progress_title_state_},{"pane_clock",matchmaking_pane_clock_},{"party",matchmaking_search_party_}};}
};
#include "reference/frontend_search_before_rust_20261102/frontend_search_policy.cpp.inc"
}}
namespace candidate {
using namespace fixture;
struct MenuState:Model,bridge::NativeFrontendSearchIo{
 rust::Box<bridge::FrontendSearch> owner=bridge::new_frontend_search();mutable std::exception_ptr captured;mutable FrontendLaunch result;mutable std::vector<Player>locals;mutable std::vector<unsigned>admitted_slots;
 template<class F>decltype(auto)capture(F&&f)const{try{return f();}catch(...){captured=std::current_exception();throw std::runtime_error("Native frontend search IO failed");}}
 bool run(unsigned action,unsigned index=0){captured={};try{return bridge::frontend_search_run(*owner,action,index,*this);}catch(const std::exception&){if(captured)std::rethrow_exception(captured);throw;}}
 MenuState&m()const{return const_cast<MenuState&>(*this);}
 void opened(unsigned s)override{if(s==14&&mode_==3)run(6);if(s==210)run(7);}
 Json state()const{const auto s=bridge::frontend_search_state(*owner);std::vector<std::string>party;for(const auto&p:s.party)party.push_back(std::string(p));return {{"active",s.active},{"quick",s.quick},{"consent",s.consent},{"selected",std::string(s.selected)},{"name",vec(s.name)},{"couch",s.couch_revision},{"pending",s.lobby_pending},{"create",s.create_submitted},{"countdown",s.countdown},{"fun",s.have_fun},{"pane",s.pane},{"title",s.title},{"pane_clock",s.pane_clock},{"party",party}};}
 bridge::SearchContext search_context()const override{return capture([&]{bridge::SearchContext c{};c.mode=mode_;c.clock=ui_clock_;c.couch_revision=couch_revision_;c.queue=bool(online_matchmaking_);c.account=bool(online_account_);c.party_present=account_.party.has_value();if(account_.party){c.leader=account_.party->leader;c.members=projection(*account_.party);}for(const auto&f:stack_)c.frames.push_back({f.screen,f.context,online_party_screen(f),!f.entries.empty(),f.entries.empty()?0:f.entries.front().destination});return c;});}
 bridge::SearchSnapshot search_view()const override{return capture([&]{return projection(matchmaking_);});}
 bool search_identity()const override{return capture([&]{return m().online_identity();});}
 rust::Vec<std::uint8_t>search_profile()const override{return capture([&]{return bytes(m().profile_name());});}
 std::uint32_t search_local_count()const override{return capture([&]{return unsigned(m().local_players().size());});}
 bool search_pull_after(std::uint64_t r)const override{return capture([&]{if(auto v=online_matchmaking_->snapshot_after(r)){m().matchmaking_=*v;return true;}return false;});}
 void search_pull()const override{capture([&]{m().matchmaking_=online_matchmaking_->snapshot();});}
 bridge::SearchAccount search_account()const override{return capture([&]{const auto p=online_account_->snapshot();bridge::SearchAccount a{};a.present=p.party.has_value();a.busy=p.busy;a.message=bytes(p.party_message);if(p.party){a.leader=p.party->leader;a.members=projection(*p.party);}return a;});}
 bool search_command(rust::Str a,rust::Str p,rust::Str r)const override{return capture([&]{return online_account_->party_command({std::string(a),{},std::string(p),std::string(r)});});}
 void search_sign_out()const override{capture([&]{online_matchmaking_->sign_out();});}
 void search_refresh(rust::Slice<const std::uint8_t>n)const override{capture([&]{online_matchmaking_->refresh(text(n),m().online_local_players(),m().appearance_for_profile(signed_in_));});}
 void search_cancel()const override{capture([&]{online_matchmaking_->cancel_queue();});}
 void search_join(rust::Str p)const override{capture([&]{online_matchmaking_->join(std::string(p));});}
 bool search_lobby_countdown()const override{return capture([&]{return m().update_online_lobby_countdown();});}
 bool search_launch()const override{return capture([&]{return m().run(9);});}
 bool search_has_launch()const override{return launch.has_value();}
 void render(unsigned previous)const{for(auto&f:m().stack_){if(!cpp_before::halo2::matchmaking_screen(f))continue;if(f.screen==210&&previous!=bridge::frontend_search_pane(*owner,projection(matchmaking_))){++f.pane_generation;f.pane_created_clock=ui_clock_;}std::string focus;if((f.screen==219||f.screen==237)&&f.selected<f.entries.size())focus=f.entries[f.selected].label;f.entries.clear();m().populate_online_matchmaking(f);if(!focus.empty()){auto i=std::find_if(f.entries.begin(),f.entries.end(),[&](const auto&e){return e.label==focus;});if(i!=f.entries.end())f.selected=unsigned(i-f.entries.begin());}f.selected=std::min(f.selected,unsigned(f.entries.size()-1));}}
 void search_ui(const bridge::SearchUi&e)const override{capture([&]{using K=bridge::SearchUiKind;switch(e.kind){
 case K::ClearSnapshot:m().matchmaking_={};break;case K::NoService:m().matchmaking_.message=text(e.text);break;case K::Failure:m().matchmaking_.status=OnlineMatchmakingStatus::Failed;m().matchmaking_.message=text(e.text);break;case K::ClearFailure:m().matchmaking_.failure_state.reset();break;
 case K::ModeOnline:m().mode_=3;break;case K::PopLobbyOrOnline:while(m().stack_.size()>1&&frame().screen!=186&&frame().screen!=14)m().stack_.pop_back();break;case K::PopLobby:while(m().stack_.size()>1&&frame().screen!=14)m().stack_.pop_back();break;case K::PopTop:m().stack_.pop_back();break;
 case K::OpenLobby:m().open(14);break;case K::OpenSearch:m().open(210);break;case K::EnterLobby:m().enter_online_matchmaking_lobby();break;case K::Dialog:m().dialog(e.number,-5,true);break;case K::Revision:++m().revision;break;case K::RepopulateLobby:for(auto&f:m().stack_)if(f.screen==14){f.entries.clear();m().populate_online_party(f);}break;
 case K::Sound:if(pending_ui_sounds_.size()<16)m().pending_ui_sounds_.push_back(static_cast<FrontendUiSound>(e.number));break;case K::Render:render(e.number);break;case K::Unavailable:m().unavailable(text(e.text));break;case K::PlaylistError:m().pending_ui_sounds_.push_back(FrontendUiSound::Error);m().unavailable(text(e.text));break;default:throw std::runtime_error("Unknown fixture effect");}});}
 bridge::SearchVariant search_variant()const override{return capture([&]{result={};auto v=cpp_before::halo2::matched_variant(content_,*matchmaking_.match);if(!v)return bridge::SearchVariant{};result.variant=*v;result.variant.name=matchmaking_.match->variant;return bridge::SearchVariant{true,true};});}
 rust::Vec<std::uint8_t>search_map()const override{return bytes(matchmaking_.match->map);}
 void search_prepare_map(rust::Slice<const std::uint8_t>map)const override{capture([&]{result.map=text(map);result.multiplayer=true;result.match_voice=match_voice_||(party_voice_wanted_&&voice_party_&&voice_.joined);});}
 bool search_self_present()const override{return std::any_of(matchmaking_.match->players.begin(),matchmaking_.match->players.end(),[](const auto&p){return p.self;});}
 bridge::SearchTake search_take()const override{return capture([&]{result.online_match=online_matchmaking_->take_launch();bridge::SearchTake t{};t.present=bool(result.online_match);if(t.present){auto teams=result.online_match->team_mode();t.teams_present=teams.has_value();t.teams=teams.value_or(false);}return t;});}
 bridge::SearchCounts search_local_counts()const override{return capture([&]{locals=m().local_players();admitted_slots=result.online_match->local_slots();return bridge::SearchCounts{unsigned(locals.size()),unsigned(admitted_slots.size())};});}
 void search_set_local_count(std::uint32_t n)const override{result.local_players=n;}
 rust::Vec<std::uint8_t>search_admitted_name(std::uint32_t i)const override{return capture([&]{return bytes(result.online_match->roster().at(admitted_slots.at(i)).name);});}
 rust::Vec<std::uint8_t>search_profile_name(std::uint32_t i)const override{return bytes(locals.at(i).name);}
 void search_append_profile(std::uint32_t i,bool teams)const override{capture([&]{const auto&p=locals.at(i);const auto&a=roster.at(admitted_slots.at(i));result.selected_profiles.push_back(p.name);result.input_instances.push_back(p.input_instance);if(teams)result.selected_teams.push_back(std::int8_t(a.team));});}
 bool search_validate_map()const override{return capture([&]{try{if(multiplayer_map_validator_)multiplayer_map_validator_(result);}catch(const FormatError&){return false;}return true;});}
 void search_reject_content()const override{capture([&]{online_matchmaking_->reject_content();});}
 void search_publish()const override{m().launch=std::move(result);}
 void search_discard()const override{result={};locals.clear();admitted_slots.clear();}
 bridge::SearchFailure search_failure()const override{bridge::SearchFailure f{};if(captured){try{std::rethrow_exception(captured);}catch(const FormatError&e){f.format=true;f.text=bytes(e.what());}catch(...){}}return f;}
};
}
template<class M,class F>Json invoke(M&m,F&&f){Json value;std::string error;bool identity=true;try{value=f();}catch(const std::exception&e){error=e.what();identity=m.native_address==&e;}return {{"value",value},{"error",error},{"native_identity",identity},{"state",m.state()},{"io",m.io_view()}};}
int main()try{
 std::string line;while(std::getline(std::cin,line)){
  const auto request=Json::parse(line);
  if(request.contains("variant")){
   const auto&v=request.at("variant");fixture::OnlineMatch match;match.variant=v.at("name");fixture::FrontendContent content;content.variants.clear();content.bundled_variants.clear();std::vector<bridge::SearchName> c,b;
   for(const auto&n:v.at("content")){const auto name=n.get<std::string>();content.variants.push_back({name,unsigned(content.variants.size()+10),{}});c.push_back({fixture::bytes(name)});}
   for(const auto&n:v.at("bundled")){const auto name=n.get<std::string>();content.bundled_variants.push_back({name,unsigned(content.bundled_variants.size()+20),{}});b.push_back({fixture::bytes(name)});}
   rust::Vec<std::uint8_t>raw;if(v.contains("raw")){match.settings=fixture::SourceSettings{};for(unsigned i=0;i<304;++i){match.settings->bytes[i]=v.at("raw").at(i).get<std::uint8_t>();raw.push_back(match.settings->bytes[i]);}}
   const auto old=cpp_before::halo2::matched_variant(content,match);const auto name=fixture::bytes(match.variant);const auto next=bridge::frontend_search_variant({name.data(),name.size()},match.settings.has_value(),{raw.data(),raw.size()},{c.data(),c.size()},{b.data(),b.size()});std::optional<fixture::UiVariant>chosen;
   if(next.kind==1){chosen=fixture::UiVariant{match.variant,next.index,{}};chosen->source_settings=match.settings;}else if(next.kind==2)chosen=content.variants.at(next.index);else if(next.kind==3)chosen=content.bundled_variants.at(next.index);else if(next.kind!=0)throw std::runtime_error("Invalid variant ABI");
   const auto project=[](const auto&x)->Json{if(!x)return nullptr;Json r={{"name",x->name},{"engine",x->engine}};if(x->source_settings)r["raw"]=x->source_settings->bytes;return r;};
   std::cout<<Json{{"cpp",project(old)},{"rust",project(chosen)}}.dump()<<'\n';continue;
  }
  cpp_before::halo2::MenuState before;candidate::MenuState after;before.inspect=[&]{return before.state();};after.inspect=[&]{return after.state();};Json results=Json::array();
  for(const auto&cmd:request.at("commands")){const auto facts=cmd.value("facts",Json::object());before.load(facts);after.load(facts);const auto op=cmd.at("op").get<std::string>();const auto index=cmd.value("index",0u);
   const auto old=invoke(before,[&]()->Json{if(op=="reset"){before.matchmaking_active_=before.quickmatch_pending_=false;before.online_lobby_playlist_pending_=false;before.matchmaking_={};before.matchmaking_name_.clear();before.reset_online_matchmaking_progress();return true;}if(op=="prepare"||op=="quickmatch"){before.prepare_online_matchmaking_lobby(op=="quickmatch");return true;}if(op=="refresh"){before.refresh_online_matchmaking();return true;}if(op=="update"){before.update_online_matchmaking();return true;}if(op=="party")return before.update_online_party_search();if(op=="open_lobby"||op=="open_search"){before.opened(op=="open_lobby"?14:210);return true;}if(op=="launch")return before.launch_online_match();
   if(op=="cancel_and_return"){before.cancel_and_return();return true;}if(op=="select"){if(index<before.matchmaking_.playlists.size()){const auto&p=before.matchmaking_.playlists[index];if(!p.eligible){before.pending_ui_sounds_.push_back(fixture::FrontendUiSound::Error);before.unavailable("This playlist is unavailable for your ONLINE client, party, or installed maps. Use an updated ONLINE build and check the playlist's party and map requirements.");return true;}before.selected_playlist_=p.id;before.prepare_online_matchmaking_lobby(false);}else before.refresh_online_matchmaking();return true;}throw std::runtime_error("Unknown fixture action");});
   const auto next=invoke(after,[&]()->Json{unsigned action;if(op=="reset")action=0;else if(op=="prepare")action=1;else if(op=="quickmatch")action=2;else if(op=="refresh")action=3;else if(op=="update")action=4;else if(op=="party")action=5;else if(op=="open_lobby")action=6;else if(op=="open_search")action=7;else if(op=="select")action=8;else if(op=="launch")action=9;
   else if(op=="cancel_and_return")action=10;else throw std::runtime_error("Unknown fixture action");return after.run(action,index);});
   results.push_back({{"cpp",old},{"rust",next}});
  }
  // The public native wrapper shares one Rust controller across callbacks;
  // cloning it must create an independent state owner.
  halo2::FrontendMatchmakingSearch wrapper;wrapper.quick(true);auto copy=wrapper;wrapper.reset(77);wrapper.stop();const bool clone_ok=copy.quick()&&!wrapper.quick();
  struct ReentrantIo:candidate::MenuState{const halo2::FrontendMatchmakingSearch*wrapper{};mutable bool completed{};bool search_identity()const override{return capture([&]{wrapper->reset(91);wrapper->quick(false);completed=true;m().call("identity");return false;});}} io;
  io.wrapper=&wrapper;io.fault="identity";bool exception_identity=false;try{wrapper.run(halo2::FrontendSearchAction::Update,io);}catch(const std::exception&){try{if(io.captured)std::rethrow_exception(io.captured);}catch(const fixture::NativeRefusal&e){exception_identity=io.native_address==&e;}}
  const bool reentry=io.completed&&wrapper.clock()==91&&!wrapper.quick()&&copy.quick();
  std::cout<<Json{{"results",results},{"wrapper_clone",clone_ok},{"wrapper_reentry",reentry},{"wrapper_exception_identity",exception_identity}}.dump()<<'\n';
 }
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
