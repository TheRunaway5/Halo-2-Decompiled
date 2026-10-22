#include "halo2/player_command.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> bytes, std::size_t offset,
                   unsigned count = 4) {
  std::uint32_t result = 0;
  for (unsigned i = 0; i < count; ++i)
    result |= std::uint32_t(std::to_integer<unsigned>(bytes[offset + i])) << (i * 8);
  return result;
}
float scalar(std::span<const std::byte> bytes, std::size_t offset) {
  return std::bit_cast<float>(word(bytes, offset));
}
bool finite_bits(std::span<const std::byte> bytes, std::size_t offset) {
  return (word(bytes, offset) & 0x7f800000u) != 0x7f800000u;
}
bool signed_selection(std::uint32_t bits, unsigned width, int limit) {
  const auto none = width == 1 ? 0xffu : 0xffffu;
  const auto negative = width == 1 ? 0x80u : 0x8000u;
  return bits == none || (!(bits & negative) && bits < unsigned(limit));
}
using Vector = std::array<float,3>;
// Each scalar operation owns an original SSE spill. Preserve its order and
// signed zero even when this translation unit is built outside the main target.
float mul(float a, float b) { volatile float r = a * b; return r; }
float add(float a, float b) { volatile float r = a + b; return r; }
float sub(float a, float b) { volatile float r = a - b; return r; }
float source_constant(std::uint32_t bits) { return std::bit_cast<float>(bits); }
float normalize(Vector &v) {
  const double x = double(v[0]) * double(v[0]);
  const double y = double(v[1]) * double(v[1]);
  const double z = double(v[2]) * double(v[2]);
  const double xy = x + y;
  const double length = std::sqrt(xy + z);
  const float spill = static_cast<float>(length);
  if (double(source_constant(0x38d1b717u)) > std::abs(length)) return 0.f;
  const float inverse = 1.f / spill;
  for (auto &c : v) c = mul(c, inverse);
  return spill;
}
struct Basis { Vector forward, side; };
Basis flat_basis(Vector aim, Vector fallback) {
  aim[2] = 0.f;
  if (normalize(aim) == 0.f) {
    fallback[2] = 0.f;
    aim = fallback;
    if (normalize(aim) == 0.f) aim = {1.f,0.f,0.f}; // Original 46FC48 pointee.
  }
  return {aim, {sub(0.f,aim[1]),aim[0],0.f}};
}
Vector selected_facing(Vector movement, const Vector &fallback, Vector aim) {
  movement[2] = 0.f; // E7B10 is called with its first flag zero.
  if (normalize(movement) == 0.f) {
    // 1218C0 owns its own fallback copy; a zero/short flat look is replaced
    // with the unflattened current-unit forward, without a second normalize.
    aim[2] = 0.f;
    if (normalize(aim) == 0.f) aim = fallback;
    return aim;
  }
  const Vector side{sub(0.f,movement[1]),movement[0],0.f};
  const float fallback_dot = add(mul(fallback[1],movement[1]),
                                 mul(fallback[0],movement[0]));
  const float forward_dot = add(mul(aim[0],movement[0]),mul(aim[1],movement[1]));
  const float side_dot = add(mul(aim[0],side[0]),mul(aim[1],side[1]));
  const float up_dot = add(add(mul(1.f,aim[2]),mul(0.f,aim[1])),mul(aim[0],0.f));
  // E7D84 compares threshold .5 against abs(fallback_dot), then JBE.
  const float threshold = .5f > std::abs(fallback_dot)
      ? source_constant(0x3ecccccdu) : source_constant(0x3e99999au);
  const bool forward = std::abs(forward_dot) > threshold ||
                       std::abs(up_dot) > source_constant(0x3f666666u);
  Vector chosen = forward ? movement : side;
  const float sign = forward ? forward_dot : side_dot;
  // Original COMISS/JB includes unordered in the negative branch.
  if (!(sign >= 0.f)) for (auto &c : chosen) c = sub(0.f,c);
  return chosen;
}
void put_word(std::span<std::byte> bytes, std::size_t offset,
              std::uint32_t bits, unsigned count = 4) {
  for (unsigned i=0; i<count; ++i) bytes[offset+i] = std::byte((bits>>(i*8))&255u);
}
void put_vector(std::span<std::byte> bytes, std::size_t offset, const Vector &v) {
  for (unsigned i=0; i<3; ++i) put_word(bytes,offset+i*4,std::bit_cast<std::uint32_t>(v[i]));
}
} // namespace

void initialize_source_neutral_command(std::span<std::byte, 0x5c> command) {
  std::fill(command.begin(), command.end(), std::byte{0}); // 14E78E, 17 dwords.
  for (const auto offset : {0x1e, 0x22, 0x24}) {
    command[offset] = std::byte{0xff};
    command[offset + 1] = std::byte{0xff};
  }
  command[0x20] = command[0x21] = std::byte{0xff};
  for (const auto offset : {0x34, 0x38, 0x3c, 0x2c, 0x30})
    for (unsigned i = 0; i < 4; ++i) command[offset + i] = std::byte{0xff};
  // Repeated source zero stores, including the command+28 subrecord's type0,
  // preserve the same literal final buffer; no native field meaning is added.
}

