#pragma once
#include "halo2/mcc_normalized_pointer.hpp"
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
// The canonical clips have a 0x6c stride and shifted event descriptors; their
// resources are reached through the added 20-byte group table. Enumerates
// metadata addresses only, preserving the groups' file-offset descriptors.
std::vector<MccNormalizedPointerField> normalized_mcc_animation_pointer_fields(
    std::span<const std::byte> payload, std::uint32_t source_address);
}
