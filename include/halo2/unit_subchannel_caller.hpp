#pragma once
#include "halo2/unit_channel_request.hpp"
#include "halo2/unit_constructors.hpp"
namespace halo2 {
struct SourceUnitSubchannelCall {
  std::uint32_t instruction, unit;
  std::size_t manager_offset, channel_offset;
  std::uint32_t weapon_class, weapon;
  bool returning, result;
  friend bool operator==(const SourceUnitSubchannelCall &,
                         const SourceUnitSubchannelCall &) = default;
};
using SourceUnitSubchannelObserver =
    std::function<void(const SourceUnitSubchannelCall &)>;
// Complete114F00. Actual raw header+8/body, signed +12A/+33E and retained
// component storage are required. No salt predicate is added to its low16
// lookup. Lower callbacks must retain body spans through the three calls.
// Return value is ignored by its source callers; no fabricated AL is exposed.
void update_source_unit_subchannels(
    std::uint32_t unit, const UnitConstructorObjectPool &,
    const SourceUnitChannelRequestServices &,
    const SourceUnitSubchannelObserver & = {},
    const UnitAnimationSelectionObserver &body_stores = {});
} // namespace halo2
