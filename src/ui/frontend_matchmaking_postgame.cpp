#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
bridge::PostgameFlowReport postgame_report(const VistaPcrScreenOwner&r){bridge::PostgameFlowReport out{};out.online=r.online;out.results_present=bool(r.results);if(r.results)out.player_count=r.results->player_count;out.select=bool(r.select_row);return out;}
}
// Actual report/result resources and their native callbacks remain genuine.
// Rust owns host/follower, first-report, review, leave and navigation decisions.
struct FrontendPostgameNativeIo final:bridge::NativeFrontendPostgameIo {
 MenuState&m;const MenuFrame*view;mutable std::shared_ptr<const VistaPcrScreenOwner>held;mutable std::exception_ptr exception;
 FrontendPostgameNativeIo(MenuState&state,const MenuFrame*f):m(state),view(f){}
 template<class F>auto capture(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native postgame flow IO failed");}}
 template<class F>auto invoke(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){if(exception)std::rethrow_exception(exception);throw;}}
 bridge::PostgameFlowFacts postgame_flow_facts()const override{return capture([&]{bridge::PostgameFlowFacts f{};f.authenticated=m.account_.authenticated;f.party=m.account_.party.has_value();if(m.account_.party){const auto&p=*m.account_.party;f.leader=p.leader;for(const auto&r:p.members)f.members.push_back({r.self,r.leader,r.postgame.has_value(),r.postgame.value_or(false)});}f.mode=m.mode_;for(const auto&v:m.stack_){bridge::PostgameFlowFrame r{};r.screen=v.screen;r.report_present=bool(v.vista_pcr);if(v.vista_pcr)r.report=postgame_report(*v.vista_pcr);f.frames.push_back(std::move(r));}f.prior_present=bool(m.online_prior_report_);if(m.online_prior_report_)f.prior=postgame_report(*m.online_prior_report_);f.view_present=view&&bool(view->vista_pcr);if(f.view_present)f.view=postgame_report(*view->vista_pcr);return f;});}
 bool postgame_flow_hold(std::uint32_t i)const override{return capture([&]{held=m.stack_.at(i).vista_pcr;return bool(held->input.continue_session);});}
 void postgame_flow_continue()const override{capture([&]{held->input.continue_session();});}
 bool postgame_flow_request_leave()const override {
  return capture([&]{return m.online_account_ && m.online_account_->party_command({"leave"});});
 }
 void postgame_flow_leave_held()const override {
  capture([&]{if(held->leave_session)held->leave_session();});
 }
 void postgame_flow_unavailable(rust::Str message)const override {
  capture([&]{m.unavailable(std::string(message));});
 }
 void postgame_flow_effect(std::uint32_t k)const override{capture([&]{switch(k){
  case 0:m.online_prior_report_->select_row(m.online_prior_report_->selected_row);break;
  case 1:m.open_vista_pcr(185,m.online_prior_report_);break;
  case 2:m.stack_.pop_back();break;
  case 3:++m.revision;break;
  case 4:if(!view)throw std::logic_error("Native postgame capture frame absent");m.online_prior_report_=view->vista_pcr->lobby_review;break;
  case 5:m.stack_.clear();break;
  case 6:m.back();break;
  case 7:m.update_online_account();break;
  case 8:m.return_online();break;
  default:throw std::logic_error("Invalid native postgame flow effect");
 }});}
};
bool MenuState::postgame_flow_drive(unsigned action,const MenuFrame*frame,unsigned id)const{FrontendPostgameNativeIo io(const_cast<MenuState&>(*this),frame);return io.invoke([&]{return bridge::frontend_postgame_flow(action,id,io);});}
} // namespace halo2
