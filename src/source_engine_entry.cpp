#include "halo2/source_engine_entry.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
template<class Entry>void apply(Entry& entry,std::span<const SourceEngineEntryDefinition> definitions,
    std::uint32_t mask,const SourceEngineEntryBlock& block,std::span<std::uint8_t> live) {
  const auto kind=entry.prefix.definition;
  if(kind<0 || std::size_t(kind)>=definitions.size())
    throw std::runtime_error("Original entry definition table input is unavailable");
  const auto& definition=definitions[std::size_t(kind)];
  const auto count=std::bit_cast<std::uint32_t>(entry.state_size);
  if(block.data.size()<count || live.size()<count)
    throw std::runtime_error("Original entry bank backing is unavailable");
  // Literal forward REP MOVSD then MOVSB, including well-backed overlapping
  // spans. A temporary dword avoids host alignment and aliasing requirements.
  std::uint32_t at=0;
  for(;count-at>=4;at+=4) {
    std::array<std::uint8_t,4> word;
    std::memcpy(word.data(),block.data.data()+at,4);std::memcpy(live.data()+at,word.data(),4);
  }
  for(;at<count;++at)live[at]=block.data[at];
  if(!definition.apply)throw std::runtime_error("Original entry application owner unavailable");
  definition.apply({reinterpret_cast<std::uint8_t*>(&entry),sizeof(entry)},mask,entry.state_size,live.first(count));
}
}
void source_engine_entry_update_apply(SourceEngineEntryRetail& entry,
    std::span<const SourceEngineEntryDefinition> definitions,std::uint32_t mask,
    const SourceEngineEntryBlock& block,std::span<std::uint8_t> live) {apply(entry,definitions,mask,block,live);}
void source_engine_entry_update_apply(SourceEngineEntryTitleUpdate& entry,
    std::span<const SourceEngineEntryDefinition> definitions,std::uint32_t mask,
    const SourceEngineEntryBlock& block,std::span<std::uint8_t> live) {apply(entry,definitions,mask,block,live);}
} // namespace halo2
