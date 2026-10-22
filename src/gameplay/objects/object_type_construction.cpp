#include "halo2/object_type_construction.hpp"
#include <bit>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
void require(bool v, const char *message) {
  if (!v)
    throw std::runtime_error(message);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  require(at <= b.size() && n <= b.size() - at,
          "Truncated original object construction backing");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[at + i]) << (i * 8);
  return v;
}
std::uint32_t address(std::uint32_t base, std::int32_t offset) {
  const auto p = std::int64_t(base) + offset;
  require(p >= 0 && p <= std::numeric_limits<std::uint32_t>::max(),
          "Object construction pointer escapes32-bit backing");
  return std::uint32_t(p);
}
std::uint32_t word(const SourceObjectConstructionMetadata &m, std::uint32_t a,
                   unsigned n = 4) {
  require(bool(m.read),
          "Original object construction metadata owner unavailable");
  const auto b = m.read(a, n);
  require(b.size() == n, "Original object construction read extent differs");
  return read(b, 0, n);
}
std::uint32_t type(const SourceObjectConstructionMetadata &m, std::int16_t t) {
  require(bool(m.type_address),
          "Original object type-record provider unavailable");
  return m.type_address(t);
}
std::span<const std::byte> body(std::uint32_t unit,
                                const UnitConstructorObjectPool &p) {
  const auto i = unit & 0xffffu;
  require(i < p.capacity && std::size_t(p.capacity) <= p.headers.size() / 12,
          "Original object type header index outside owned pool");
  require(bool(p.body), "Original constructed object body owner unavailable");
  const auto b = p.body(std::uint16_t(i));
  const auto n = read(p.headers, std::size_t(i) * 12 + 6, 2);
  require(n <= b.size(),
          "Original object type header extent exceeds owned body");
  return b.first(n);
}
std::uint8_t invoke(const SourceObjectConstructionServices &s,
                    std::uint32_t entry, std::uint32_t unit,
                    ObjectConstructionCallback kind,
                    std::span<std::byte> request,
                    std::span<std::byte> failure = {}) {
  require(bool(s.invoke), "Original object type callback owner unavailable");
  return s.invoke({entry, unit, kind, request, failure});
}
} // namespace
std::int16_t
source_object_definition_type(std::uint32_t definition,
                              const SourceObjectConstructionMetadata &m) {
  require(bool(m.tag_group), "Original tag-table group owner unavailable");
  const auto index = std::bit_cast<std::int16_t>(std::uint16_t(definition));
  const auto group = m.tag_group(index);
  for (std::int16_t i = 0; i < 13; ++i)
    if (word(m, address(type(m, i), 4)) == group)
      return i;
  return -1;
}
void prepare_source_object_type(std::span<std::byte> request,
                                const SourceObjectConstructionServices &s) {
  require(request.size() >= 0xc4,
          "Original object preparation needs completeC4 request");
  const auto record = type(
      s.metadata, source_object_definition_type(read(request, 0), s.metadata));
  // SI/DI are signed16 loop indices in the source. Reject a malformed bank
  // before its index wraps rather than manufacture a NULL or success result.
  auto pointer = address(record, 0x84);
  if (!word(s.metadata, pointer))
    return;
  for (std::int32_t i = 0; i <= std::numeric_limits<std::int16_t>::max(); ++i) {
    const auto slot = word(s.metadata, pointer);
    const auto target = word(s.metadata, address(slot, 0x28));
    if (target)
      invoke(s, target, 0xffffffffu, ObjectConstructionCallback::Prepare,
             request);
    if (i == std::numeric_limits<std::int16_t>::max())
      break;
    pointer = address(record, 0x84 + (i + 1) * 4);
    if (!word(s.metadata, pointer))
      return;
  }
  throw std::runtime_error(
      "Original object preparation bank has no bounded terminator");
}
bool construct_source_object_type(std::uint32_t unit,
                                  std::span<std::byte> request,
                                  std::span<std::byte> failure,
                                  const SourceObjectConstructionServices &s) {
  require(request.size() >= 0xc4 && !failure.empty(),
          "Original object constructor request/failure backing unavailable");
  const auto t = std::bit_cast<std::int8_t>(
      std::uint8_t(read(body(unit, s.objects), 0xaa, 1)));
  const auto record = type(s.metadata, t);
  bool result = true;
  auto pointer = address(record, 0x84);
  if (!word(s.metadata, pointer))
    return result;
  for (std::int32_t i = 0; i <= std::numeric_limits<std::int16_t>::max(); ++i) {
    const auto slot = word(s.metadata, pointer);
    const auto target = word(s.metadata, address(slot, 0x2c));
    if (target && result)
      result = invoke(s, target, unit, ObjectConstructionCallback::Construct,
                      request, failure) != 0;
    if (i == std::numeric_limits<std::int16_t>::max())
      break;
    pointer = address(record, 0x84 + (i + 1) * 4);
    if (!word(s.metadata, pointer))
      return result;
  }
  throw std::runtime_error(
      "Original object constructor bank has no bounded terminator");
}
} // namespace halo2
