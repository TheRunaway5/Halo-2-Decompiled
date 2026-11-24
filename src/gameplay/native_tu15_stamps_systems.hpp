#pragma once
// Native TU1.5 stamps: two stamp advances of the combat action unit.
//
//   stamp::advance            43980()                    the stamp record at [515F28]
//   stamp::advance_framed     12F0C0()                   the frame counter, then the record at [4F2D34]
//
// A stamp record's stamp (+0x38) advances by one, except at its limit
// (0x7FFFFFFF), where 13FD10 (EAX the record) renumbers the record instead;
// then its eight slots (+0x44..+0x60) are reset to the limit. The framed
// advance first counts the frame ([4F2D58]); a count that wraps to zero
// runs 12F040.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_stamp_advance.hpp, native_tu15_stamp_framed.hpp) map the role
// to the original instruction and own the ABI of the children; the record
// and the limit, which the original keeps in ESI and EDI across them, are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_stamps {
using native_object_update::at;
namespace globals {
inline constexpr std::uint32_t record=0x515f28u,framed_record=0x4f2d34u,frames=0x4f2d58u;
}
inline constexpr std::uint32_t limit=0x7fffffffu,stamp_field=0x38u,slots=0x44u,slot_count=8u;

namespace stamp {
enum class Access {frames,frames_store,record,stamp,stamp_store,slot_store};
enum class Outcome {advanced,renumbered,wrapped_advanced,wrapped_renumbered};
struct Kept {std::uint32_t record,limit;};   // ESI, EDI
// Frame: renumber(kept) 13FD10, refreshes kept; wrapped() 12F040.
template<class Memory,class Frame> bool advance(Memory &memory,Frame &frame,std::uint32_t record_global) {
    using enum Access;
    Kept kept{memory.load32(Access::record,record_global),limit};
    const auto stamp_value=memory.load32(Access::stamp,at(kept.record,stamp_field));
    const bool renumbered=stamp_value==limit;
    if(renumbered)frame.renumber(kept);
    else memory.store32(stamp_store,at(kept.record,stamp_field),stamp_value+1u);
    for(std::uint32_t k=0;k<slot_count;++k)memory.store32(slot_store,at(kept.record,slots+4u*k),kept.limit);
    return renumbered;
}
template<class Memory,class Frame> bool advance_framed(Memory &memory,Frame &frame,bool &wrapped) {
    using enum Access;
    const auto count=memory.load32(Access::frames,globals::frames)+1u;
    memory.store32(frames_store,globals::frames,count);
    wrapped=count==0u;
    if(wrapped)frame.wrapped();
    return advance(memory,frame,globals::framed_record);
}
} // namespace stamp
} // namespace halo2::native_stamps
