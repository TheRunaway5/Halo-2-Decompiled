#pragma once
// Native TU1.5 effect element, entered at 17BD40 (death cycle unit): an
// effect part's element made (the maker 17BEB0 calls for a part without one).
//
//   effect_element::make   17BD40(EAX = part index; effect handle) -> EAX the element or 0
//
// The effect is a 0x190-byte record of the effect array (4F720C); its tag
// (+4, through the tag instances at 4F0414) holds the events (+0x18), the
// current one (+0x5C, 0x38-byte events) the parts (+0x34, 0x38 bytes). An
// owner (+0x58, unless NONE) that 158290 (through the dispatch) answers 0
// for marks the call (ESI 1, otherwise 0). 17E100 (ECX the part, EDX the
// effect) must accept the part, whose location (+8, NONE for none) names
// one of the effect's locations (+0x7C); 17DCE0 (the effect, the location,
// the part's word +0x12) answers the location's frame, 17AEB0 (EAX the
// part's type +0xE, ECX the effect's +0x1C, EDX the frame's +0x30) accepts
// it, and 176E70 (the effect handle, the effect's tag, the part index and
// the event) answers the element's handle (NONE for none). The element (a
// 0x54-byte record of the array at 51DF74) is marked (+0xC bit 0), its age
// (+4) cleared and its rate (+8) the reciprocal of the effect's scale (+0x64)
// when positive, else one; 177F10 (the effect's +0xFC, EBX its +0x100)
// prepares it, it takes the location (+0x24), and 17BB10 (the effect, the
// element, the part, 0 and whether the part is active, word +0xC) starts it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_effect_element.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_effect_element {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t effects=0x4f720cu,tag_instances=0x4f0414u,elements=0x51df74u,one=0x46a0d0u;}
inline constexpr std::uint32_t datum_data=0x44u,tag_size=16u,tag_data=8u,handle_mask=0xffffu,none=0xffffffffu;
namespace effect {inline constexpr std::uint32_t size=0x190u,tag=4u,frame_base=0x1cu,owner=0x58u,event=0x5cu,scale=0x64u,locations=0x7cu,
    first_list=0xfcu,second_list=0x100u;}
namespace definition {inline constexpr std::uint32_t events=0x18u;}
namespace event {inline constexpr std::uint32_t size=0x38u,parts=0x34u;}
namespace part {inline constexpr std::uint32_t size=0x38u,location=8u,active=0xcu,type=0xeu,word=0x12u;}
namespace frame {inline constexpr std::uint32_t data=0x30u;}
namespace element {inline constexpr std::uint32_t size=0x54u,age=4u,rate=8u,flags=0xcu,location=0x24u;inline constexpr std::uint32_t marked=1u;}
// The frame below the saved registers: the location's value, the location.
namespace local {inline constexpr std::uint32_t location_value=0x10u,location=0x14u,effect_handle=0x1cu,size=8u;}

namespace effect_element {
enum class Access {instances,effect_handle,effects_load,datums_load,tag_load,definition_load,event_index,events_load,owner_load,parts_load,
    location_load,location_store,location_value,location_value_store,part_word,part_type,event_again,tag_again,handle_again,
    elements_load,element_datums,scale_load,flags_load,flags_store,age_store,one_load,rate_store,location_again,element_location,
    active_load};
enum class Outcome {failed,made};
// The original's EBX (the part index), EBP (the effect), ESI (the owner's
// mark, then the part's type, then the element) and EDI (the part) across
// the children.
struct Kept {std::uint32_t index,effect_address,held,part_address;};
// Frame: see native_tu15_effect_element.hpp.
template<class Memory,class Frame> Outcome make(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The effect, its event's parts, the part.
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    frame.framing();
    const auto handle=memory.load32(effect_handle,frame.local(local::effect_handle));
    const auto effect_offset=frame.effect_offset(handle);
    kept.index=frame.part_index();
    const auto effects=memory.load32(effects_load,globals::effects);
    kept.effect_address=frame.effect_address(effect_offset,memory.load32(datums_load,at(effects,datum_data)));
    const auto tag=memory.load32(tag_load,at(kept.effect_address,effect::tag));
    const auto definition_address=memory.load32(definition_load,frame.tag_offset(tag)+instances_address+tag_data);
    const auto event_number=memory.load16(event_index,at(kept.effect_address,effect::event));
    const auto events=memory.load32(events_load,at(definition_address,definition::events));
    const auto event_offset=frame.event_offset(event_number);
    const auto owner=memory.load32(owner_load,at(kept.effect_address,effect::owner));
    frame.saving(0u);
    const auto parts=memory.load32(parts_load,events+event_offset+event::parts);
    frame.saving(1u);
    kept.part_address=frame.part_address(kept.index,parts);
    kept.held=frame.unmarked();
    frame.owner_compared(owner);
    if(owner!=none && !frame.classified(kept))kept.held=frame.marked();
    // The part accepted; its location's frame accepted; the element's handle.
    const auto fail=[&]{frame.restoring(false);return Outcome::failed;};
    if(!frame.accepted(kept))return fail();
    const auto location=memory.load32(location_load,at(kept.part_address,part::location));
    frame.location_compared(location);
    memory.store32(location_store,frame.local(local::location),location);
    if(location==none)return fail();
    const auto value=memory.load32(location_value,at(kept.effect_address,effect::locations+4u*location));
    memory.store32(location_value_store,frame.local(local::location_value),value);
    frame.word_cleared();
    const auto word=memory.load16(part_word,at(kept.part_address,part::word));
    const auto frame_address=frame.located(word,kept);
    if(!frame_address)return fail();
    kept.held=frame.type_cleared();
    kept.held=memory.load16(part_type,at(kept.part_address,part::type));
    if(!frame.frame_accepted(kept.held,frame_address,kept))return fail();
    const auto event_now=memory.load16(event_again,at(kept.effect_address,effect::event));
    const auto tag_now=memory.load32(tag_again,at(kept.effect_address,effect::tag));
    const auto handle_now=memory.load32(handle_again,frame.local(local::effect_handle));
    const auto element_handle=frame.handled(event_now,tag_now,handle_now,kept);
    if(element_handle==none)return fail();
    // The element: marked, its age cleared, its rate the scale's reciprocal (or one).
    const auto elements=memory.load32(elements_load,globals::elements);
    const auto datums=memory.load32(element_datums,at(elements,datum_data));
    const auto scale=memory.load32(scale_load,at(kept.effect_address,effect::scale));
    kept.held=frame.element_address(element_handle,datums);
    const auto flags=memory.load8(flags_load,at(kept.held,element::flags));
    memory.store8(flags_store,at(kept.held,element::flags),frame.ored(flags,element::marked));
    const bool positive=frame.scale_compared(scale);
    memory.store32(age_store,at(kept.held,element::age),0u);
    const auto one=memory.load32(one_load,globals::one);
    const auto rate=positive?sse::divide(one,scale):one;
    frame.rated(positive,rate);
    memory.store32(rate_store,at(kept.held,element::rate),rate);
    // Prepared, located and started.
    frame.prepared(kept);
    const auto location_now=memory.load32(location_again,frame.local(local::location));
    memory.store32(element_location,at(kept.held,element::location),location_now);
    const auto active=memory.load16(active_load,at(kept.part_address,part::active));
    frame.active_compared(active);
    frame.started(kept);
    frame.restoring(true);
    return Outcome::made;
}
} // namespace effect_element
} // namespace halo2::native_effect_element
