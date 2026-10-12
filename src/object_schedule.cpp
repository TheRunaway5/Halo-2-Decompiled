#include "halo2/object_schedule.hpp"
#include <bit>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
std::uint32_t word(std::span<const std::byte> bytes, std::size_t offset,
                   unsigned count) {
  require(offset <= bytes.size() && count <= bytes.size() - offset,
          "Truncated original object-schedule storage");
  std::uint32_t result = 0;
  for (unsigned i = 0; i < count; ++i)
    result |= std::uint32_t(std::to_integer<unsigned>(bytes[offset + i])) << (8 * i);
  return result;
}
void validate(const SourceObjectScheduleStorage &storage) {
  require(storage.pool.size() >= 0x4c && storage.unit_queue.size() >= 0x1d88 &&
              storage.update_context.size() >= 4,
          "Original object-schedule owning storage is unavailable");
  require(word(storage.pool, 0x24, 4) == 12,
          "Unsupported original object-header stride");
}
void store(std::span<std::byte> bytes, std::size_t offset, std::uint8_t after,
           std::uint32_t instruction, SourceObjectScheduleArea area,
           const SourceObjectScheduleObserver &observe) {
  const auto before = std::uint8_t(word(bytes, offset, 1));
  bytes[offset] = std::byte(after);
  if (observe) observe({instruction, area, offset, before, after});
}
std::size_t header(const SourceObjectScheduleStorage &storage,
                   std::uint32_t datum) {
  const auto offset = std::size_t(datum & 0xffffu) * 12;
  require(offset <= storage.headers.size() && 12 <= storage.headers.size() - offset,
          "Original object-schedule header index is outside native storage");
  return offset;
}
std::uint8_t flags(const SourceObjectScheduleStorage &storage, std::size_t offset) {
  return std::uint8_t(word(storage.headers, offset + 2, 1));
}
std::uint32_t full_datum(const SourceObjectScheduleStorage &storage,
                         std::int32_t index) {
  const auto offset = header(storage, std::uint32_t(index));
  return (word(storage.headers, offset, 2) << 16) | std::uint32_t(index);
}
enum class ScanEnd { none, negative, range, bitmap };
struct ScanResult { std::int32_t index; ScanEnd end; };
ScanResult next(const SourceObjectScheduleStorage &storage, std::int32_t previous) {
  // The source rereads high-water and the occupied bits at each scan. A leaf
  // may change those fields; the dispatcher does not copy a visit list.
  require(previous < std::numeric_limits<std::int32_t>::max(),
          "Original object-schedule cursor overflow");
  auto index = previous + 1;
  if (index < 0) return {index, ScanEnd::negative};
  const auto limit = std::bit_cast<std::int32_t>(word(storage.pool, 0x38, 4));
  if (index >= limit) return {index, ScanEnd::range};
  for (; index < limit; ++index) {
    const auto bits = word(storage.bitmap, std::size_t(index / 32) * 4, 4);
    if (bits & (std::uint32_t(1) << (unsigned(index) & 31)))
      return {index, ScanEnd::none};
  }
  return {index, ScanEnd::bitmap};
}
bool queued(const SourceObjectScheduleServices &services, std::uint32_t datum) {
  require(bool(services.object_body), "Original object body storage is unavailable");
  return (word(services.object_body(datum), 0xc0, 1) & 8) != 0;
}
void ordinary(const SourceObjectScheduleServices &services, std::uint32_t datum) {
  require(bool(services.ordinary_update), "Original ordinary object producer is unavailable");
  services.ordinary_update(datum);
}
void dirty(const SourceObjectScheduleServices &services, std::uint32_t datum) {
  require(bool(services.dirty_update), "Original dirty object producer is unavailable");
  services.dirty_update(datum);
}
void finish_dirty(const SourceObjectScheduleServices &services) {
  require(bool(services.finish_dirty_updates), "Original dirty object cleanup is unavailable");
  services.finish_dirty_updates();
}
} // namespace

