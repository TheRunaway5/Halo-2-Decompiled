#include "halo2/object_garbage.hpp"
#include <algorithm>
#include <bit>
namespace halo2 {
void schedule_object_garbage(ObjectGarbageQueue &queue, std::uint32_t datum,
    ObjectGarbageState &body, bool active, bool enable, std::uint32_t tick,
    std::int32_t interval,
    const std::function<ObjectGarbageState &(std::uint32_t)> &lookup) {
  if (enable) {
    if (!(body.flags & 0xc000u)) {
      body.next = queue.head; queue.head = datum; body.flags |= 0x4000u;
      if (active) queue.active_count = std::bit_cast<std::int16_t>(
          std::uint16_t(std::uint16_t(queue.active_count) + 1u));
    }
    const auto deadline = std::bit_cast<std::int32_t>(tick + std::uint32_t(interval));
    body.deadline = std::max(body.deadline, deadline);
  } else if (body.flags & 0x4000u) {
    auto *link = &queue.head;
    while (*link != datum) link = &lookup(*link).next;
    *link = body.next; body.next = 0xffffffffu; body.flags &= ~0x4000u;
    if (active) queue.active_count = std::bit_cast<std::int16_t>(
        std::uint16_t(std::uint16_t(queue.active_count) - 1u));
  }
}
int object_garbage_level(bool requested, bool unretained_only,
    std::int32_t remaining_heap, std::int32_t allocated_objects,
    std::int16_t active_garbage) {
  if (requested) return unretained_only ? 0 : 1;
  if (remaining_heap <= 0xcccc || 0x800 - allocated_objects <= 0x66) return 3;
  return active_garbage >= 50 ? 2 : -1;
}
ObjectGarbagePressure object_garbage_pressure(int level,
    std::int32_t remaining_heap, std::int32_t free_highwater_headers,
    std::int16_t active_garbage) {
  if (level < 2) return {true, false};
  if (level == 2) return {active_garbage > 30, false};
  return {remaining_heap < 0x19999 || free_highwater_headers < 0xcc,
          remaining_heap < 0x6666 || free_highwater_headers < 0x33};
}
std::optional<bool> object_garbage_collectible(int level, std::uint32_t tick,
    const ObjectGarbageState &body, bool active, std::optional<bool> retained) {
  if (std::bit_cast<std::int32_t>(tick) < body.deadline || (level == 2 && !active))
    return false;
  if (level == 0) return true;
  if (!retained) return std::nullopt;
  return !*retained;
}
}
