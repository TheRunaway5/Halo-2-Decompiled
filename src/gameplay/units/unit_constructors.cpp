#include "halo2/unit_constructors.hpp"
#include <bit>
#include <initializer_list>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
void require(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  require(at <= b.size() && n <= b.size() - at,
          "Truncated source constructor storage");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
std::int32_t signed_word(std::uint32_t v) {
  return v < 0x8000 ? std::int32_t(v) : std::int32_t(v) - 0x10000;
}
void write(std::span<std::byte> b, std::size_t at, unsigned n, std::uint32_t v,
           std::uint32_t ip, UnitConstructorBuffer kind,
           const UnitConstructorObserver &o) {
  auto before = read(b, at, n);
  for (unsigned i = 0; i < n; ++i)
    b[at + i] = std::byte((v >> (8 * i)) & 255);
  if (o)
    o({ip, kind, at, n, before, read(b, at, n)});
}
struct Op {
  std::uint32_t ip;
  std::size_t at;
  unsigned bytes;
  enum Value { Zero, None, One, Graph, ByteOne } value;
};
void ops(std::span<std::byte> b, std::span<const Op> rows, std::uint32_t one,
         std::uint32_t graph, const UnitConstructorObserver &o,
         UnitConstructorBuffer kind = UnitConstructorBuffer::Manager) {
  for (const auto &r : rows)
    write(b, r.at, r.bytes,
          r.value == Op::None      ? none
          : r.value == Op::One     ? one
          : r.value == Op::Graph   ? graph
          : r.value == Op::ByteOne ? 1
                                   : 0,
          r.ip, kind, o);
}
constexpr Op construction[]{
    {0x001ce8ce, 0x4, 2, Op::None},  {0x001ce8d2, 0x6, 2, Op::None},
    {0x001ce8d6, 0x0, 4, Op::None},  {0x001ce8d8, 0x4, 2, Op::None},
    {0x001ce8dc, 0x6, 2, Op::None},  {0x001ce8e0, 0x1c, 4, Op::Zero},
    {0x001ce8e5, 0x18, 4, Op::One},  {0x001ce8ea, 0x8, 4, Op::None},
    {0x001ce8ed, 0xe, 2, Op::None},  {0x001ce8f1, 0xc, 1, Op::None},
    {0x001ce8f4, 0xd, 1, Op::None},  {0x001ce8fd, 0x10, 1, Op::Zero},
    {0x001ce900, 0x11, 1, Op::Zero}, {0x001ce903, 0x12, 2, Op::Zero},
    {0x001ce907, 0x14, 2, Op::Zero}, {0x001ce90b, 0x16, 2, Op::Zero},
    {0x001ce90f, 0x24, 2, Op::None}, {0x001ce913, 0x26, 2, Op::None},
    {0x001ce917, 0x20, 4, Op::None}, {0x001ce919, 0x24, 2, Op::None},
    {0x001ce91d, 0x26, 2, Op::None}, {0x001ce921, 0x30, 1, Op::Zero},
    {0x001ce924, 0x31, 1, Op::Zero}, {0x001ce927, 0x32, 2, Op::Zero},
    {0x001ce92b, 0x34, 2, Op::Zero}, {0x001ce92f, 0x36, 2, Op::Zero},
    {0x001ce936, 0x3c, 4, Op::Zero}, {0x001ce93b, 0x38, 4, Op::One},
    {0x001ce940, 0x28, 4, Op::None}, {0x001ce943, 0x2e, 2, Op::None},
    {0x001ce947, 0x2c, 1, Op::None}, {0x001ce94a, 0x2d, 1, Op::None},
    {0x001ce94d, 0x44, 2, Op::None}, {0x001ce951, 0x46, 2, Op::None},
    {0x001ce955, 0x40, 4, Op::None}, {0x001ce957, 0x44, 2, Op::None},
    {0x001ce95b, 0x46, 2, Op::None}, {0x001ce95f, 0x50, 1, Op::Zero},
    {0x001ce962, 0x51, 1, Op::Zero}, {0x001ce965, 0x52, 2, Op::Zero},
    {0x001ce969, 0x54, 2, Op::Zero}, {0x001ce96d, 0x56, 2, Op::Zero},
    {0x001ce971, 0x5c, 4, Op::Zero}, {0x001ce976, 0x58, 4, Op::One},
    {0x001ce97b, 0x48, 4, Op::None}, {0x001ce97e, 0x4e, 2, Op::None},
    {0x001ce982, 0x4c, 1, Op::None}, {0x001ce985, 0x4d, 1, Op::None},
    {0x001ce988, 0x60, 1, Op::Zero}, {0x001ce98b, 0x61, 1, Op::Zero},
    {0x001ce98e, 0x62, 1, Op::Zero}, {0x001ce991, 0x63, 1, Op::Zero},
    {0x001ce994, 0x64, 1, Op::Zero}, {0x001ce997, 0x65, 1, Op::Zero},
    {0x001ce99a, 0x66, 1, Op::Zero}, {0x001ce99d, 0x67, 1, Op::Zero},
};
constexpr Op reset[]{
    {0x001ce9c0, 0x0, 4, Op::None},  {0x001ce9c2, 0x4, 2, Op::None},
    {0x001ce9c6, 0x6, 2, Op::None},  {0x001ce9ca, 0x1c, 4, Op::Zero},
    {0x001ce9cf, 0x10, 1, Op::Zero}, {0x001ce9d2, 0x11, 1, Op::Zero},
    {0x001ce9d5, 0x12, 2, Op::Zero}, {0x001ce9d9, 0x18, 4, Op::One},
    {0x001ce9de, 0x14, 2, Op::Zero}, {0x001ce9e2, 0x16, 2, Op::Zero},
    {0x001ce9e6, 0x8, 4, Op::None},  {0x001ce9e9, 0xe, 2, Op::None},
    {0x001ce9ed, 0xc, 1, Op::None},  {0x001ce9f0, 0xd, 1, Op::None},
    {0x001ce9f3, 0x20, 4, Op::None}, {0x001ce9f6, 0x24, 2, Op::None},
    {0x001ce9fa, 0x26, 2, Op::None}, {0x001ce9fe, 0x3c, 4, Op::Zero},
    {0x001cea03, 0x30, 1, Op::Zero}, {0x001cea06, 0x31, 1, Op::Zero},
    {0x001cea09, 0x32, 2, Op::Zero}, {0x001cea0d, 0x38, 4, Op::One},
    {0x001cea12, 0x34, 2, Op::Zero}, {0x001cea16, 0x36, 2, Op::Zero},
    {0x001cea1a, 0x28, 4, Op::None}, {0x001cea1d, 0x2e, 2, Op::None},
    {0x001cea21, 0x2c, 1, Op::None}, {0x001cea24, 0x2d, 1, Op::None},
    {0x001cea27, 0x40, 4, Op::None}, {0x001cea2a, 0x44, 2, Op::None},
    {0x001cea2e, 0x46, 2, Op::None}, {0x001cea32, 0x5c, 4, Op::Zero},
    {0x001cea37, 0x50, 1, Op::Zero}, {0x001cea3a, 0x51, 1, Op::Zero},
    {0x001cea3d, 0x52, 2, Op::Zero}, {0x001cea41, 0x58, 4, Op::One},
    {0x001cea46, 0x54, 2, Op::Zero}, {0x001cea4a, 0x56, 2, Op::Zero},
    {0x001cea4e, 0x48, 4, Op::None}, {0x001cea51, 0x4e, 2, Op::None},
    {0x001cea55, 0x4c, 1, Op::None}, {0x001cea58, 0x4d, 1, Op::None},
    {0x001cea5b, 0x6c, 2, Op::Zero}, {0x001cea5f, 0x6e, 2, Op::Zero},
    {0x001cea63, 0x68, 4, Op::None}, {0x001cea66, 0x65, 1, Op::Zero},
    {0x001cea69, 0x64, 1, Op::Zero}, {0x001cea6c, 0x67, 1, Op::Zero},
    {0x001cea6f, 0x61, 1, Op::Zero}, {0x001cea72, 0x60, 1, Op::Zero},
    {0x001cea75, 0x63, 1, Op::Zero}, {0x001cea78, 0x70, 4, Op::None},
    {0x001cea7b, 0x7c, 4, Op::None}, {0x001cea7e, 0x74, 4, Op::None},
    {0x001cea81, 0x78, 4, Op::None}, {0x001cea84, 0x80, 4, Op::Zero},
};
constexpr Op binding_prefix[]{
    {0x001ceb19, 0x70, 4, Op::None},  {0x001ceb1c, 0x7c, 4, Op::None},
    {0x001ceb1f, 0x74, 4, Op::None},  {0x001ceb22, 0x78, 4, Op::None},
    {0x001ceb25, 0x68, 4, Op::Graph}, {0x001ceb33, 0x0, 4, Op::None},
    {0x001ceb35, 0x4, 2, Op::None},   {0x001ceb39, 0x6, 2, Op::None},
    {0x001ceb3d, 0x8, 4, Op::None},   {0x001ceb40, 0xc, 1, Op::None},
    {0x001ceb43, 0xd, 1, Op::None},   {0x001ceb46, 0xe, 2, Op::None},
    {0x001ceb4c, 0x1c, 4, Op::Zero},  {0x001ceb51, 0x10, 1, Op::Zero},
    {0x001ceb54, 0x11, 1, Op::Zero},  {0x001ceb57, 0x12, 2, Op::Zero},
    {0x001ceb5b, 0x18, 4, Op::One},   {0x001ceb60, 0x14, 2, Op::Zero},
    {0x001ceb64, 0x16, 2, Op::Zero},  {0x001ceb68, 0x20, 4, Op::None},
    {0x001ceb6b, 0x24, 2, Op::None},  {0x001ceb6f, 0x26, 2, Op::None},
    {0x001ceb78, 0x28, 4, Op::None},  {0x001ceb7b, 0x2c, 1, Op::None},
    {0x001ceb7e, 0x2d, 1, Op::None},  {0x001ceb81, 0x2e, 2, Op::None},
    {0x001ceb85, 0x3c, 4, Op::Zero},  {0x001ceb8a, 0x30, 1, Op::Zero},
    {0x001ceb8d, 0x31, 1, Op::Zero},  {0x001ceb90, 0x32, 2, Op::Zero},
    {0x001ceb94, 0x38, 4, Op::One},   {0x001ceb99, 0x34, 2, Op::Zero},
    {0x001ceb9d, 0x36, 2, Op::Zero},  {0x001ceba1, 0x40, 4, Op::None},
    {0x001ceba4, 0x44, 2, Op::None},  {0x001ceba8, 0x46, 2, Op::None},
    {0x001cebb6, 0x48, 4, Op::None},  {0x001cebb9, 0x4c, 1, Op::None},
    {0x001cebbc, 0x4d, 1, Op::None},  {0x001cebbf, 0x4e, 2, Op::None},
    {0x001cebcf, 0x5c, 4, Op::Zero},  {0x001cebd4, 0x50, 1, Op::Zero},
    {0x001cebd7, 0x51, 1, Op::Zero},  {0x001cebda, 0x52, 2, Op::Zero},
    {0x001cebde, 0x58, 4, Op::One},   {0x001cebe3, 0x54, 2, Op::Zero},
    {0x001cebe7, 0x56, 2, Op::Zero},
};
constexpr Op binding_tail[]{
    {0x001cebf3, 0x65, 1, Op::Zero}, {0x001cebf6, 0x64, 1, Op::Zero},
    {0x001cebf9, 0x67, 1, Op::Zero}, {0x001cebfe, 0x66, 1, Op::ByteOne},
    {0x001cec01, 0x61, 1, Op::Zero}, {0x001cec04, 0x60, 1, Op::Zero},
    {0x001cec07, 0x63, 1, Op::Zero}, {0x001cec0a, 0x6e, 2, Op::Zero},
    {0x001cec0e, 0x80, 4, Op::Zero},
};
constexpr Op unit_prefix[]{
    {0x000c52fc, 0x238, 4, Op::None}, {0xc530e, 0x218, 4, Op::None},
    {0xc5310, 0x21c, 4, Op::None},    {0xc5313, 0x220, 4, Op::None},
    {0xc5316, 0x224, 4, Op::None},    {0x000c531b, 0x214, 2, Op::Zero},
    {0x000c5322, 0x210, 2, Op::Zero}, {0x000c5329, 0x213, 1, Op::None},
    {0x000c532f, 0x212, 1, Op::None}, {0x000c5335, 0x217, 1, Op::None},
    {0x000c533b, 0x216, 1, Op::None}, {0x000c5341, 0x23c, 1, Op::None},
    {0x000c5347, 0x23d, 1, Op::None}, {0x000c534d, 0x240, 1, Op::None},
    {0x000c5353, 0x241, 1, Op::None}, {0x000c5359, 0x13c, 4, Op::None},
    {0x000c535f, 0x140, 4, Op::None}, {0x000c5365, 0x12c, 4, Op::None},
    {0x000c536b, 0x130, 4, Op::None}, {0x000c5371, 0x1fc, 2, Op::None},
    {0x000c5378, 0x248, 4, Op::None}, {0x000c537e, 0x24c, 4, Op::None},
    {0x000c5384, 0x2a0, 4, Op::None}, {0x000c538a, 0x2a4, 2, Op::None},
    {0x000c5391, 0x2a8, 4, Op::None}, {0x000c5397, 0x2ac, 4, Op::None},
    {0x000c539d, 0x1ec, 4, Op::Zero}, {0x000c53a3, 0x2c8, 2, Op::Zero},
    {0x000c53aa, 0x2ca, 2, Op::Zero}, {0x000c53b1, 0x2cc, 4, Op::Zero},
    {0x000c53b9, 0x2d0, 4, Op::None}, {0x000c53bf, 0x2e0, 4, Op::None},
    {0x000c53c5, 0x250, 4, Op::None}, {0x000c53cb, 0x254, 4, Op::None},
    {0x000c53d1, 0x258, 1, Op::Zero}, {0x000c53d8, 0x25a, 2, Op::None},
    {0x000c53df, 0x1f4, 1, Op::Zero}, {0x000c53e6, 0x2d4, 4, Op::None},
    {0x000c53ec, 0x2ea, 2, Op::Zero}, {0x000c53f3, 0x2ec, 4, Op::None},
    {0x000c53f9, 0x1f6, 1, Op::None}, {0x000c53ff, 0x1f7, 1, Op::None},
    {0x000c5405, 0x334, 4, Op::Zero}, {0x000c540d, 0x338, 4, Op::Zero},
    {0x000c5415, 0x1e4, 4, Op::Zero}, {0x000c541d, 0x1e8, 4, Op::Zero},
    {0x000c5425, 0x1c8, 4, Op::None}, {0x000c542b, 0x1cc, 4, Op::None},
    {0x000c5431, 0x1d0, 4, Op::None}, {0x000c5437, 0x1e0, 2, Op::Zero},
};
std::span<std::byte> header(const UnitConstructorObjectPool &p,
                            std::uint32_t id) {
  const auto index = id & 0xffff;
  require(index < p.capacity &&
              std::size_t(p.capacity) <= p.headers.size() / 12,
          "Source constructor object index outside supplied pool");
  return p.headers.subspan(std::size_t(index) * 12, 12);
}
std::span<std::byte> body(const UnitConstructorObjectPool &p,
                          std::uint32_t id) {
  auto h = header(p, id);
  require(bool(p.body), "Source constructor body storage owner unavailable");
  auto b = p.body(std::uint16_t(id));
  require(read(h, 6, 2) <= b.size(),
          "Source body extent outside owned backing");
  return b.first(read(h, 6, 2));
}
UnitConstructorLeaf call(std::uint32_t entry, std::uint32_t eax, bool has_eax,
                         std::initializer_list<std::uint32_t> args) {
  require(args.size() <= 7, "Source constructor leaf ABI extent");
  UnitConstructorLeaf c{entry, eax, has_eax, {}, unsigned(args.size())};
  unsigned i = 0;
  for (auto v : args)
    c.arguments[i++] = v;
  return c;
}
void invoke(const UnitConstructorServices &s, const UnitConstructorLeaf &c) {
  require(bool(s.invoke), "Source unit constructor nested owner unavailable");
  s.invoke(c);
}
std::size_t component(std::span<const std::byte> b, std::size_t field,
                      unsigned minimum) {
  auto offset = signed_word(read(b, field + 2, 2));
  require(offset >= 0 && unsigned(offset) <= b.size() &&
              minimum <= b.size() - unsigned(offset),
          "Source signed component offset outside owned body");
  require(read(b, field, 2) >= minimum, "Source component extent truncated");
  return unsigned(offset);
}
} // namespace

