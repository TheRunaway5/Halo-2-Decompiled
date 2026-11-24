#include "halo2/source_sound_obstruction_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
// TU1.5 globals (retail g_4E0348 and g_4ED288).
constexpr std::uint32_t structure_bsp = 0x4ecc18u, loop_globals = 0x4f9b58u;

// Reads of the resident structure BSP. Offsets are the scenario structure
// BSP's: collision BSP +0x18 (its planes +0xC), sound environment portals
// +0x60 (0x24 each), clusters +0x9C/+0xA0 (0xB0 each), cluster pair values
// +0xE8, audibility +0x224/+0x228.
class Structure {
public:
  explicit Structure(const SourceSoundObstructionWorldTu15 &world) : world_(world) {
    if (!world_.memory) throw std::invalid_argument("Sound obstruction requires the original resident memory");
    bsp_ = u32(structure_bsp);
    if (bsp_) {
      cluster_count_ = i32(bsp_ + 0x9cu);
      clusters_ = u32(bsp_ + 0xa0u);
      if (i32(bsp_ + 0x224u) > 0) audibility_ = u32(bsp_ + 0x228u);
    }
  }
  bool present() const { return bsp_ && cluster_count_ > 0 && clusters_; }
  std::int32_t cluster_count() const { return cluster_count_; }
  std::uint32_t audibility() const { return audibility_; }
  std::uint32_t cluster(std::int32_t index) const {
    if (index < 0 || index >= cluster_count_) throw std::out_of_range("Sound obstruction cluster escapes its structure");
    return clusters_ + std::uint32_t(index) * 0xb0u;
  }
  std::uint32_t bsp() const { return bsp_; }

  std::uint32_t u32(std::uint32_t address) const { return read<std::uint32_t>(address); }
  std::int32_t i32(std::uint32_t address) const { return read<std::int32_t>(address); }
  std::int16_t i16(std::uint32_t address) const { return read<std::int16_t>(address); }
  std::uint16_t u16(std::uint32_t address) const { return read<std::uint16_t>(address); }
  std::uint8_t u8(std::uint32_t address) const { return read<std::uint8_t>(address); }
  float f32(std::uint32_t address) const { return read<float>(address); }
  Vec3 vec3(std::uint32_t address) const { return {f32(address), f32(address + 4), f32(address + 8)}; }
  bool bit(std::uint32_t words, std::int32_t index) const {
    return (u32(words + std::uint32_t(index >> 5) * 4u) >> (index & 31)) & 1u;
  }

private:
  template <class T> T read(std::uint32_t address) const {
    const auto bytes = world_.memory(address, sizeof(T));
    if (bytes.size() != sizeof(T)) throw std::out_of_range("Sound obstruction read escapes resident memory");
    std::array<std::byte, sizeof(T)> raw{};
    std::copy(bytes.begin(), bytes.end(), raw.begin());
    return std::bit_cast<T>(raw);
  }
  const SourceSoundObstructionWorldTu15 &world_;
  std::uint32_t bsp_{}, clusters_{}, audibility_{};
  std::int32_t cluster_count_{};
};

// 18CAC0: a door's scale (0 open .. 1 closed) from the loop-sound globals.
float door_value(const Structure &s, std::int32_t door) {
  const auto globals = s.u32(loop_globals);
  if (!globals || door < 0 || door >= 0x80) return 0.f;
  const auto scale = s.u16(globals + 0x28u + std::uint32_t(door) * 2u);
  if (!scale) return 0.f;
  if (scale >= 0xffffu) return 1.f;
  return float(scale) * (1.f / 65535.f);
}

// 14B240: the distance from a point to a portal disk (plane, centre, radius).
float disk_distance(const Structure &s, std::uint32_t disk, Vec3 point) {
  const auto planes = s.u32(s.u32(s.bsp() + 0x18u) + 0xcu);
  const auto normal = s.vec3(planes + std::uint32_t(s.i32(disk + 4u)) * 16u);
  auto offset = point - s.vec3(disk + 8u);
  const float along = dot(normal, offset);
  const Vec3 projection = normal * along;
  offset = offset - projection;
  const float height = dot(projection, normal);
  const float squared = dot(offset, offset);
  const float radius = s.f32(disk + 0x14u);
  if (radius * radius >= squared) return std::fabs(height);
  const float outside = std::sqrt(squared) - radius;
  return std::sqrt(outside * outside + height * height);
}

// 222150: how far the listener is from the portal of the sound environment
// nearest the source in its cluster (18CFD0), 0 at the disk and towards 1
// far away, with the source cluster's door value.
std::array<float, 2> environment_occlusion(const Structure &s, std::int32_t cluster, Vec3 source, Vec3 listener,
                                           std::span<const float> doors) {
  const auto record = s.cluster(cluster);
  const std::int32_t count = s.i32(record + 0x8cu);
  const std::uint32_t sounds = s.u32(record + 0x90u);
  const auto environments = s.u32(s.bsp() + 0x60u);
  std::int32_t nearest = -1;
  float best = std::numeric_limits<float>::max();
  for (std::int32_t n = 0; n < count; ++n) {
    const auto index = s.i16(sounds + std::uint32_t(n) * 2u);
    const auto delta = source - s.vec3(environments + std::uint32_t(index) * 0x24u + 8u);
    const float length = std::sqrt(delta.y * delta.y + (delta.z * delta.z + delta.x * delta.x));
    if (best > length) {
      best = length;
      nearest = index;
    }
  }
  if (nearest < 0) return {1.f, 0.f};
  const float distance = disk_distance(s, environments + std::uint32_t(nearest) * 0x24u, listener);
  const float door = std::size_t(cluster) < doors.size() ? doors[std::size_t(cluster)] : 0.f;
  return {float(1.0 - 1.0f / (distance + 1.0f)), door};
}
} // namespace

