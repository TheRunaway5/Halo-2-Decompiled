#include "halo2/item_motion.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using Vector = std::array<float, 3>;
constexpr std::uint32_t none = 0xffffffffu;
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
std::uint32_t word(std::span<const std::byte> data, std::size_t at, unsigned count = 4) {
  require(at <= data.size() && count <= data.size() - at, "Truncated source item-motion record");
  std::uint32_t result = 0;
  for (unsigned i = 0; i < count; ++i)
    result |= std::uint32_t(std::to_integer<unsigned>(data[at + i])) << (8 * i);
  return result;
}
Vector vector(std::span<const std::byte> data, std::size_t at) {
  return {std::bit_cast<float>(word(data, at)), std::bit_cast<float>(word(data, at + 4)),
          std::bit_cast<float>(word(data, at + 8))};
}
std::span<const std::byte, 12> raw(const Vector &value) {
  return std::span<const std::byte, 12>(std::as_bytes(std::span(value)));
}
void store(SourceItemMotionStorage storage, std::size_t at, std::uint32_t value,
           unsigned count, std::uint32_t instruction, const SourceItemMotionObserver &observe) {
  word(storage.body, at, count);
  SourceItemMotionStore event{instruction, at, {}, {}};
  event.before.assign(storage.body.begin() + at, storage.body.begin() + at + count);
  for (unsigned i = 0; i < count; ++i) storage.body[at + i] = std::byte(value >> (8 * i));
  event.after.assign(storage.body.begin() + at, storage.body.begin() + at + count);
  if (observe) observe(event);
}
void gate_orientation(SourceItemMotionStorage storage) {
  // F99A0's three measured unchanged early returns. The alternate orientation
  // producer cannot be replaced by a successful no-op.
  if (word(storage.body, 0x14) != none || (word(storage.body, 4) & 0x80)) return;
  require(!(word(storage.definition, 0xbc, 1) & 1),
          "Original item orientation producer is unavailable");
}
void velocity(SourceItemMotionStorage storage, const Vector &linear, const Vector &angular,
              const SourceItemMotionServices &services, const SourceItemMotionObserver &observe) {
  // B8630 copies raw words in original order, including angular signed zero.
  for (unsigned i = 0; i < 3; ++i)
    store(storage, 0x88 + i * 4, std::bit_cast<std::uint32_t>(linear[i]), 4,
          std::array{0xb8659u, 0xb865eu, 0xb8664u}[i], observe);
  for (unsigned i = 0; i < 3; ++i)
    store(storage, 0x94 + i * 4, std::bit_cast<std::uint32_t>(angular[i]), 4,
          std::array{0xb8679u, 0xb867eu, 0xb8684u}[i], observe);
  const auto entity = word(storage.body, 0xd4);
  if (entity != none) {
    require(bool(services.network_dirty), "Original item network velocity producer is unavailable");
    services.network_dirty(entity, 0x30);
  }
  // 1C8430: the actual physical-component NONE gate, not a successful solve.
  require(word(storage.body, 0xb4) == none, "Original physical item velocity producer is unavailable");
}
void angular_zero(SourceItemMotionStorage storage, const SourceItemMotionObserver &observe) {
  const auto angular = vector(storage.body, 0x94);
  require(angular[0] == 0.f && angular[1] == 0.f && angular[2] == 0.f,
          "Original nonzero item angular-state producer is unavailable");
  store(storage, 0x12c, word(storage.body, 0x12c, 1) & 0xef, 1, 0xfbbbdu, observe);
  store(storage, 0x164, 0, 4, 0xfbbc7u, observe);
  store(storage, 0x168, 0x3f800000u, 4, 0xfbbcfu, observe);
}
void make_resting(SourceItemMotionStorage storage, const SourceItemMotionObserver &observe) {
  const auto type_mask = std::uint32_t(1) << (storage.object_type & 31);
  require(!(type_mask & 0x1883u) || !(word(storage.body, 0xc0, 1) & 0x40u) ||
              word(storage.body, 0xb4) == none,
          "Original physical item resting producer is unavailable");
  store(storage, 0xc1, word(storage.body, 0xc1, 1) | 1, 1, 0xbab16u, observe);
  if (type_mask & 0x3c)
    require(word(storage.body, 0xd4) == none, "Original item resting network producer is unavailable");
}
} // namespace

