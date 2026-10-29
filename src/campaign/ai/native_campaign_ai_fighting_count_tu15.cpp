#include "halo2/native_campaign_ai_query_tu15.hpp"
#include <array>
#include <bit>

namespace halo2 {
namespace {
class CombatCounts {
  const SourcePhysicsContactMemory &memory_;
  std::uint32_t read(std::uint32_t address,unsigned width=4) const {
    std::array<std::byte,4> bytes{};
    memory_.read(address,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned i=0;i<width;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);
    return value;
  }
  std::int16_t short_at(std::uint32_t address) const {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(address,2)));
  }
  bool fighting(std::uint32_t actor) const {
    const auto pool=read(0x5020c8u);
    const auto rows=read(pool+0x44u);
    const auto row=rows+(actor&0xffffu)*0x888u;
    if(!read(row+9u,1) || short_at(row+0x328u)<=2)return false;
    const auto clock=read(0x51df54u);
    const auto threshold=std::int32_t(short_at(clock+2u))*20;
    if(std::int32_t(short_at(row+0x306u))>threshold)return false;
    // The behavior owner performs a fresh pool/row lookup. Preserve it: source
    // reads can fault or observe a newly published bank between these owners.
    const auto behavior_pool=read(0x5020c8u);
    const auto behavior_row=read(behavior_pool+0x44u)+(actor&0xffffu)*0x888u;
    const auto last=short_at(behavior_row+0x190u);
    if(last<0)return true;
    std::uint16_t index=0;
    do {
      const auto offset=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(index)))*0x40u;
      //1AFBD0 returns a SHORT index. A wrapped -1 is its null sentinel even
      // when that slot matches;1EA090 compares the returned bits with FFFF.
      if(short_at(behavior_row+0x90u+offset)==0x38)return index==0xffffu;
      ++index;
    } while(std::bit_cast<std::int16_t>(index)<=last);
    return true;
  }
public:
  explicit CombatCounts(const SourcePhysicsContactMemory &memory):memory_(memory) {}
  std::uint16_t query(std::uint32_t reference) const {
    const auto kind=reference>>30;
    if(kind>=2u) {
      const auto actor=native_campaign_ai_actor_tu15(memory_,reference);
      return actor!=0xffffffffu && fighting(actor)?1u:0u;
    }
    const auto index=reference&0xffffu;
    const auto scenario=read(0x4ecc20u);
    const auto count=std::bit_cast<std::int32_t>(read(scenario+(kind==0u?0x160u:0x158u)));
    if(std::int32_t(index)>=count)return 0;
    const auto pool=read(kind==0u?0x52be7cu:0x52be80u);
    const auto rows=read(pool+0x44u);
    return std::uint16_t(read(rows+index*(kind==0u?0x98u:0x38u)+(kind==0u?0x16u:0x2cu),2));
  }
};
}
std::uint16_t native_campaign_ai_fighting_count_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t reference) {
  return CombatCounts(memory).query(reference);
}
}
