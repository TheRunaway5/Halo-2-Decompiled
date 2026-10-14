#include "halo2/source_ai_actor.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2 {
namespace {
constexpr auto none = std::uint32_t{0xffffffffu};
}
SourceAiActorConstructorConstants
source_retail_ai_actor_constructor_constants() {
  return {0xffffffffu, {0, 0, 0}, {0x3f800000u, 0, 0}};
}
void source_ai_actor_initialize(
    std::span<std::uint8_t> actor, const SourceAiActorDefinition &definition,
    const SourceAiActorConstructorConstants &constants,
    const SourceAiActorCreateContext &context, std::uint16_t &counter) {
  if (actor.size() != 0x888 ||
      !std::all_of(actor.begin() + 2, actor.end(),
                   [](auto byte) { return byte == 0; }))
    throw std::invalid_argument(
        "Actor constructor requires the original freshly zeroed allocation");
  const auto tick = context.game_tick;
  const auto rate = context.ticks_per_second;
  const auto write = [&](unsigned offset, std::uint32_t value,
                         unsigned size = 4) {
    for (unsigned i = 0; i < size; ++i)
      actor[offset + i] = std::uint8_t(value >> (i * 8));
  };
  // 16B840 already established every zero field. These are all nonzero or
  // supplied writes of 1DFB90, preserving the allocator's current full salt.
  write(0x54, definition.datum);
  write(4, std::uint16_t(definition.type), 2);
  write(7, definition.general_flags & 1, 1);
  write(8, 1, 1);
  write(0x4c, std::uint32_t(tick));
  write(0x229, (definition.general_flags >> 1) & 1, 1);
  for (const auto offset :
       {0x18u,  0x1cu,  0x30u,  0x28u,  0x34u,  0x10u,  0x14u,  0x58u,  0x7cu,
        0x32cu, 0x3f4u, 0x3f8u, 0x858u, 0x85cu, 0x28cu, 0x26cu, 0x274u, 0x3e8u,
        0x5acu, 0x634u, 0x718u, 0x724u, 0x7c8u, 0x7e0u, 0x6c4u, 0x7f8u, 0x338u,
        0x350u, 0x344u, 0x348u, 0x2e8u, 0x2fcu, 0x300u, 0x308u})
    write(offset, none);
  for (const auto offset :
       {0x2cu, 0x3fcu, 0x5b0u, 0x2ecu, 0x2eeu, 0x2f2u, 0x310u, 0x31cu})
    write(offset, none, 2);
  for (unsigned i = 0; i < 8; ++i)
    write(0x5c + i * 4, none);
  for (unsigned i = 0; i < 14; ++i)
    write(0x1e8 + i * 4, none);
  for (unsigned i = 0; i < 3; ++i)
    write(0x1c4 + i * 12, none, 2);
  for (unsigned i = 0; i < 4; ++i)
    write(0x194 + i * 12, none, 2);
  write(0x3e, std::uint16_t(context.bsp_index), 2);
  write(0x418, constants.empty_reference);
  for (unsigned i = 0; i < 4; ++i)
    write(0x402 + i * 6, constants.empty_reference);
  write(0x3fe, 3, 2);
  write(0x84, 3, 2);
  write(0x86, 1, 2);
  // Whole 1A80E0's cold call has data=NULL, current=-1, slot=0 and no
  // start/stop callback for either actual base handler 1 or0x72. Its resulting
  // slot 0 clock/type are retained; untouched slots remain exactly NONE.
  for (unsigned i = 1; i < 4; ++i) {
    write(0x90 + i * 0x40, none, 2);
    write(0x98 + i * 0x40, none);
  }
  write(0x90, definition.general_flags & 1 ? 0x72 : 1, 2);
  write(0x98, std::uint32_t(tick));
  write(0x6fe, 1, 2);
  write(0x714, 1, 1);
  write(0x6f8, 1, 1);
  write(0x710, 0x3f800000u);
  for (unsigned i = 0; i < 3; ++i) {
    write(0x5dc + i * 4, constants.point_bits[i]);
    for (const auto offset : {0x6e0u, 0x6d4u, 0x6ecu})
      write(offset + i * 4, constants.direction_bits[i]);
  }
  write(0x2f4, std::uint16_t(std::int32_t(rate) * 10), 2);
  if (definition.general_flags & 8)
    counter = std::uint16_t(counter + 1u);
}
SourceAiActorBank::SourceAiActorBank() : actors_("actor", 256, 0x888, 0) {}
SourceAiActorBank::SourceAiActorBank(SourceRuntimeMemory &memory, std::uint32_t pool)
    : actors_(memory,pool) {
  if (actors_.capacity() != 256 || actors_.stride() != 0x888)
    throw std::invalid_argument("Actor bank view requires the actual actor pool");
}
SourceAiActorBank::SourceAiActorBank(SourceRuntimeMemory &memory, NativeRuntimeAddress pool)
    : actors_(memory,pool) {
  if (actors_.capacity() != 256 || actors_.stride() != 0x888)
    throw std::invalid_argument("Actor bank view requires the actual actor pool");
}
std::uint32_t
SourceAiActorBank::allocate(const SourceAiActorDefinition &definition,
                            const SourceAiActorConstructorConstants &constants,
                            const SourceAiActorCreateContext &context,
                            std::uint16_t &counter) {
  if (definition.datum == none)
    return none;
  const auto id = actors_.allocate();
  if (id == none)
    return id;
  source_ai_actor_initialize(actors_.lookup(id), definition, constants, context,
                             counter);
  return id;
}
std::span<const std::uint8_t>
SourceAiActorBank::record(std::uint32_t id) const {
  const auto data = actors_.lookup(id);
  if (data.empty())
    throw std::invalid_argument("AI actor datum is not live in its own pool");
  return data;
}
} // namespace halo2