void source_item_motion(SourceItemMotionStorage storage, const SourceItemMotionInputs &input,
    const SourceItemMotionServices &services, const SourceItemMotionObserver &observe) {
  const auto flags = word(storage.body, 4);
  if (!(flags & 0x100)) return; // F9EEB→FAEDB, listed-only alternate exit.
  if (!(flags & 0x20000000u) && storage.datum != none && word(storage.body, 0x112, 2) != 0xffff)
    store(storage, 4, flags | 0x20000000u, 4, 0xf9f0fu, observe);
  gate_orientation(storage);
  if (!(word(storage.body, 0xc1, 1) & 1)) {
    // 10DB90's measured AL0 branch leaves both original scratch packets intact.
    require(!(word(storage.body, 0xc0, 1) & 4), "Original queued item velocity producer is unavailable");
    const auto root = vector(storage.body, 0x64);
    auto linear = vector(storage.body, 0x88);
    auto angular = vector(storage.body, 0x94);
    const bool definition_motion = (word(storage.definition, 0xbc) & 4) != 0;
    if (!definition_motion) {
      const float gravity_step = input.gravity * input.seconds_per_tick;
      linear[2] = linear[2] - gravity_step;
    }
    Vector endpoint, delta;
    for (unsigned i = 0; i < 3; ++i) {
      const float step = linear[i] * input.seconds_per_tick;
      endpoint[i] = step + root[i];
      delta[i] = endpoint[i] - root[i];
    }
    require(bool(services.cast), "Original item terrain/world query producer is unavailable");
    const auto ignored = word(storage.body, 0x14c);
    const bool hit = services.cast(0x24909c0du, raw(root), raw(delta), ignored, none,
                                  input.collision_scratch);
    bool angular_changed = false;
    if (hit) {
      auto contact = input.collision_scratch;
      auto normal = vector(contact, 0x28);
      const float x = linear[0] * normal[0], y = linear[1] * normal[1];
      const float xy = y + x, z = normal[2] * linear[2];
      const float dot = xy + z;
      const float opposed = 0.f - dot;
      const bool floor = normal[2] > input.floor_normal_limit;
      const bool slow = input.settling_speed_limit > opposed;
      Vector biased;
      auto point = vector(contact, 8);
      for (unsigned i = 0; i < 3; ++i) {
        const float bias = normal[i] * input.contact_point_bias;
        biased[i] = bias + endpoint[i];
      }
      require(bool(services.contact_effect), "Original item contact-effect producer is unavailable");
      services.contact_effect(storage.datum, raw(linear), contact, raw(biased));
      const auto kind = word(contact, 0);
      const auto contact_flags = word(contact, 0x58, 1);
      // Executed FA1C7→FA2BE: floor and small opposed speed, no existing
      // contact20. Other impacts require their original bounce/contestants.
      require((kind == 1 || kind == 3) && floor && slow && !(contact_flags & 0x20),
              "Original alternate item contact response is unavailable");
      store(storage, 0x13a, 0, 1, 0xfa2f3u, observe);
      require(bool(services.contact_transform), "Original item contact transform producer is unavailable");
      // FBBE0 consumes the cast contact point/normal and writes a separate
      // adjusted point. The older oracle's EAX matrix-window label is not ABI.
      services.contact_transform(storage.datum,
          std::span<const std::byte, 12>(contact.data() + 8, 12),
          std::span<const std::byte, 12>(contact.data() + 0x28, 12),
          std::span<std::byte, 12>(reinterpret_cast<std::byte *>(point.data()), 12));
      normal = vector(contact, 0x28);
      const float ay = angular[1] * normal[1], ax = angular[0] * normal[0];
      const float axy = ay + ax, az = angular[2] * normal[2];
      const float projection = axy + az;
      linear = input.resting_linear;
      store(storage, 0x158, word(contact, 0x28), 4, 0xfa372u, observe);
      store(storage, 0x15c, word(contact, 0x2c), 4, 0xfa382u, observe);
      for (unsigned i = 0; i < 3; ++i) angular[i] = normal[i] * projection;
      store(storage, 0x160, word(contact, 0x30), 4, 0xfa3a7u, observe);
      make_resting(storage, observe);
      store(storage, 0x12c, word(storage.body, 0x12c, 1) | 0x20, 1, 0xfa410u, observe);
      store(storage, 0x12c, word(storage.body, 0x12c, 1) & 0xbf, 1, 0xfa417u, observe);
      store(storage, 0x130, input.contact_bank, 2, 0xfa435u, observe);
      store(storage, 0x132, word(contact, 0x3c, 2), 2, 0xfa443u, observe);
      store(storage, 0x134, word(contact, 0x50, 2), 2, 0xfa451u, observe);
      store(storage, 0x136, word(contact, 0x58, 1), 1, 0xfa458u, observe);
      store(storage, 0x137, word(contact, 0x59, 1), 1, 0xfa45eu, observe);
      store(storage, 0x14c, none, 4, 0xfa7c9u, observe);
      angular_changed = true;
      require(!definition_motion, "Original alternate post-contact root producer is unavailable");
    } else {
      require(bool(services.root_transform), "Original moving item root transform producer is unavailable");
      services.root_transform(storage.datum, raw(endpoint),
          std::span<const std::byte, 8>(input.collision_scratch.data() + 0x1c, 8));
    }
    velocity(storage, linear, angular, services, observe);
    if (angular_changed) angular_zero(storage, observe);
    require(!(word(storage.body, 4) & 0x4000), "Original item lifespan producer is unavailable");
  }
  require(!(word(storage.body, 0x12c, 1) & 0x10), "Original rotating item pose producer is unavailable");
}

bool source_item_update(SourceItemMotionStorage storage, const SourceItemMotionInputs &input,
    const SourceItemMotionServices &services, const SourceItemMotionObserver &observe) {
  require((word(storage.body, 4) & 0x100) != 0 && word(storage.body, 0x14) == none,
          "Original parented/held item update path is unavailable");
  const auto item_flags = word(storage.body, 0x12c, 1);
  require(!(item_flags & 0x70), "Original item impulse/contact/rotation update producer is unavailable");
  const auto timer = word(storage.body, 0x12e, 2);
  require(!(timer > 0 && timer < 0x8000), "Original item timer/effect producer is unavailable");
  bool changed = false;
  if (!(word(storage.body, 0xc1, 1) & 1)) {
    require(bool(services.make_moving), "Original item moving-state producer is unavailable");
    services.make_moving(storage.datum); // BC910, including its owning B8250.
    changed = true;
  }
  require(word(storage.body, 0xc8) == none || std::bit_cast<std::int32_t>(word(storage.definition, 0x104)) <= 0,
          "Original item looping-sound producer is unavailable");
  source_item_motion(storage, input, services, observe);
  return changed;
}
} // namespace halo2
