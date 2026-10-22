#include "halo2/vista_lobby_input.hpp"
#include "halo2/cache.hpp"
#include "halo2/vista_session_storage.hpp"
#include <bit>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A... a) {
  if(!f)throw FormatError("Missing consumed Vista lobby input service");
  return f(a...);
}
void bank(std::span<const std::byte> b) {
  if(b.size()<vista_session_source_size)throw FormatError("Vista lobby input requires complete source session storage");
}
std::uint32_t get(std::span<const std::byte> b,std::size_t at) {
  if(at>b.size() || 4>b.size()-at)throw FormatError("Vista lobby input read outside source storage");
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));
}
bool admitted(std::span<const std::byte> b) {const auto state=get(b,0x73a0);return state>=3 && state<=8;}
}
bool vista_lobby_cancel_request(std::int32_t controller,std::int32_t requested,
    const VistaLobbyCountdownServices& s) {
  const auto user=call(s.controller_user,controller);
  if(user==-1 || call(s.session_stage)==-1)return false;
  if(call(s.host_permission))
    return call(s.write,std::uint8_t{},requested,std::uint8_t{},user)!=0;
  if(call(s.ui_state)!=11)return false;
  return call(s.write,std::uint8_t{1},requested,std::uint8_t{1},user)!=0;
}
bool vista_lobby_cancel_action(std::int32_t controller,const VistaLobbyCountdownServices& s) {
  call(s.duration_context);
  return vista_lobby_cancel_request(controller,10,s);
}
VistaLobbyInputRoute vista_lobby_input(VistaLobbyInputAction action,std::int32_t controller,
    const VistaLobbyInputServices& s) {
  if(action==VistaLobbyInputAction::setup) {
    //6462A8..646378. Setup is not gated by the event handler's channel test.
    if(!call(s.signed_in,controller) || !call(s.host_permission))return VistaLobbyInputRoute::consumed;
    if(call(s.team_edit_active,controller))call(s.finish_team_edit,controller);
    if(!call(s.ready))return VistaLobbyInputRoute::consumed;
    call(s.settings_change,std::uint8_t{1});
    (void)call(s.cancel_action,controller);
    return VistaLobbyInputRoute::setup;
  }
  if(!call(s.pregame) || call(s.channel_in_use,1u))return VistaLobbyInputRoute::consumed;
  if(!call(s.signed_in,controller))return VistaLobbyInputRoute::unsigned_controller;
  if(call(s.team_edit_active,controller))return VistaLobbyInputRoute::team_edit;
  switch(action) {
  case VistaLobbyInputAction::start:
    if(call(s.countdown_active)) {
      call(s.start_action,controller);
      return VistaLobbyInputRoute::start_and_base_input;
    }
    return VistaLobbyInputRoute::base_input;
  case VistaLobbyInputAction::back:
    if(call(s.host_permission) && call(s.countdown_active)) {
      (void)call(s.cancel_action,controller);
      return VistaLobbyInputRoute::consumed;
    }
    return VistaLobbyInputRoute::leave;
  case VistaLobbyInputAction::auxiliary:
    if(call(s.countdown_active) && call(s.cancel_action,controller))return VistaLobbyInputRoute::consumed;
    return VistaLobbyInputRoute::team_edit;
  case VistaLobbyInputAction::setup:break;
  }
  throw FormatError("Unknown Vista lobby input action");
}
bool vista_lobby_event_admitted(std::span<const std::byte> b,bool enabled) {
  bank(b);if(!enabled)return false;
  const auto state=get(b,0x73a0);
  if(!state || state==2 || state==4 || state==6)return false;
  return (state!=7 && state!=8) || b[0x73a4]==std::byte{};
}
bool vista_lobby_setup_ready(std::span<const std::byte> b,bool current) {
  bank(b);return current && admitted(b) && get(b,0x71c0)==get(b,0x74) && get(b,0x4c68)==1;
}
bool vista_lobby_controller_signed_in(std::span<const std::byte> b,std::int32_t controller) {
  if(controller<0 || controller>=4)throw FormatError("Vista lobby controller is outside actual storage");
  return (get(b,std::size_t(controller)*0x127c)>>3)&1;
}
std::uint32_t vista_lobby_controller_count(std::span<const std::byte> b) {
  std::uint32_t count{};
  for(std::int32_t i=0;i<4;++i)count+=vista_lobby_controller_signed_in(b,i);
  return count;
}
std::uint32_t vista_lobby_controller_mask(std::span<const std::byte> b) {
  std::uint32_t mask{};
  for(std::int32_t i=0;i<4;++i)if(vista_lobby_controller_signed_in(b,i))mask|=1u<<i;
  return mask;
}
std::int32_t vista_lobby_peer_count(std::span<const std::byte> b,bool current) {
  bank(b);return current && admitted(b)?std::bit_cast<std::int32_t>(get(b,0x84)):0;
}
bool vista_lobby_settings_change_local(std::span<std::byte> b,bool current,std::uint8_t value) {
  bank(b);if(!current || !admitted(b))return false;
  if(get(b,0x73a0)<5)throw FormatError("Vista lobby settings change requires unbound remote transport");
  if(get(b,0x71c0)!=get(b,0x74))return false;
  put(b,0x4c60,get(b,0x4c60)+1);b[0x4c8d]=std::byte(value);return true;
}
bool vista_lobby_countdown_write_request(std::span<const std::byte> b,
    std::span<const std::byte> users,bool current,std::uint8_t running,std::int32_t stage,
    std::uint32_t reason,std::int32_t user,std::uint8_t flag,
    const std::function<std::uint8_t(const VistaSessionCountdownWrite&)>& write) {
  bank(b);if(!current || !admitted(b))return false;
  if(user<0 || user>=4 || users.size()<4*0xb8)throw FormatError("Vista countdown user is outside actual storage");
  const auto row=users.subspan(std::size_t(user)*0xb8,0xb8);
  if(row[0]==std::byte{} && b[0x4c74]==std::byte{})return false;
  return call(write,VistaSessionCountdownWrite{running,stage,flag,reason,
      std::array<std::uint32_t,2>{get(row,1),get(row,5)}})!=0;
}
} // namespace halo2
