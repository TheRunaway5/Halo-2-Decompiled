#pragma once
#include "halo2/object_construction.hpp"
#include "halo2/object_creation_request.hpp"
#include <optional>

namespace halo2 {
struct SourceObjectConstructionTables;
struct SourceTagMetadataProviders;
struct OriginalData;
struct SourceScenarioRequestOperands { std::uint32_t zero_bits, one_bits, color_scale_bits; };
struct SourceScenarioRequestInput {
  std::uint32_t request, placement, palette;
  std::int32_t type, ordinal;
  std::uint8_t force;
};
struct SourceScenarioRequestServices {
  std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  std::function<std::uint32_t(std::int32_t)> type_address;
  std::function<std::uint16_t(std::int32_t)> placement_extent;
  // Read the actual current game-options pointer only if type6 reaches it.
  std::function<std::uint32_t()> game_options;
  std::function<std::uint32_t(const ObjectConstructionCall &,
                             ObjectConstructionFloatingPointContext &)> invoke;
};
// Complete D6660 direct owner, including writes on definition-NONE failure.
// Every reached lower owner is mandatory; an absent owner retains its prefix
// and throws. Only the defined AL return is represented by bool.
bool source_scenario_creation_request(const SourceScenarioRequestInput &,
    const SourceScenarioRequestOperands &,ObjectConstructionFloatingPointContext &,
    const SourceScenarioRequestServices &,const ObjectConstructionObserver & = {});
// Complete raw D7070 policy, including signed bytes and masked x86 shifts.
std::uint32_t source_scenario_request_policy(std::uint32_t placement,
    const SourceScenarioRequestServices &);
// Bind B8820,11F680,D7070,133320 for request bytes in the nearest domain.
// Read/memory/type/game owners remain those supplied by the caller. The
// initializer's owner is the literal NONE from D6660, so no pool is read.
SourceScenarioRequestServices source_scenario_request_byte_owners(
    SourceScenarioRequestServices,const ObjectCreationRequestOperands &,
    const SourceScenarioRequestOperands &,std::uint16_t x87_control,
    const ObjectConstructionObserver & = {});

struct SourceScenarioGameOptions { std::uint32_t game_type, engine; };
struct SourceScenarioRequestResult {
  bool admitted;
  std::array<std::byte,0xc4> bytes;
  ObjectConstructionFloatingPointContext comparison;
};
// Actual retained cache placement and palette bytes. The caller selects the
// current scnr datum/type/ordinal and supplies the prior request contents.
// This binding produces request bytes in the verified nearest math domain:
// x87 CW027F and SSE controls1F80. Callee arithmetic exception/status flags
// and non-nearest Euler behavior are outside this binding's contract. The
// COMISS state is returned explicitly. No constructor admission is supplied.
SourceScenarioRequestResult source_cache_scenario_creation_request(
    const SourceTagMetadataProviders &,const OriginalData &,std::uint32_t scenario,
    std::int32_t type,std::int32_t ordinal,std::uint8_t force,
    const std::array<std::byte,0xc4> &prior,std::optional<SourceScenarioGameOptions>,
    std::uint16_t x87_control,ObjectConstructionFloatingPointContext,
    const ObjectConstructionObserver & = {});
} // namespace halo2