float source_sound_cluster_distance_tu15(const SourceSoundObstructionWorldTu15 &world, std::int32_t a, std::int32_t b) {
  if (a == b) return 0.f;
  const Structure s(world);
  if (!s.present() || a < 0 || b < 0 || a >= s.cluster_count() || b >= s.cluster_count()) return 0.f;
  const auto low = std::min(a, b), high = std::max(a, b), n = s.cluster_count();
  const auto pair = std::uint32_t((n - 1) * low - (low + 1) * low / 2 + high - 1);
  if (const auto audibility = s.audibility()) {
    const auto value = s.u8(s.u32(audibility + 0x28u) + pair);
    if (!value) return 0.f;
    if (value == 0xff) return 256.f;
    const float lower = s.f32(audibility + 4u), upper = s.f32(audibility + 8u);
    return float(value - 1) * (1.f / 253.f) * (upper - lower) + lower;
  }
  return float(s.u8(s.u32(s.bsp() + 0xe8u) + pair) & ~0x80u) * (256.f / 127.f);
}

std::vector<float> source_sound_door_values_tu15(const SourceSoundObstructionWorldTu15 &world, std::int32_t listener) {
  const Structure s(world);
  if (!s.present() || listener < 0 || listener >= s.cluster_count()) return {};
  std::vector<float> values(std::size_t(s.cluster_count()), 0.f);
  const auto audibility = s.audibility();
  if (!audibility) return values;
  const auto doors = s.i32(audibility);
  if (doors <= 0) return values;
  // 249D10: the listener cluster's row of door bits; 249C90: each door's two
  // sides as cluster rows.
  const auto door_words = std::uint32_t((doors + 31) >> 5), cluster_words = std::uint32_t((s.cluster_count() + 31) >> 5);
  const auto door_row = s.u32(audibility + 0x18u) + door_words * 4u * std::uint32_t(listener);
  const auto sides = s.u32(audibility + 0x10u), door_clusters = s.u32(audibility + 0x30u);
  for (std::int32_t door = 0; door < doors; ++door) {
    if (!s.bit(door_row, door)) continue;
    const auto index = std::int8_t(s.u8(door_clusters + std::uint32_t(door)));
    if (index < 0) continue;
    const float value = door_value(s, index);
    const auto a = sides + cluster_words * 4u * std::uint32_t(2 * door), b = a + cluster_words * 4u;
    std::uint32_t far;
    if (s.bit(a, listener)) far = b;
    else if (s.bit(b, listener)) far = a;
    else continue;
    for (std::int32_t cluster = 0; cluster < s.cluster_count(); ++cluster)
      if (cluster != listener && s.bit(far, cluster))
        values[std::size_t(cluster)] = std::max(values[std::size_t(cluster)], value);
  }
  return values;
}

float source_sound_line_obstruction_tu15(const SourceSoundObstructionWorldTu15 &world, Vec3 listener, Vec3 source) {
  if (!world.line) throw std::invalid_argument("Sound obstruction requires its structure line test");
  const auto ahead = world.line(listener, source);
  if (!ahead) return 0.f;
  if (const auto behind = world.line(source, listener)) return std::clamp(1.f - *behind - *ahead, 0.f, 1.f);
  return 1.f - *ahead;
}

SourceSoundObstructionTu15 source_sound_obstruction_tu15(const SourceSoundObstructionWorldTu15 &world,
                                                         std::span<const float> doors,
                                                         const SourceSoundObstructionPointTu15 &listener,
                                                         const SourceSoundObstructionPointTu15 &source, bool skip_line_test,
                                                         std::optional<float> known_line_obstruction) {
  SourceSoundObstructionTu15 result;
  // A source in the listener's own leaf is heard directly.
  if (listener.leaf == source.leaf) return result;
  float obstruction = 0;
  if (!skip_line_test) {
    obstruction = known_line_obstruction ? *known_line_obstruction
                                         : source_sound_line_obstruction_tu15(world, listener.position, source.position);
  } else if (source.cluster != listener.cluster) {
    const float travel = source_sound_cluster_distance_tu15(world, source.cluster, listener.cluster);
    const auto delta = listener.position - source.position;
    const float distance = std::sqrt(dot(delta, delta));
    obstruction = 1.f - distance / std::max(distance + travel, 0.001f);
  }
  if (source.cluster >= 0 && listener.cluster >= 0 && source.cluster != listener.cluster) {
    const Structure s(world);
    if (s.present() && source.cluster < s.cluster_count() && listener.cluster < s.cluster_count()) {
      auto occlusion = environment_occlusion(s, source.cluster, source.position, listener.position, doors);
      // Clusters sharing one sound region (+0x7A) lose no room gain.
      if (s.i16(s.cluster(source.cluster) + 0x7au) == s.i16(s.cluster(listener.cluster) + 0x7au)) occlusion[0] = 0.f;
      result.gain = occlusion[0];
      result.occlusion = occlusion[1];
    }
  }
  result.obstruction = obstruction;
  return result;
}
} // namespace halo2
