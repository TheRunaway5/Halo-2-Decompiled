#pragma once
#include "halo2/unit_constructors.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
enum class UnitActionBuffer { Header, Body, Request, View };
struct UnitActionStore {
  std::uint32_t instruction, identity;
  UnitActionBuffer buffer;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const UnitActionStore &, const UnitActionStore &) = default;
};
using UnitActionObserver = std::function<void(const UnitActionStore &)>;

// Complete BD170. These services execute the actual header allocator, owned
// body allocation and failure release. Body allocation updates header+8 but
// leaves header+6 to BD170. No allocation result or initial bytes are supplied
// by this owner; its original stores clear only the requested backing extent.
struct SourceObjectBodyAllocationServices {
  UnitConstructorObjectPool objects;
  std::function<std::uint32_t()> allocate_header;
  std::function<bool(std::uint32_t datum, std::int32_t bytes)> allocate_body;
  std::function<void(std::uint32_t datum)> release_header;
};
std::uint32_t allocate_source_object_body(std::int16_t bytes,
    const SourceObjectBodyAllocationServices &, const UnitActionObserver & = {});

enum class UnitActionCallback { Begin, Update, Interrupt };
struct UnitActionCall {
  // Literal callback address imported from the selected image's actual table.
  std::uint32_t entry, unit, type;
  UnitActionCallback callback;
  // Exactly20h for Begin; absent for Update/Interrupt. Valid only during call.
  std::span<const std::byte> request;
};
struct SourceUnitActionServices {
  UnitConstructorObjectPool objects;
  // Original60 pointers at46ec68 -> literal16-byte records. Original image
  // addresses are opaque callback identities, never executable host pointers.
  std::function<std::span<const std::byte>(std::uint32_t type)> descriptor;
  // Required callback owner. Defined low AL is consumed for begin/update;
  // interruption ignores its return. Missing callbacks never mean completion.
  std::function<std::uint8_t(const UnitActionCall &)> invoke;
  // Original1eb150 reads player+28 by low16 index. The provider must retain
  // source constructor-owned records; a native DatumPool zero slot is not a
  // constructed source player or a produced local-user assignment.
  std::function<std::span<const std::byte>(std::uint32_t player)> player_body;
  // Original local-user view record has1b0 bytes, with four8-byte notifications
  // at+150. Require the real view constructor/assignment; no view-index default.
  std::function<std::span<std::byte>(std::int16_t local_user)> view_record;
  // Actual source clock+8, read only when the local notification branch runs.
  std::function<std::int32_t()> source_tick;
};

// Complete B8250: an already-set initial bit2 returns immediately; otherwise
// set bit2 on that object and every parent throughNONE, without rechecking an
// ancestor's prior bit. Caller owns valid original storage.
void mark_source_object_for_update(std::uint32_t unit,
    const UnitConstructorObjectPool &, const UnitActionObserver & = {});
// Complete1ea310; ties keep the earliest slot, comparisons are signed.
void record_source_unit_action(std::span<std::byte> view_record,
    std::int16_t local_user, std::int32_t tick, std::uint32_t type,
    std::uint8_t result, const UnitActionObserver & = {});
// Complete1eb150, including its tail-jumped1ea310 local branch.
void notify_source_unit_action(std::uint32_t player, std::uint32_t type,
    std::uint8_t result, const SourceUnitActionServices &,
    const UnitActionObserver & = {});
// Follows the actual signed unit+346 offset. No component/manager is created.
std::span<std::byte> source_unit_action_component(std::span<std::byte> body);

// Complete E7F30/E7EF0/E7FF0/E7E60 direct owners. Required nested callbacks
// operate on the same owned header/body/components. Those backing spans must
// remain valid throughout each call, as the source retains component pointers.
std::uint8_t dispatch_source_unit_action(std::uint32_t unit,
    std::span<const std::byte> request, const SourceUnitActionServices &,
    const UnitActionObserver & = {});
std::uint8_t begin_source_unit_action(std::uint32_t unit, std::uint32_t type,
    const SourceUnitActionServices &, const UnitActionObserver & = {});
void interrupt_source_unit_action(std::uint32_t unit, std::uint32_t type,
    const SourceUnitActionServices &, const UnitActionObserver & = {});
bool update_source_unit_actions(std::uint32_t unit,
    const SourceUnitActionServices &, const UnitActionObserver & = {});
} // namespace halo2
