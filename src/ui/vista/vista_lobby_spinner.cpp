#include "halo2/vista_lobby_spinner.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A&&... args) {
  if(!f)throw FormatError("Missing consumed Vista lobby spinner service");
  return f(std::forward<A>(args)...);
}
std::uint32_t word(std::span<const std::byte> b,std::size_t at) {
  if(at>b.size() || b.size()-at<4)
    throw FormatError("Vista lobby spinner requires its actual consumed session field");
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;
}
std::optional<std::span<const std::byte>> membership(const VistaLobbySpinnerSessionQuery& query) {
  const auto b=call(query);if(!b)return {};
  const auto state=word(*b,0x73a0);if(state<3 || state>8)return {};
  return b;
}
std::int32_t increment(std::int32_t value) {
  return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+1u);
}
}
std::uint8_t vista_lobby_spinner_player_present(std::uint32_t id,
    const VistaLobbySpinnerSessionQuery& query) {
  if(id==0xffffffffu)return 0;
  const auto b=membership(query);if(!b)return 0;
  const auto index=id&0xffffu;
  return index<16 && (word(*b,0x1258)&(1u<<index))?1:0;
}
std::uint8_t vista_lobby_spinner_peer_loading(std::uint32_t id,
    const VistaLobbySpinnerSessionQuery& query) {
  if(!vista_lobby_spinner_player_present(id,query))return 0;
  const auto b=membership(query);if(!b)return 0;
  //615B4F indexes the full producer-supplied ID here, unlike6156E1's
  //low16 occupancy test. The actual PC14 producer only enumerates0..15.
  if(id>=16)throw FormatError("Vista lobby spinner player lacks an owned source record");
  const auto peer=word(*b,0x125c+std::size_t(id)*0x128+8);
  if(peer>=17)throw FormatError("Vista lobby spinner player lacks an owned peer record");
  return word(*b,0x88+std::size_t(peer)*0x10c+0x88)==5?1:0;
}
void vista_lobby_spinner(VistaLobbySpinnerState& state,
    std::span<const std::uint32_t> ids,std::int32_t count,
    const VistaLobbySpinnerServices& s) {
  if(count>16 || (count>0 && std::size_t(count)>ids.size()))
    throw FormatError("Vista lobby spinner requires the original ordered player ID bank");
  for(std::int16_t i=42;i<58;++i)if(auto* b=call(s.bitmap,i))b->visible=0;
  for(std::int32_t row=0;row<count;++row) {
    const auto id=ids[std::size_t(row)];std::int32_t requested{};
    if(call(s.occupied,id) && call(s.peer_loading,id))requested=state.frame;
    const auto index=std::int16_t(row+42);
    if(auto* b=call(s.bitmap,index)) {
      b->visible=1;
      if(s.frame_requested)s.frame_requested(index,requested);
      const auto frame=std::bit_cast<std::int16_t>(std::uint16_t(std::bit_cast<std::uint32_t>(requested)));
      if(b->bound && frame>=0 && frame<b->pages)b->frame=std::uint16_t(frame);
    }
  }
  state.counter=increment(state.counter);
  if(state.counter>=3) {
    state.frame=increment(state.frame);
    if(state.frame>12)state.frame=2;
    state.counter=0;
  }
}
} //namespace halo2
