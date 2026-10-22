#include "halo2/player_screen_flash.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <cmath>

namespace halo2 {
std::vector<PlayerScreenResponse> load_player_screen_responses(const Cache &cache,std::uint32_t datum) {
  const auto source=cache.load_source_tag_metadata(datum);
  if(source.group()!="jpt!")throw FormatError("Player screen flash requires an authored damage effect");
  const Bytes root(source.bytes_at(source.source_address(),0x74));
  const auto count=root.u32(0x6c);
  if(count>256)throw FormatError("Excessive player screen response block");
  std::vector<PlayerScreenResponse> result;
  if(!count)return result;
  // Interned blocks can precede this tag's own allocation, as with rumble.
  const Bytes rows(cache.read_tag_bytes(root.u32(0x70),count*0x4c));
  for(unsigned i=0;i<count;++i) {
    const auto at=i*0x4c;
    PlayerScreenResponse row;row.type=std::int16_t(rows.u16(at));
    auto &f=row.flash;f.type=std::int16_t(rows.u16(at+4));f.priority=std::int16_t(rows.u16(at+6));
    f.duration=rows.f32(at+8);f.curve=std::int16_t(rows.u16(at+0xc));f.intensity=rows.f32(at+0x10);
    for(unsigned c=0;c<4;++c)f.color[c]=rows.f32(at+0x14+c*4);
    if(!std::isfinite(f.duration) || !std::isfinite(f.intensity))throw FormatError("Invalid authored player screen response");
    result.push_back(row);
  }
  return result;
}
}
