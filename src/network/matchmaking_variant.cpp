#include "halo2/matchmaking_variant.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"

namespace halo2 {
bool matchmaking_variant_valid(std::span<const std::uint8_t> settings) {
  return online_protocol_bridge::matchmaking_variant_valid({settings.data(),settings.size()});
}
}