void reset_source_unit_animation_manager(std::span<std::byte> b,
                                         std::uint32_t one,
                                         const UnitConstructorObserver &o) {
  require(b.size() == 0x90, "Source animation manager must have90 bytes");
  ops(b, reset, one, none, o);
}
void construct_source_unit_animation_manager(std::span<std::byte> b,
                                             std::uint32_t one,
                                             const UnitConstructorObserver &o) {
  require(b.size() == 0x90, "Source animation manager must have90 bytes");
  ops(b, construction, one, none, o);
  reset_source_unit_animation_manager(b, one, o);
}
bool bind_source_unit_animation_manager(std::span<std::byte> b,
                                        std::uint32_t graph,
                                        std::uint32_t model, bool flag,
                                        const UnitConstructorServices &s,
                                        const UnitConstructorObserver &o) {
  require(b.size() == 0x90, "Source animation manager must have90 bytes");
  if (model != none) {
    if (graph == none)
      return false;
    require(bool(s.graph_nodes) && bool(s.model_nodes),
            "Source graph/model node owner unavailable");
    if (std::int32_t(s.graph_nodes(graph)) != s.model_nodes(model))
      return false;
  }
  write(b, 0x6c, 1, flag ? (read(b, 0x6c, 1) | 2) : (read(b, 0x6c, 1) & ~2u),
        flag ? 0x1ceb0f : 0x1ceb15, UnitConstructorBuffer::Manager, o);
  ops(b, binding_prefix, s.one_bits, graph, o);
  require(bool(s.select_default),
          "Source default graph selection1d0a60 unavailable");
  s.select_default(b, call(0x1d0a60, 0x07000001, true,
                           {0x07000001, 0x07000001, 0x07000001, 0x317, 0x3f}));
  ops(b, binding_tail, s.one_bits, graph, o);
  return true;
}
bool append_source_unit_component(std::uint32_t id, std::int16_t field,
                                  std::int16_t n, std::uint8_t align,
                                  const UnitConstructorObjectPool &p,
                                  const UnitConstructorObserver &o) {
  auto h = header(p, id);
  auto b = body(p, id);
  require(field >= 0 && std::size_t(field) <= b.size() &&
              4 <= b.size() - std::size_t(field),
          "Source descriptor outside owned body");
  if (n == 0) {
    write(b, unsigned(field) + 2, 2, none, 0xbd2a8, UnitConstructorBuffer::Unit,
          o);
    write(b, unsigned(field), 2, 0, 0xbd2ae, UnitConstructorBuffer::Unit, o);
    return true;
  }
  const std::uint32_t mask = (std::uint32_t(1) << (align & 31)) - 1;
  const std::uint32_t extra = std::uint32_t(std::int32_t(n)) + mask;
  const auto target = std::bit_cast<std::int32_t>(
      std::uint32_t(signed_word(read(h, 6, 2))) + extra);
  require(bool(p.grow), "Source dynamic body growth18fd70 unavailable");
  if (!p.grow(id, target))
    return false;
  // Original growth changes backing/header+8; it leaves header+6 toBD270.
  const auto old = read(h, 6, 2);
  require(bool(p.body), "Source body storage owner unavailable");
  b = p.body(std::uint16_t(id));
  const auto start = signed_word(old);
  require(start >= 0 && std::size_t(start) <= b.size() &&
              extra <= b.size() - std::size_t(start),
          "Source padded append outside owned backing");
  require(std::size_t(field) <= b.size() && 4 <= b.size() - std::size_t(field),
          "Relocated descriptor outside owned backing");
  write(h, 6, 2, old + extra, 0xbd2fc, UnitConstructorBuffer::Header, o);
  write(b, unsigned(field) + 2, 2, (old + mask) & ~mask, 0xbd317,
        UnitConstructorBuffer::Unit, o);
  write(b, unsigned(field), 2, std::uint16_t(n), 0xbd320,
        UnitConstructorBuffer::Unit, o);
  std::size_t at = unsigned(start);
  for (std::uint32_t i = 0; i < (extra >> 2); ++i, at += 4)
    write(b, at, 4, 0, 0xbd333, UnitConstructorBuffer::Unit, o);
  for (unsigned i = 0; i < (extra & 3); ++i, ++at)
    write(b, at, 1, 0, 0xbd33a, UnitConstructorBuffer::Unit, o);
  return true;
}
bool construct_source_unit(std::uint32_t id,
                           std::span<const std::byte> descriptor,
                           std::span<std::byte> failure,
                           const UnitConstructorServices &s,
                           const UnitConstructorObserver &o) {
  require(descriptor.size() >= 0xc4 && failure.size() >= 1,
          "Truncated source unit construction request");
  auto b = body(s.objects, id);
  require(b.size() >= 0x348, "Truncated source unit construction body");
  require(bool(s.unit_definition), "Source unit definition owner unavailable");
  const auto definition = s.unit_definition(read(b, 0));
  require(definition.size() >= 0x1b8, "Truncated source unit definition");
  ops(b, unit_prefix, s.one_bits, none, o, UnitConstructorBuffer::Unit);
  for (unsigned i = 0; i < 4; ++i) {
    const auto at = 0x2f0 + i * 16;
    write(b, at, 4, none, 0xc5446, UnitConstructorBuffer::Unit, o);
    write(b, at + 4, 4, 0, 0xc5449, UnitConstructorBuffer::Unit, o);
    write(b, at + 8, 4, none, 0xc544d, UnitConstructorBuffer::Unit, o);
    write(b, at + 12, 4, none, 0xc5450, UnitConstructorBuffer::Unit, o);
    write(b, 0x330 + i, 1, 0, 0xc5453, UnitConstructorBuffer::Unit, o);
  }
  if (read(body(s.objects, id), 0x12a, 2) == 0xffff)
    return false;
  for (const auto [field, n] :
       {std::pair<std::int16_t, std::int16_t>{0x33c, 0x124},
        {0x340, 0x58},
        {0x344, 0x44}}) {
    if (!append_source_unit_component(id, field, n, 0, s.objects, o)) {
      write(failure, 0, 1, 1, 0xc54c7, UnitConstructorBuffer::Failure, o);
      return false;
    }
  }
  b = body(s.objects, id);
  const auto control = component(b, 0x344, 0x44);
  write(b, control + 0x10, 4, none, 0xc54e3, UnitConstructorBuffer::Unit, o);
  write(b, control + 0x14, 2, none, 0xc54e6, UnitConstructorBuffer::Unit, o);
  write(b, control + 0x18, 4, none, 0xc54ea, UnitConstructorBuffer::Unit, o);
  write(b, control + 0x2c, 4, none, 0xc54ed, UnitConstructorBuffer::Unit, o);
  const auto extension = component(b, 0x33c, 0x124);
  write(b, extension + 0x2c, 4, none, 0xc54f8, UnitConstructorBuffer::Unit, o);
  invoke(s, call(0x110a20, 0, false, {id}));
  b = body(s.objects, id);
  const auto extra = component(b, 0x340, 0x58);
  write(b, extra, 4, none, 0xc550a, UnitConstructorBuffer::Unit, o);
  write(b, extra + 0x44, 4, none, 0xc550c, UnitConstructorBuffer::Unit, o);
  invoke(s, call(0x116680, id, true, {read(descriptor, 0xc)}));
  b = body(s.objects, id);
  const std::uint32_t copy_ips[5][3]{{0xc552b, 0xc5533, 0xc5536},
                                     {0xc5543, 0xc554b, 0xc554e},
                                     {0xc555b, 0xc5563, 0xc5566},
                                     {0xc5573, 0xc557b, 0xc5582},
                                     {0xc558d, 0xc5595, 0xc5598}};
  const unsigned targets[]{0x18c, 0x180, 0x168, 0x15c, 0x150};
  for (unsigned j = 0; j < 5; ++j)
    for (unsigned i = 0; i < 3; ++i)
      write(b, targets[j] + 4 * i, 4, read(b, 0x70 + 4 * i), copy_ips[j][i],
            UnitConstructorBuffer::Unit, o);
  write(b, 0x14c, 4, 0x06000086, 0xc559b, UnitConstructorBuffer::Unit, o);
  const auto grenade = signed_word(read(definition, 0x1b4, 2));
  if (grenade >= 0 && grenade < 2 &&
      signed_word(read(definition, 0x1b6, 2)) >= 0)
    write(b, 0x23e + unsigned(grenade), 1, read(definition, 0x1b6, 1), 0xc55ca,
          UnitConstructorBuffer::Unit, o);
  write(b, 4, 4, read(b, 4) | 0x1000, 0xc55df, UnitConstructorBuffer::Unit, o);
  auto category = read(descriptor, 0x64, 2);
  write(b, 0x138, 2, category, 0xc55ea, UnitConstructorBuffer::Unit, o);
  // C55D4 resolves the game pointer; the actual game+8 read is C55F1,
  // after the flag/category stores. This provider is that value read.
  require(bool(s.game_type), "Source game type owner unavailable");
  const auto game_type = s.game_type();
  if (game_type == 1 && (category == 0 || category == 0xffff))
    write(b, 0x138, 2, read(definition, 0xc0, 2), 0xc5608,
          UnitConstructorBuffer::Unit, o);
  invoke(s, call(0x110bf0, 2, true,
                 {id, 0x07000001, 0x07000101, 0x07000101, 0x07000001, 0, 0}));
  b = body(s.objects, id);
  const auto model = read(definition, 0x38);
  std::span<const std::byte> regions;
  if (model != none) {
    require(bool(s.model_node_records),
            "Source unit model node owner unavailable");
    regions = s.model_node_records(model);
    require(regions.size() % 0x5c == 0,
            "Truncated source unit model node block");
  }
  auto index = [&](std::uint32_t name) {
    for (std::size_t i = 0; i < regions.size() / 0x5c; ++i) {
      if (read(regions, i * 0x5c) == name)
        return std::uint32_t(i);
    }
    return none;
  };
  write(b, 0x1f6, 1, index(0x080001a2), 0xc5674, UnitConstructorBuffer::Unit,
        o);
  const auto second = index(0x090001a3);
  // Miss with a nonempty model takes the early-return store atC56B1.
  write(b, 0x1f7, 1, second,
        (model != none && !regions.empty() && second == none) ? 0xc56b1
                                                              : 0xc56c2,
        UnitConstructorBuffer::Unit, o);
  return true;
}
} // namespace halo2
