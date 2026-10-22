#include "halo2/vista_session_disposal.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t at) {
  if(at>b.size() || 4>b.size()-at)throw FormatError("Vista disposal read outside actual owner");
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(i*8);return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(i*8));
}
void clear(std::span<std::byte> b,std::size_t at,std::size_t n) {
  std::fill_n(b.begin()+at,n,std::byte{});
}
template<class F> const F& required(const F& f) {
  if(!f)throw FormatError("Missing reached Vista disposal owner service");
  return f;
}
}
bool vista_session_dispose_single_offline(std::span<std::byte> source,
    VistaSessionDisposalBanks owners,bool network,bool qos,const VistaSessionDisposalServices& s) {
  if(source.size()<vista_session_source_size)throw FormatError("Vista disposal requires complete source session");
  auto b=source.first(vista_session_source_size);const auto state=get(b,0x73a0);
  if(!state)return false;
  if(state<5 || state>8 || get(b,0x84)!=1 || get(b,0x74)!=0 || get(b,0x71c0)!=0 ||
      b[0x6c]!=std::byte{} || get(b,0x7b78)!=0 || b[0x71c4]==std::byte{} ||
      b[0x71c5]!=std::byte{} || get(b,0x71c8)!=0xffffffffu)
    throw FormatError("Vista disposal requires genuine sole local host without transport association");
  for(unsigned i=1;i<17;++i)if(b[0x71c4+i*0x1c]!=std::byte{})
    throw FormatError("Vista disposal reached an unbound remote association");
  const auto key=std::uint64_t(get(b,0x5c))*32;
  const auto registration=std::uint64_t(get(b,0x14))*36+0x18;
  const bool has_key=b[0x48]!=std::byte{};
  if(has_key) {
    if(get(b,8)!=owners.transport_owner_word || key+32>owners.keys.size() || registration+4>owners.transport_registrations.size())
      throw FormatError("Vista disposal requires actual key/transport registration owners");
    if(qos)(void)required(s.release_qos);
    if(owners.keys[key]!=std::byte{} && network && owners.keys[key+2]==std::byte{})
      (void)required(s.unregister_key);
  }
  //5CC0A8(true): state6 is already closing;7/8 with closing byte also return.
  // State8's distinct jump-table entry sets only the closing byte. States5/7
  // reach5CA1CD and broadcast13h; the actual local association byte+1 is zero,
  // so5C7F0C returns before any transport send.
  if(state==8 && b[0x73a4]==std::byte{})b[0x73a4]=std::byte{1};
  else if(state==5 || (state==7 && b[0x73a4]==std::byte{})) {
    clear(b,0x73a4,0x58c);put(b,0x73a0,6);
  }
  // Actual5C73A3. Local channel is NONE, so no5BD8BB owner is consumed.
  clear(b,0x71c4,0x1c);put(b,0x71c8,0xffffffffu);
  put(b,0x7960,get(b,0x7960)+1);put(b,0x71c0,0xffffffffu);
  clear(b,0x793c,9*4);put(b,0x7964,0xffffffffu);put(b,0x7968,0xffffffffu);
  b[0x7b7c]=std::byte{};b[0x796c]=std::byte{};
  if(has_key) {
    if(qos)required(s.release_qos)(b.subspan(0x20,8));
    if(owners.keys[key]!=std::byte{}) {
      if(network && owners.keys[key+2]==std::byte{})required(s.unregister_key)(owners.keys.subspan(key+8,8));
      owners.keys[key]=std::byte{};
    }
    put(owners.transport_registrations,registration,0xffffffffu);
    clear(b,0x20,8);clear(b,0x49,16);put(b,0x60,0xffffffffu);b[0x48]=std::byte{};
  }
  clear(b,0x20,8);put(b,0x64,0xffffffffu);b[0x6c]=std::byte{};
  for(const auto& [at,n]:std::array<std::pair<std::size_t,std::size_t>,4>{{{0x70,0x2470},{0x4c60,0x12b0},{0x4950,0xc8},{0x4ae0,0xa0}}}) {
    clear(b,at,n);put(b,at,0xffffffffu);
  }
  clear(b,0x7978,0x200);put(b,0x73a0,0);return true;
}
void vista_browser_reset(std::span<std::byte> b,std::uint32_t mode,const VistaBrowserResetServices& s) {
  if(b.size()<vista_browser_source_size)throw FormatError("Vista browser reset requires complete source owner");
  // Validate reached services before making the source's first write.
  if(get(b,0x428)!=0xffffffffu)(void)required(s.close_handle);
  if(get(b,0xc))(void)required(s.reset_child);
  if(mode && mode!=0xffffffffu && b[0x4044c]!=std::byte{})(void)required(s.release_query);
  put(b,8,mode);
  if(get(b,0x428)!=0xffffffffu){required(s.close_handle)(get(b,0x428));put(b,0x428,0xffffffffu);}
  if(get(b,0xc))required(s.reset_child)(get(b,0xc));
  if(mode && mode!=0xffffffffu && b[0x4044c]!=std::byte{})required(s.release_query)(b.subspan(0x21e,0x208));
  put(b,4,0);clear(b,0x10,0x20e);clear(b,0x21e,0x208);
  b[0x426]=std::byte{1};b[0x42c]=std::byte{1};b[0x42d]=std::byte{1};
  put(b,0,0);put(b,0x428,0xffffffffu);put(b,0x430,0);clear(b,0x434,0x40000);
  for(const auto at:{0x40434,0x4043c,0x40440,0x40444,0x40448,0x40454,0x40458})put(b,at,0);
  b[0x4044c]=std::byte{};
}
bool vista_lobby_leave_single(std::int32_t controller,const VistaLobbyLeaveServices& s) {
  if(required(s.peer_count)()>1)return false;
  required(s.browser_reset)();
  required(s.disconnect)(required(s.peer_count)()<=1);
  required(s.create_task)(controller);return true;
}
bool vista_lobby_leave_task_update(const VistaLobbyLeaveTaskServices& s) {
  const auto context=required(s.lifecycle_context)();required(s.set_status)(0x0d000441);
  if(context)return false;
  required(s.complete)();return true;
}
} // namespace halo2
