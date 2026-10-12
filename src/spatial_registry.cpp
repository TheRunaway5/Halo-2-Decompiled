#include "halo2/spatial_registry.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
template<class T> T read(std::span<const std::byte> bytes, std::size_t offset) {
  if (offset + sizeof(T) > bytes.size()) throw std::runtime_error("Incomplete original registry storage");
  T value; std::memcpy(&value, bytes.data() + offset, sizeof value); return value;
}
void write(std::span<std::byte> bytes, SourceSpatialTarget target, std::size_t offset,
    std::span<const std::byte> value, std::uint32_t instruction, const SourceSpatialObserver &observer) {
  if (offset + value.size() > bytes.size()) throw std::runtime_error("Incomplete original registry write backing");
  SourceSpatialStore event{target, instruction, offset,
      {bytes.begin() + offset, bytes.begin() + offset + value.size()}, {value.begin(), value.end()}};
  std::copy(value.begin(), value.end(), bytes.begin() + offset); if (observer) observer(event);
}
template<class T> void word(std::span<std::byte> bytes, SourceSpatialTarget target,
    std::size_t offset, T value, std::uint32_t instruction, const SourceSpatialObserver &observer) {
  write(bytes, target, offset, std::as_bytes(std::span(&value, 1)), instruction, observer);
}
struct Pool {
  SourceSpatialPoolStorage storage;
  SourceSpatialTarget header, elements, bitmap;
  const SourceSpatialObserver &observer;
  std::uint32_t stride() const { return read<std::uint32_t>(storage.header, 0x24); }
  std::size_t at(std::uint32_t datum) const {
    const auto index = datum & 0xffffu;
    const auto capacity = read<std::int32_t>(storage.header, 0x20);
    if (capacity <= 0 || index >= unsigned(capacity) ||
        std::uint64_t(index + 1) * stride() > storage.elements.size())
      throw std::runtime_error("Original spatial pool element backing unavailable");
    return std::size_t(index) * stride();
  }
  std::uint32_t allocate() {
    auto index = read<std::int32_t>(storage.header, 0x34);
    const auto high = read<std::int32_t>(storage.header, 0x38);
    if (index < 0 || high < 0) throw std::runtime_error("Unsupported original spatial pool cursor");
    while (index < high && (read<std::uint32_t>(storage.bitmap, unsigned(index) / 32 * 4) & (1u << (index & 31)))) ++index;
    if (index >= high) index = high;
    if (index >= read<std::int32_t>(storage.header, 0x20)) return none;
    const auto offset = at(unsigned(index)); const auto bitmap_offset = unsigned(index) / 32 * 4;
    word(storage.bitmap, bitmap, bitmap_offset,
        read<std::uint32_t>(storage.bitmap, bitmap_offset) | (1u << (index & 31)), 0x16d32e, observer);
    word(storage.header, header, 0x3c, read<std::uint32_t>(storage.header, 0x3c) + 1u, 0x16d33c, observer);
    word(storage.header, header, 0x34, std::uint32_t(index + 1), 0x16d33f, observer);
    if (high <= index) word(storage.header, header, 0x38, std::uint32_t(index + 1), 0x16d344, observer);
    for (unsigned i = 0; i < stride() / 4; ++i)
      word(storage.elements, elements, offset + i * 4, 0u, 0x16d353, observer);
    for (unsigned i = stride() / 4 * 4; i < stride(); ++i)
      word(storage.elements, elements, offset + i, std::uint8_t(0), 0x16d35a, observer);
    const auto salt = read<std::uint16_t>(storage.header, 0x40);
    word(storage.elements, elements, offset, salt, 0x16d360, observer);
    auto next = std::uint16_t(salt + 1u);
    word(storage.header, header, 0x40, next, 0x16d363, observer);
    if (next == 0xffffu) word(storage.header, header, 0x40, std::uint16_t(0x8000), 0x16d36e, observer);
    return std::uint32_t(salt) << 16 | unsigned(index);
  }
  void release(std::uint32_t datum) {
    const auto index = datum & 0xffffu; const auto offset = at(datum);
    if (read<std::uint8_t>(storage.header, 0x2a) & 8u)
      throw std::runtime_error("Unmeasured original spatial pool debug fill");
    const auto bitmap_offset = index / 32 * 4;
    word(storage.bitmap, bitmap, bitmap_offset,
        read<std::uint32_t>(storage.bitmap, bitmap_offset) & ~(1u << (index & 31)), 0x16d518, observer);
    word(storage.elements, elements, offset, std::uint16_t(0), 0x16d51a, observer);
    if (index < read<std::uint32_t>(storage.header, 0x34))
      word(storage.header, header, 0x34, index, 0x16d524, observer);
    auto high = read<std::int32_t>(storage.header, 0x38);
    if (index + 1u == unsigned(high)) do {
      --high; word(storage.header, header, 0x38, high, 0x16d53c, observer);
    } while (high > 0 && !read<std::uint16_t>(storage.elements, std::size_t(high - 1) * stride()));
    word(storage.header, header, 0x3c, read<std::uint32_t>(storage.header, 0x3c) - 1u, 0x16d54d, observer);
  }
};
Pool forward(SourceSpatialRegistryStorage storage, const SourceSpatialObserver &observer) {
  return {storage.forward, SourceSpatialTarget::forward_header, SourceSpatialTarget::forward_elements,
      SourceSpatialTarget::forward_bitmap, observer};
}
Pool reverse(SourceSpatialRegistryStorage storage, const SourceSpatialObserver &observer) {
  return {storage.reverse, SourceSpatialTarget::reverse_header, SourceSpatialTarget::reverse_elements,
      SourceSpatialTarget::reverse_bitmap, observer};
}
} // namespace

