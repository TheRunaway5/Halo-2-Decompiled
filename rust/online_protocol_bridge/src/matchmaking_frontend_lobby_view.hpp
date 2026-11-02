#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct LobbyViewCategory;
struct LobbyViewParameter;
struct LobbyViewI32;
// Cache records and the native gameplay descriptor getter are observations.
// Rust selects their keys, preserves holes and chooses the displayed values.
class NativeFrontendLobbyViewIo {
public:
  virtual ~NativeFrontendLobbyViewIo() = default;
  virtual LobbyViewCategory lobby_view_category(std::uint32_t) const = 0;
  virtual LobbyViewParameter lobby_view_parameter(std::uint32_t) const = 0;
  virtual LobbyViewI32 lobby_view_value(std::uint32_t) const = 0;
  virtual LobbyViewI32 lobby_view_default(std::uint32_t, std::uint32_t) const = 0;
};
}
