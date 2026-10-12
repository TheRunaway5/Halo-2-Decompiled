#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace halo2 {
// Literal24-byte3BAB0 selection record, distinct from a3B9D0 command.
// Unused fields and source token/pointer words move with the complete record.
struct SourceOpaqueSortRecord {std::array<std::uint32_t,6> words{};};
static_assert(sizeof(SourceOpaqueSortRecord)==24);
// Literal3BB80 unsigned lexicographic greater predicate. Layer5 uses a
// different comparator and must not be routed through this entry.
bool source_opaque_compare(const SourceOpaqueSortRecord&,const SourceOpaqueSortRecord&);
// Original13DA70/13DCD0 in-place24-byte sort, including equal-key swaps.
void source_opaque_sort(std::span<SourceOpaqueSortRecord>);
}
