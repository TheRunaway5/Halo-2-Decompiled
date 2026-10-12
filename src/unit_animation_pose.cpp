#include "halo2/unit_animation_pose.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
void require(bool v, const char *s) {
  if (!v)
    throw std::runtime_error(s);
}
std::uint32_t get(std::span<const std::byte> b, std::size_t p, unsigned n = 4) {
  require(p + n <= b.size(),
          "Original unit local-pose input extent is unavailable");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[p + i])) << (8 * i);
  return v;
}
float fp(std::span<const std::byte> b, std::size_t p) {
  return std::bit_cast<float>(get(b, p));
}
bool valid(std::span<const std::byte> b) {
  return get(b, 0) != 0xffffffffu && get(b, 6, 2) != 0xffff;
}
float mul(float a, float b) {
  volatile float r = a * b;
  return r;
}
} // namespace
void sample_source_unit_animation_locals(
    std::span<const std::byte> c, float weight,
    std::span<const std::byte> caller_mask, unsigned nodes,
    std::span<std::byte> out, const UnitAnimationLocalPoseServices &s) {
  require(c.size() >= 32 && nodes <= 32767 &&
              out.size() >= std::size_t(nodes) * 32,
          "Original local bank is unavailable");
  require(caller_mask.empty() || caller_mask.size() >= 32,
          "Original node mask is unavailable");
  require(bool(s.definition6c),
          "Original local animation definition owner is unavailable");
  const auto graph = get(c, 0), packed = get(c, 4);
  auto definition = s.definition6c(graph, packed);
  require(definition.size() >= 0x6c,
          "Original local animation definition is unavailable");
  if (get(c, 0x10, 1) && get(definition, 0x10, 1)) {
    require(bool(s.notify_delayed_resource),
            "Original delayed resource notification owner is unavailable");
    s.notify_delayed_resource(graph, packed);
    return;
  }
  const auto options = get(c, 0x12, 2);
  const auto rate = fp(c, 0x18), frame = fp(c, 0x1c);
  require(std::isfinite(rate) && std::isfinite(frame) && std::isfinite(weight),
          "Unmeasured nonfinite local sampler input");
  const bool interpolate =
      (options & 0x40) ||
      ((options & 1) && rate != 0.f && (rate != 1.f || s.tick_frequency != 30));
  std::span<const std::byte> mask = caller_mask;
  auto owner = mask.empty() ? UnitAnimationMaskOwner::None
                            : UnitAnimationMaskOwner::Caller;
  std::array<std::byte, 32> intersection{};
  if (options & 0xc00) {
    require(bool(s.graph_masks),
            "Original animation graph mask owner is unavailable");
    auto masks = s.graph_masks(graph);
    require(masks.size() >= 64, "Original graph node masks are unavailable");
    auto selected = masks.subspan(options & 0x800 ? 0 : 32, 32);
    owner = options & 0x800 ? UnitAnimationMaskOwner::Graph5C
                            : UnitAnimationMaskOwner::Graph7C;
    if (!caller_mask.empty() && selected.data() != caller_mask.data()) {
      for (unsigned i = 0; i < 32; ++i)
        intersection[i] = selected[i] & caller_mask[i];
      mask = intersection;
      owner = UnitAnimationMaskOwner::Intersection;
    } else
      mask = selected;
  }
  std::uint32_t mapping = 0;
  if (get(c, 6, 2) != 0xffff && get(c, 4, 2) != 0xffff) {
    require(bool(s.inheritance_mapping),
            "Original inherited animation node-map owner is unavailable");
    mapping = s.inheritance_mapping(graph, packed);
  }
  require(bool(s.decode),
          "Original packed local animation decoder is unavailable");
  s.decode(
      {graph, packed, mapping, nodes, frame, weight, interpolate, owner, mask},
      out.first(std::size_t(nodes) * 32));
}
void build_source_unit_animation_locals(
    std::span<const std::byte> m, UnitAnimationLocalContext context,
    float weight, std::span<const std::byte> mask, unsigned nodes,
    std::span<std::byte> out, const UnitAnimationLocalPoseServices &s) {
  require(m.size() >= 0x90, "Original animation manager is unavailable");
  auto third = m.subspan(64, 32), second = m.subspan(32, 32);
  if (s.third_channel_override && valid(third)) {
    sample_source_unit_animation_locals(third, weight, mask, nodes, out, s);
    if (nodes && (mask.empty() || (get(mask, 0, 1) & 1))) {
      require(bool(s.override_root),
              "Original override root transform owner is unavailable");
      s.override_root(m, context, weight, nodes, mask,
                      out.first(std::size_t(nodes) * 32));
    }
    return;
  }
  if (s.layer_synchronization && get(second, 0) != 0xffffffffu) {
    const float blend = fp(m, 0x80);
    require(std::isfinite(blend),
            "Unmeasured nonfinite animation layer weight");
    const float upper = std::bit_cast<float>(std::uint32_t(0x3f7ff972));
    if (valid(second) && blend >= upper) {
      sample_source_unit_animation_locals(second, weight, mask, nodes, out, s);
      return;
    }
    if (valid(second) &&
        blend > std::bit_cast<float>(std::uint32_t(0x38d1b717)) &&
        blend < upper) {
      require(bool(s.blend_channel),
              "Original primary blend-channel producer is unavailable");
      const auto primary = s.blend_channel(m);
      sample_source_unit_animation_locals(primary.channel, weight, mask, nodes,
                                          out, s);
      sample_source_unit_animation_locals(
          second, mul(primary.second_weight, weight), mask, nodes, out, s);
      return;
    }
  }
  sample_source_unit_animation_locals(m.first(32), weight, mask, nodes, out, s);
}
} // namespace halo2
