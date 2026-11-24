#pragma once
// Native TU1.5 object allocation, entered at BE2B0 (physics live support
// unit): a new object datum with its memory.
//
//   object_allocate::allocate   BE2B0(size) -> EAX the handle or NONE, RET 4
//
// A datum of the object headers ([4ECBD0], 16E6A0) gets its memory from the
// object pool ([4EABBC], 1910A0: the header's address slot +8, the size
// (int16), 0). Then the header records the size (word +6) and the memory is
// cleared as REP STOSD then REP STOSB clear it, in the direction the string
// direction flag gives. Without the memory the datum is deleted again
// (16E8A0) and the answer is NONE.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the argument it reads twice; the
// boundary adapter (native_tu15_object_allocate.hpp) maps the role to the
// original instruction and owns the ABI of the children (16E6A0, 1910A0,
// 16E8A0). The values the original keeps in callee-saved registers across
// them are a Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_allocate {
using native_object_update::at;
using native_object_update::widen16;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,object_pool=0x4eabbcu;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_bytes=6u,header_address=8u;

namespace object_allocate {
enum class Access {headers,datums,size,pool,size_again,size_store,memory_address,word_clear,byte_clear,headers_again};
enum class Outcome {no_datum,allocated,no_memory};
// The original's ESI (the handle), EBX (the size), EBP (the address slot)
// and EDI (the header) across the children.
struct Kept {std::uint32_t handle,size,slot,header;};
// Frame:
//   argument()                         the size's slot now
//   new_datum(headers) -> handle       16E6A0(EDX the array)
//   headers_after()                    the array EDX holds after 16E6A0
//   saving()                           EBX saved
//   allocate(pool, kept) -> AL         1910A0(ECX the pool; the slot, the size, 0), refreshes kept
//   step(width), words_started(), bytes_started()   the clear's direction and registers
//   delete_datum(handle, kept)         16E8A0(EDX the array, EAX the handle), refreshes kept
template<class Memory,class Frame> Outcome allocate(Memory &memory,Frame &frame,std::uint32_t &answer) {
    using enum Access;
    Kept kept{};
    kept.handle=frame.new_datum(memory.load32(headers,globals::object_headers));
    answer=kept.handle;
    if(kept.handle==none)return Outcome::no_datum;
    const auto datums_address=memory.load32(datums,at(frame.headers_after(),datum_data));
    frame.saving();
    kept.size=widen16(memory.load16(size,frame.argument()));
    kept.header=datums_address+header_size*(kept.handle&0xffffu);
    const auto pool_address=memory.load32(pool,globals::object_pool);
    kept.slot=at(kept.header,header_address);
    if(!(frame.allocate(pool_address,kept)&0xffu)) {
        frame.delete_datum(memory.load32(headers_again,globals::object_headers),kept);
        answer=none;
        return Outcome::no_memory;
    }
    memory.store16(size_store,at(kept.header,header_bytes),memory.load16(size_again,frame.argument()));
    auto to=memory.load32(memory_address,kept.slot);
    frame.words_started(kept.size);
    const auto word_step=frame.step(4u);
    for(std::uint32_t k=0;k<kept.size>>2;++k,to+=word_step)memory.store32(word_clear,to,0u);
    frame.bytes_started(kept.size);
    const auto byte_step=frame.step(1u);
    for(std::uint32_t k=0;k<(kept.size&3u);++k,to+=byte_step)memory.store8(byte_clear,to,0u);
    answer=kept.handle;
    return Outcome::allocated;
}
} // namespace object_allocate
} // namespace halo2::native_object_allocate
