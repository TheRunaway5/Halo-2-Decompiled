#include "halo2/vista_lobby_leave_return.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
namespace {
template<class F,class...A> decltype(auto) call(const F& f,A...a) {
  if(!f)throw FormatError("Missing reached Vista lobby Leave return service");
  return f(a...);
}
}
bool vista_lobby_leave_observe(VistaLobbyLeaveReturnState& state,std::uint32_t screen,
    std::uint32_t old_class,std::uint32_t context,const VistaLobbyLeaveReturnServices& s) {
  if(context) {
    const auto protocol=call(s.protocol);
    if(protocol!=0xffffffffu)state.cached_protocol=protocol;
    return false;
  }
  if(call(s.request_pending))return false;
  const auto mask=call(s.controller_mask)&0xffffu;
  const auto protocol=state.cached_protocol;
  // Unsupported corridors need a separate original dialog/signout owner;
  // reject before touching the retained cache rather than inventing a route.
  if(old_class>2 || (old_class==2 && (screen!=14 || protocol<2 || protocol>5)))
    throw FormatError("Unbound Vista lobby Leave observer suffix route");
  state.cached_protocol=0xffffffffu;
  if(old_class<2 || protocol>=4)return false;
  if(call(s.screen_present,5u,4u,13u) || call(s.screen_present,5u,4u,6u))return false;
  call(s.create_screen,std::array<std::uint32_t,8>{(mask<<16)|7u,5,4,0,
      0xffffffffu,0xffffffffu,0xffffffffu,0x61a238});
  return true;
}
} //namespace halo2
