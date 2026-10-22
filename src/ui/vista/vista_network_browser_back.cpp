#include "halo2/vista_network_browser_back.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
namespace {
template<class F,class...A> decltype(auto) call(const F& f,A...a) {
  if(!f)throw FormatError("Missing reached Vista browser Back service");
  return f(a...);
}
}
bool vista_network_browser_back_event(std::uint32_t type,std::uint32_t component) noexcept {
  return type==5 && (component==1 || component==13);
}
std::uint8_t vista_network_browser_back(std::uint8_t modal,const VistaBrowserBackServices& s) {
  if(!modal) {
    const auto channel=call(s.channel);
    const auto window=call(s.window);
    if(!call(s.screen_present,channel,window,6u))call(s.mainmenu_reason,2u);
    return 1;
  }
  const auto window=call(s.window);
  const auto channel=call(s.channel);
  call(s.close_channel,channel,window);
  return call(s.base_input);
}
void vista_network_browser_mainmenu_return_single(VistaBrowserMainmenuReturnState& state,
    const VistaBrowserMainmenuReturnServices& s) {
  if(!state.initialized) {
    call(s.initialize_profiles);
    state.initialization_clock=call(s.ui_clock);
  }
  state.initialized=1;
  if((call(s.controller_count)&0xffffu)!=1)
    throw FormatError("Vista browser Back requires a separately bound factory for this controller count");
  const auto mask=call(s.controller_mask)&0xffffu;
  const std::array<std::uint32_t,8> descriptor{(mask<<16)|4u,5,4,0,
      0xffffffffu,0xffffffffu,0xffffffffu,0x40b7b9};
  state.pending_special_menu=0;
  call(s.create_screen,descriptor);
}
void vista_network_browser_mainmenu_return_local(VistaBrowserMainmenuReturnState& state,
    const VistaBrowserMainmenuReturnServices& s) {
  if(!state.initialized) {
    call(s.initialize_profiles);state.initialization_clock=call(s.ui_clock);
  }
  state.initialized=1;
  const auto count=call(s.controller_count)&0xffffu;
  if(!count || count>4)throw FormatError("Vista local browser Back requires1..4 actual profiles");
  const auto mask=count==1?(call(s.controller_mask)&0xffffu):0xffffu;
  const std::array<std::uint32_t,8> descriptor{(mask<<16)|4u,5,4,0,
      0xffffffffu,0xffffffffu,0xffffffffu,count==1?0x40b7b9u:0x63f6b7u};
  state.pending_special_menu=0;call(s.create_screen,descriptor);
}
void vista_network_browser_refresh_profiles(std::span<std::byte> controllers,
    std::uint8_t& pending,const VistaBrowserProfileRefreshServices& services) {
  vista_refresh_selected_profiles(controllers,pending,services);
}
} //namespace halo2
