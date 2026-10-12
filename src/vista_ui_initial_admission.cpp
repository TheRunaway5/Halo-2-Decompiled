#include "halo2/vista_ui_initial_admission.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t at) {
  std::uint32_t v{};
  for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);
  return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));
}
}
void vista_ui_context_initialize(std::span<std::byte> ui,bool enabled,
    const VistaUiContextConstructionServices& s) {
  if(ui.size()<0x60a || !s.create_listener || !s.resource_context_available ||
      !s.initialize_localized_messages)
    throw FormatError("Vista UI construction requires its complete context and platform/resource owners");
  put(ui,0,0);ui[4]=std::byte{};put(ui,8,0);
  ui[0xe]=ui[0xf]=std::byte{};
  std::fill_n(ui.begin()+0x11,0x3c,std::byte{});
  ui[0x4d]=ui[0xd]=ui[0x608]=ui[0x4e]=std::byte{};
  put(ui,0x60,0);ui[0xc]=std::byte{1};put(ui,0x604,0xffffffff);
  for(std::size_t at=0x5e8;at<=0x600;at+=4)put(ui,at,0);
  if(enabled)put(ui,0,s.create_listener(0xb));
  ui[0x10]=std::byte{};
  if(s.resource_context_available())s.initialize_localized_messages(ui);
  for(std::size_t i=0;i<4;++i) {
    ui[0x4e8+i*0x3c]=std::byte{};put(ui,0x520+i*0x3c,0);
  }
}
bool vista_ui_initial_signin_poll(std::span<std::byte> ui,
    const std::function<std::uint32_t(std::uint32_t)>& query,
    const std::function<void(std::uint32_t)>& apply) {
  if(ui.size()<0xd || !query || !apply)
    throw FormatError("Vista initial UI poll requires context, XUser query and source sign-in application");
  if(!get(ui,0) || ui[0xc]==std::byte{})return false;
  const auto state=query(0);apply(state);ui[0xc]=std::byte{};return true;
}
std::uint32_t vista_ui_xuser_signin_state(std::span<const std::byte> user) {
  if(user.size()<0x28)throw FormatError("Vista XUser sign-in query requires the complete supplied-DLL user bank");
  const auto state=get(user,0xc);return state==1 || state==2?state:0;
}
void vista_ui_apply_fresh_offline_signin(std::span<std::byte> ui,
    std::span<std::byte> manager,std::span<std::byte> game,std::span<std::byte> profile,
    const std::function<bool()>& special,
    const std::function<void(std::span<std::byte>)>& cancel) {
  if(ui.size()<0xc || manager.size()<0x1b44 || game.size()<0xad || profile.size()<0x1c0 ||
      !special || !cancel)
    throw FormatError("Vista fresh offline sign-in requires actual UI, manager, game, profile and platform owners");
  if(get(ui,8)!=0 || manager[0x1b42]!=std::byte{} || manager[0x1b43]!=std::byte{})
    throw FormatError("Vista fresh offline sign-in cannot substitute the previous-online or active-manager branches");
  put(ui,8,1);
  if(special())throw FormatError("Vista special UI sign-in requires its session and channel-clear owners");
  //5AFD25, state1 and both manager guard bytes zero.
  manager[0x1b40]=std::byte{};
  //44A7B5's argument high bits are irrelevant to its AL and48C8E4 body.
  game[4]=game[0]=game[1]=std::byte{};
  std::fill_n(profile.begin(),0x29,std::byte{});profile[0x29]=std::byte{};
  if(profile[0x30]!=std::byte{} && get(profile,0x1a4)==0x3e5)
    cancel(profile.subspan(0x1a4,0x1c));
  std::fill_n(profile.begin()+0x31,0x29,std::byte{});profile[0x30]=std::byte{};
  std::fill_n(profile.begin()+0x1a4,0x1c,std::byte{});
}
} // namespace halo2
