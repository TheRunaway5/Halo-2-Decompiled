#pragma once
// Native TU1.5 behavior tables, entered at 1AF970 (AI actor and final startup
// storage units): each category's slots for each AI behavior.
//
//   behavior_tables::build   1AF970()
//
// For each of the 20 categories (8 bytes each at 47D590: their entries, 12
// bytes each with a behavior index in word +0, and a count, word +4), each
// behavior's entry (the last of its index) is found, and for each of the
// 0x83 behaviors (descriptors at 47B478) of kind 1 (word +2) its slots are
// filled by 1AFA90 (the frame's 50 slots, the entry, the descriptor's
// defaults +0x48 and their count +0x44) and copied (REP MOVSD, REP MOVSB, in
// the string direction flag's direction) into the slot pool at 4FD538 at the
// running index; the category's record for the behavior (0x50 bytes apart,
// 4 bytes a category, from 4FF79A) holds the index (word -2) and the slots'
// count (byte +0), or NONE and 0 for any other behavior.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (0x2F0 bytes,
// eight-byte aligned: the running index, the cursors and counts, the slots
// and the entries); the boundary adapter (native_tu15_behavior_tables.hpp)
// maps the role to the original instruction and owns the ABI of 1AFA90. The
// values the original keeps in callee-saved registers across it are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_behavior_tables {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t descriptors=0x47b478u,categories=0x47d590u,records=0x4ff79au,slot_pool=0x4fd538u;}
inline constexpr std::uint32_t category_total=0x14u,behavior_count=0x83u,category_size=8u,entry_size=0xcu,record_size=0x50u,
    record_step=4u,fill_kind=1u;
namespace category {inline constexpr std::uint32_t entries=0u,count=4u;}
namespace descriptor {inline constexpr std::uint32_t kind=2u,default_count=0x44u,defaults=0x48u;}
namespace record {inline constexpr std::int32_t start=-2;inline constexpr std::uint32_t count=0u;}
// The frame's slots, by offset from its base (ESP after the saves).
namespace local {inline constexpr std::uint32_t index=0x14u,record=0x18u,categories_left=0x1cu,category=0x20u,behaviors_left=0x24u,
    slots=0x28u,entries=0xf0u;}

