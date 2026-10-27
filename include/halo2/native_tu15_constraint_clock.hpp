#pragma once
#include <cstdint>
#include <functional>
namespace halo2 {
// Exact unsigned64 source conversion used by actual32BBAE/32AE00 after
// KeQuerySystemTime: wrapped subtraction of the1601 epoch, divide10^7,
// then the actual low32-bit seconds return. No date/success is supplied.
std::uint32_t native_tu15_seconds_from_filetime(std::uint64_t filetime) noexcept;
// The world supplies its actual recorded UtcFiletime input, including the
// selected clock source and adjustment. This owner never queries another
// clock, authors a timestamp or consumes a supplied seconds result.
using NativeTu15SystemFiletimeSource=std::function<std::uint64_t()>;
// Proper native owner for the actual37C5C0 wrapper(nullptr).
std::uint32_t native_tu15_constraint_clock_seconds(const NativeTu15SystemFiletimeSource&);
} // namespace halo2
