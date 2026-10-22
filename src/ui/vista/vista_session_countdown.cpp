#include "halo2/vista_session_countdown.hpp"
#include <bit>
#include <initializer_list>

namespace halo2 {
namespace {
void call(VistaSessionCountdownResult& result,VistaSessionCountdownService service,
          std::initializer_list<std::uint32_t> arguments={}) {
  auto& intent=result.calls[result.call_count++];
  intent.service=service;
  intent.argument_count=unsigned(arguments.size());
  unsigned index{};
  for(const auto argument:arguments)intent.arguments[index++]=argument;
}
std::uint32_t word(std::int32_t value) {return std::bit_cast<std::uint32_t>(value);}
}
VistaSessionCountdownResult vista_session_countdown_start(
    const VistaSessionCountdownStartRequest& request,
    const VistaSessionCountdownStartQueries& queries) {
  VistaSessionCountdownResult result;
  call(result,VistaSessionCountdownService::ui_state,{0});
  call(result,VistaSessionCountdownService::controller_user,{word(request.controller)});
  if(queries.controller_user==-1)return result;
  if(queries.ui_state==0xa) {
    call(result,VistaSessionCountdownService::host_permission);
    if(!queries.host_permission)return result;
    call(result,VistaSessionCountdownService::write,
        {1,word(request.requested_stage),0,word(queries.controller_user),0});
  } else if(queries.ui_state==0xb) {
    call(result,VistaSessionCountdownService::active_stage);
    if(queries.active_stage<=request.minimum_stage)return result;
    call(result,VistaSessionCountdownService::write,
        {1,word(queries.active_stage)-1u,0,word(queries.controller_user),0});
  } else return result;
  result.succeeded=queries.write_succeeded;
  return result;
}
VistaSessionCountdownResult vista_session_countdown_timer(
    VistaSessionCountdownTaskState& state,
    const VistaSessionCountdownTimerQueries& queries) {
  VistaSessionCountdownResult result;
  call(result,VistaSessionCountdownService::session_stage);
  call(result,VistaSessionCountdownService::session_flag);
  if(queries.session_stage!=state.last_stage) {
    call(result,VistaSessionCountdownService::clock);
    state.last_clock=queries.stage_change_clock;
    state.last_stage=queries.session_stage;
  }
  if(queries.session_stage<0)return result;
  call(result,VistaSessionCountdownService::elapsed,{state.last_clock});
  const auto elapsed=std::bit_cast<std::int32_t>(queries.elapsed_clock-state.last_clock);
  if(elapsed<1000 || queries.session_stage<=0)return result;
  call(result,VistaSessionCountdownService::clock);
  state.last_clock=queries.decrement_clock;
  const auto next=word(queries.session_stage)-1u;
  state.last_stage=std::bit_cast<std::int32_t>(next);
  call(result,VistaSessionCountdownService::write,
      {1,next,queries.session_flag,0,0});
  return result;
}
} // namespace halo2
