#pragma once
// Native TU1.5 effect elements draw, entered at 1770C0 (effect particle draw
// unit): every effect element considered, and the visible ones drawn.
//
//   effect_elements_draw::draw   1770C0(first person byte)
//
// The elements (0x54-byte records of the array at 51DF74) are walked with
// 16EA60 (through the dispatch: ECX the array, EDX the next index; the
// handle the record's salt word over the index). Each element's definition
// (178070, through the dispatch, ECX the element) may always draw (bit 9 of
// its word +0x16); otherwise its cluster (word +0x20, NONE for none) must be
// set in the visible clusters (bits at 557034) or 3EB10 must answer. An
// element with no parent (+0x4C NONE) and a location (+0x30) is drawn when
// its view (the local unit [4C66F8], which 158290 may mark, against the
// effect's owner +0x58 and 17DE60) matches the first person byte: 1772C0
// (the handle, the first person flag, the active flag, 0) draws it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_effect_elements_draw.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_effect_elements_draw {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t elements=0x51df74u,effects=0x4f720cu,local_unit=0x4c66f8u,players=0x4f9b54u,
    visible_clusters=0x557034u;}
inline constexpr std::uint32_t none=0xffffffffu,word_none=0xffffu,handle_mask=0xffffu,salt_shift=16u;
namespace array {inline constexpr std::uint32_t size=0x24u,data=0x44u;}
namespace element {inline constexpr std::uint32_t size=0x54u,effect=0x18u,cluster=0x20u,location=0x30u,parent=0x4cu;}
namespace effect {inline constexpr std::uint32_t size=0x190u,owner=0x58u;}
namespace definition {inline constexpr std::uint32_t active=0xcu,kind=0x12u,flags=0x16u;inline constexpr unsigned always_bit=9u,shown_bit=6u;
    inline constexpr std::uint32_t hidden_kind=2u;}
namespace player {inline constexpr std::uint32_t size=0x94u,view=0x42u;}
// The frame below the saved registers: the handle, the definition, the first person flag, the active flag, the index; the argument.
namespace local {inline constexpr std::uint32_t handle=0x10u,definition=0x14u,first_person=0x18u,active=0x1cu,index=0x20u,argument=0x28u,
    size=0x14u;}

