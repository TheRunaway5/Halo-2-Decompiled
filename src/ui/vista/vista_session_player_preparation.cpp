#include "halo2/vista_session_player_preparation.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t o,unsigned n=4) noexcept {
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[o+i]))<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v,unsigned n=4) noexcept {
  for(unsigned i=0;i<n;++i)b[o+i]=std::byte(v>>(8*i));
}
void validate(std::span<const std::byte> b) {
  if(b.size()<vista_session_source_size)throw FormatError("Vista player preparation requires the complete source session owner bank");
}
void increment(std::span<std::byte> b) noexcept {put(b,0x4c60,get(b,0x4c60)+1);}
std::uint32_t mask(std::uint32_t count) noexcept {return (1u<<(count&31))-1u;}
} // namespace
bool vista_session_prepare_players(std::span<std::byte> source) {
  validate(source);auto b=source.first(vista_session_source_size);
  if(get(b,0x73a0)!=5)return false;
  if(b[0x5101]!=std::byte{})return true;
  const auto count=std::bit_cast<std::int32_t>(get(b,0x84));
  if(count>18)throw FormatError("Vista preparation peer addresses exceed the bounded source scratch bank");
  const auto active=get(b,0x1258);
  for(unsigned i=0;i<16;++i) {
    const auto player=0x125cu+i*0x128u;
    if((active&(1u<<i)) && get(b,player+0x14)!=0xffffffffu && get(b,player+8)>17)
      throw FormatError("Vista preparation player references an unowned peer address");
  }
  std::array<std::byte,0x74> peers{};std::array<std::byte,0xd40> players{};
  put(peers,0,mask(std::bit_cast<std::uint32_t>(count)));
  for(std::int32_t i=0;i<count;++i)
    std::copy_n(b.begin()+0x88+std::size_t(i)*0x10c+0xa,6,peers.begin()+4+std::size_t(i)*6);
  for(unsigned i=0;i<16;++i) {
    const auto player=0x125cu+i*0x128u;
    if(!(active&(1u<<i)) || get(b,player+0x14)==0xffffffffu)continue;
    auto row=std::span(players).subspan(i*0xd4,0xd4);
    put(row,0,1,1);put(row,2,get(b,player+0xc,2),2);
    put(row,4,get(b,player+0x14));
    const auto address=4+get(b,player+8)*6;
    std::copy_n(peers.begin()+address,6,row.begin()+8);
    put(row,0xe,get(b,player));put(row,0x12,get(b,player+4));
    std::copy_n(b.begin()+player+0x9c,0x84,row.begin()+0x18);
    const auto team=std::to_integer<std::uint8_t>(row[0x94]);
    if(team!=0xff) {
      if(get(b,0x4e84,1)&1) {
        const auto signed_team=std::bit_cast<std::int8_t>(team);
        row[0x94]=std::byte(std::clamp(int(signed_team),0,7));
      } else row[0x94]=std::byte(i);
    }
  }
  b[0x5101]=std::byte{1};std::copy(peers.begin(),peers.end(),b.begin()+0x5104);
  std::copy(players.begin(),players.end(),b.begin()+0x5178);increment(b);
  if(get(b,0x5eb8,2)!=0xffff) {
    put(b,0x4c80,2);increment(b);put(b,0x4c7c,1);increment(b);
  }
  return true;
}
VistaSessionParameterRefreshResult vista_session_refresh_parameters(
    std::span<std::byte> source,std::int32_t target,std::uint8_t flag,
    std::optional<std::uint32_t> clock) {
  validate(source);auto b=source.first(vista_session_source_size);
  if(get(b,0x73a0)!=5)return {};
  if(std::bit_cast<std::int32_t>(get(b,0x84))<=1)return {true,false};
  if(!clock)throw FormatError("Vista parameter refresh requires its actual network-clock result");
  auto peers=1u<<(std::bit_cast<std::uint32_t>(target)&31);
  if(target==-1)peers=mask(get(b,0x84))&~(1u<<(get(b,0x71c0)&31));
  std::fill_n(b.begin()+0x73a4,0x58c,std::byte{});
  put(b,0x73a8,peers);put(b,0x73ac,*clock);put(b,0x73b0,0xffffffffu);put(b,0x73b4,flag,1);
  put(b,0x73a0,7);return {true,true};
}
} // namespace halo2
