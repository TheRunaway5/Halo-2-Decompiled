#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
// C8140's exact byte consumer. The caller owns actual source unit storage and
// a separate, marshalled 0x7C-byte input; this does not generate either producer.
void apply_source_unit_input(std::span<std::byte> unit,
    std::span<const std::byte,0x7c> input);
// CEA70 scans the four full-datum slots in ascending order. AX is the defined
// result; a full inventory returns -1. This does not admit or transfer an item.
std::int16_t source_inventory_empty_slot(std::span<const std::byte> unit);
} // namespace halo2
