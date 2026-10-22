#include "halo2/rumble.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
RumbleDefinition load_rumble_definition(const Cache &cache,
                                        std::uint32_t datum) {
  const auto source = cache.load_source_tag_metadata(datum);
  if (source.group() != "jpt!")
    throw FormatError("Rumble requires an authored damage effect");
  const Bytes root(source.bytes_at(source.source_address(), 0x74));
  const auto count = root.u32(0x6c);
  if (count > 256)
    throw FormatError("Excessive rumble effect block");
  RumbleDefinition result;
  if (!count)
    return result;
  // Retail caches intern identical response blocks and curves across damage
  // tags. Their literal pointers belong to the cache metadata allocation,
  // and can precede this tag's own payload (e.g. Midship plasma responses).
  const Bytes rows(cache.read_tag_bytes(root.u32(0x70), count * 0x4c));
  for (unsigned i = 0; i < count; ++i) {
    RumbleEffect effect;
    effect.type = std::int16_t(rows.u16(i * 0x4c));
    for (unsigned m = 0; m < 2; ++m) {
      const auto at = i * 0x4c + 0x24 + m * 12;
      auto &motor = effect.motors[m];
      motor.duration = rows.f32(at);
      if (!std::isfinite(motor.duration) || motor.duration < 0)
        throw FormatError("Invalid rumble motor duration");
      const auto size = rows.u32(at + 4);
      if (size > 4096)
        throw FormatError("Excessive rumble curve");
      if (size) {
        const auto bytes = cache.read_tag_bytes(rows.u32(at + 8), size);
        motor.curve.assign(bytes.begin(), bytes.end());
      }
    }
    result.push_back(std::move(effect));
  }
  return result;
}
int send_controller_rumble(SDL_GameController *pad, RumbleOutput motors,
                           bool enabled, bool paused, bool focused) {
  if (!pad)
    return 0;
  const bool active = enabled && !paused && focused;
  return SDL_GameControllerRumble(pad, active ? motors.left : 0,
                                  active ? motors.right : 0, active ? 100 : 0);
}
void RumblePlayer::play(std::shared_ptr<const RumbleDefinition> definition,
                        unsigned effect, float scale) {
  if (!definition || effect >= definition->size() || !std::isfinite(scale))
    throw FormatError("Invalid rumble request");
  unsigned oldest = 0;
  for (unsigned i = 1; i < 8; ++i)
    if (slots_[i].time > slots_[oldest].time)
      oldest = i;
  slots_[oldest] = {std::move(definition), effect, 0, scale};
}
void RumblePlayer::play(std::shared_ptr<const RumbleDefinition> definition,
                        bool shielded, float scale) {
  if (!definition)
    return;
  // 153D10 chooses the first row whose type differs from unit shield
  // vitality>0.
  for (unsigned i = 0; i < definition->size(); ++i)
    if ((*definition)[i].type != int(shielded)) {
      play(std::move(definition), i, scale);
      return;
    }
}
RumbleOutput RumblePlayer::evaluate(const ShaderFunctionTables &tables) const {
  std::array<float, 2> motors{};
  for (const auto &slot : slots_)
    if (slot.definition)
      for (unsigned m = 0; m < 2; ++m) {
        const auto &motor = slot.definition->at(slot.effect).motors[m];
        if (motor.duration > slot.time)
          motors[m] +=
              shader_function_value(
                  motor.curve, std::clamp(slot.time / motor.duration, 0.f, 1.f),
                  1.f, tables) *
              slot.scale;
      }
  const auto word = [](float x) {
    return std::uint16_t(std::nearbyint(std::clamp(x * 65535.f, 0.f, 65535.f)));
  };
  return {word(motors[0]), word(motors[1])};
}
void RumblePlayer::advance(float seconds) {
  if (!std::isfinite(seconds) || seconds < 0)
    throw FormatError("Invalid rumble clock");
  for (auto &slot : slots_)
    slot.time += seconds;
}
void RumblePlayer::clear() { slots_ = {}; }
void validate_rumble_journal(std::span<const RumbleJournalEvent> rows,
                             std::uint64_t tick) {
  if (rows.size() > 64)
    throw FormatError("Rumble journal exceeds retention");
  std::uint64_t previous = 0;
  for (const auto &row : rows) {
    if (!row.serial || row.serial <= previous || row.tick > tick ||
        row.event.definition == 0xffffffffu || row.event.owner < -1 ||
        row.event.owner >= 15 || !std::isfinite(row.event.scale) ||
        row.event.scale < 0 || !std::isfinite(row.event.flash_scale) || row.event.flash_scale<0)
      throw FormatError("Invalid native rumble journal event");
    previous = row.serial;
  }
}
} // namespace halo2
