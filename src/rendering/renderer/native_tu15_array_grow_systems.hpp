#pragma once
// Native TU1.5 array grow, entered at 2E3250 (scene device constructor
// unit): a growing array's storage doubled.
//
//   array_grow::grow   2E3250(array*, element size) (cdecl)
//
// An array (+0 its data, +4 its count, +8 its capacity; the capacity's sign
// bit marks storage it does not own) takes twice its count (one for an
// empty array) from the allocator at [48C938] (its vtable's +0x10: the
// size and the tag 0x12), copies its elements there (2E2E30), and gives its
// old storage back (the vtable's +0x14: the data, the old capacity's size
// and the tag) unless it did not own it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_array_grow.hpp) maps the role to the original instruction and
// owns the ABI of the children; the array, the element size, the new data
// and the new capacity, which the original keeps in ESI, EBX, EDI and EBP
// across them, are a Kept, refreshed from the registers each returned. Each
// call's target is read at its CALL, after its arguments are pushed.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_array_grow {
using native_object_update::at;
using native_object_update::signed_value;
namespace globals {inline constexpr std::uint32_t allocator=0x48c938u;}   // the allocator object (+0 its vtable)
namespace vtable {inline constexpr std::uint32_t allocate=0x10u,release=0x14u;}
namespace array {inline constexpr std::uint32_t data=0u,count=4u,capacity=8u;}
inline constexpr std::uint32_t tag=0x12u,unowned=0x80000000u;

namespace array_grow {
enum class Access {array,count,size,allocator,vtable,allocate,old_data,count_again,capacity,allocator_again,vtable_again,release,
    data_again,data_store,capacity_store};
enum class Outcome {kept_storage,released};
struct Kept {std::uint32_t array,size,data,capacity;};   // ESI, EBX, EDI, EBP
// Frame:
//   argument(k)                                the k-th argument's slot now
//   allocating(kept, bytes)                    the capacity's register, the bytes and the tag pushed
//   allocate(kept, function) -> EAX            the allocator's allocate, refreshes kept
//   copy(kept, to, from, bytes)                2E2E30, refreshes kept
//   releasing(bytes)                           the release's tag and size pushed
//   release(kept, function, data, bytes)       the allocator's release, refreshes kept
template<class Memory,class Frame> Outcome grow(Memory &memory,Frame &frame) {
    using enum Access;
    Kept kept{memory.load32(Access::array,frame.argument(1u)),0u,0u,0u};
    const auto count_value=memory.load32(count,at(kept.array,array::count));
    kept.capacity=count_value?count_value*2u:1u;
    kept.size=memory.load32(size,frame.argument(2u));
    const auto allocator_address=memory.load32(Access::allocator,globals::allocator);
    const auto table=memory.load32(Access::vtable,allocator_address);
    frame.allocating(kept,kept.capacity*kept.size);
    kept.data=frame.allocate(kept,memory.load32(Access::allocate,at(table,vtable::allocate)));
    const auto previous=memory.load32(Access::old_data,at(kept.array,array::data));
    frame.copy(kept,kept.data,previous,memory.load32(count_again,at(kept.array,array::count))*kept.size);
    const auto capacity_value=memory.load32(Access::capacity,at(kept.array,array::capacity));
    auto outcome=Outcome::kept_storage;
    if(!(capacity_value&unowned)) {
        const auto table_again=memory.load32(vtable_again,memory.load32(allocator_again,globals::allocator));
        const auto bytes=(capacity_value&~unowned)*kept.size;
        frame.releasing(bytes);
        const auto data_value=memory.load32(data_again,at(kept.array,array::data));
        frame.release(kept,memory.load32(Access::release,at(table_again,vtable::release)),data_value,bytes);
        outcome=Outcome::released;
    }
    memory.store32(data_store,at(kept.array,array::data),kept.data);
    memory.store32(capacity_store,at(kept.array,array::capacity),kept.capacity);
    return outcome;
}
} // namespace array_grow
} // namespace halo2::native_array_grow
