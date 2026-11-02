#pragma once
#include <cstdint>
#include <span>

namespace halo2 {
// Shared Rust admission only. Invalid settings are never normalized into a
// different game; native gameplay retains its separate reset operation.
bool matchmaking_variant_valid(std::span<const std::uint8_t>);
}