namespace behavior_tables {
enum class Access {index_clear,record_store,category_store,categories_store,entries_clear,category_count,category_entries,entry_index,
    entry_store,behaviors_store,descriptor,kind,defaults,default_count,entry_load,index_load,start_store,count_store,copy_read,
    copy_write,index_add,index_added,start_none,count_none,behaviors_load,behaviors_again,category_load,record_load,categories_load,
    category_again,record_again,categories_again};
// The original's EBX (the record) and EBP (the behavior's offset) across 1AFA90.
struct Kept {std::uint32_t record,offset;};
// Frame:
//   framing(), local(offset), step()             PUSH EBP; MOV EBP,ESP; AND ESP,-8; SUB ESP,2F0; PUSH EBX, EBP, ESI, EDI
//   clearing(entries), counted(count), listed(entry, left), indexed(entry)   REP STOSD; TEST CX,CX; the list's walk
//   behavior(kept), described(descriptor), kinded(kind)   XOR EBP,EBP; TEST EAX,EAX; CMP word,1
//   fill(defaults, count, entry, kept) -> AX     PUSH count, defaults, entry, the slots; CALL 1AFA90 (RET 10)
//   copying(count, start), step(), bytes(bytes)  the REP MOVSD/MOVSB registers
//   advanced(kept, left), next_category(category, record, left)   ADD EBP,4; ADD EBX,50; DEC; the outer cursors
//   restoring()                                  POP EDI, ESI, EBP, EBX; MOV ESP,EBP; POP EBP
template<class Memory,class Frame> void build(Memory &memory,Frame &frame) {
    using enum Access;
    frame.framing();
    memory.store32(index_clear,frame.local(local::index),0u);
    memory.store32(record_store,frame.local(local::record),globals::records);
    memory.store32(category_store,frame.local(local::category),globals::categories);
    memory.store32(categories_store,frame.local(local::categories_left),category_total);
    Kept kept{globals::records,0u};
    std::uint32_t category_address=globals::categories,categories_left;
    do {
        // Each behavior's entry in this category: the last that names it.
        auto entry_slot=frame.clearing(frame.local(local::entries));
        for(std::uint32_t k=0;k<behavior_count;++k,entry_slot+=4u*frame.step())memory.store32(entries_clear,entry_slot,0u);
        const auto count=memory.load16(Access::category_count,at(category_address,category::count));
        frame.counted(count);
        if(std::int16_t(count)>0) {
            auto entry=memory.load32(category_entries,at(category_address,category::entries));
            for(std::uint32_t left=count;left;) {
                const auto index=widen16(memory.load16(entry_index,entry));
                frame.indexed(index);
                memory.store32(entry_store,frame.local(local::entries)+4u*index,entry);
                entry+=entry_size;
                --left;
                frame.listed(entry,left);
            }
        }
        kept.offset=0u;
        frame.behavior(kept);
        memory.store32(behaviors_store,frame.local(local::behaviors_left),behavior_count);
        std::uint32_t behaviors_left;
        do {
            bool recorded=false;
            const auto descriptor_address=memory.load32(Access::descriptor,globals::descriptors+kept.offset);
            frame.described(descriptor_address);
            if(descriptor_address) {
                const auto kind_value=memory.load16(kind,at(descriptor_address,descriptor::kind));
                frame.kinded(kind_value);
                if(kind_value==fill_kind) {
                    const auto defaults_address=memory.load32(defaults,at(descriptor_address,descriptor::defaults));
                    const auto defaults_count=memory.load16(default_count,at(descriptor_address,descriptor::default_count));
                    const auto entry=memory.load32(entry_load,frame.local(local::entries)+kept.offset);
                    const auto filled=frame.fill(defaults_count,defaults_address,entry,kept);
                    const auto start=memory.load16(index_load,frame.local(local::index));
                    frame.started(start);
                    memory.store16(start_store,kept.record-2u,start);
                    memory.store8(count_store,at(kept.record,record::count),filled);
                    if(std::int16_t(filled)>0) {
                        // The slots copied into the pool at the running index, which moves on past them.
                        auto from=frame.local(local::slots),to=globals::slot_pool+4u*widen16(start);
                        const auto bytes=frame.copying(widen16(filled),widen16(start),from);
                        const auto step=frame.step();
                        for(std::uint32_t word=0;word<(bytes>>2);++word,from+=4u*step,to+=4u*step)
                            memory.store32(copy_write,to,memory.load32(copy_read,from));
                        frame.bytes(bytes);
                        for(std::uint32_t byte=0;byte<(bytes&3u);++byte,from+=step,to+=step)
                            memory.store8(copy_write,to,memory.load8(copy_read,from));
                        const auto index_now=memory.load32(index_add,frame.local(local::index));
                        memory.store32(index_added,frame.local(local::index),frame.added(index_now,filled));
                    }
                    recorded=true;
                }
            }
            if(!recorded) {
                memory.store16(start_none,kept.record-2u,0xffffu);
                memory.store8(count_none,at(kept.record,record::count),0u);
            }
            behaviors_left=memory.load32(behaviors_load,frame.local(local::behaviors_left))-1u;
            kept.offset+=4u;
            kept.record+=record_size;
            frame.advanced(kept,behaviors_left);
            memory.store32(behaviors_again,frame.local(local::behaviors_left),behaviors_left);
        } while(behaviors_left);
        // The next category: its entries, its records' column, one fewer left.
        category_address=memory.load32(category_load,frame.local(local::category))+category_size;
        kept.record=memory.load32(record_load,frame.local(local::record))+record_step;
        categories_left=memory.load32(categories_load,frame.local(local::categories_left))-1u;
        frame.next_category(category_address,kept.record,categories_left);
        memory.store32(category_again,frame.local(local::category),category_address);
        memory.store32(record_again,frame.local(local::record),kept.record);
        memory.store32(categories_again,frame.local(local::categories_left),categories_left);
    } while(categories_left);
    frame.restoring();
}
} // namespace behavior_tables
} // namespace halo2::native_behavior_tables