void source_ordinary_object_schedule(SourceObjectScheduleStorage storage,
    const SourceObjectScheduleServices &services, const SourceObjectScheduleObserver &observe) {
  validate(storage);
  const auto count = std::bit_cast<std::int32_t>(word(storage.unit_queue, 0x1d80, 4));
  store(storage.update_context, 3, 1, 0xb7e24, SourceObjectScheduleArea::update_context, observe);
  for (std::int32_t i = 0; i < count; ++i) {
    const auto datum = word(storage.unit_queue, 0x1d00 + std::size_t(i) * 4, 4);
    const auto at = header(storage, datum);
    auto value = flags(storage, at);
    if (!(value & 1) || !(value & 2) || (value & 8)) continue;
    ordinary(services, datum);
    value = flags(storage, at);
    if (!(value & 4) || (value & 8)) continue;
    require(bool(services.queue_dirty_update), "Original dirty-unit queue producer is unavailable");
    services.queue_dirty_update(datum);
  }
  std::int32_t previous = -1;
  for (;;) {
    const auto scan = next(storage, previous);
    if (scan.end != ScanEnd::none) {
      const auto instruction = scan.end == ScanEnd::negative ? 0xb7f33u :
          scan.end == ScanEnd::range ? 0xb7f43u : 0xb7ec4u;
      store(storage.update_context, 3, 0, instruction,
            SourceObjectScheduleArea::update_context, observe);
      return;
    }
    previous = scan.index;
    const auto datum = full_datum(storage, previous);
    const auto value = flags(storage, header(storage, datum));
    if (!(value & 1) || !(value & 2) || (value & 8)) continue;
    if (!queued(services, datum)) ordinary(services, datum);
  }
}

void source_dirty_object_schedule(SourceObjectScheduleStorage storage,
    const SourceObjectScheduleServices &services, const SourceObjectScheduleObserver &observe) {
  validate(storage);
  const auto count = std::bit_cast<std::int32_t>(word(storage.unit_queue, 0x1d80, 4));
  store(storage.update_context, 3, 1, 0xb7f6e, SourceObjectScheduleArea::update_context, observe);
  for (std::int32_t i = 0; i < count; ++i) {
    const auto datum = word(storage.unit_queue, 0x1d00 + std::size_t(i) * 4, 4);
    const auto value = flags(storage, header(storage, datum));
    if (!(value & 4) || (value & 8)) continue;
    dirty(services, datum);
    require(bool(services.finish_queued_dirty_update), "Original dirty-unit queue removal is unavailable");
    services.finish_queued_dirty_update(datum, i);
  }
  std::int32_t previous = -1;
  for (;;) {
    const auto scan = next(storage, previous);
    if (scan.end != ScanEnd::none) break;
    previous = scan.index;
    const auto datum = full_datum(storage, previous);
    const auto value = flags(storage, header(storage, datum));
    if (!(value & 4) || (value & 8)) continue;
    if (!queued(services, datum)) dirty(services, datum);
  }
  finish_dirty(services);
  store(storage.update_context, 3, 0, 0xb7fef, SourceObjectScheduleArea::update_context, observe);
}

void source_new_object_schedule(SourceObjectScheduleStorage storage,
    const SourceObjectScheduleServices &services, const SourceObjectScheduleObserver &observe) {
  validate(storage);
  store(storage.update_context, 3, 1, 0xb804f, SourceObjectScheduleArea::update_context, observe);
  std::int32_t previous = -1;
  for (;;) {
    const auto scan = next(storage, previous);
    if (scan.end != ScanEnd::none) break;
    previous = scan.index;
    const auto datum = full_datum(storage, previous);
    const auto at = header(storage, datum);
    const auto value = std::uint8_t(flags(storage, at) & 0xdf);
    store(storage.headers, at + 2, value, 0xb8104, SourceObjectScheduleArea::header, observe);
    if (!(value & 8) || !(value & 1) || !(value & 2) || (value & 0x10)) continue;
    store(storage.headers, at + 2, std::uint8_t(value & 0xf7), 0xb8128,
          SourceObjectScheduleArea::header, observe);
    ordinary(services, datum);
    if (flags(storage, at) & 4) dirty(services, datum);
  }
  finish_dirty(services);
  previous = -1;
  for (;;) {
    const auto scan = next(storage, previous);
    if (scan.end != ScanEnd::none) break;
    previous = scan.index;
    const auto datum = full_datum(storage, previous);
    if (!(flags(storage, header(storage, datum)) & 0x10)) continue;
    require(bool(services.pending_delete_first), "Original pending-object first removal is unavailable");
    services.pending_delete_first(datum, 1);
    require(bool(services.pending_delete_second), "Original pending-object second removal is unavailable");
    services.pending_delete_second(datum, 1);
  }
  store(storage.update_context, 3, 0, 0xb80d5, SourceObjectScheduleArea::update_context, observe);
  require(bool(services.finish_new_updates), "Original new-object cleanup is unavailable");
  services.finish_new_updates();
}
} // namespace halo2
