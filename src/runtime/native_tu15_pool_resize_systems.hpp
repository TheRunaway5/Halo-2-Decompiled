#pragma once
// Native TU1.5 pool resize, entered at 191130 (physics live support unit): a
// pool block resized in place, or moved.
//
//   pool_resize::resize   191130(EAX = size, EBX = handle*; pool*, label, line) -> AL, RET 0xC
//
// A block's data (at the handle) follows its 16-byte header, and a guarded
// pool's (byte +0x3F) blocks have a second 16-byte guard below that. The
// block needs the size plus those headers, rounded up to 16. When that ends
// at or before the next block (the header's +8, or without one the pool's
// end, +0x28 plus +0x2C) the block keeps its place: the pool's free bytes
// (+0x30) take the difference, the header the new size, and a guard (when
// there is one) the label, the line and the time (33B590). Otherwise 1910A0
// allocates a new block (the pool in ECX, the line in EAX; the new data into
// the label's slot, the size and the label pushed): the old data's payload
// is copied over (REP MOVSD, then REP MOVSB for the rest, in the string
// direction flag's direction), 191220 frees the old block (EAX the handle,
// EDX the pool), the new block's owner (-0xC) is the handle unless the pool
// keeps no owners (byte +0x3C), and the handle takes the new data. The
// answer is whether the block has its size.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the argument slot the new data lands
// in; the boundary adapter (native_tu15_pool_resize.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_pool_resize {
using native_object_update::at;
namespace pool {inline constexpr std::uint32_t base=0x28u,size=0x2cu,free=0x30u,unowned=0x3cu,guarded=0x3fu;}
namespace header {inline constexpr std::uint32_t size=0u,next=8u,owner=0xcu;}
namespace guard {inline constexpr std::uint32_t label=4u,line=8u,time=0xcu;}
inline constexpr std::uint32_t header_size=0x10u,guard_size=0x10u,alignment=0x10u;
namespace argument {inline constexpr std::uint32_t pool=0u,label=1u,line=2u;}

