#include "halo2/animation_variant.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
std::uint32_t word(std::span<const std::byte> data, std::size_t offset,
                   unsigned bytes = 4) {
  require(offset <= data.size() && bytes <= data.size() - offset,
          "Original animation variant backing is truncated");
  std::uint32_t result = 0;
  for (unsigned i = 0; i < bytes; ++i)
    result |= std::uint32_t(std::to_integer<unsigned>(data[offset + i]))
              << (i * 8);
  return result;
}
std::uint32_t read(const AnimationVariantServices &s, std::uint32_t address,
                   unsigned bytes = 4) {
  require(bool(s.memory.read), "Original animation variant graph unavailable");
  return word(s.memory.read(address, bytes), 0, bytes);
}
std::int32_t signed16(std::uint32_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
std::uint32_t inherited(std::uint32_t graph, std::int32_t index,
                        const AnimationVariantServices &s) {
  if (index >= std::bit_cast<std::int32_t>(read(s, graph + 0x4c)))
    return 0;
  const auto datum = read(s, read(s, graph + 0x50) +
                                std::uint32_t(index) * 0x20 + 4);
  if (datum == 0xffffffffu)
    return 0;
  require(bool(s.memory.graph_address),
          "Original inherited animation graph owner unavailable");
  return s.memory.graph_address(datum);
}
std::uint32_t animation(std::uint32_t graph, std::int32_t index,
                        const AnimationVariantServices &s) {
  if (index == -1)
    return 0;
  return read(s, graph + 0x30) + std::uint32_t(index) * 0x6c;
}
void resource(std::uint32_t graph, std::uint32_t packed,
              const AnimationVariantServices &s) {
  require(bool(s.resource), "Original1E13C0 animation resource owner unavailable");
  s.resource(graph, packed);
}
std::uint32_t definition(std::uint32_t graph, std::uint32_t packed,
                         const AnimationVariantServices &s) {
  const auto index = signed16(packed >> 16);
  if (index == -1)
    return 0;
  const auto inheritance = signed16(packed);
  const auto address = animation(inheritance == -1 ? graph : inherited(graph, inheritance, s), index, s);
  if (address)
    resource(graph, packed, s);
  return address;
}
bool linked(std::uint32_t address, const AnimationVariantServices &s) {
  return read(s, address + 0x30, 2) != 0xffff ||
         read(s, address + 0x32, 2) != 0xffff;
}
std::uint32_t first(std::uint32_t graph, std::uint32_t packed,
                    const AnimationVariantServices &s) {
  if (std::uint16_t(packed >> 16) != 0xffff &&
      linked(definition(graph, packed, s), s)) {
    const auto head = read(s, definition(graph, packed, s) + 0x30, 2);
    if (head != 0xffff)
      return (head << 16) | (packed & 0xffff);
  }
  return packed;
}
float number(std::uint32_t bits) { return std::bit_cast<float>(bits); }
} // namespace

std::uint32_t choose_source_animation_variant(
    std::uint32_t graph, std::uint32_t packed, std::uint8_t stream0,
    std::array<std::uint32_t, 2> &rng, const AnimationVariantServices &s) {
  if (std::uint16_t(packed >> 16) == 0xffff)
    return packed;
  if (linked(definition(graph, packed, s), s)) {
    const auto inheritance = signed16(packed);
    const auto owner = inheritance == -1 ? graph : inherited(graph, inheritance, s);
    packed = first(graph, packed, s);
    auto address = animation(owner, signed16(packed >> 16), s);
    // Original COMISS one,threshold / JBE skips equal and unordered values.
    if (number(s.constants.one_bits) > number(read(s, address + 0x1c))) {
      const unsigned stream = stream0 ? 0u : 1u;
      const auto before = rng[stream];
      const auto after = before * 0x19660du + 0x3c6ef35fu;
      rng[stream] = after;
      if (s.rng_store)
        s.rng_store({stream ? 0x1e10efu : 0x1e10d1u, stream, before, after});
      // FILD(unsigned high16) * source float, then original FSTP float.
      // The sign-correction FADD cannot execute for a shifted uint32 value.
      const double product = double(after >> 16) * double(number(s.constants.rng_scale_bits));
      volatile float rounded = static_cast<float>(product);
      const float random = rounded;
      while (read(s, address + 0x32, 2) != 0xffff) {
        // COMISS threshold,random / JAE stops on ordered >= only; an
        // unordered threshold follows the original chain-advance branch.
        if (number(read(s, address + 0x1c)) >= random)
          break;
        const auto next = read(s, address + 0x32, 2);
        packed = (next << 16) | (packed & 0xffff);
        address = animation(owner, signed16(next), s);
      }
    }
  }
  if (std::uint16_t(packed >> 16) != 0xffff)
    resource(graph, packed, s);
  return packed;
}

std::uint32_t select_source_animation_variant(
    std::span<const std::byte> manager, std::uint32_t packed,
    std::array<std::uint32_t, 2> &rng, const AnimationVariantServices &s) {
  if (std::uint16_t(packed >> 16) == 0xffff)
    return packed;
  require(bool(s.memory.graph_address), "Original manager animation graph owner unavailable");
  const auto graph = s.memory.graph_address(word(manager, 0x68));
  if (!graph)
    return packed;
  return choose_source_animation_variant(graph, packed,
      std::uint8_t((word(manager, 0x6c, 1) >> 1) & 1), rng, s);
}
} // namespace halo2
