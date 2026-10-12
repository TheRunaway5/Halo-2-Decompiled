#pragma once
#include <cstdint>
#include <functional>
#include <optional>
namespace halo2 {
struct ObjectGarbageState {
  std::uint32_t flags{}, next = 0xffffffffu;
  std::int32_t deadline{};
};
struct ObjectGarbageQueue {
  std::uint32_t head = 0xffffffffu;
  std::int16_t active_count{};
};
// TU BC840. Lookup supplies full object identity and retained body storage;
// active is original header+2 bit0. This operation never deletes an object.
void schedule_object_garbage(ObjectGarbageQueue &, std::uint32_t datum,
    ObjectGarbageState &, bool active, bool enable, std::uint32_t tick,
    std::int32_t interval,
    const std::function<ObjectGarbageState &(std::uint32_t)> &lookup);
// TU C0270 dispatcher inputs. Heap/request producers are caller boundaries.
int object_garbage_level(bool requested, bool unretained_only,
    std::int32_t remaining_heap, std::int32_t allocated_objects,
    std::int16_t active_garbage);
struct ObjectGarbagePressure { bool proceed{}, critical{}; };
ObjectGarbagePressure object_garbage_pressure(int level,
    std::int32_t remaining_heap, std::int32_t free_highwater_headers,
    std::int16_t active_garbage);
// TU C01B0 removal gate, after candidate enumeration. A missing retention
// result stays unresolved; marker/view state must come from its real producer.
std::optional<bool> object_garbage_collectible(int level, std::uint32_t tick,
    const ObjectGarbageState &, bool active, std::optional<bool> retained);
}
