#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct PartyAppearanceData;
struct PartySeatData;
struct PartyAuthData;
struct NativeFrontendPartyIo {
  virtual ~NativeFrontendPartyIo()=default;
  virtual PartyAppearanceData party_primary_appearance() const=0;
  virtual rust::Vec<PartySeatData> party_local_players() const=0;
  virtual PartyAuthData party_authorization(std::uint32_t) const=0;
};
} // namespace halo2::online_protocol_bridge
