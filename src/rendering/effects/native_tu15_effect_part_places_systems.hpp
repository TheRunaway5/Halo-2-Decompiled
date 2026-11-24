#pragma once
// Native TU1.5 effect part places, entered at 17CD10 (shadow frame and lens
// flare units): the completed effect-part bookkeeping, each part of an
// effect's current event placed at each of its locations.
//
//   effect_part_places::place   17CD10(effect) -> RET 4
//
// The effect's definition (through the tag instances at 4F0414) has its
// events (0x38 bytes from +0x18), the effect its current event (int16
// +0x5C); the event its parts (a count +0x20, 0x3C-byte parts from +0x24).
// Each part, in order, takes the effect's amount over its scale (+0x60 over
// +0x64, 0 unless the scale is above 0) and walks its location chain: the
// effect's location for the part (+0x7C by the part's int16 +8), then each
// location's next (+4) in the location pool ([4F7208]: 0x3C-byte records)
// until NONE. A location on a node (word +2 not NONE) has its transform
// composed: the effect's node matrix (17DD50, ECX the first frame buffer,
// EDX the effect) with the location's local transform (+8; 145560, ECX the
// first buffer, EDX the transform, the second buffer pushed); 159690 then
// places the part (the part, the ratio and the composed transform pushed).
// A location on no node hands 159690 its local transform as it is. The
// event's part count is read again after each part.
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (Access), including the
// original's frame (the event, the part's index and offset, the ratio, the
// next location); the boundary adapter (native_tu15_effect_part_places.hpp)
// maps the role to the original instruction and owns the ABI of the
// children. The values the original keeps in callee-saved registers across
// them are a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_effect_part_places {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u,locations=0x4f7208u;}
inline constexpr std::uint32_t tag_size=16u,tag_data=8u,datum_data=0x44u,event_size=0x38u,part_size=0x3cu,location_size=0x3cu;
namespace effect {inline constexpr std::uint32_t tag=4u,event=0x5cu,amount=0x60u,scale=0x64u,locations=0x7cu;}
namespace definition {inline constexpr std::uint32_t events=0x18u;}
namespace event {inline constexpr std::uint32_t parts=0x20u,first_part=0x24u;}
namespace part {inline constexpr std::uint32_t location=8u;}
namespace location {inline constexpr std::uint32_t node=2u,next=4u,transform=8u;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t next_location=-0x7c,part_offset=-0x78,ratio=-0x74,event_address=-0x70,part_index=-0x6c,
    node_frame=-0x68,composed=-0x34;}

namespace effect_part_places {
enum class Access {instances,effect_argument,effect_tag,definition_address,event_index,events,part_count,event_store,index_store,
    offset_store,first_part,offset_load,scale,amount,location_index,first_location,ratio_store,next_store,ratio_load,pool,pool_data,
    next_location,next_again,node,next_load,event_load,index_load,offset_again,count_again,index_again,offset_store_again};
enum class Outcome {no_parts,placed};
// The original's EBX (the ratio), EBP (the effect), ESI (the location) and EDI (the part) across the children.
struct Kept {std::uint32_t ratio,effect,location,part;};
// Frame:
//   argument(), local(offset)           the argument's slot and the frame's slots now
//   counted(count), saving(), saved()   the part count's CMP; EBX and EDI saved
//   scaled(scale, scaled, ratio)        the ratio's XORPS, COMISS and DIVSS in the original's registers
//   located(handle), on_node(node)      the location loop's CMP EAX,-1 and CMP AX,FFFF
//   node_frame(kept), compose(kept)     17DD50 and 145560, refresh kept
//   place(transform, kept)              159690(the part, the ratio, transform), refreshes kept
//   advanced(index, offset, count)      INC EAX; ADD ESI,3C; CMP EAX,EDX
template<class Memory,class Frame> Outcome place(Memory &memory,Frame &frame) {
    using enum Access;
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    Kept kept{};
    kept.effect=memory.load32(effect_argument,frame.argument());
    const auto tag_value=memory.load32(effect_tag,at(kept.effect,effect::tag));
    const auto definition_value=memory.load32(definition_address,tag_size*(tag_value&0xffffu)+instances_address+tag_data);
    const auto event_offset=event_size*widen16(memory.load16(event_index,at(kept.effect,effect::event)));
    auto event_address=event_offset+memory.load32(events,at(definition_value,definition::events));
    const auto count=memory.load32(part_count,at(event_address,event::parts));
    frame.counted(count);
    memory.store32(event_store,frame.local(local::event_address),event_address);
    memory.store32(index_store,frame.local(local::part_index),0u);
    if(signed_value(count)<=0)return Outcome::no_parts;
    frame.saving();
    memory.store32(offset_store,frame.local(local::part_offset),0u);
    frame.saved();
    for(;;) {
        const auto parts=memory.load32(first_part,at(event_address,event::first_part));
        kept.part=parts+memory.load32(offset_load,frame.local(local::part_offset));
        const auto scale_value=memory.load32(scale,at(kept.effect,effect::scale));
        std::uint32_t ratio_value=0u;
        const bool scaled=!sse::not_above(scale_value,0u);
        std::uint32_t amount_value=0u;
        if(scaled) {
            amount_value=memory.load32(amount,at(kept.effect,effect::amount));
            ratio_value=sse::divide(amount_value,scale_value);
        }
        frame.scaled(scale_value,scaled,ratio_value);
        const auto slot=widen16(memory.load16(location_index,at(kept.part,part::location)));
        auto handle=memory.load32(first_location,kept.effect+effect::locations+4u*slot);
        memory.store32(ratio_store,frame.local(local::ratio),ratio_value);
        memory.store32(next_store,frame.local(local::next_location),handle);
        kept.ratio=memory.load32(ratio_load,frame.local(local::ratio));
        for(;;) {
            frame.located(handle);
            if(handle==none)break;
            const auto data=memory.load32(pool_data,at(memory.load32(pool,globals::locations),datum_data));
            kept.location=location_size*(handle&0xffffu)+data;
            const auto next=memory.load32(next_location,at(kept.location,location::next));
            memory.store32(next_again,frame.local(local::next_location),next);
            const auto node_value=memory.load16(node,at(kept.location,location::node));
            frame.on_node(node_value);
            if(node_value!=0xffffu) {
                frame.node_frame(kept);
                frame.compose(at(kept.location,location::transform),kept);
                frame.place(frame.local(local::composed),kept);
            } else {
                frame.place(at(kept.location,location::transform),kept);
            }
            handle=memory.load32(next_load,frame.local(local::next_location));
            event_address=memory.load32(event_load,frame.local(local::event_address));
        }
        const auto index=memory.load32(index_load,frame.local(local::part_index))+1u;
        const auto offset=memory.load32(offset_again,frame.local(local::part_offset))+part_size;
        const auto parts_now=memory.load32(count_again,at(event_address,event::parts));
        frame.advanced(index,offset,parts_now);
        memory.store32(index_again,frame.local(local::part_index),index);
        memory.store32(offset_store_again,frame.local(local::part_offset),offset);
        if(!(signed_value(index)<signed_value(parts_now)))return Outcome::placed;
    }
}
} // namespace effect_part_places
} // namespace halo2::native_effect_part_places
