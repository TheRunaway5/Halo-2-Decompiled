#include "halo2/vista_lobby_leave_task.hpp"
#include "halo2/vista_session_disposal.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte>b,std::size_t at) {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;
}
void put(std::span<std::byte>b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));
}
void checked(std::span<const std::byte>b) {
  if(b.size()<vista_lobby_leave_task_size)throw FormatError("Vista Leave task requires genuine source-sized widget owner");
}
template<class F,class...A> decltype(auto) call(const F&f,A...a) {
  if(!f)throw FormatError("Missing reached Vista Leave task owner service");
  return f(a...);
}
}
std::array<std::uint32_t,8> vista_lobby_leave_task_descriptor(std::int32_t controller) {
  if(controller<0 || controller>=4)throw FormatError("Vista Leave task requires an actual selected controller");
  return {(1u<<controller)<<16,1,4,0,0xffffffffu,0xffffffffu,0xffffffffu,0x63941d};
}
void vista_lobby_leave_task_bind(std::span<std::byte>b) {
  checked(b);put(b,0xa68,0xffffffffu);put(b,0xa5c,0x642e8c);put(b,0xa60,0);put(b,0xa64,0);
}
void vista_lobby_leave_task_frame(std::span<std::byte>b,const VistaLobbyLeaveTaskFrameServices&s) {
  checked(b);const auto callback=get(b,0xa5c);
  if(get(b,0xa68)!=0xffffffffu || (callback && callback!=0x642e8c))
    throw FormatError("Unbound Vista Leave task async or callback owner");
  if(callback) {
    VistaLobbyLeaveTaskServices service;
    service.lifecycle_context=s.lifecycle_context;service.set_status=s.set_status;
    service.complete=[&](){put(b,0xa5c,0);};
    (void)vista_lobby_leave_task_update(service);
  }else if(!(std::to_integer<unsigned>(b[0x42])&2))call(s.close,3u);
  call(s.base_update);
}
} //namespace halo2
