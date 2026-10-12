#include "halo2/vista_lobby_child_layout.hpp"
#include "halo2/cache.hpp"
#include <bit>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A... a) {
  if(!f)throw FormatError("Missing consumed Vista lobby child-layout service");
  return f(a...);
}
void hide(VistaLobbyChildHandle child,const VistaLobbyChildLayoutServices& s) {
  if(!child)throw FormatError("Missing required Vista lobby chat arrow child");
  call(s.store_visible,child,std::uint8_t{0});
}
}
void vista_lobby_child_layout(VistaLobbyChildLayoutState& state,
    const VistaLobbyChildLayoutServices& s) {
  state.arrows[0]=call(s.bitmap,std::int16_t{39});
  state.arrows[1]=call(s.bitmap,std::int16_t{40});
  if(!state.chat_enabled) {
    hide(state.arrows[0],s);
    hide(state.arrows[1],s);
  }
  if(const auto text=call(s.text,std::int16_t{51})) {
    auto bounds=call(s.get_bounds,text);
    //642F78..642F87 retains the low word of2*bottom-top. Evaluate in32 bits
    //and preserve those bits explicitly, including original signed16 wrapping.
    const auto bottom=2*std::int32_t{bounds[2]}-std::int32_t{bounds[0]};
    bounds[2]=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(bottom));
    call(s.set_bounds,text,bounds);
  }
  call(s.base_layout);
}
} // namespace halo2
