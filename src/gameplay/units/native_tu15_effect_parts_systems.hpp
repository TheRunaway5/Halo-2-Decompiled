#pragma once
// Native TU1.5 effect event parts, entered at 17BEB0 (death cycle unit): the
// parts of an effect's current event that apply are created.
//
//   effect_parts::create   17BEB0(effect handle, scale, location)
//
// The effect is a 0x190-byte record of the effect array (4F720C); its tag
// (+4, through the tag instances at 4F0414) holds the events (+0x18), of
// which the effect's current one (+0x5C, 0x38-byte events) holds the parts
// (+0x30 count, +0x34 the 0x38-byte parts). The owner (+0x58) unless NONE
// is classified by 158290; a result of 0 marks the owner.
//
// For each part, in order: the part is active when its word +0xC is not 0.
// Its environment (+0x10) must differ from the effect's (1 when the
// effect's flag 0x20 at +2 is set, else 2). With a single local player (the
// word at [4F54F0]+8 is 1) its mode (+0x12) further decides: mode 2 parts
// apply only for an unmarked owner, mode 1 parts only for a marked one. An
// applying part finds its element among the effect's sixteen (part, handle)
// slots (+0x110; the handles name 0x54-byte elements of the array at
// 51DF74), or makes one (17BD40 for the part's index); with an element and
// a type (+8, NONE for none), 17BB10 creates the part with the scale.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_effect_parts.hpp) maps the role to the original instruction
// and owns the ABI of the children (158290, 17BD40, 17BB10). The values the
// original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_effect_parts {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;

namespace globals {
inline constexpr std::uint32_t effects=0x4f720cu;          // DataArray* of 0x190-byte effects
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
inline constexpr std::uint32_t players=0x4f54f0u;          // +8: the local player count (word)
inline constexpr std::uint32_t elements=0x51df74u;         // DataArray* of 0x54-byte elements
}
inline constexpr std::uint32_t datum_data=0x44u,effect_size=0x190u,element_size=0x54u,tag_instance_shift=4u,
    instance_address=8u,event_size=0x38u,part_size=0x38u,slot_count=16u,slot_size=8u;
namespace effect {
inline constexpr std::uint32_t tag=4u,flags=2u,owner=0x58u,event=0x5cu,slots=0x110u;
inline constexpr std::uint32_t environment_flag=5u;   // flag 0x20 at +2
}
namespace tag {inline constexpr std::uint32_t events=0x18u;}
namespace event {inline constexpr std::uint32_t part_count=0x30u,parts=0x34u;}
namespace part {inline constexpr std::uint32_t type=8u,active=0xcu,environment=0x10u,mode=0x12u;}
namespace mode {inline constexpr std::uint32_t marked_owner=1u,unmarked_owner=2u;}