bool source_player_command_valid(std::span<const std::byte, 0x5c> command) {
  // 14E7E0 rejects exponent-all-ones before its ordered scalar comparisons.
  for (const auto offset : {8, 4, 0x14, 0x18})
    if (!finite_bits(command, offset)) return false;
  const auto first = scalar(command, 0x14);
  if (first < 0.f || first > 1.f) return false; // Original 465030 is 1.0.
  for (const auto offset : {0xc, 0x10})
    if (!finite_bits(command, offset)) return false;
  for (const auto offset : {0xc, 0x10}) {
    const auto value = scalar(command, offset);
    if (value < -1.f || value > 1.f) return false; // Original 46503C is -1.0.
  }
  const auto slot0 = word(command, 0x20, 1), slot1 = word(command, 0x21, 1);
  if (!signed_selection(slot0, 1, 4) || !signed_selection(slot1, 1, 4) ||
      (slot0 != 0xffu && slot0 == slot1)) return false;
  if (!signed_selection(word(command, 0x22, 2), 2, 2) ||
      !signed_selection(word(command, 0x24, 2), 2, 4) ||
      (word(command, 0x1d, 1) & 0xfeu)) return false;
  const auto a = scalar(command, 0x50), b = scalar(command, 0x54);
  // COMISS/JB rejects unordered values in these range tests. UCOMISS followed
  // by LAHF/TEST44/JP treats unordered or nonzero as the reference branch.
  const bool range = a >= 0.f && a <= 1.f && b >= 0.f && b <= 1.f;
  if (a == 0.f && b == 0.f && !(word(command, 0x4c, 1) & 1u)) {
    if (!range) return false;
  } else if (!range || word(command, 0x34) == 0xffffffffu) return false;
  const auto kind = word(command, 0x28, 2);
  if ((kind & 0x8000u) || kind >= 9) return false;
  return kind == 0 || word(command, 0x2c) != 0xffffffffu;
}

bool deliver_source_cached_command(std::span<const std::byte, 0x88> record,
    std::span<std::byte, 0x5c> output, std::uint32_t tick,
    float seconds_per_tick, float expiration) {
  const auto kind = std::bit_cast<std::int32_t>(word(record, 8));
  if (kind == 4 || kind == 5)
    throw std::runtime_error("Original command watcher refresh producer is unavailable");
  const auto recorded_tick = word(record, 0x28);
  if (kind == 2 || recorded_tick == 0xffffffffu) return false;
  // The source subtracts modulo32, interprets the difference as signed and
  // spills CVTSI2SS before MULSS. Equal and unordered expiration do not pass.
  const auto elapsed_ticks = std::bit_cast<std::int32_t>(tick - recorded_tick);
  const float elapsed_count = static_cast<float>(elapsed_ticks);
  const float age = elapsed_count * seconds_per_tick;
  if (!(expiration > age)) return false;
  const std::span<const std::byte, 0x5c> command(record.data() + 0x2c, 0x5c);
  if (!source_player_command_valid(command)) return false;
  std::copy(command.begin(), command.end(), output.begin());
  return true;
}

std::optional<SourcePlayerControl> produce_source_player_control(
    std::span<const std::byte,0x5c> command,
    const SourceCommandCurrentUnit &current,
    std::uint32_t target_unit_flags134) {
  if (current.datum != 0xffffffffu && current.related_object_13c != 0xffffffffu &&
      current.seat != -1 &&
      (!current.parent_seat_flags || !(*current.parent_seat_flags & 0x10u)))
    return std::nullopt; // Original CCC60/BAEB0 seat transform remains separate.

  const double yaw = double(scalar(command,4)), pitch = double(scalar(command,8));
  const double cp = std::cos(pitch);
  const Vector aim{float(std::cos(yaw)*cp),float(std::sin(yaw)*cp),float(std::sin(pitch))};
  Vector facing = aim;
  Vector motion{scalar(command,0xc),scalar(command,0x10),0.f};
  // x86 SHL masks the header type count to five bits before TEST BL,1.
  const bool admitted = current.datum != 0xffffffffu &&
      (current.object_type & 31u) == 0 && current.parent == 0xffffffffu &&
      current.physical_state == 1 && !current.restricted_e5680;
  if (admitted) {
    const auto first = flat_basis(aim,current.forward);
    Vector world;
    for (unsigned i=0; i<3; ++i)
      world[i] = add(mul(first.side[i],motion[1]),mul(first.forward[i],motion[0]));
    facing = selected_facing(world,current.forward,aim);
    const auto second = flat_basis(facing,current.forward);
    motion[0] = add(add(mul(second.forward[2],world[2]),mul(second.forward[1],world[1])),
                    mul(second.forward[0],world[0]));
    motion[1] = add(add(mul(second.side[2],world[2]),mul(second.side[1],world[1])),
                    mul(second.side[0],world[0]));
    const float squared = add(mul(motion[1],motion[1]),mul(motion[0],motion[0]));
    if (squared > 1.f) {
      const double inverse = 1.0 / std::sqrt(double(squared));
      motion[0] = float(double(motion[0])*inverse);
      motion[1] = float(double(motion[1])*inverse);
    }
  }
  SourcePlayerControl out{};
  out.movement_basis_admitted = admitted;
  auto &p = out.packet; // Complete original C8250 constructor, then 151C90.
  put_word(p,0,0x06000086u);
  for (const auto offset : {6,8,9,0xa,0xc})
    put_word(p,offset,0xffffu,offset==8 || offset==9 ? 1 : 2);
  put_word(p,0x10,word(command,0));
  put_vector(p,0x14,motion);
  put_word(p,0x20,word(command,0x14));
  put_word(p,0x24,word(command,0x18));
  put_vector(p,0x28,facing);
  put_vector(p,0x34,aim);
  put_vector(p,0x40,aim);
  put_word(p,6,word(command,0x1e));
  put_word(p,0xa,word(command,0x22,2),2);
  put_word(p,0xc,word(command,0x24,2),2);
  std::copy_n(command.begin()+0x34,0x24,p.begin()+0x58);
  out.target_unit_flags134 = word(command,0x58,1)
      ? target_unit_flags134 | 0x40000000u : target_unit_flags134 & 0xbfffffffu;
  return out;
}
} // namespace halo2
