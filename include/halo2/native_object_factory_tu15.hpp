#pragma once
#include "halo2/physics_contact_record.hpp"
#include <cstdint>

namespace halo2::native_object_factory_tu15 {
// Native owners of TU1.5 object_new (B9B70), ported owner by owner (see
// docs/NATIVE_OBJECT_FACTORY.md). Each replicates the original routine's
// stores, widths and order over the live world bank; none executes original
// code. Production callers pass a journaled world memory.

// Object memory pool (the pool object at [0x4EABBC]): +28 base, +2C size,
// +30 free bytes, +34/+38 first/last block, +3C no-owner flag, +3D gap-fit
// flag, +3F debug-header flag. Blocks are 16-byte rounded: +0 size, +4 owner
// slot, +8 next, +C previous, then the payload.
//
// 1910A0: allocate `size` payload bytes; `slot` receives the payload pointer.
// Gap fit (191480) when +3D, then before the first block (1913F0), then
// after the last (191350). Debug headers record `tag_b`, `tag_a` and the
// kernel tick ([[0x41B774]]).
bool memory_pool_allocate(SourcePhysicsContactMemory&,std::uint32_t pool,std::uint32_t slot,std::uint32_t size,
    std::uint32_t tag_a,std::uint32_t tag_b);
// 191130: resize the block whose payload pointer `slot` holds; grows in place
// up to the next block (or pool end), otherwise allocates, copies the old
// payload, unlinks the old block (191220) and retargets `slot`. The temporary
// payload slot of the relocating 1910A0 call is `scratch_slot` (the original's
// caller stack argument).
bool memory_pool_resize(SourcePhysicsContactMemory&,std::uint32_t pool,std::uint32_t slot,std::uint32_t size,
    std::uint32_t tag_a,std::uint32_t tag_b,std::uint32_t scratch_slot);
// BE2B0: object datum (native 16E6A0 on [0x4ECBD0]) with a zeroed `size`-byte
// body from the object memory pool; the datum is released (16E8A0) when the
// pool refuses. Returns the datum or NONE.
std::uint32_t object_allocate(SourcePhysicsContactMemory&,std::int16_t size);
// BE3B0: append a variable body block: header at body+`header`, `bytes` long,
// aligned to 1 << `alignment`; zero bytes leave an empty header. The body
// grows through 191130; `scratch_slot` is the original's stack slot that its
// relocation records as the moved block's provisional owner before 191130
// retargets it (production may pass any value: it never survives).
bool object_block_allocate(SourcePhysicsContactMemory&,std::uint32_t object,std::int16_t header,std::int16_t bytes,
    std::uint8_t alignment,std::uint32_t scratch_slot);

// Leaf owners every birth runs. Object arguments are datum handles; like the
// original, only the low 16 bits select the row (no salt check).
//
// C19D0: the object definition's model (+38) tag indices at model +4 and
// +C. Both are stored to `out_first`/`out_second` once both are present
// (nothing is stored otherwise); returns whether the +4 tag's blocks at
// +74 and +7C are both non-empty (signed counts).
bool object_model_tag_pair(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t out_first,std::uint32_t out_second);
// D80A0: the model's damage-information row ([model+64] when the signed
// count at +60 is positive), else 0; NONE objects and models give 0.
std::uint32_t object_damage_info(const SourcePhysicsContactMemory&,std::uint32_t object);
// BBD50: header byte +2 bit 6 of the object's root parent (body +14 chain).
std::uint32_t object_root_header_bit6(const SourcePhysicsContactMemory&,std::uint32_t object);
// BC400: clears bytes +1, +0 and +3 of the body block at body+[body+12A]+64,
// then marks the object for update (native B9390).
void object_clear_block_12a(SourcePhysicsContactMemory&,std::uint32_t object);
// 10E440: an object whose definition has byte +2 bit 6 joins the 32-entry
// list at [0x51DAF0]+1D00 (count +1D80) and sets body +C0 bit 3.
void object_queue_definition_flagged(SourcePhysicsContactMemory&,std::uint32_t object);
// 1CC4D0: biped, vehicle, machine, crate and creature definitions need
// physics capacity: [[0x52BE44]] < 0x200, or with byte [0x48B870] set,
// [[0x52BE5C]+3C] < [[0x52BE5C]+20]. Other types always pass.
bool object_definition_physics_capacity(const SourcePhysicsContactMemory&,std::uint32_t definition);
// BA850: [0x4EABC4] is non-null and its first byte is set.
std::uint32_t object_memory_locked(const SourcePhysicsContactMemory&);
// Havok record (0x84 bytes: three 0x20-byte links, flags +60..+67, +68..+80).
// 1D3180 writes its own defaults then runs 1D3270; 1D3350 clears the links.
void havok_record_initialize(SourcePhysicsContactMemory&,std::uint32_t record);
void havok_record_reset(SourcePhysicsContactMemory&,std::uint32_t record);
void havok_record_clear_links(SourcePhysicsContactMemory&,std::uint32_t record);

// BE330: release a failed birth. Clears header byte +2, unlinks the body
// block from the object memory pool (when present) and frees the datum.
void object_free(SourcePhysicsContactMemory&,std::uint32_t object);
// 10D950: object type (0..12) whose group tag matches the tag instance's
// group, or NONE. The tag index is the sign-extended low word.
std::uint32_t definition_object_type(const SourcePhysicsContactMemory&,std::uint32_t tag);
// 16FFE0: model variant row named `name` (+50 count, +54 rows of 0x38);
// name 0 selects row 0 when any exist. NONE when absent or the model is NONE.
std::uint32_t model_variant_index(const SourcePhysicsContactMemory&,std::uint32_t model_tag,std::uint32_t name);
// BC570: body +B1 = variant index of `name` (0: the definition's default
// variant at +30) in the definition's model (+38).
void object_set_variant(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t name);
// BAC30: node matrix block (body + [+116]) and its 0x34-byte matrix count.
std::uint32_t object_node_matrices(const SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t& count);
// 2E1C0: retire every active lens query slot (flag 0x10; owner index in bits
// 6..17) of the object through native 20E30.
void object_retire_lens_queries(SourcePhysicsContactMemory&,std::uint32_t object);
}