namespace effect_elements_draw {
enum class Access {elements_load,array_size,array_data,salt_load,handle_store,handle_load,data_load,index_store,cluster_load,flags_load,
    visible_load,parent_load,location_load,effect_load,definition_store,effects_load,effect_datums,owner_load,local_unit_load,
    first_person_set,first_person_clear,local_unit_again,first_person_load,argument_load,active_clear,players_load,view_load,
    definition_load,shown_load,definition_again,active_word,active_set,definition_kind,kind_word,active_dword,first_person_dword,
    handle_argument,handle_next,index_next,elements_next,array_size_next,array_data_next,salt_next,handle_store_next};
// The original's EBX (the mark), EBP (NONE, then the owner), ESI (the element, then the local unit) and EDI (the array, then the effect).
struct Kept {std::uint32_t mark,owner,held,array_or_effect;};
// Frame: see native_tu15_effect_elements_draw.hpp.
template<class Memory,class Frame> void draw(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The element at an index's handle (its salt word over the index), or NONE.
    const auto handle_of=[&](Access size_role,Access data_role,Access salt_role,std::uint32_t array_address,std::uint32_t index) {
        if(!frame.found(index))return frame.no_handle();
        const auto element_size=memory.load32(size_role,at(array_address,array::size));
        const auto data=memory.load32(data_role,at(array_address,array::data));
        const auto salt=memory.load16(salt_role,frame.element_offset(element_size,index)+data);
        return frame.handle(salt,index);
    };
    frame.framing();
    kept.array_or_effect=memory.load32(elements_load,globals::elements);
    frame.zero_index();
    const auto first_index=frame.next(kept);
    kept.owner=frame.reset_owner();
    auto handle=handle_of(array_size,array_data,salt_load,kept.array_or_effect,first_index);
    frame.handle_compared(handle,kept.owner);
    memory.store32(handle_store,frame.local(local::handle),handle);
    if(handle==kept.owner){frame.restoring(false);return;}
    frame.saving();
    for(;;) {
        // The element, drawn when visible and placed.
        const auto current=memory.load32(handle_load,frame.local(local::handle));
        const auto data=memory.load32(data_load,at(kept.array_or_effect,array::data));
        const auto index=frame.masked(current);
        memory.store32(index_store,frame.local(local::index),index);
        kept.held=frame.element_address(index,data);
        const auto definition_address=frame.defined(kept);
        const auto cluster=memory.load16(cluster_load,at(kept.held,element::cluster));
        bool considered=frame.cluster_compared(cluster,kept.owner);
        if(considered && !frame.always(memory.load16(flags_load,at(definition_address,definition::flags)))) {
            const auto bit=frame.cluster_bit(cluster);
            const auto visible=memory.load32(visible_load,globals::visible_clusters+4u*frame.cluster_word(cluster));
            if(!frame.visible(visible,bit))considered=frame.drawn_anyway(kept);
        }
        if(considered)considered=frame.parent_compared(memory.load32(parent_load,at(kept.held,element::parent)),kept.owner);
        if(considered)considered=frame.location_compared(memory.load32(location_load,at(kept.held,element::location)),kept.owner);
        if(considered) {
            // Its effect's owner, the local unit's mark and whether the owner holds it.
            kept.array_or_effect=memory.load32(effect_load,at(kept.held,element::effect));
            const auto definition_now=frame.defined_again(kept);
            const bool effect_known=frame.effect_compared(kept.array_or_effect,kept.owner);
            memory.store32(definition_store,frame.local(local::definition),definition_now);
            if(effect_known) {
                const auto effects=memory.load32(effects_load,globals::effects);
                const auto datums=memory.load32(effect_datums,at(effects,array::data));
                kept.owner=memory.load32(owner_load,frame.effect_offset(kept.array_or_effect)+datums+effect::owner);
            }
            kept.held=memory.load32(local_unit_load,globals::local_unit);
            kept.mark=frame.unmarked();
            if(frame.unit_compared(kept.held) && !frame.classified(kept))kept.mark=frame.marked();
            bool checked;
            if(frame.owner_tested(kept.owner) && frame.owner_compared(kept.held,kept.owner)) {
                const bool mark=frame.mark_tested(kept.mark);
                memory.store8(first_person_set,frame.local(local::first_person),1u);
                checked=mark;
                if(!mark) {
                    frame.mark_tested(kept.mark);
                    memory.store8(first_person_clear,frame.local(local::first_person),0u);
                }
            } else {
                checked=frame.mark_tested(kept.mark);
                memory.store8(first_person_clear,frame.local(local::first_person),0u);
            }
            bool held=false;
            if(checked && frame.effect_tested(kept.array_or_effect)) {
                held=frame.held_by(kept);
                kept.held=memory.load32(local_unit_again,globals::local_unit);
            }
            frame.seen(held);
            // The first person byte against the argument; the active flag; drawn.
            const auto first_person=memory.load8(first_person_load,frame.local(local::first_person));
            const auto argument=memory.load8(argument_load,frame.local(local::argument));
            bool draw_it=frame.first_person_compared(first_person,argument);
            memory.store8(active_clear,frame.local(local::active),0u);
            bool skipped=false;
            if(first_person&0xffu) {
                if(frame.unit_compared(kept.held)) {
                    const auto players=memory.load32(players_load,globals::players);
                    const auto view=memory.load16(view_load,frame.player_offset(kept.held)+players+player::view);
                    if(frame.view_compared(view)) {
                        const auto definition_value=memory.load32(definition_load,frame.local(local::definition));
                        draw_it=frame.shown(memory.load8(shown_load,at(definition_value,definition::flags)));
                    }
                }
                const auto definition_value=memory.load32(definition_again,frame.local(local::definition));
                if(frame.active_compared(memory.load16(active_word,at(definition_value,definition::active))))
                    memory.store8(active_set,frame.local(local::active),1u);
            } else if(frame.seen_tested()) {
                const auto definition_value=memory.load32(definition_kind,frame.local(local::definition));
                skipped=frame.kind_compared(memory.load16(kind_word,at(definition_value,definition::kind)));
            }
            if(!skipped && frame.draw_tested(draw_it)) {
                const auto active=memory.load32(active_dword,frame.local(local::active));
                const auto flag=memory.load32(first_person_dword,frame.local(local::first_person));
                const auto element_handle=memory.load32(handle_argument,frame.local(local::handle));
                frame.drawn(element_handle,flag,active,kept);
            }
        }
        // The next element.
        const auto current_now=memory.load32(handle_next,frame.local(local::handle));
        kept.owner=frame.reset_owner();
        if(frame.handle_compared(current_now,kept.owner))frame.incremented(memory.load32(index_next,frame.local(local::index)));
        else frame.zero_index();
        kept.array_or_effect=memory.load32(elements_next,globals::elements);
        const auto next_index=frame.next(kept);
        handle=handle_of(array_size_next,array_data_next,salt_next,kept.array_or_effect,next_index);
        frame.handle_compared(handle,kept.owner);
        memory.store32(handle_store_next,frame.local(local::handle),handle);
        if(handle==kept.owner)break;
    }
    frame.restoring(true);
}
} // namespace effect_elements_draw
} // namespace halo2::native_effect_elements_draw
