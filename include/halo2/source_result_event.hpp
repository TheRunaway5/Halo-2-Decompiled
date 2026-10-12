#pragma once
#include "halo2/source_result_statistics.hpp"
#include <functional>
#include <optional>

namespace halo2 {
using SourceResultEvent = std::array<std::uint8_t,36>;
using SourceResultEventRing = std::array<SourceResultEvent,1000>;
// Original recording+4/+8 and retained context+374. The ring is the literal
// retained context+4F84 store, independent of its completion byte+1.
struct SourceResultEventState {
  std::uint32_t index{},last{},start{};
  SourceResultEventRing events{};
};
struct SourceResultEventServices {
  // Actual time32 seconds since the Unix epoch, represented as raw32 bits.
  // This API boundary does not substitute a simulation tick for wall time.
  std::function<std::uint32_t()> time;
  // Whole1994D0/19AFD0/19DE80 position service. Caller owns current/previous
  // units and the real object/parent transform. XYZ retains literal float bits.
  std::function<std::optional<std::array<std::uint32_t,3>>(std::uint32_t player)> position;
};
// Whole197120/198C20/19BB20. Both recording gates are checked here again.
void source_result_event_append(SourceResultEventState&,const SourceResultRecording&,
    const SourceResultEvent&);
// Whole1972A0/198DA0/19BCA0: raw player, first stack arg, second stack arg.
// The unsigned last-now comparison and both clock calls are source behavior.
void source_result_event_record(SourceResultEventState&,const SourceResultRecording&,
    std::uint32_t player,std::uint32_t first,std::uint32_t second,const SourceResultEventServices&);
} // namespace halo2