namespace effect_parts {
// The arguments, and the locals of the 0xC-byte frame: the owner mark, the
// part index and the part's active byte.
enum class Local {handle,scale,location,owner_mark,index,active};
enum class Access {
    effects,effects_data,tag_instances,handle_argument,effect_tag,effect_event,tag_address,owner,events,owner_mark_init,
    owner_mark_set,part_count,index_init,parts,effect_flags,part_environment,part_active,active_store,players,
    player_count,part_mode,owner_mark_unmarked,owner_mark_marked,slot_part,slot_handle,elements,element_data,
    handle_again,index_for_element,part_type,active_argument,location_argument,scale_argument,index_read,
    part_count_again,index_store
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t effect;      // EDI: the effect record
    std::uint32_t event;       // EBP: the event
    std::uint32_t offset;      // EBX: the part's offset in the parts
    std::uint32_t part;        // ESI: the part (once the parts are walked)
};
enum class Exit {no_parts,walked};

// Frame:
//   local(Local)                 an argument's or local's address now
//   classify_owner(owner, kept)  158290(EAX the owner); returns EAX,
//                                refreshes kept
//   search_begins()              the slot search's registers
//   make_element(kept)           17BD40(EAX the index; the handle);
//                                returns EAX, refreshes kept
//   create_part(element, kept)   17BB10(effect, element, part, location,
//                                active; XMM0 the scale); refreshes kept
template<class Memory,class Frame> Exit create(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Kept kept{};
    // The effect, its tag and its current event.
    const auto array=memory.load32(effects,globals::effects);
    const auto effect_data=memory.load32(effects_data,at(array,datum_data));
    const auto instances=memory.load32(tag_instances,globals::tag_instances);
    const auto handle=memory.load32(handle_argument,local(Local::handle));
    const auto tag_index=memory.load32(effect_tag,at(effect_data+(handle&0xffffu)*effect_size,effect::tag));
    kept.effect=effect_data+(handle&0xffffu)*effect_size;
    const auto event_index=widen16(memory.load16(effect_event,at(kept.effect,effect::event)));
    const auto tag_definition=memory.load32(tag_address,at(instances+((tag_index&0xffffu)<<tag_instance_shift),instance_address));
    const auto events_address=memory.load32(events,at(tag_definition,tag::events));
    const auto owner_handle=memory.load32(owner,at(kept.effect,effect::owner));
    kept.event=event_index*event_size+events_address;
    kept.offset=0;
    memory.store32(owner_mark_init,local(Local::owner_mark),0u);
    if(owner_handle!=none && frame.classify_owner(owner_handle,kept)==0)
        memory.store32(owner_mark_set,local(Local::owner_mark),1u);
    // The first part's offset (EBX, 0) is also the first index.
    const auto count=memory.load32(part_count,at(kept.event,event::part_count));
    memory.store32(index_init,local(Local::index),kept.offset);
    if(std::int32_t(count)<=std::int32_t(kept.offset))return Exit::no_parts;
    for(;;) {
        const auto parts_address=memory.load32(parts,at(kept.event,event::parts));
        const auto effect_flags_byte=memory.load8(effect_flags,at(kept.effect,effect::flags));
        const auto environment=memory.load16(part_environment,at(parts_address+kept.offset,part::environment));
        kept.part=parts_address+kept.offset;
        const bool active=memory.load16(part_active,at(kept.part,part::active))!=0;
        memory.store8(active_store,local(Local::active),active?1u:0u);
        const auto effect_environment=(effect_flags_byte>>effect::environment_flag)&1u?1u:2u;
        const bool differs=environment!=effect_environment;
        const auto player_globals=memory.load32(players,globals::players);
        bool applies;
        if(memory.load16(player_count,at(player_globals,8u))!=1u)applies=differs;
        else if(!differs)applies=false;
        else {
            const auto mode_word=memory.load16(part_mode,at(kept.part,part::mode));
            applies=true;
            if(mode_word==mode::unmarked_owner && memory.load32(owner_mark_unmarked,local(Local::owner_mark))!=0)
                applies=false;
            else if(mode_word==mode::marked_owner)
                applies=memory.load32(owner_mark_marked,local(Local::owner_mark))==1u;
        }
        if(applies) {
            // The part's element: from the effect's slots, or made.
            std::uint32_t element=0;
            frame.search_begins();
            for(std::uint32_t slot=0;slot<slot_count;++slot) {
                const auto entry=at(kept.effect,effect::slots+slot*slot_size);
                if(memory.load32(slot_part,entry)==kept.part) {
                    const auto element_handle=memory.load32(slot_handle,at(entry,4u));
                    const auto element_array=memory.load32(elements,globals::elements);
                    element=(element_handle&0xffffu)*element_size+memory.load32(element_data,at(element_array,datum_data));
                    break;
                }
            }
            if(element==0) {
                (void)memory.load32(handle_again,local(Local::handle));
                (void)memory.load32(index_for_element,local(Local::index));
                element=frame.make_element(kept);
            }
            if(element!=0 && memory.load32(part_type,at(kept.part,part::type))!=none) {
                (void)memory.load32(active_argument,local(Local::active));
                (void)memory.load32(location_argument,local(Local::location));
                (void)memory.load32(scale_argument,local(Local::scale));
                frame.create_part(element,kept);
            }
        }
        const auto index=memory.load32(index_read,local(Local::index))+1u;
        const auto parts_now=memory.load32(part_count_again,at(kept.event,event::part_count));
        kept.offset+=part_size;
        memory.store32(index_store,local(Local::index),index);
        if(!(std::int32_t(index)<std::int32_t(parts_now)))break;
    }
    return Exit::walked;
}
} // namespace effect_parts
} // namespace halo2::native_effect_parts
