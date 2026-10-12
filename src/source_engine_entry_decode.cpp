#include "halo2/source_engine_entry_decode.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
template<class Callback>const Callback& required(const Callback& value) {
  if(!value)throw std::runtime_error("Original entry decoding owner unavailable");
  return value;
}
template<class Entry>std::int32_t decode(const Entry& entry,std::uint32_t handle,
    std::span<const SourceEngineEntryDecoder> definitions,std::span<const std::uint8_t> live,
    SourceEngineEntryAllocator& allocator,std::uint32_t& mask,std::int32_t& produced,
    std::span<SourceEngineEntryBlock> blocks,SourceEngineBitstream& stream) {
  if(handle==0xffffffffu || entry.prefix.entity!=handle)return 3;
  const auto kind=entry.prefix.definition;
  if(kind<0 || std::size_t(kind)>=definitions.size())
    throw std::runtime_error("Original entry decoder definition input unavailable");
  const auto& definition=definitions[std::size_t(kind)];
  auto allocation=required(allocator.allocate)(entry.state_size);
  if(!allocation.address) {
    required(allocator.compact)();allocation=required(allocator.allocate)(entry.state_size);
    if(!allocation.address)return 3;
  }
  ++allocator.outstanding;
  const auto count=std::bit_cast<std::uint32_t>(entry.state_size);
  if(allocation.bytes.size()<count)
    throw std::runtime_error("Original entry private bank backing unavailable");
  auto bank=allocation.bytes.first(count);
  const auto reject=[&] {
    required(allocator.inspect)(allocation);required(allocator.release)(allocation);
    --allocator.outstanding;return 3;
  };
  if(stream.state.mode==4) {
    std::fill(bank.begin(),bank.end(),0);
    if(!required(definition.initialize)(entry.creation_size,entry.creation_address,entry.state_size,bank))return reject();
  }else {
    if(live.size()<count)throw std::runtime_error("Original entry live bank backing unavailable");
    // The original private allocation is distinct from the live bank.
    std::memcpy(bank.data(),live.data(),count);
  }
  std::uint32_t read_mask=0;
  if(!required(definition.read)(false,read_mask,entry.state_size,bank,stream))return reject();
  mask=read_mask;
  const auto index=std::bit_cast<std::uint32_t>(produced);
  if(index>=blocks.size())throw std::runtime_error("Original entry output block backing unavailable");
  blocks[index]={std::bit_cast<std::int16_t>(std::uint16_t(entry.state_size)),13,allocation.address,bank};
  produced=std::bit_cast<std::int32_t>(index+1u);return 0;
}
}
std::int32_t source_engine_entry_update_decode(const SourceEngineEntryRetail& entry,
    std::uint32_t handle,std::span<const SourceEngineEntryDecoder> definitions,std::span<const std::uint8_t> live,
    SourceEngineEntryAllocator& allocator,std::uint32_t& mask,std::int32_t,std::int32_t& produced,
    std::span<SourceEngineEntryBlock> blocks,SourceEngineBitstream& stream) {
  return decode(entry,handle,definitions,live,allocator,mask,produced,blocks,stream);
}
std::int32_t source_engine_entry_update_decode(const SourceEngineEntryTitleUpdate& entry,
    std::uint32_t handle,std::span<const SourceEngineEntryDecoder> definitions,std::span<const std::uint8_t> live,
    SourceEngineEntryAllocator& allocator,std::uint32_t& mask,std::int32_t,std::int32_t& produced,
    std::span<SourceEngineEntryBlock> blocks,SourceEngineBitstream& stream) {
  return decode(entry,handle,definitions,live,allocator,mask,produced,blocks,stream);
}
} // namespace halo2
