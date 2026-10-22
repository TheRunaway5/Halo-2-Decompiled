#include "halo2/source_vehicle_construction.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
std::uint32_t read(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v{};
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
void write(M &m, const O &observer, std::uint32_t site, std::uint32_t p,
           unsigned width, std::uint32_t v) {
  SourcePhysicsContactRecordStore store{site, p, {}, {}};
  store.before.resize(width);
  store.written.resize(width);
  m.read(p, store.before);
  for (unsigned i = 0; i < width; ++i)
    store.written[i] = std::byte((v >> (8 * i)) & 255u);
  m.write(p, store.written);
  if (observer)
    observer(store);
}
template <class F, class... A> decltype(auto) owner(const F &f, A &&...a) {
  if (!f)
    throw std::runtime_error(
        "Vehicle construction requires original dependency owner");
  return f(std::forward<A>(a)...);
}
bool later(SourcePhysicsContactImage image) {
  return image == SourcePhysicsContactImage::TitleUpdate15;
}
std::uint32_t body(const M &m, std::uint32_t vehicle,
                   SourcePhysicsContactImage image) {
  return read(m, read(m, read(m, later(image) ? 0x4ecbd0 : 0x4e78d0) + 0x44) +
                     (vehicle & 0xffffu) * 12u + 8);
}
struct Store {
  std::uint32_t site;
  unsigned offset, width;
  std::uint32_t value;
};
constexpr Store reset_prefix[]{
    {0xf0014, 0x34a, 1, 0},    {0xf001a, 0x348, 2, 0},
    {0xf0021, 0x34b, 1, 0},    {0xf0027, 0x34c, 1, 0},
    {0xf002d, 0x34d, 1, 0},    {0xf0033, 0x34e, 1, 0},
    {0xf0039, 0x34f, 1, 0},    {0xf003f, 0x350, 1, 0},
    {0xf0045, 0x384, 1, 0},    {0xf004b, 0x385, 1, 0},
    {0xf0051, 0x386, 1, 0},    {0xf0057, 0x387, 1, 0},
    {0xf005d, 0x388, 1, 0},    {0xf0063, 0x38e, 2, 0},
    {0xf006a, 0x351, 1, 0xff}, {0xf0071, 0x354, 4, 0},
    {0xf0079, 0x358, 4, 0},    {0xf0081, 0x35c, 4, 0},
    {0xf0089, 0x334, 4, 0},    {0xf0091, 0x338, 4, 0},
    {0xf0099, 0x360, 4, 0},    {0xf00a1, 0x364, 4, 0},
    {0xf00a9, 0x368, 4, 0},    {0xf00b1, 0x36c, 4, 0},
    {0xf00b9, 0x374, 4, 0},    {0xf00c1, 0x370, 4, 0},
    {0xf00c9, 0x378, 4, 0},    {0xf00d1, 0x37c, 4, 0},
    {0xf00d9, 0x38c, 1, 0xfe}, {0xf00e0, 0x380, 4, 0},
    {0xf00eb, 0x38b, 1, 0xff}, {0xf00f9, 0x398, 4, 0},
    {0xf00fb, 0x39c, 4, 0},    {0xf0104, 0x390, 4, 0},
    {0xf0106, 0x394, 4, 0},    {0xf0109, 0x3ac, 4, 0}};
constexpr Store reset_suffix[]{
    {0xf0121, 0x3b8, 4, 0xffffffff}, {0xf0127, 0x3bc, 4, 0xffffffff},
    {0xf012d, 0x3c8, 4, 0xffffffff}, {0xf0133, 0x3c0, 4, 0xffffffff},
    {0xf0139, 0x3cc, 4, 0xffffffff}, {0xf013f, 0x400, 4, 0xffffffff},
    {0xf0145, 0x404, 2, 0xffff},     {0xf014c, 0x406, 2, 0xffff},
    {0xf0153, 0x408, 2, 0xffff},     {0xf015a, 0x40a, 2, 0xffff},
    {0xf0161, 0x40c, 2, 0xffff},     {0xf0168, 0x40e, 2, 0xffff},
    {0xf016f, 0x410, 2, 0xffff},     {0xf0176, 0x412, 2, 0xffff},
    {0xf017d, 0x414, 2, 0xffff},     {0xf0184, 0x416, 2, 0xffff},
    {0xf018b, 0x418, 2, 0xffff},     {0xf0192, 0x41a, 2, 0xffff}};
} // namespace
void source_vehicle_reset(M &m, std::uint32_t vehicle,
                          SourcePhysicsContactImage image,
                          const SourceVehicleConstructionServices &services,
                          const O &observer) {
  const auto p = body(m, vehicle, image), delta = later(image) ? 0xf20u : 0u;
  const auto stores = [&](auto rows) {
    for (const auto &row : rows)
      write(m, observer, row.site + delta, p + row.offset, row.width,
            row.value);
  };
  stores(std::span(reset_prefix));
  write(m, observer, 0xf010f + delta, p + 0x349, 1, read(m, p + 0x349, 1) | 1u);
  owner(services.point, vehicle, p + 0x3d0);
  stores(std::span(reset_suffix));
  for (unsigned n = 0; n < 3; ++n) {
    write(m, observer, 0xf01a4 + delta, p + 0x41c + n * 4, 2, 0xffff);
    write(m, observer, 0xf01a7 + delta, p + 0x41e + n * 4, 2, 0xffff);
  }
}
bool source_vehicle_construct(M &m, std::uint32_t vehicle,
                              SourcePhysicsContactImage image,
                              const SourceVehicleConstructionServices &services,
                              const O &observer) {
  const auto p = body(m, vehicle, image), definition = read(m, p);
  const auto tags = read(m, later(image) ? 0x4f0414 : 0x4eb114);
  const auto tag = read(m, tags + (definition & 0xffffu) * 16u + 8);
  const auto physics =
      read(m, read(m, tags + (read(m, tag + 0x38) & 0xffffu) * 16u + 8) + 0x24);
  if (physics == 0xffffffffu)
    return false;
  const auto delta = later(image) ? 0xf20u : 0u;
  write(m, observer, 0xf0217 + delta, p + 0x3a0, 4, 0xffffffff);
  write(m, observer, 0xf021d + delta, p + 0x3a4, 4, 0xffffffff);
  write(m, observer, 0xf0223 + delta, p + 0x3a8, 4, 0);
  write(m, observer, 0xf0229 + delta, p + 0x3b0, 4, 0);
  write(m, observer, 0xf022f + delta, p + 0x3b4, 4, 0);
  source_vehicle_reset(m, vehicle, image, services, observer);
  if (read(m, tag + 0x2ac, 1) & 1u)
    owner(services.original_baa80, vehicle);
  const auto current_body = body(m, vehicle, image);
  if (read(m, current_body + 0x12a, 2) != 0xffffu) {
    const std::array<std::uint32_t, 7> request{
        vehicle, 0x07000001, 0x07000101, 0x07000101, 0x0700005c, 0, 0};
    if (owner(services.animation_request, request)) {
      const auto offset =
          std::bit_cast<std::int16_t>(std::uint16_t(read(m, p + 0x12a, 2)));
      owner(services.reset_animation, p + std::uint32_t(std::int32_t(offset)));
      return true;
    }
    auto fallback = request;
    fallback[4] = 0x0400000c;
    owner(services.animation_request, fallback);
  }
  return true;
}
} // namespace halo2
