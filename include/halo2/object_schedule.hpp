#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// B7E00/B7F50/B8040 consume the original object pool, occupied bitmap,
// 1D88-byte unit queue and update-context byte. Address words in the raw
// records remain opaque; the caller resolves body storage by full datum.
struct SourceObjectScheduleStorage {
  std::span<std::byte> pool, headers, bitmap, unit_queue, update_context;
};
struct SourceObjectScheduleServices {
  std::function<std::span<const std::byte>(std::uint32_t)> object_body;
  std::function<void(std::uint32_t)> ordinary_update, dirty_update;
  std::function<void(std::uint32_t)> queue_dirty_update;
  std::function<void(std::uint32_t, std::int32_t)> finish_queued_dirty_update;
  std::function<void()> finish_dirty_updates, finish_new_updates;
  std::function<void(std::uint32_t, std::uint32_t)> pending_delete_first,
      pending_delete_second;
};
enum class SourceObjectScheduleArea { header, update_context };
struct SourceObjectScheduleStore {
  std::uint32_t instruction;
  SourceObjectScheduleArea area;
  std::size_t offset;
  std::uint8_t before, after;
  friend bool operator==(const SourceObjectScheduleStore &,
                         const SourceObjectScheduleStore &) = default;
};
using SourceObjectScheduleObserver =
    std::function<void(const SourceObjectScheduleStore &)>;

// These are the owning object dispatchers, not the BD360/BD710 producers,
// Havok world step, queue leaves or deletion/cleanup implementations. A needed
// unavailable service fails at its source phase; no result is manufactured.
// Storage must remain in place while callbacks execute, including the header
// pointer retained across an ordinary update. Only the original 12-byte
// object-header layout is supported.
void source_ordinary_object_schedule(SourceObjectScheduleStorage,
    const SourceObjectScheduleServices &, const SourceObjectScheduleObserver & = {});
void source_dirty_object_schedule(SourceObjectScheduleStorage,
    const SourceObjectScheduleServices &, const SourceObjectScheduleObserver & = {});
void source_new_object_schedule(SourceObjectScheduleStorage,
    const SourceObjectScheduleServices &, const SourceObjectScheduleObserver & = {});
} // namespace halo2
