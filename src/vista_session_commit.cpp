#include "halo2/vista_session_commit.hpp"
#include "halo2/cache.hpp"
#include <initializer_list>

namespace halo2 {
namespace {
void require_local(const VistaSessionCommitContext& context) {
  if(context.session_state<5 || context.session_state>8 ||
      context.host_peer!=context.local_peer)
    throw FormatError("Vista local countdown requires actual host states 5..8");
}
template<class T> void require_span(std::int32_t count,std::span<T> values) {
  if(count>0 && std::uint32_t(count)>values.size())
    throw FormatError("Truncated Vista session peer fields");
}
void call(VistaSessionZeroResult& result,VistaSessionZeroService service,
          std::initializer_list<std::uint32_t> arguments={}) {
  auto& item=result.calls[result.call_count++];
  item.service=service;item.argument_count=unsigned(arguments.size());
  unsigned i{};for(auto value:arguments)item.arguments[i++]=value;
}
}
bool vista_session_countdown_commit(VistaSessionCountdownCommitState& destination,
    const VistaSessionCommitContext& context,std::uint32_t sender_peer,
    const VistaSessionCountdownWrite& request) {
  auto state=destination;
  bool changed{};
  auto flag=request.flag;
  auto running=request.running;
  if(sender_peer!=context.local_peer) {
    flag=0;
    running=state.flag && request.reason ? 0 : state.running;
  }
  if(state.reason && state.reason_stage>request.stage) {
    state.reason=0;state.reason_stage=-1;state.identity={};
    ++state.revision;changed=true;
  }
  const bool retain_reason=request.reason && running && state.running;
  if(state.running!=running || state.stage!=request.stage ||
      state.reason!=request.reason || state.flag!=flag) {
    state.running=running;state.stage=request.stage;
    state.flag=context.session_state>=5 && context.session_state<=8 && context.local_flag ? 1 : flag;
    if(retain_reason) {
      state.reason=request.reason;state.reason_stage=request.stage;
      if(request.reason==1) {
        if(!request.identity)throw FormatError("Missing consumed Vista countdown identity");
        state.identity=*request.identity;
      } else state.identity={};
    } else {state.reason=0;state.reason_stage=-1;state.identity={};}
    ++state.revision;changed=true;
  }
  destination=state;
  return changed;
}
VistaSessionLocalWriteResult vista_session_countdown_local_write(
    VistaSessionCountdownCommitState& state,const VistaSessionCommitContext& context,
    const VistaSessionCountdownWrite& request) {
  require_local(context);
  return {true,vista_session_countdown_commit(state,context,context.host_peer,request)};
}
VistaSessionReadiness vista_session_peer_readiness(std::int32_t count,
    std::span<const std::int32_t> statuses) {
  require_span(count,statuses);
  VistaSessionReadiness result{true,0};
  for(std::int32_t i=0;i<count;++i) {
    const auto value=std::uint32_t(statuses[std::uint32_t(i)]);
    if(value==0 || value==1 || value==2 || value==5) {
      result.ready=false;result.blocked_mask|=1u<<(std::uint32_t(i)&31u);
    }
  }
  return result;
}
unsigned vista_session_phase_write(VistaSessionPhaseState& state,
    std::uint32_t& revision,std::uint32_t requested,std::int32_t count,
    std::span<VistaSessionPhasePeer> peers,std::array<std::uint32_t,2> clocks) {
  if(state.phase==requested)return 0;
  require_span(count,peers);
  bool marked{};
  for(std::int32_t i=0;i<count;++i)if(peers[std::uint32_t(i)].present) {
    peers[std::uint32_t(i)].marked=1;marked=true;
  }
  state.notification=1;state.previous_phase=state.phase;
  state.phase=requested;++state.phase_revision;state.previous_clock=clocks[0];
  ++revision;state.phase_clock=clocks[1];
  if(!marked)state.notification=0;
  return 2;
}
VistaSessionZeroResult vista_session_countdown_zero(
    VistaSessionCountdownCommitState& countdown,VistaSessionPhaseState& phase,
    const VistaSessionCommitContext& context,std::uint32_t preflight,
    std::int32_t count,std::span<const std::int32_t> statuses,
    std::span<VistaSessionPhasePeer> peers,std::array<std::uint32_t,2> clocks) {
  require_local(context);
  VistaSessionZeroResult result;
  call(result,VistaSessionZeroService::stage);
  if(!countdown.running || countdown.stage!=0)return result;
  call(result,VistaSessionZeroService::preflight);
  if(preflight>1)return result;
  call(result,VistaSessionZeroService::readiness);
  if(vista_session_peer_readiness(count,statuses).ready) {
    call(result,VistaSessionZeroService::phase,{2});
    const auto clock_count=vista_session_phase_write(phase,countdown.revision,2,count,peers,clocks);
    for(unsigned i=0;i<clock_count;++i)call(result,VistaSessionZeroService::clock);
    result.action=VistaSessionZeroAction::advance_phase;
  } else {
    call(result,VistaSessionZeroService::write,{1,1,0,2,0});
    vista_session_countdown_local_write(countdown,context,{1,1,0,2,std::nullopt});
    result.action=VistaSessionZeroAction::retry_stage;
  }
  return result;
}
} // namespace halo2
