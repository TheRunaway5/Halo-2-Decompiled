#include "halo2/vista_lobby_countdown.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
VistaLobbyCountdownUpdate vista_lobby_countdown_consume(
    VistaLobbyCountdownState& state,const VistaLobbyCountdownQueryResults& query) {
  VistaLobbyCountdownUpdate update;
  const auto call=[&](VistaLobbyCountdownService service,
                      std::optional<std::uint32_t> argument={}) {
    update.calls[update.call_count++]={service,argument};
  };
  call(VistaLobbyCountdownService::active_query);
  if(!query.active) {
    state.stage=-1;
    return update;
  }
  const auto previous=state.stage;
  call(VistaLobbyCountdownService::stage_query);
  //644892 stores every signed stage before its bounds/unchanged checks.
  state.stage=query.stage;
  if(state.stage<0 || state.stage>3 || state.stage==previous)return update;
  call(VistaLobbyCountdownService::ui_sound,10);
  //6448A5 precedes6448B6: the sound intent comes before the clock query.
  if(state.stage==3) {
    call(VistaLobbyCountdownService::clock_query);
    state.stage3_clock=query.clock;
  }
  return update;
}

namespace {
template<class Function,class... Args>
decltype(auto) service(const Function& function,Args... args) {
  if(!function)throw FormatError("Unbound consumed Vista lobby countdown service");
  return function(args...);
}
}
bool vista_lobby_countdown_active(const VistaLobbyCountdownServices& s) {
  const auto ui=service(s.ui_state);
  std::uint32_t slot{};
  if(service(s.lifecycle_active))slot=service(s.lifecycle_slot);
  return ui==11 || slot==2 || slot==3;
}
std::int32_t vista_lobby_countdown_stage(const VistaLobbyCountdownServices& s) {
  if(service(s.ui_state)!=11)return 0;
  const auto stage=service(s.session_stage);
  return stage>0?stage:0;
}
void vista_lobby_countdown_update(VistaLobbyCountdownState& state,
    const VistaLobbyCountdownServices& s) {
  if(!vista_lobby_countdown_active(s)){state.stage=-1;return;}
  const auto previous=state.stage;
  state.stage=vista_lobby_countdown_stage(s);
  if(state.stage<0 || state.stage>3 || state.stage==previous)return;
  service(s.ui_sound,10u);
  if(state.stage==3)state.stage3_clock=service(s.ui_clock);
}
bool vista_lobby_countdown_start_action(std::int32_t controller,
    const VistaLobbyCountdownServices& s) {
  const bool active=vista_lobby_countdown_active(s);
  service(s.duration_context);
  const auto ui=service(s.ui_state);
  const auto user=service(s.controller_user,controller);
  bool accepted{};
  if(user!=-1) {
    if(ui==10 && service(s.host_permission))
      accepted=service(s.write,std::uint8_t{1},10,std::uint8_t{},user)!=0;
    else if(ui==11) {
      const auto stage=vista_lobby_countdown_stage(s);
      if(stage>3)accepted=service(s.write,std::uint8_t{1},stage-1,std::uint8_t{},user)!=0;
    }
  }
  if(!active && !accepted)service(s.ui_sound,2u);
  return accepted;
}
} // namespace halo2
