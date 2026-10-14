#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
// Normalize owned PC/MCC jmad layout for the existing Xbox graph consumers.
// This preserves source-authored clips, events, mode trees and codec bytes;
// it does not assert that MCC's authored animation set equals Xbox TU1.5.
// file_offset is the exact offset where the result will live in Cache storage.
std::vector<std::byte> normalize_mcc_animation_graph(
    std::span<const std::byte> payload,std::uint32_t source_address,
    std::uint32_t target_address,std::uint32_t file_offset,
    const std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>& resident_read={});
}
