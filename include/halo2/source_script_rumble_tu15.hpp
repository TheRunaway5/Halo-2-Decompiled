#pragma once
#include "halo2/cache.hpp"
#include "halo2/physics_contact_record.hpp"
#include <bit>

namespace halo2 {
// The checked writer is the owning world's write_source_call_input boundary.
// Exact TU1.5 2B4A69..2B4A83 stores; no feedback event or hardware result is
// synthesized. The normal player-effect update consumes this retained state.
inline void source_script_max_rumble_tu15(const SourcePhysicsContactMemory &memory,
    const std::function<void(std::uint32_t,std::span<const std::byte>)> &write,
    float left,float right) {
  if(!write)throw FormatError("Script rumble requires its checked source writer");
  std::array<std::byte,4> bytes{};memory.read(0x50ebf8u,bytes);
  std::uint32_t owner{};
  for(unsigned n=0;n<4;++n)owner|=std::uint32_t(std::to_integer<unsigned>(bytes[n]))<<(n*8u);
  if(!owner || owner>UINT32_MAX-0x227u)
    throw FormatError("Script rumble requires its original player-effects owner");
  std::array<std::byte,8> payload{};
  // Admit the whole destination before the single journaled publication.
  memory.read(owner+0x220u,payload);
  const std::array words{std::bit_cast<std::uint32_t>(left),std::bit_cast<std::uint32_t>(right)};
  for(unsigned n=0;n<8;++n)payload[n]=std::byte(words[n/4u]>>((n%4u)*8u));
  write(owner+0x220u,payload);
}
}
