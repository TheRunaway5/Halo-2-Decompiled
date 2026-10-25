#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
// Defined34-byte1A87F0 event plus the actual dispatch-time source inputs.
// Serial is a native delivery cursor, independent of all gameplay banks.
struct SourceMultiplayerEventPlayerTu15 {
    std::uint32_t datum=0xffffffffu;
    std::uint16_t flags{};
    std::int8_t membership=-1;
    bool operator==(const SourceMultiplayerEventPlayerTu15&)const=default;
};
struct SourceMultiplayerEventTu15 {
    std::uint32_t serial{},tick{},random_seed{};
    std::array<std::uint32_t,8> words{};
    std::uint16_t data{};
    std::uint8_t mode{};
    bool game_over{};
    std::array<SourceMultiplayerEventPlayerTu15,16> players;
    bool operator==(const SourceMultiplayerEventTu15&)const=default;
};
} // namespace halo2
