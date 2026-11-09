#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <openssl/rand.h>
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t>signin_bytes(const std::string&s){rust::Vec<std::uint8_t>out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string signin_text(const rust::Vec<std::uint8_t>&s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
bridge::SigninFlowFrame signin_frame(const MenuFrame&f){bridge::SigninFlowFrame out{};out.screen=f.screen;out.context=f.context;out.selected=f.selected;out.instance=f.instance;out.pane_clock=f.pane_created_clock;out.description=signin_bytes(f.description);for(const auto&e:f.entries)out.entries.push_back({signin_bytes(e.label),e.destination,e.value});return out;}
}
// Actual native entropy, credentials, workers and retained frame operations.
// Saved namespaces and all caller decisions/state belong to the Rust owner.
struct FrontendSigninNativeIo final:bridge::NativeFrontendSigninIo {
 MenuState&m;mutable std::optional<MenuFrame>built;mutable std::exception_ptr exception;
 explicit FrontendSigninNativeIo(MenuState&state):m(state){}
 template<class F>auto capture(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native sign-in flow IO failed");}}
 template<class F>auto invoke(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){if(exception)std::rethrow_exception(exception);throw;}}
 bridge::SigninFlowFacts signin_flow_facts()const override{return capture([&]{bridge::SigninFlowFacts f{};f.login=bool(m.discord_login_);f.account=bool(m.online_account_);f.queue=bool(m.online_matchmaking_);f.voice=bool(m.online_voice_);f.social=bool(m.discord_social_);f.directory=bool(m.online_directory_);f.mode=m.mode_;f.signed_in=signin_bytes(m.signed_in_);f.keyboard=signin_bytes(m.keyboard_text);for(const auto&v:m.stack_)f.frames.push_back(signin_frame(v));return f;});}
 rust::Vec<std::uint8_t> signin_flow_random()const override{return capture([&]{std::array<unsigned char,16>b{};if(RAND_bytes(b.data(),int(b.size()))!=1)throw FormatError("Could not prepare the profile's saved sign-in slot");rust::Vec<std::uint8_t>out;for(auto v:b)out.push_back(v);return out;});}
 bool signin_flow_steam()const override{return capture([&]{return m.discord_login_->uses_steam();});}
 bool signin_flow_authorized()const override{return capture([&]{return m.discord_login_->authorization().has_value();});}
 bridge::SigninFlowSnapshot signin_flow_snapshot()const override{return capture([&]{const auto s=m.discord_login_->snapshot();return bridge::SigninFlowSnapshot{static_cast<std::int32_t>(s.status),s.identity.has_value()};});}
 bool signin_flow_identity()const override{return capture([&]{return m.online_identity().has_value();});}
 rust::Vec<std::uint8_t> signin_flow_profile_name()const override{return capture([&]{return signin_bytes(m.profile_name());});}
 bridge::SigninFlowFrame signin_flow_build(std::uint32_t s,std::uint32_t c)const override{return capture([&]{built=m.build(s,c);return signin_frame(*built);});}
 void signin_flow_effect(const bridge::SigninFlowEffect&e)const override{capture([&]{switch(e.kind){
  case 0:m.save();break;
  case 1:m.discord_login_->bind_profile(m.preferences_.empty()?std::string{}:std::filesystem::absolute(m.preferences_).lexically_normal().string(),signin_text(e.text));break;
  case 2:m.online_prior_report_.reset();break;
  case 3:m.online_account_->sign_out();break;
  case 4:m.account_={};break;
  case 5:m.online_matchmaking_->sign_out();break;
  case 6:m.matchmaking_search_.stop();break;
  case 7:m.online_voice_->leave();break;
  case 8:m.discord_social_->disconnect();break;
  case 9:m.update_online_couch();break;
  case 10:m.update_online_friends();break;
  case 11:m.update_online_account();break;
  case 12:m.update_online_privacy();break;
  case 13:m.update_online_lobby_leave();break;
  case 14:m.update_online_postgame();break;
  case 15:m.discord_login_->restore();break;
  case 16:m.discord_login_->start();break;
  case 17:m.discord_login_->cancel();break;
  case 18:m.discord_login_->sign_out();break;
  case 19:m.online_account_->connect(m.profile_name());break;
  case 20:m.clear_online_couch();break;
  case 21:m.online_directory_->cancel();break;
  case 22:m.online_directory_active_=false;m.online_servers_={};m.online_detail_id_.clear();break;
  case 23:m.stack_.clear();m.launch.reset();m.quit=false;m.mode_=3;break;
  case 24:m.matchmaking_search_.quick(false);break;
  case 25:m.reset_online_matchmaking_progress();break;
  case 26:m.enter_online_matchmaking_lobby();break;
  case 27:m.open(e.a,e.b);break;
  case 28:m.stack_.pop_back();break;
  case 29:m.unavailable(signin_text(e.text));break;
  case 30:m.keyboard_text=m.profile_name();break;
  case 31:m.keyboard_text=m.profile_service_tag();break;
  case 32:if(!built)throw std::logic_error("Native sign-in rebuilt frame absent");built->instance=e.instance;built->pane_created_clock=e.pane_clock;built->selected=e.a;m.stack_.at(e.index)=std::move(*built);built.reset();break;
  case 33:built.reset();break;
  case 34:++m.revision;break;
  case 35:m.online_matchmaking_->cancel_queue();break;
  default:throw std::logic_error("Invalid native sign-in flow effect");
 }});}
};
FrontendSigninResult MenuState::signin_flow_drive(unsigned action,unsigned screen,unsigned context,std::string text,bool flag)const{FrontendSigninNativeIo io(const_cast<MenuState&>(*this));bridge::SigninFlowArgument a{};a.screen=screen;a.context=context;a.text=signin_bytes(text);a.flag=flag;const auto r=io.invoke([&]{return party_flow_.signin(action,a,io);});return {r.handled,r.screen,r.context,signin_text(r.text)};}
void MenuState::signin_restore_key(const std::optional<std::string>&key){signin_flow_drive(8,0,0,key.value_or(std::string{}),key.has_value());}
} // namespace halo2
