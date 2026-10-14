#include "halo2/source_ai_actor.hpp"
#include "halo2/source_unit_control.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read(std::span<const std::uint8_t> bytes, unsigned offset) {
  std::uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i)
    value |= std::uint32_t(bytes[offset+i]) << (8*i);
  return value;
}
void write(std::span<std::uint8_t> bytes, unsigned offset, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i)
    bytes[offset+i] = std::uint8_t(value >> (8*i));
}
}
void SourceAiActorBank::bind_unit(
    std::uint32_t id, std::uint32_t unit_id,
    const SourceAiActorBindingServices &services) {
  if (!services.release_actor)
    throw std::invalid_argument("AI binding requires the real actor release owner");
  const auto actor = actors_.lookup(id);
  if (actor.empty())
    throw std::invalid_argument("AI binding actor is not live in its source pool");
  const auto unit = services.units.object(unit_id, 0x130);
  const auto previous_actor = read(unit, 0x12c);
  // Preserve the original early-out even if the two records disagree about
  // their relationship. This routine does not repair unrelated ownership.
  if (previous_actor == id)
    return;
  if (previous_actor != none)
    services.release_actor(previous_actor, false);
  if (read(actor, 0x18) != none)
    unbind_unit(id, services);
  write(actor, 0x18, unit_id);
  write(unit, 0x12c, id);
  services.units.set_active(unit_id, true);
}
void SourceAiActorBank::unbind_unit(
    std::uint32_t id, const SourceAiActorBindingServices &services) {
  if (!services.release_actor)
    throw std::invalid_argument("AI binding requires the real actor release owner");
  const auto actor = actors_.lookup(id);
  if (actor.empty())
    throw std::invalid_argument("AI binding actor is not live in its source pool");
  const auto previous_unit = read(actor, 0x18);
  if (previous_unit == none)
    return;
  const auto unit = services.units.object(previous_unit, 0x130);
  write(unit, 0x12c, none);
  write(actor, 0x18, none);
  services.units.set_active(previous_unit, false);
}
} // namespace halo2
