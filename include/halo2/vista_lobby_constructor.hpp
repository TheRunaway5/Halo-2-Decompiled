#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
inline constexpr std::size_t vista_lobby_constructor_bank_size = 0x3594;

struct VistaLobbyConstructorArguments {
  std::uint32_t argument1{};
  std::uint32_t argument2{};
  std::uint32_t controller_mask{}; // Original DWORD argument; widgets consume its low WORD.
};

struct VistaLobbyConstructorGlobals {
  std::uint32_t chat_owner_count{}; // Actual D78388; the chat constructor increments it.
  std::uint32_t last_screen_id{}; // Actual D758D8; base screen construction writes 14.
  std::array<std::byte, 8> screen_bounds{}; // Raw actual E3E418 rectangle, not inferred dimensions.
};

// Complete original 6463AB constructor and its reached constructor helpers.
// The bank contains literal little-endian, 32-bit source identities. Vtables,
// callback function IDs and source_base + offsets are metadata, never native
// pointers or callable functions. Allocation, 643FF9 initialization, widget
// registration and 612604 activation are separate owners.
//
// read_cached_ui_clock corresponds to 6096AE's D71900 load. It is called nine
// times in source order and must peek the retained clock, not advance time.
// Only source-written bytes are changed; caller-provided padding is retained.
std::uint32_t vista_lobby_construct(std::span<std::byte> bank,
    std::uint32_t source_base, const VistaLobbyConstructorArguments&,
    VistaLobbyConstructorGlobals&,
    const std::function<std::uint32_t()>& read_cached_ui_clock);
} // namespace halo2