void source_register_spatial_object(SourceSpatialRegistryStorage storage,
    const SourceSpatialRegistration &input, const SourceRegistryClusterService &cluster_service,
    const SourceSpatialObserver &observer) {
  const auto cluster = read<std::int16_t>(input.location, 4);
  if (cluster == -1 || !(input.radius > input.zero))
    throw std::runtime_error("Unmeasured original absent/nonpositive spatial location");
  if (!cluster_service) throw std::runtime_error("Original portal cluster producer unavailable");
  word(input.visit_prefix, SourceSpatialTarget::visit_prefix, 4,
      read<std::uint32_t>(input.visit_prefix, 4) + 1u, 0x1ce6b2, observer);
  word(input.visit_prefix, SourceSpatialTarget::visit_prefix, 1, std::uint8_t(1), 0x1ce6cc, observer);
  const auto count = std::int16_t(cluster_service(cluster, input.center, input.radius, 256, input.cluster_scratch));
  word(input.visit_prefix, SourceSpatialTarget::visit_prefix, 1, std::uint8_t(0), 0x1ce6d8, observer);
  const auto admitted = std::min<std::int16_t>(count, 256);
  word(input.overflow, SourceSpatialTarget::overflow, 0, std::uint8_t(count > admitted), 0x1ce708, observer);
  auto f = forward(storage, observer); auto r = reverse(storage, observer);
  for (int i = 0; i < admitted; ++i) {
    const auto selected = read<std::int16_t>(input.cluster_scratch, unsigned(i) * 2);
    if (selected < 0 || unsigned(selected) * 4 + 4 > storage.heads.size())
      throw std::runtime_error("Original registry cluster-head backing unavailable");
    const auto rd = r.allocate();
    if (rd != none) {
      const auto at = r.at(rd);
      word(storage.reverse.elements, SourceSpatialTarget::reverse_elements, at + 4,
          std::int32_t(selected), 0x1ce744, observer);
      word(storage.reverse.elements, SourceSpatialTarget::reverse_elements, at + 8,
          read<std::uint32_t>(storage.reverse_head, 0), 0x1ce750, observer);
      word(storage.reverse_head, SourceSpatialTarget::reverse_head, 0, rd, 0x1ce753, observer);
    }
    const auto fd = f.allocate();
    if (fd == none) continue;
    const auto at = f.at(fd); const auto head_offset = unsigned(selected) * 4;
    word(storage.forward.elements, SourceSpatialTarget::forward_elements, at + 4,
        input.object, 0x1ce785, observer);
    word(storage.forward.elements, SourceSpatialTarget::forward_elements, at + 8,
        read<std::uint32_t>(storage.heads, head_offset), 0x1ce78a, observer);
    word(storage.heads, SourceSpatialTarget::heads, head_offset, fd, 0x1ce78d, observer);
    if (input.payload.size() + 12 > f.stride()) throw std::runtime_error("Original forward payload backing unavailable");
    for (std::size_t byte = 0; byte < input.payload.size() / 4 * 4; byte += 4)
      write(storage.forward.elements, SourceSpatialTarget::forward_elements, at + 12 + byte,
          input.payload.subspan(byte, 4), 0x1ce7a9, observer);
    for (std::size_t byte = input.payload.size() / 4 * 4; byte < input.payload.size(); ++byte)
      write(storage.forward.elements, SourceSpatialTarget::forward_elements, at + 12 + byte,
          input.payload.subspan(byte, 1), 0x1ce7b0, observer);
  }
}

void source_unregister_spatial_object(SourceSpatialRegistryStorage storage,
    std::uint32_t object, const SourceSpatialObserver &observer) {
  auto f = forward(storage, observer); auto r = reverse(storage, observer);
  auto datum = read<std::uint32_t>(storage.reverse_head, 0);
  unsigned reverse_steps = 0;
  while (datum != none) {
    if (++reverse_steps > read<std::uint32_t>(storage.reverse.header, 0x20))
      throw std::runtime_error("Invalid cyclic source reverse registry");
    const auto at = r.at(datum);
    const auto cluster = read<std::int16_t>(storage.reverse.elements, at + 4);
    const auto next = read<std::uint32_t>(storage.reverse.elements, at + 8);
    r.release(datum);
    if (cluster < 0) throw std::runtime_error("Original reverse cluster-head backing unavailable");
    std::span<std::byte> link = storage.heads;
    auto target = SourceSpatialTarget::heads;
    std::size_t offset = unsigned(cluster) * 4;
    unsigned forward_steps = 0;
    while (read<std::uint32_t>(link, offset) != none) {
      if (++forward_steps > read<std::uint32_t>(storage.forward.header, 0x20))
        throw std::runtime_error("Invalid cyclic source forward registry");
      const auto current = read<std::uint32_t>(link, offset); const auto position = f.at(current);
      if (read<std::uint32_t>(storage.forward.elements, position + 4) == object) {
        const auto successor = read<std::uint32_t>(storage.forward.elements, position + 8);
        f.release(current); word(link, target, offset, successor, 0x1ce8a5, observer); break;
      }
      link = storage.forward.elements; target = SourceSpatialTarget::forward_elements; offset = position + 8;
    }
    datum = next;
  }
  word(storage.reverse_head, SourceSpatialTarget::reverse_head, 0, none, 0x1ce8b6, observer);
}
} // namespace halo2
