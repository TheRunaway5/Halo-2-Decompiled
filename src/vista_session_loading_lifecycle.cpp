#include "halo2/vista_session_loading_lifecycle.hpp"
#include "halo2/cache.hpp"
#include <initializer_list>

namespace halo2 {
namespace {
bool local(std::uint32_t state) noexcept { return state>=5 && state<=8; }
void call(VistaSessionLoadingUpdate& r,VistaSessionLoadingService service,
          std::initializer_list<std::uint32_t> arguments={}) {
  auto& c=r.calls[r.call_count++];c.service=service;
  c.argument_count=unsigned(arguments.size());
  unsigned i{};for(const auto value:arguments)c.arguments[i++]=value;
}
bool gate(VistaSessionLoadingHandlerState& s,
          const VistaSessionLoadingGateQueries& q,VistaSessionLoadingUpdate& r) {
  if(!local(q.session_state)) { s.flag10=0;return false; }
  call(r,VistaSessionLoadingService::network_query);
  if(q.local_flag || !q.network_query) return true;
  if(q.session_state_after_query==7 && !q.transition_flag) return false;
  if(q.session_state_after_query!=5) return false;
  if(s.flag10) return true;
  const auto previous=s.flag11;s.flag11=0;s.flag10=1;
  call(r,VistaSessionLoadingService::refresh_parameters,{0xffffffffu,previous});
  return !q.refresh_accepted;
}
} // namespace
VistaSessionLoadingUpdate vista_session_loading_enter(
    VistaSessionLoadingHandlerState& s,std::uint32_t old_handler,
    const VistaSessionLoadingEnterQueries& q) {
  const bool initialize=local(q.session_state) && q.phase==2;
  if(initialize && !q.values)
    throw FormatError("Vista start-game enter requires actual seed/random service outputs");
  VistaSessionLoadingUpdate r;
  call(r,VistaSessionLoadingService::base_enter,{old_handler,0,0});
  if(initialize) {
    call(r,VistaSessionLoadingService::seed_query);
    call(r,VistaSessionLoadingService::random_bytes,{8});
    call(r,VistaSessionLoadingService::identity_write,
         {q.values->identity[0],q.values->identity[1]});
    call(r,VistaSessionLoadingService::seed_write,{q.values->seed});
    call(r,VistaSessionLoadingService::clear_parameter_byte);
    call(r,VistaSessionLoadingService::clear_prepared_banks,{0,0});
  }
  s.flag10=0;s.flag11=0;
  return r;
}
VistaSessionLoadingUpdate vista_session_loading_update(
    VistaSessionLoadingHandlerState& s,const VistaSessionLoadingUpdateQueries& q) {
  if(!q.preflight && !q.pending_after_preflight && q.admitted && q.phase==2 &&
      q.authority && q.players_prepared && !q.gate)
    throw FormatError("Vista start-game update requires actual post-preparation gate fields");
  VistaSessionLoadingUpdate r;r.repeat=q.preflight;
  call(r,VistaSessionLoadingService::preflight);
  if(q.preflight || q.pending_after_preflight) return r;
  call(r,VistaSessionLoadingService::admitted);
  if(!q.admitted) return r;
  if(q.phase==4) {
    call(r,VistaSessionLoadingService::request,{3,0,0});return r;
  }
  if(q.phase!=2) {
    call(r,VistaSessionLoadingService::request,{1,0,0});return r;
  }
  call(r,VistaSessionLoadingService::authority);
  if(!q.authority) return r;
  call(r,VistaSessionLoadingService::prepare_players);
  if(!q.players_prepared) return r;
  auto next=s;
  if(gate(next,*q.gate,r)) {
    call(r,VistaSessionLoadingService::clear_countdown);
    call(r,VistaSessionLoadingService::phase_write,{4});r.repeat=1;
  }
  s=next;return r;
}
} // namespace halo2
