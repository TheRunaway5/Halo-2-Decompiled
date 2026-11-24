#pragma once
// Native TU1.5 record sort, entered at 4B150 (shadow frame unit): a bubble
// sort of a range of 36-byte records by a key, largest first.
//
//   record_sort::sort   4B150(EAX = first; records, mode, last) -> RET C
//
// It makes a pass for each record of the range (a counter from first to
// last, kept in the first's slot); in each pass every record from first+1 to
// last (the start kept in the frame) is compared with the one before it and,
// when it sorts higher, the two swap (through a 36-byte record in the frame, each copy as REP MOVSD
// copies it, in the direction the string direction flag gives). The key is
// the float +0x18 in mode 0, the signed int +8 in mode 1 and the float +0x10
// otherwise (COMISS: an unordered pair never swaps).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the pass
// counter, the start, the swap record) and the arguments it rereads; the boundary
// adapter (native_tu15_record_sort.hpp) maps the role to the original
// instruction and keeps the registers and flags.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_record_sort {
using native_object_update::at;
using native_object_update::signed_value;
namespace sse=native_aim_clusters::sse;
inline constexpr std::uint32_t record_size=36u,record_words=9u;
namespace key {inline constexpr std::uint32_t float_mode0=0x18u,integer=8u,float_other=0x10u;}
inline constexpr std::uint32_t integer_mode=1u;
namespace argument {inline constexpr std::uint32_t records=0u,mode=1u,last=2u;}

namespace record_sort {
enum class Access {last,counter_init,start_store,start_load,records,mode,later_key,earlier_key,save_read,save_write,last_again,
    down_read,down_write,up_read,up_write,counter_load,counter_store};
enum class Outcome {empty,sorted};
// Frame:
//   argument(k), counter(), start(), swap_record()   the slots now
//   saving(), saved()                  the registers saved around the start's store
//   compared(mode, higher)             the key compare's answer register and flags
//   copying(which, from, to), step()   a REP MOVSD's registers (which: 0 save, 1 down, 2 up) and direction
//   advanced(j, earlier, last), counted(counter, last)   the loops' INC, ADD and CMP
template<class Memory,class Frame> void copy(Memory &memory,Frame &frame,Access read,Access write,std::uint32_t from,std::uint32_t to,unsigned which) {
    frame.copying(which,from,to);
    const auto step=frame.step();
    for(std::uint32_t word=0;word<record_words;++word,from+=step,to+=step)memory.store32(write,to,memory.load32(read,from));
}
template<class Memory,class Frame> Outcome sort(Memory &memory,Frame &frame,std::uint32_t first) {
    using enum Access;
    auto last_value=memory.load32(last,frame.argument(argument::last));
    memory.store32(counter_init,frame.counter(),first);
    if(signed_value(first)>signed_value(last_value))return Outcome::empty;
    frame.saving();
    memory.store32(start_store,frame.start(),first+1u);
    frame.saved();
    for(;;) {
        auto j=memory.load32(start_load,frame.start());
        if(!(signed_value(j)>signed_value(last_value))) {
            auto earlier=memory.load32(records,frame.argument(argument::records))+record_size*(j-1u);
            do {
                const auto mode_value=memory.load32(mode,frame.argument(argument::mode));
                const auto later=earlier+record_size;
                bool higher;
                if(mode_value==integer_mode) {
                    const auto earlier_value=memory.load32(earlier_key,at(earlier,key::integer));
                    higher=signed_value(earlier_value)<signed_value(memory.load32(later_key,at(later,key::integer)));
                } else {
                    const auto offset=mode_value?key::float_other:key::float_mode0;
                    const auto later_value=memory.load32(later_key,at(later,offset));
                    higher=!sse::not_above(later_value,memory.load32(earlier_key,at(earlier,offset)));
                }
                frame.compared(mode_value,higher);
                if(higher) {
                    copy(memory,frame,save_read,save_write,earlier,frame.swap_record(),0u);
                    last_value=memory.load32(last_again,frame.argument(argument::last));
                    copy(memory,frame,down_read,down_write,later,earlier,1u);
                    copy(memory,frame,up_read,up_write,frame.swap_record(),later,2u);
                }
                ++j;earlier+=record_size;
                frame.advanced(j,earlier,last_value);
            } while(!(signed_value(j)>signed_value(last_value)));
        }
        const auto counter_value=memory.load32(counter_load,frame.counter())+1u;
        frame.counted(counter_value,last_value);
        memory.store32(counter_store,frame.counter(),counter_value);
        if(signed_value(counter_value)>signed_value(last_value))return Outcome::sorted;
    }
}
} // namespace record_sort
} // namespace halo2::native_record_sort
