#pragma once
#include <compare>
#include <cstdint>
#include <type_traits>

namespace halo2 {
// Engine address, never a serialized cache pointer or salted tag handle.
// Deliberately has no conversion to a 32-bit integer. Original-code adapters
// must resolve an address through its owner instead of truncating a pointer.
struct NativeTagAddress {
    std::uint64_t value{};
    constexpr explicit NativeTagAddress(std::uint64_t address = 0) : value(address) {}
    constexpr bool is_null() const { return value == 0; }
    auto operator<=>(const NativeTagAddress&) const = default;
};
static_assert(sizeof(NativeTagAddress) == 8);
static_assert(!std::is_convertible_v<NativeTagAddress,std::uint32_t>);
} // namespace halo2
