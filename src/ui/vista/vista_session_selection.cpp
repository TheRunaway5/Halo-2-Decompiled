#include "halo2/vista_session_selection.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t read(std::span<const std::byte> b,std::size_t at) {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;
}
void write(std::span<std::byte> b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));
}
bool admitted(std::span<const std::byte> b) {
  if(b.size()<vista_session_source_size)throw FormatError("Truncated Vista selected-parameter owner");
  const auto state=read(b,0x73a0);
  if(state==3 || state==4)throw FormatError("Vista remote selected-parameter transport is unbound");
  return state>=5 && state<=8 && read(b,0x71c0)==read(b,0x74);
}
}
bool vista_session_select_map(std::span<std::byte> b,std::uint32_t campaign_id,
    std::uint32_t id,std::u16string_view path) {
  if(!admitted(b))return false;
  write(b,0x4cb8,campaign_id);write(b,0x4dd8,id);
  // Actual43804A(dst,src,80h) is the bounded UTF16 copy used by this setter.
  // Like the original, only the copied units and terminating word are written.
  const auto n=std::min<std::size_t>(path.find(u'\0'),std::min<std::size_t>(path.size(),127));
  for(std::size_t i=0;i<=n;++i) {
    const auto c=i==n?char16_t{}:path[i];
    b[0x4cbc+2*i]=std::byte(c);b[0x4cbd+2*i]=std::byte(c>>8);
  }
  std::fill_n(b.begin()+0x4ddc,0x60,std::byte{});
  write(b,0x4c60,read(b,0x4c60)+1);return true;
}
bool vista_session_select_variant(std::span<std::byte> b,
    const std::array<std::byte,0x130>* variant) {
  if(!admitted(b))return false;
  write(b,0x4c60,read(b,0x4c60)+1);
  if(variant && read(*variant,0x44)!=0)std::copy(variant->begin(),variant->end(),b.begin()+0x4e3c);
  else std::fill_n(b.begin()+0x4e3c,0x130,std::byte{});
  return true;
}
std::int32_t vista_session_frontend_protocol(std::span<const std::byte> b,
    bool enabled,std::int32_t lifecycle_kind) {
  //5AD736 returns before dereferencing its retained session when disabled.
  if(!enabled)return -1;
  if(b.size()<vista_session_source_size)
    throw FormatError("Vista frontend protocol requires the actual complete session owner");
  const auto state=read(b,0x73a0);
  //5B1643's original table admits precisely3..8 after5AD736's state0 gate.
  if(state<3 || state>8)return -1;
  const auto session_class=std::bit_cast<std::int32_t>(read(b,0x1c));
  if(session_class<0 || session_class>2)return -1;
  //5B1313 and5B126F independently pass through5AD782, which adds this
  // lifecycle-kind gate to5AD736. Their field tables then also require3..8.
  const bool parameters=lifecycle_kind>=1 && lifecycle_kind<=5;
  std::int32_t campaign_id=-1;
  if(parameters && read(b,0x4dd8)!=0xffffffffu &&
      (b[0x4cbc]!=std::byte{} || b[0x4cbd]!=std::byte{}))
    campaign_id=std::bit_cast<std::int32_t>(read(b,0x4cb8));
  //5C2321's missing-map branch supplies NONE campaign/map IDs; its custom
  // metadata pointer does not convert the map to campaign in615EA9.
  if(session_class==2 && read(b,0x4c90)!=1)return -1; //5B0E43->5C225B.
  if(campaign_id!=-1)return session_class==0?0:session_class==1?2:4;
  if(!parameters)return -1; //5B1313 returned null.
  const auto engine=read(b,0x4e3c+0x44);
  // Exact615FBC/615FD0 nine-byte dispatch tables, indexed by engine-1.
  if(engine<1 || engine>9 || engine==5 || engine==6)return -1;
  return session_class==0?1:session_class==1?3:5;
}
} // namespace halo2
