#pragma once
#include "halo2/source_king_replication.hpp"

namespace halo2 {
// Original52-byte stream state. data_address is retained source metadata;
// native access uses the separately supplied backing span. Word operations
// require explicit padding beyond capacity, as the original owners do.
struct SourceEngineBitstreamState {
  std::uint32_t data_address{};
  std::int32_t capacity_bytes{};
  std::uint32_t unknown_08{};
  std::int32_t mode{},position{};
  std::uint32_t error_and_padding{};
  std::int32_t checkpoint_count{};
  std::array<std::int32_t,4> checkpoints{};
  std::int32_t discarded_bits{},discard_count{};
};
static_assert(sizeof(SourceEngineBitstreamState)==0x34);
static_assert(offsetof(SourceEngineBitstreamState,position)==0x10);
static_assert(offsetof(SourceEngineBitstreamState,checkpoint_count)==0x18);
struct SourceEngineBitstream {
  SourceEngineBitstreamState& state;
  std::span<std::uint8_t> backing;
  SourceEngineBitstream(SourceEngineBitstreamState&,std::span<std::uint8_t>);
  bool overflowed()const noexcept;
};
// Shared A4AB0/A4E20 and King9A3A0/9A530 in retail, with complete TU1.5
// counterparts AECC0/AF030 and A43D0/A4560. Capacity rollback and partial
// validation are literal original contracts. Success alone is not acceptance:
// readers can return true with an overflowed stream and partial bank writes.
bool source_engine_wire_write(SourceEngineBitstream&,std::uint32_t requested,
    std::uint32_t& written,const SourceEngineReplicationBank&,std::int32_t reserved_bits);
bool source_engine_wire_read(SourceEngineBitstream&,SourceEngineReplicationBank&,std::uint32_t& read);
bool source_king_wire_write(SourceEngineBitstream&,std::uint32_t requested,
    std::uint32_t& written,const SourceKingReplicationBank&,std::int32_t reserved_bits);
bool source_king_wire_read(SourceEngineBitstream&,SourceKingReplicationBank&,std::uint32_t& read);
// These original scheduling hints are minima over requested fields. They are
// not the encoded length or sufficient packet-buffer allocations.
std::int32_t source_engine_wire_minimum_bits(std::uint32_t requested) noexcept;
std::int32_t source_king_wire_minimum_bits(std::uint32_t requested) noexcept;
} // namespace halo2
