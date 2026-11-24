#pragma once
// Native TU1.5 multiplayer object cleanup, entered at 15C4E0 (multiplayer
// engines unit): objects lying around too long are deleted.
//
//   object_cleanup::sweep   15C4E0
//
// Unless the game engine (4F3218, its mode byte at +0xC) is in mode 4, the
// objects are walked with the object iterator (BCEE0; the iterator lives in
// the frame, typed 0x81F). Each object is considered only on its own tick
// slot (its index % 30 == the tick % 30, signed division as IDIV). By type
// (the byte at +0xAA, as a bit):
//   - items (types 2..4): a weapon (type 2) whose definition (+0x290) is
//     tied to the engine asks the engine variant's virtual check (+0x98) and
//     164750 instead; otherwise the object's holder state decides (+0xAF,
//     +0x12C, 105FD0/164750). A free item is deleted once its age since
//     +0x150 reaches the variant's item timeout (44E9F0 + setting * 4);
//   - bipeds (type 0) flagged at +0x10A bit 2: deleted once their age since
//     +0xBC reaches the corpse timeout (44E9F8), or the time since +0x2E0
//     reaches 44EA00;
//   - other objects flagged at +4 bit 14, past their start tick (+0x20):
//     deleted once their age since +0xBC reaches 44EA08, or reaches 44EA10.
// Ages are ticks times the seconds per tick (CVTSI2SS, MULSS), compared
// with COMISS. Deleting is BBBC0 then BA570.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_cleanup.hpp) maps the role to the original instruction
// and owns the ABI of the children (the iterator, the holder and engine
// checks, the deletion). The iterator, the tick, the setting and the current
// object live in the original's frame and are read and written as memory
// through Frame::local, in the original's order; the values the original
// keeps in callee-saved registers across the children are a Kept, refreshed
// from the registers each child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_cleanup {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;

namespace globals {
inline constexpr std::uint32_t game_engine=0x4f3218u;      // +0xC: the engine mode byte
inline constexpr std::uint32_t engine_variant=0x4f63b8u;   // +0xC0C the timeout setting, +0xC14 the variant's index
inline constexpr std::uint32_t game_time=0x51df54u;        // +4 seconds per tick, +8 the tick
inline constexpr std::uint32_t object_headers=0x4ecbd0u;
inline constexpr std::uint32_t tag_instances=0x4f0414u;
inline constexpr std::uint32_t variants=0x56dfa0u;         // the engine variants (objects with a table at +0)
inline constexpr std::uint32_t item_timeouts=0x44e9f0u,corpse_timeouts=0x44e9f8u,corpse_hold_timeouts=0x44ea00u,
    other_timeouts=0x44ea08u,other_late_timeouts=0x44ea10u;   // float per setting
}
inline constexpr std::uint32_t engine_mode=0xcu,quiet_mode=4u;
inline constexpr std::uint32_t variant_setting=0xc0cu,variant_index=0xc14u,variant_check=0x98u;
inline constexpr std::uint32_t iterator_types=0x81fu,iterator_unset=0x86868686u;
inline constexpr std::uint32_t slot_period=30u;
namespace object {
inline constexpr std::uint32_t flags=4u,start_tick=0x20u,type=0xaau,holder=0xafu,created=0xbcu,fade_flags=0x10au,
    item_flags=0x12cu,dropped=0x150u,held=0x2e0u;
}
inline constexpr std::uint32_t weapon_engine_field=0x290u;
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_data=8u,tag_size=16u,tag_data=8u;
struct GameTime {std::uint32_t unused_here;float seconds_per_tick;std::int32_t tick;};
static_assert(offsetof(GameTime,seconds_per_tick)==4 && offsetof(GameTime,tick)==8);
// CVTSI2SS of a 32-bit integer.
inline std::uint32_t scalar_from_integer(std::uint32_t x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 r=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(r):"r"(std::int32_t(x)));return sse::bits(r);
#else
    return sse::bits(_mm_cvtsi32_ss(_mm_setzero_ps(),std::int32_t(x)));
#endif
}
namespace object_type {inline constexpr std::uint32_t biped=1u,weapon=4u,items=0x1cu;}

