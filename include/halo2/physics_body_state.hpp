#pragma once
#include "halo2/physics_contact_lifecycle.hpp"

namespace halo2 {
// Source-address storage must belong to the caller's real allocator/backend.
// Shape references, property banks and broadphase tables are retained here;
// no callback may return a captured address or substitute an authored sphere.
std::uint32_t source_initialize_physics_body_info_1_1(
    SourcePhysicsContactMemory &, std::uint32_t info,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_entity_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entity, std::uint32_t info,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_entity_base_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entity, std::uint32_t info,
    const SourcePhysicsContactRecordObserver & = {});
void source_import_physics_entity_properties_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entity, std::uint32_t info,
    const SourcePhysicsContactRecordObserver & = {});
void source_add_physics_entity_property_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entity, std::uint32_t key,
    std::uint32_t value, const SourcePhysicsContactRecordObserver & = {});

// Original22E950 class allocator through0x2000, including empty-list pages.
// Backing is real already-owned allocator storage; no OS allocation, captured
// return address or artificial free-list node is supplied. Returns source0
// at page exhaustion; the adapter's alternate provider remains required.
std::uint32_t source_allocate_physics_body_small_block_1_1(SourcePhysicsContactMemory &,
    std::uint32_t allocator, std::uint32_t bytes,
    const SourcePhysicsContactRecordObserver & = {});
// Original22EB20 class allocator free and source bank copy/growth.
// The real allocator class table/freelist/counters must already exist. The
// OS and alternate allocation providers have no success default.
void source_free_physics_body_small_block_1_1(SourcePhysicsContactMemory &,
    std::uint32_t allocator, std::uint32_t block, std::uint32_t bytes,
    const SourcePhysicsContactRecordObserver & = {});
void source_grow_physics_body_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor, std::uint32_t capacity, std::uint32_t stride,
    const SourcePhysicsContactRecordObserver & = {});
void source_double_physics_body_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor, std::uint32_t stride,
    const SourcePhysicsContactRecordObserver & = {});
void source_copy_physics_body_bank_bytes_1_1(SourcePhysicsContactMemory &,
    std::uint32_t destination, std::uint32_t source, std::uint32_t bytes,
    const SourcePhysicsContactRecordObserver & = {});

struct SourcePhysicsQuantizedBounds {
    std::uint32_t backend, packed_bounds, output;
    std::uint16_t x87_control_word;
    std::uint32_t mxcsr;
};
// Original2E65A0: exact table-index decode and x87 reciprocal/spill ordering,
// then source SUBPS. The packed bounds must come from the actual broadphase.
// Returns the original output pointer; masked FP status is not represented.
std::uint32_t source_decode_physics_current_bounds_1_1(
    SourcePhysicsContactMemory &, const SourcePhysicsQuantizedBounds &,
    const SourcePhysicsContactRecordObserver & = {});
// Original2E7610 changes only argument0 and tail-jumps to2E65A0. body_handle
// addresses the source broadphase index, never an object or tag datum.
std::uint32_t source_physics_current_bounds_1_1(SourcePhysicsContactMemory &,
    std::uint32_t backend, std::uint32_t body_handle, std::uint32_t output,
    std::uint16_t x87_control_word, std::uint32_t mxcsr,
    const SourcePhysicsContactRecordObserver & = {});

struct SourcePhysicsContactManagerDelivery {
    std::uint32_t context; // Original ESI; actual0x14-byte callback context.
    std::array<std::uint32_t, 5> stack_arguments; // Original1D4160 ret0x14.
    // Original stack local at1D41E7 changes only low8. Upper24 are retained
    // for literal ABI comparisons;1D2F10 consumes only the low8 flip flag.
    std::uint32_t flip_scratch_word;
    std::uint16_t x87_control_word;
    std::uint32_t mxcsr;
};
using SourcePhysicsContactRefreshObserver =
    std::function<void(const SourcePhysicsContactRefresh &)>;
// Complete1D4160 row/index/component/slot match and manager delivery. This
// consumes an admitted source/backend callback and does not make a contact.
// The existing exact refresh owner retains its unmeasured positive branches.
void source_deliver_physics_contact_manager_1_1(SourcePhysicsContactMemory &,
    const SourcePhysicsContactManagerDelivery &,
    const SourcePhysicsContactRecordObserver & = {},
    const SourcePhysicsContactDispatchObserver & = {},
    const SourcePhysicsContactMathObserver & = {},
    const SourcePhysicsContactRefreshObserver & = {});
} // namespace halo2
