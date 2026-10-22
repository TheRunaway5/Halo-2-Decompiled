#include "halo2/unit_action_dispatch.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at, unsigned n = 4) {
  require(at <= b.size() && n <= b.size() - at, "Truncated source action backing");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
void write(std::span<std::byte> b, std::size_t at, unsigned n,
    std::uint32_t value, std::uint32_t instruction, std::uint32_t id,
    UnitActionBuffer kind, const UnitActionObserver &o) {
  const auto before = read(b, at, n);
  for (unsigned i = 0; i < n; ++i) b[at + i] = std::byte((value >> (8 * i)) & 255);
  if (o) o({instruction, id, kind, at, n, before, read(b, at, n)});
}
std::span<std::byte> header(const UnitConstructorObjectPool &p, std::uint32_t id) {
  const auto index = id & 0xffffu;
  require(index < p.capacity && std::size_t(p.capacity) <= p.headers.size() / 12,
      "Source action header index outside owned pool");
  return p.headers.subspan(std::size_t(index) * 12, 12);
}
std::span<std::byte> body(const UnitConstructorObjectPool &p, std::uint32_t id) {
  const auto h = header(p, id);
  require(bool(p.body), "Source action body owner unavailable");
  auto b = p.body(std::uint16_t(id));
  const auto size = read(h, 6, 2);
  require(size <= b.size(), "Source action header extent exceeds owned body");
  return b.first(size);
}
std::uint32_t callback(const SourceUnitActionServices &s, std::uint32_t type,
    std::size_t offset) {
  require(type < 60, "Source action type outside original60-entry table");
  require(bool(s.descriptor), "Original source action descriptor provider unavailable");
  const auto d = s.descriptor(type);
  require(d.size() == 16, "Source action descriptor must retain16 bytes");
  return read(d, offset);
}
std::uint8_t invoke(const SourceUnitActionServices &s, std::uint32_t target,
    std::uint32_t unit, std::uint32_t type, UnitActionCallback kind,
    std::span<const std::byte> request = {}) {
  require(target != 0, "Original source action begin calls a NULL callback");
  require(bool(s.invoke), "Source action callback owner unavailable");
  return s.invoke({target, unit, type, kind, request});
}
} // namespace

std::uint32_t allocate_source_object_body(std::int16_t bytes,
    const SourceObjectBodyAllocationServices &s, const UnitActionObserver &o) {
  require(bool(s.allocate_header), "Source16d2c0 header allocator unavailable");
  const auto id = s.allocate_header();
  if (id == none) return none;
  auto h = header(s.objects, id);
  require(bool(s.allocate_body), "Source18fce0 body allocation owner unavailable");
  if (!s.allocate_body(id, std::int32_t(bytes))) {
    require(bool(s.release_header), "Source16d4c0 failed-body header release unavailable");
    s.release_header(id);
    return none;
  }
  // Source accepts a signed allocation size; malformed successful backing for
  // a negative request cannot be cleared safely by a host and is a hard stop.
  require(bytes >= 0, "Source body allocator admitted a negative extent");
  write(h, 6, 2, std::uint16_t(bytes), 0xbd1b6, id, UnitActionBuffer::Header, o);
  require(bool(s.objects.body), "Allocated source body backing unavailable");
  auto b = s.objects.body(std::uint16_t(id));
  const auto n = std::size_t(bytes);
  require(n <= b.size(), "Source allocator returned undersized owned backing");
  std::size_t at = 0;
  for (; at < (n & ~std::size_t(3)); at += 4)
    write(b, at, 4, 0, 0xbd1c6, id, UnitActionBuffer::Body, o);
  for (; at < n; ++at)
    write(b, at, 1, 0, 0xbd1cd, id, UnitActionBuffer::Body, o);
  return id;
}

void mark_source_object_for_update(std::uint32_t id,
    const UnitConstructorObjectPool &p, const UnitActionObserver &o) {
  // Original first reads the header flag even forNONE. Host bounds reject that
  // malformed access; NONE is not converted into a fabricated harmless call.
  auto h = header(p, id);
  if ((read(h, 2, 1) & 2) || id == none) return;
  for (std::uint32_t visits = 0; ; ++visits) {
    require(visits < p.capacity, "Source object parent chain does not terminate");
    h = header(p, id);
    write(h, 2, 1, read(h, 2, 1) | 2, 0xb8288, id, UnitActionBuffer::Header, o);
    id = read(body(p, id), 0x14);
    if (id == none) return;
    // The source loop intentionally does not recheck an ancestor's update bit.
  }
}

