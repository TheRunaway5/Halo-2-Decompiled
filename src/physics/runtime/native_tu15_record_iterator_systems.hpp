#pragma once
// Native TU1.5 keyed record iterator, entered at 1E5250 (physics live
// support unit): the next record of a two-level keyed lookup, widening the
// keys stage by stage.
//
//   record_iterator::next   1E5250(EBX = owner, ESI = iterator) -> AL
//
// The iterator holds the last record (+0, +4), its index (+8, 16-bit), the
// stage (+0xA, 16-bit) and two keys (+0xC, +0x10). Each stage looks a block
// up in two levels through the record search 1E5810: first in the owner's
// table (+0x34) by the first key, then in that block's table (+4) by the
// second key; stage 0 uses both keys, stage 1 the first key and the any key
// (0x30000D9), stage 2 the any key and the second key, stage 3 the any key
// twice. A found block (count +0xC, eight-byte records at +0x10) yields its
// record after the last index; the iterator then holds that record and
// index, and the result is 1. A stage that finds no block, or whose block
// has no record past the index, passes to the next stage from index zero;
// after stage 3 the result is 0. Stages past 3 find nothing.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_record_iterator.hpp) maps the role to the original
// instruction, owns the stage jump table (through the unit's canonical
// branch map) and the ABI of the child 1E5810. The next index lives in the
// original's one local; the iterator is read again for every step.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_record_iterator {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;

struct Iterator {std::uint32_t record[2];std::uint16_t index,stage;std::uint32_t first_key,second_key;};
static_assert(offsetof(Iterator,index)==8 && offsetof(Iterator,stage)==0xa && offsetof(Iterator,first_key)==0xc &&
              offsetof(Iterator,second_key)==0x10);
inline constexpr std::uint32_t owner_table=0x34u,block_table=4u,block_count=0xcu,block_records=0x10u;
inline constexpr std::uint32_t record_size=8u;
inline constexpr std::uint32_t any_key=0x30000d9u;
inline constexpr std::uint32_t stage_count=4u;
inline constexpr std::uint32_t stage_table=0x1e5350u;   // the original's jump table (one target per stage)

namespace record_iterator {
enum class Local {index};
enum class Access {
    index_read,index_store,stage_check,stage_read,stage_jump,first_key,second_key,stage_store,index_clear,stage_bound,
    index_reload,count,records,record_first,result_first,record_second,result_second,result_index,
    stage_advance_read,stage_advance
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t owner;      // EBX
    std::uint32_t iterator;   // ESI
    std::uint32_t stage;      // EBP: the stage in its low 16 bits
    std::uint32_t found;      // EDI: the found block (cleared before each lookup)
};

// Frame:
//   local(Local)                       the index local's address
//   entry()                            EBX and ESI at entry
//   stage_case(target)                 the jump table target's stage case
//   search(kept, stage, level, key)    1E5810 for the owner's table (level
//                                      0) or the block's (level 1); returns
//                                      the block or zero (EAX); kept.found is
//                                      EDI as the child returned it
template<class Memory,class Frame> bool next(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Kept kept=frame.entry();
    memory.store32(index_store,local(Local::index),widen16(memory.load16(index_read,at(kept.iterator,offsetof(Iterator,index))))+1u);
    for(;;) {
        std::uint32_t found=0;
        if(!(std::int16_t(memory.load16(stage_check,at(kept.iterator,offsetof(Iterator,stage))))<std::int16_t(stage_count)))return false;
        for(;;) {
            const auto stage=memory.load16(stage_read,at(kept.iterator,offsetof(Iterator,stage)));
            kept.stage=(kept.stage&0xffff0000u)|stage;
            if(widen16(stage)<stage_count) {
                const auto stage_case=frame.stage_case(memory.load32(stage_jump,stage_table+widen16(stage)*4u));
                const bool first_from_iterator=stage_case==0 || stage_case==1;
                const bool second_from_iterator=stage_case==0 || stage_case==2;
                const auto first=first_from_iterator?memory.load32(first_key,at(kept.iterator,offsetof(Iterator,first_key))):any_key;
                if(frame.search(kept,stage_case,0,first)) {
                    const auto second=second_from_iterator?
                        memory.load32(second_key,at(kept.iterator,offsetof(Iterator,second_key))):any_key;
                    found=frame.search(kept,stage_case,1,second);
                } else found=kept.found;
            }
            if(!found) {
                kept.stage+=1u;
                memory.store16(stage_store,at(kept.iterator,offsetof(Iterator,stage)),kept.stage&0xffffu);
                memory.store32(index_clear,local(Local::index),0);
            }
            const bool more=std::int16_t(memory.load16(stage_bound,at(kept.iterator,offsetof(Iterator,stage))))<std::int16_t(stage_count);
            if(found)break;
            if(!more)return false;
        }
        // The block's record after the last index, or the next stage.
        const auto index=memory.load32(index_reload,local(Local::index));
        if(signed_value(index)<signed_value(memory.load32(count,at(found,block_count)))) {
            const auto record=memory.load32(records,at(found,block_records))+index*record_size;
            memory.store32(result_first,at(kept.iterator,0),memory.load32(record_first,record));
            memory.store32(result_second,at(kept.iterator,4),memory.load32(record_second,record+4u));
            memory.store16(result_index,at(kept.iterator,offsetof(Iterator,index)),index&0xffffu);
            return true;
        }
        memory.store16(stage_advance,at(kept.iterator,offsetof(Iterator,stage)),
                       memory.load16(stage_advance_read,at(kept.iterator,offsetof(Iterator,stage)))+1u);
    }
}
} // namespace record_iterator
} // namespace halo2::native_record_iterator