namespace object_cleanup {
// The frame: the tick and the setting, the current object, and the object
// iterator (its kind, a byte, a word, the current handle and a marker).
enum class Local {tick,setting,current,iterator,iterator_byte,iterator_word,iterator_handle,iterator_marker};
enum class Access {
    engine,mode,variant,setting,game_time,tick,tick_store,setting_store,marker_store,kind_store,byte_clear,word_clear,
    handle_clear,current_store,
    slot_tick,slot_handle,type,headers,rows,weapon_data,weapon_tag,instances,weapon_definition,engine_field,
    holder_handle,holder,item_flags,checked_handle,item_flags_again,dropped,item_tick,item_setting,item_timeout,
    variant_again,variant_index_read,variant_object,check_handle,vtable,check_target,
    fade_flags,created,corpse_tick,corpse_setting,corpse_timeout,corpse_seconds,hold_setting,held,hold_timeout,
    object_flags,start_tick,other_tick,other_created,other_setting,other_timeout,other_seconds,late_timeout,
    age_tick,age_seconds,delete_handle,time_reload,current_next
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t time;      // EBP: the game time
    std::uint32_t object;    // ESI: the current object's data (the handle while checking or deleting)
    std::uint32_t index;     // EDI: the current handle's index (the weapon's engine field after it)
    std::uint32_t held;      // EBX: BL the holder check
};
enum class Exit {quiet,empty,swept};
// How an age compared with a timeout (the second stage of a corpse or
// another object follows a first one that did not delete).
enum class Age {item,corpse,hold,other,late};

// Frame:
//   local(Local)                      a local's address now
//   iterate(kept)                     BCEE0 for the frame's iterator; returns EAX
//   holder_check(kept)                105FD0 for the current handle; returns AL
//   free_check(kept, engine)          164750 for the handle (EAX); returns AL
//   engine_check(kept, target)        the variant's check at target; returns AL
//   aged(stage, timeout, age)         an age comparison ran
//   remove(kept)                      BBBC0 then BA570 for the loaded handle
template<class Memory,class Frame> Exit sweep(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto engine_state=memory.load32(engine,globals::game_engine);
    if(memory.load8(mode,at(engine_state,engine_mode))==quiet_mode)return Exit::quiet;
    const auto setting_value=memory.load32(setting,at(memory.load32(variant,globals::engine_variant),variant_setting));
    Kept kept{};
    kept.time=memory.load32(game_time,globals::game_time);
    memory.store32(tick_store,local(Local::tick),memory.load32(tick,at(kept.time,offsetof(GameTime,tick))));
    memory.store32(setting_store,local(Local::setting),setting_value);
    memory.store32(marker_store,local(Local::iterator_marker),iterator_unset);
    memory.store32(kind_store,local(Local::iterator),iterator_types);
    memory.store8(byte_clear,local(Local::iterator_byte),0);
    memory.store16(word_clear,local(Local::iterator_word),0);
    memory.store32(handle_clear,local(Local::iterator_handle),0xffffffffu);
    kept.object=frame.iterate(kept);
    memory.store32(current_store,local(Local::current),kept.object);
    if(!kept.object)return Exit::empty;
    // An age in seconds against a timeout: true when it is not below it.
    const auto expired=[&](Age stage,std::uint32_t timeout,std::uint32_t ticks,Access seconds_role) {
        const auto converted=scalar_from_integer(ticks);
        const auto age=sse::multiply(converted,memory.load32(seconds_role,at(kept.time,offsetof(GameTime,seconds_per_tick))));
        frame.aged(stage,timeout,age);
        return sse::not_above(timeout,age);
    };
    for(;;) {
        const auto tick_slot=std::int32_t(memory.load32(slot_tick,at(kept.time,offsetof(GameTime,tick))))%std::int32_t(slot_period);
        kept.index=memory.load32(slot_handle,local(Local::iterator_handle))&0xffffu;
        if(std::int32_t(kept.index)%std::int32_t(slot_period)==tick_slot) {
            bool remove=false,reload=false;
            const auto type_bit=1u<<(std::uint32_t(std::int32_t(std::int8_t(memory.load8(type,at(kept.object,object::type)))))&31u);
            if(type_bit&object_type::items) {
                // Items: who holds them, then how long they lay free.
                kept.held=(kept.held&0xffffff00u)|1u;
                bool engine_tied=false;
                if(type_bit&object_type::weapon) {
                    const auto rows_table=memory.load32(rows,at(memory.load32(headers,globals::object_headers),datum_data));
                    const auto data=memory.load32(weapon_data,rows_table+kept.index*header_size+header_data);
                    const auto tag=memory.load32(weapon_tag,data);
                    const auto definition=memory.load32(weapon_definition,
                        (tag&0xffffu)*tag_size+memory.load32(instances,globals::tag_instances)+tag_data);
                    kept.index=widen16(memory.load16(engine_field,at(definition,weapon_engine_field)));
                    (void)memory.load32(holder_handle,local(Local::iterator_handle));
                    const auto held_now=frame.holder_check(kept)&0xffu;
                    kept.held=(kept.held&0xffffff00u)|held_now;
                    engine_tied=kept.index!=0;
                }
                if(engine_tied) {
                    // The engine variant decides whether a tied weapon may go.
                    const auto index=memory.load32(variant_index_read,at(memory.load32(variant_again,globals::engine_variant),variant_index));
                    const auto variant_state=memory.load32(variant_object,globals::variants+index*4u);
                    kept.object=memory.load32(check_handle,local(Local::iterator_handle));
                    const auto target=memory.load32(check_target,at(memory.load32(vtable,variant_state),variant_check));
                    if((frame.engine_check(kept,target)&0xffu) && !(frame.free_check(kept,true)&0xffu))remove=true;
                    reload=true;
                } else {
                    bool check=true;
                    if(memory.load8(holder,at(kept.object,object::holder))!=0xffu &&
                       !((memory.load8(item_flags,at(kept.object,object::item_flags))>>2)&1u) && (kept.held&0xffu))
                        check=false;
                    if(check) {
                        (void)memory.load32(checked_handle,local(Local::iterator_handle));
                        const bool free_item=frame.free_check(kept,false)&0xffu;
                        if(!(free_item && (kept.held&0xffu)) &&
                           !(memory.load8(item_flags_again,at(kept.object,object::item_flags))&1u)) {
                            const auto dropped_tick=memory.load32(dropped,at(kept.object,object::dropped));
                            const auto ticks=memory.load32(item_tick,local(Local::tick))-dropped_tick;
                            const auto timeout=memory.load32(item_timeout,globals::item_timeouts+memory.load32(item_setting,local(Local::setting))*4u);
                            remove=expired(Age::item,timeout,ticks,age_seconds);
                        }
                    }
                }
            } else if(type_bit&object_type::biped) {
                // Corpses: their age, then how long since they were held.
                if((memory.load8(fade_flags,at(kept.object,object::fade_flags))>>2)&1u) {
                    const auto created_tick=memory.load32(created,at(kept.object,object::created));
                    const auto ticks=memory.load32(corpse_tick,local(Local::tick))-created_tick;
                    const auto timeout=memory.load32(corpse_timeout,globals::corpse_timeouts+memory.load32(corpse_setting,local(Local::setting))*4u);
                    remove=expired(Age::corpse,timeout,ticks,corpse_seconds);
                    if(!remove) {
                        const auto setting_now=memory.load32(hold_setting,local(Local::setting));
                        const auto held_tick=memory.load32(held,at(kept.object,object::held));
                        const auto hold=memory.load32(hold_timeout,globals::corpse_hold_timeouts+setting_now*4u);
                        remove=expired(Age::hold,hold,memory.load32(age_tick,local(Local::tick))-held_tick,age_seconds);
                    }
                }
            } else if((memory.load32(object_flags,at(kept.object,object::flags))>>14)&1u) {
                // Other objects past their start tick: their age, then since the start.
                const auto start=memory.load32(start_tick,at(kept.object,object::start_tick));
                const auto now=memory.load32(other_tick,local(Local::tick));
                if(!(signed_value(now)<signed_value(start))) {
                    const auto ticks=now-memory.load32(other_created,at(kept.object,object::created));
                    const auto setting_now=memory.load32(other_setting,local(Local::setting));
                    const auto timeout=memory.load32(other_timeout,globals::other_timeouts+setting_now*4u);
                    remove=expired(Age::other,timeout,ticks,other_seconds);
                    if(!remove) {
                        const auto late=memory.load32(late_timeout,globals::other_late_timeouts+setting_now*4u);
                        remove=expired(Age::late,late,memory.load32(age_tick,local(Local::tick))-start,age_seconds);
                    }
                }
            }
            if(remove) {
                (void)memory.load32(delete_handle,local(Local::iterator_handle));
                frame.remove(kept);
                reload=true;
            }
            if(reload)kept.time=memory.load32(time_reload,globals::game_time);
        }
        kept.object=frame.iterate(kept);
        memory.store32(current_next,local(Local::current),kept.object);
        if(!kept.object)return Exit::swept;
    }
}
} // namespace object_cleanup
} // namespace halo2::native_object_cleanup
