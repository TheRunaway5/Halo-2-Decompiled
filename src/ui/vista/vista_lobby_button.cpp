#include "halo2/vista_lobby_button.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista lobby button service");
  return f(std::forward<A>(a)...);
}
std::int32_t signed_controller(const VistaLobbyButtonEvent& e) {
  return std::bit_cast<std::int32_t>(e.controller);
}
void copy_chat(VistaLobbyButtonChat& dst,std::optional<std::u16string_view> src,
    const VistaLobbyButtonServices& s) {
  if(!src) {
    dst[0]=0;call(s.set_errno,22u);call(s.invalid_parameter);return;
  }
  //687567 copies through the terminator, preserving all later bank units.
  //If the121st copied unit is nonzero, only the first unit is then cleared.
  for(std::size_t i=0;i<dst.size();++i) {
    const auto ch=i<src->size()?(*src)[i]:char16_t{};
    dst[i]=ch;if(!ch)return;
  }
  dst[0]=0;call(s.set_errno,34u);call(s.invalid_parameter);
}
void fill_task(VistaLobbyButtonTask& request,std::uint32_t controller,std::uint32_t factory) {
  //The source SHL masks CL by31, then60B0C6 stores the low16 of the mask.
  const auto mask=std::uint16_t(1u<<(controller&31u));
  request[0]=4u|(std::uint32_t(mask)<<16);request[1]=4;request[2]=4;
  request[4]=0xffffffff;request[5]=0xffffffff;request[6]=0xffffffff;request[7]=factory;
}
}
void vista_lobby_button(VistaLobbyButtonEvent& event,std::uint16_t& ordinal,
    VistaLobbyButtonState& state,VistaLobbyButtonChat& chat,const VistaLobbyButtonServices& s) {
  const auto first=ordinal;
  if(first==4 && state.chat_enabled) {
    const auto owner=call(s.chat_owner);
    copy_chat(chat,call(s.chat_characters,owner),s);
    if(call(s.language)==6)return;
    if(call(s.language)==1)return;
    call(s.keyboard,signed_controller(event),15u,std::span<const char16_t>(chat),121u);
    return;
  }
  if(first==3) {call(s.game_details);return;}
  if(!call(s.signed_in,signed_controller(event)))return;
  if(!call(s.host))return;
  //This is a new load after signed_in/host, not a captured initial controller.
  const auto controller=event.controller;
  if(controller>=state.team_edit.size())throw FormatError("Vista lobby button team load outside actual controller storage");
  if(state.team_edit[controller])call(s.finish_team_edit,std::bit_cast<std::int32_t>(controller),std::uint8_t{});
  if(!call(s.setup_ready))return;
  switch(std::bit_cast<std::int16_t>(ordinal)) {
  case 0:
    if(call(s.readiness)==10)call(s.start_action,signed_controller(event));
    return;
  case 1: {
    VistaLobbyButtonTask request{};request[3]=0; //60B0BC precedes the session calls.
    call(s.settings_change,std::uint8_t{1});
    call(s.cancel_action,signed_controller(event));
    fill_task(request,event.controller,0x64fe89);call(s.task_factory,0x64fe89u,request);
    return;
  }
  case 2: {
    VistaLobbyButtonTask request{};request[3]=0;
    fill_task(request,event.controller,0x646186);call(s.task_factory,0x646186u,request);
    return;
  }
  default:return;
  }
}
} //namespace halo2