void record_source_unit_action(std::span<std::byte> view, std::int16_t local,
    std::int32_t tick, std::uint32_t type, std::uint8_t result,
    const UnitActionObserver &o) {
  require(view.size() >= 0x1b0, "Source action notification needs the owned1b0 view record");
  unsigned slot = 0;
  auto oldest = std::bit_cast<std::int32_t>(read(view, 0x150));
  for (unsigned i = 1; i < 4; ++i) {
    const auto candidate = std::bit_cast<std::int32_t>(read(view, 0x150 + i * 8));
    if (candidate < oldest) { slot = i; oldest = candidate; }
  }
  const auto at = 0x150 + slot * 8;
  const auto identity = std::uint32_t(std::int32_t(local));
  write(view, at, 4, std::uint32_t(tick), 0x1ea345, identity, UnitActionBuffer::View, o);
  write(view, at + 4, 2, type, 0x1ea34d, identity, UnitActionBuffer::View, o);
  write(view, at + 6, 2, result, 0x1ea358, identity, UnitActionBuffer::View, o);
}

void notify_source_unit_action(std::uint32_t player, std::uint32_t type,
    std::uint8_t result, const SourceUnitActionServices &s, const UnitActionObserver &o) {
  require(bool(s.player_body), "Constructed source player record unavailable for1eb150");
  const auto p = s.player_body(player);
  const auto local = std::bit_cast<std::int16_t>(std::uint16_t(read(p, 0x28, 2)));
  if (local == -1) return;
  require(bool(s.view_record), "Source local-user view ownership unavailable");
  auto v = s.view_record(local);
  require(bool(s.source_tick), "Source action notification clock owner unavailable");
  record_source_unit_action(v, local, s.source_tick(), type, result, o);
}

std::span<std::byte> source_unit_action_component(std::span<std::byte> b) {
  const auto offset = std::bit_cast<std::int16_t>(std::uint16_t(read(b, 0x346, 2)));
  require(offset >= 0 && std::size_t(offset) <= b.size() &&
      0x44 <= b.size() - std::size_t(offset),
      "Source action component offset outside constructed unit body");
  return b.subspan(std::size_t(offset), 0x44);
}

std::uint8_t dispatch_source_unit_action(std::uint32_t id, std::span<const std::byte> request,
    const SourceUnitActionServices &s, const UnitActionObserver &o) {
  require(request.size() == 32, "Source action begin request must have20h bytes");
  const auto type = read(request, 0);
  const auto target = callback(s, type, 0);
  mark_source_object_for_update(id, s.objects, o);
  const auto result = invoke(s, target, id, type, UnitActionCallback::Begin, request);
  if (id != none) {
    const auto player = read(body(s.objects, id), 0x13c);
    if (player != none) notify_source_unit_action(player, type, result, s, o);
  }
  return result;
}

std::uint8_t begin_source_unit_action(std::uint32_t id, std::uint32_t type,
    const SourceUnitActionServices &s, const UnitActionObserver &o) {
  std::array<std::byte, 32> request; // All bytes are written by the source owner.
  constexpr std::uint32_t instructions[]{0xe7ef5, 0xe7ef8, 0xe7efc, 0xe7f00,
      0xe7f04, 0xe7f08, 0xe7f0c, 0xe7f11};
  // Observer prior bytes cannot read uninitialized C++ storage. This request
  // has no externally owned incoming backing; only its resulting stores have
  // a defined native before value, so initialize it for host instrumentation.
  request.fill(std::byte{});
  for (unsigned i = 0; i < 8; ++i)
    write(request, i * 4, 4, 0, instructions[i], id, UnitActionBuffer::Request, o);
  write(request, 0, 4, type, 0xe7f1d, id, UnitActionBuffer::Request, o);
  return dispatch_source_unit_action(id, request, s, o);
}

void interrupt_source_unit_action(std::uint32_t id, std::uint32_t type,
    const SourceUnitActionServices &s, const UnitActionObserver &o) {
  auto b = body(s.objects, id);
  auto component = source_unit_action_component(b);
  const auto base = std::size_t(component.data() - b.data());
  const auto target = callback(s, type, 12);
  if (target) invoke(s, target, id, type, UnitActionCallback::Interrupt);
  const auto at = 4 + (type >> 5) * 4;
  write(b, base + at, 4, read(component, at) & ~(1u << (type & 31)),
      0xe8040, id, UnitActionBuffer::Body, o);
}

bool update_source_unit_actions(std::uint32_t id, const SourceUnitActionServices &s,
    const UnitActionObserver &o) {
  auto b = body(s.objects, id);
  auto component = source_unit_action_component(b);
  const auto base = std::size_t(component.data() - b.data());
  bool retained = false;
  for (std::uint32_t type = 0; type < 60; ++type) {
    const auto at = 4 + (type >> 5) * 4, mask = 1u << (type & 31);
    if (!(read(component, at) & mask)) continue;
    const auto target = callback(s, type, 4);
    if (!target) continue;
    if (invoke(s, target, id, type, UnitActionCallback::Update)) retained = true;
    else write(b, base + at, 4, read(component, at) & ~mask,
        0xe7ed3, id, UnitActionBuffer::Body, o);
  }
  return retained;
}
} // namespace halo2
