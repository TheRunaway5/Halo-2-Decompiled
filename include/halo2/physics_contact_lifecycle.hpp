#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// TU1.1 source-address storage, not a native collision or Havok admission
// service. The adapter must retain actual allocator/body/property/manager,
// pool, clock and executable-constant backing. Missing reads are fatal.
struct SourcePhysicsContactAppend {
    std::array<std::uint32_t, 13> stack_arguments; // Original1D3DF0 ret0x34.
};
struct SourcePhysicsContactRefresh {
    std::uint32_t manager, row;
    std::array<std::uint32_t, 4> stack_arguments; // Original1D2F10 ret0x10.
    std::uint16_t x87_control_word;
    std::uint32_t mxcsr;
};
struct SourcePhysicsContactDispatch {
    std::uint32_t instruction, target, this_pointer;
    std::vector<std::uint32_t> stack_arguments;
};
using SourcePhysicsContactDispatchObserver =
    std::function<void(const SourcePhysicsContactDispatch &)>;
struct SourcePhysicsContactMath {
    std::uint32_t instruction;
    // Low scalar XMM words at2FF8/300E/3029/3037, source float spill at30A8,
    // source FISTP integer at30B6. Full SIMD/x87 status is outside this API.
    std::vector<std::uint32_t> words;
};
using SourcePhysicsContactMathObserver = std::function<void(const SourcePhysicsContactMath &)>;

// Exact measured fast branch22E950: actual signed class-table entry, existing
// freelist head, counters and returned backing. Empty/growth/large branches
// are hard unavailable boundaries; no allocation success default exists.
std::uint32_t source_physics_contact_fast_allocate_1_1(
    SourcePhysicsContactMemory &, std::uint32_t allocator, std::uint32_t bytes,
    const SourcePhysicsContactRecordObserver & = {});
// Exact2DF710 cold descriptor growth through original487638/148A10/480DA0
// backend dispatch. Old nonempty copy/free branches remain unavailable.
void source_grow_physics_contact_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor, std::uint32_t capacity, std::uint32_t stride,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_append_physics_contact_1_1(SourcePhysicsContactMemory &,
    const SourcePhysicsContactAppend &, const SourcePhysicsContactRecordObserver & = {});

// Complete actual31F4B0/31F4D0 index-map consumers; their returned pointers
// address the retained manager's own32/28-byte packets. No contact is made.
std::uint32_t source_physics_contact_point_1_1(const SourcePhysicsContactMemory &,
    std::uint32_t manager, std::uint32_t index_word);
std::uint32_t source_physics_contact_scalar_1_1(const SourcePhysicsContactMemory &,
    std::uint32_t manager, std::uint32_t index_word);

// Measured nonflipped/noncharacter/nonimpulse refresh. Exact original SSE
// operands and x87 FISTP rounding; masked FP status flags are not outputs.
// Positive character/normal-flip/dirty-wakeup/impulse branches throw at their
// owning boundary after the preceding source stores. No void EAX provenance
// is promoted to an admission result.
void source_refresh_physics_contact_1_1(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRefresh &, const SourcePhysicsContactRecordObserver & = {},
    const SourcePhysicsContactDispatchObserver & = {},
    const SourcePhysicsContactMathObserver & = {});
// Exact measured type0 impact decrement and row removal/retained capacity.
// The source-last-reference type1 effect dispatch is still unavailable.
void source_remove_physics_contact_row_1_1(SourcePhysicsContactMemory &,
    std::uint32_t component, std::uint32_t row_index,
    const SourcePhysicsContactRecordObserver & = {});
// Original1D3FC0 property/index/slot match and tail delivery to1D40B0.
// callback_packet must address the actual Havok callback's consumed low16.
void source_remove_physics_contact_1_1(SourcePhysicsContactMemory &,
    std::uint32_t counterpart_body, std::uint32_t own_body,
    std::uint32_t component, std::uint32_t callback_packet,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