namespace pool_resize {
enum class Access {pool_argument,data,guarded,next,pool_size,pool_base,block_size,pool_free,free_store,size_store,guarded_again,
    label,line,label_store,line_store,time_store,move_label,move_line,guarded_moved,old_size,old_data,new_data,copy_read,copy_write,
    unowned,new_data_again,owner_store,new_data_final,handle_store};
enum class Outcome {in_place,moved,failed};
// The original's EBX (the handle), EBP (the pool), ESI (the header) and EDI (the size) across the children.
struct Kept {std::uint32_t handle,pool,header,size;};
// Frame:
//   argument(k), saving(n)                       the arguments' slots now; PUSH EBP, ESI, EDI (MOV EDI,EAX)
//   headed(data, guarded) -> overhead            SUB ESI,10; TEST AL,AL; MOV ECX,10 or 20
//   sized(overhead, size) -> needed              ADD ECX,EDI; TEST CL,F; (OR ECX,F; INC ECX)
//   limit_tested(next), fitted(needed, header, limit)   TEST EDX,EDX; LEA EAX; CMP EAX,EDX
//   freed(old, needed, free), guarded_tested(flag), guard_tested(header) -> guard   SUB/ADD; TEST AL,AL; ADD ESI,-10
//   clock(kept) -> EAX                           33B590, refreshes kept
//   pool_end(size, base)                         ADD EDX,[EBP+28]
//   pushing(size, label), allocate(line, kept) -> AL   PUSH label, size, the label's slot; MOV ECX,EBP; 1910A0 (RET C)
//   overhead(guarded) -> 10 or 20                TEST CL,CL; MOV EAX,10 or 20
//   moving(old, overhead, source, dest) -> payload, step(), moved_word(n), bytes(payload, kept), moved_byte(n)
//                                                SUB ECX,EAX; MOV EDX,ECX; SHR ECX,2; the REP MOVSD/MOVSB registers
//   release(kept)                                MOV EAX,EBX; MOV EDX,EBP; 191220, refreshes kept
//   owners_tested(unowned), owned(unowned, kept) -> owner   XOR EAX,EAX; TEST CL,CL; SETNZ AL; POP EDI, ESI, EBP; DEC EAX; AND EAX,EBX
template<class Memory,class Frame> Outcome resize(Memory &memory,Frame &frame,std::uint32_t size,Kept kept) {
    using enum Access;
    frame.saving(0u);
    kept.pool=memory.load32(pool_argument,frame.argument(argument::pool));
    frame.saving(1u);
    const auto data_address=memory.load32(data,kept.handle);
    frame.saving(2u);
    kept.size=size;
    const auto is_guarded=memory.load8(guarded,at(kept.pool,pool::guarded));
    kept.header=data_address-header_size;
    const auto overhead=frame.headed(data_address,is_guarded);
    const auto needed=frame.sized(overhead,size);
    auto limit=memory.load32(next,at(kept.header,header::next));
    frame.limit_tested(limit);
    if(!limit) {
        const auto pool_bytes=memory.load32(pool_size,at(kept.pool,pool::size));
        const auto base=memory.load32(pool_base,at(kept.pool,pool::base));
        limit=pool_bytes+base;
        frame.pool_end(pool_bytes,base);
    }
    frame.fitted(needed,kept.header,limit);
    if(!(needed+kept.header>limit)) {
        // In place: the free bytes take the difference, the header the size, a guard the label, line and time.
        const auto old=memory.load32(block_size,at(kept.header,header::size));
        const auto free_bytes=memory.load32(pool_free,at(kept.pool,pool::free));
        const auto free_now=frame.freed(old,needed,free_bytes);
        memory.store32(free_store,at(kept.pool,pool::free),free_now);
        memory.store32(size_store,at(kept.header,header::size),needed);
        const auto guarded_now=memory.load8(guarded_again,at(kept.pool,pool::guarded));
        frame.guarded_tested(guarded_now);
        if(guarded_now) {
            kept.header=frame.guard_tested(kept.header);
            if(kept.header) {
                const auto label_value=memory.load32(label,frame.argument(argument::label));
                const auto line_value=memory.load32(line,frame.argument(argument::line));
                memory.store32(label_store,at(kept.header,guard::label),label_value);
                memory.store32(line_store,at(kept.header,guard::line),line_value);
                const auto now=frame.clock(kept);
                memory.store32(time_store,at(kept.header,guard::time),now);
            }
        }
        return Outcome::in_place;
    }
    // Moved: a new block, the payload copied, the old block freed, the handle pointed at the new data.
    const auto label_value=memory.load32(move_label,frame.argument(argument::label));
    frame.pushing(kept.size,label_value);
    const auto line_value=memory.load32(move_line,frame.argument(argument::line));
    if(!(frame.allocate(line_value,kept)&0xffu))return Outcome::failed;
    const auto overhead_now=frame.overhead(memory.load8(guarded_moved,at(kept.pool,pool::guarded)));
    const auto old=memory.load32(old_size,at(kept.header,header::size));
    const auto source=memory.load32(old_data,kept.handle);
    const auto dest=memory.load32(new_data,frame.argument(argument::label));
    const auto payload=frame.moving(old,overhead_now,source,dest);
    const auto step=frame.step();
    auto from=source,to=dest;
    for(std::uint32_t word=0;word<(payload>>2);++word,from+=4u*step,to+=4u*step) {
        memory.store32(copy_write,to,memory.load32(copy_read,from));
        frame.moved_word(word);
    }
    frame.bytes(payload,kept);
    for(std::uint32_t byte=0;byte<(payload&3u);++byte,from+=step,to+=step) {
        memory.store8(copy_write,to,memory.load8(copy_read,from));
        frame.moved_byte(byte);
    }
    frame.release(kept);
    const auto keeps_none=memory.load8(Access::unowned,at(kept.pool,pool::unowned));
    frame.owners_tested(keeps_none);
    const auto block=memory.load32(new_data_again,frame.argument(argument::label));
    const auto owner=frame.owned(keeps_none,kept);
    memory.store32(owner_store,block-header::owner,owner);
    memory.store32(handle_store,kept.handle,memory.load32(new_data_final,frame.argument(argument::label)));
    return Outcome::moved;
}
} // namespace pool_resize
} // namespace halo2::native_pool_resize
