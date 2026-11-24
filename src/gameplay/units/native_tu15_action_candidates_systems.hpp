#pragma once
// Native TU1.5 action candidates, entered at 1AE7F0 (combat action unit): a
// unit's candidate actions gathered into a list of up to 64.
//
//   action_candidates::gather   1AE7F0(scorer, unit, context, a, b, count*, candidates) -> EAX the count
//
// The unit's scale is one, or for a unit or vehicle (object type 0 or 1)
// whose definition has bit 19 of +0xBC, the game globals' value
// ([[4ECC1C]+0xF4]+8). Below 64 candidates (the count argument), 1AEA30 (EAX
// and ECX 0, EDX the unit, ESI the next 0x60-byte candidate; the context,
// NONE, 0, a and b) fills one for the unit itself, and 1AECE0 (EDI the
// scorer; the scale) keeps it, counting it. Then each entry (0x1C bytes from
// +0x6C, +0x68 of them) of the definition the unit's definition names (+0x38,
// NONE for none) is looked up (BAD60, through the dispatch: EBX a lookup
// record, the unit, the entry's kind and 2); a found entry (below 64; its
// record +0x38 when the lookup answered 2) is filled (1AEA30: ECX the entry,
// EDX the unit; the context, the entry's index, a work record, a and b) and
// kept with its weight (+0x10) times the scale (1AECE0), counting it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_action_candidates.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_action_candidates {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u,one=0x46a0d0u,game=0x4ecc1cu;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,handle_mask=0xffffu,
    none=0xffffffffu;
inline constexpr std::uint32_t most=0x40u,candidate_size=0x60u,typed_mask=3u,lookup_kind=2u,found_kind=2u;
namespace object {inline constexpr std::uint32_t tag=0u,type=0xaau;}
namespace definition {inline constexpr std::uint32_t actions=0x38u,count=0x68u,entries=0x6cu,flags=0xbcu;
    inline constexpr unsigned scaled_bit=19u;}
namespace game {inline constexpr std::uint32_t scales=0xf4u,scale=8u;}
namespace entry {inline constexpr std::uint32_t size=0x1cu,kind=0u,weight=0x10u;}
// The frame below the saved registers, and the arguments above it.
namespace local {inline constexpr std::uint32_t definition=0x10u,offset=0x14u,index=0x18u,scale=0x1cu,lookup=0x20u,work=0x58u,
    found=0x90u,found_part=0x38u,size=0xf0u;}
namespace argument {inline constexpr std::uint32_t scorer=0x104u,unit=0x108u,context=0x10cu,first=0x110u,second=0x114u,count=0x118u,
    candidates=0x11cu;}

namespace action_candidates {
enum class Access {headers_load,datums_load,one_load,unit_argument,object_load,tag_load,instances_load,definition_load,type_load,
    definition_store,scale_store,tag_again,definition_again,flags_load,game_load,scales_load,scale_override,scale_override_store,
    count_load,candidates_load,second_load,context_load,first_load,scale_load,scorer_load,count_store,definition_reload,actions_load,
    instances_again,actions_definition,entry_count,actions_store,index_store,offset_store,candidates_again,definition_loop,entries_load,
    offset_load,kind_load,count_loop,second_loop,first_loop,index_loop,context_loop,weight_load,scale_loop,scorer_loop,weight_store,
    count_increment_load,count_increment_store,offset_next,index_next,offset_store_next,definition_next,count_next,index_store_next,
    total_load};
// The original's EBX (the count, then the lookup record), EBP (the unit), ESI (the next candidate) and EDI (the scorer, then the entry).
struct Kept {std::uint32_t count,unit,candidate,held;};
// Frame: see native_tu15_action_candidates.hpp.
template<class Memory,class Frame> std::uint32_t gather(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The unit, its definition and its scale.
    frame.framing();
    const auto headers=memory.load32(headers_load,globals::object_headers);
    const auto datums=memory.load32(datums_load,at(headers,datum_data));
    const auto one=memory.load32(one_load,globals::one);
    frame.saving(0u);
    kept.unit=memory.load32(unit_argument,frame.local(argument::unit));
    const auto object_address=memory.load32(object_load,datums+header_size*frame.indexed(kept.unit)+header_address);
    const auto tag=memory.load32(tag_load,at(object_address,object::tag));
    const auto tag_index=frame.masked(tag);
    frame.saving(1u);
    const auto instances=memory.load32(instances_load,globals::tag_instances);
    const auto definition_address=memory.load32(definition_load,frame.shifted(tag_index)+instances+tag_data);
    const auto type=memory.load8(type_load,at(object_address,object::type));
    memory.store32(definition_store,frame.local(local::definition),definition_address);
    const auto bit=frame.type_bit(type);
    frame.saving(2u);
    memory.store32(scale_store,frame.local(local::scale),one);
    if(frame.typed_tested(bit)) {
        const auto tag_now=memory.load32(tag_again,at(object_address,object::tag));
        const auto definition_now=memory.load32(definition_again,frame.tag_offset(tag_now)+instances+tag_data);
        if(frame.scaled_tested(memory.load32(flags_load,at(definition_now,definition::flags)))) {
            const auto game_address=memory.load32(game_load,globals::game);
            const auto scales=memory.load32(scales_load,at(game_address,game::scales));
            const auto scale=memory.load32(scale_override,at(scales,game::scale));
            memory.store32(scale_override_store,frame.local(local::scale),scale);
        }
    }
    // The unit itself as a candidate.
    kept.count=memory.load32(count_load,frame.local(argument::count));
    if(frame.room(kept.count)) {
        const auto candidates=memory.load32(candidates_load,frame.local(argument::candidates));
        const auto second=memory.load32(second_load,frame.local(argument::second));
        const auto context=memory.load32(context_load,frame.local(argument::context));
        kept.candidate=frame.candidate(kept.count,candidates,second);
        const auto first=memory.load32(first_load,frame.local(argument::first));
        frame.unit_arguments(first,context);
        if(frame.filled_unit(kept)) {
            const auto scale=memory.load32(scale_load,frame.local(local::scale));
            kept.held=memory.load32(scorer_load,frame.local(argument::scorer));
            if(frame.kept_scored(scale,kept)) {
                kept.count=frame.counted(kept.count);
                memory.store32(count_store,frame.local(argument::count),kept.count);
            }
        }
    }
    // Each entry of the actions' definition.
    const auto definition_now=memory.load32(definition_reload,frame.local(local::definition));
    const auto actions=memory.load32(actions_load,at(definition_now,definition::actions));
    frame.compared(actions,none);
    if(actions==none){frame.restoring(false);return kept.count;}
    const auto instances_now=memory.load32(instances_again,globals::tag_instances);
    const auto actions_address=memory.load32(actions_definition,frame.tag_offset(actions)+instances_now+tag_data);
    const auto entries_count=memory.load32(entry_count,at(actions_address,definition::count));
    frame.entries_compared(entries_count);
    memory.store32(actions_store,frame.local(local::definition),actions_address);
    memory.store32(index_store,frame.local(local::index),0u);
    if(std::int32_t(entries_count)<=0){frame.restoring(false);return kept.count;}
    frame.candidate_index(kept.count);
    memory.store32(offset_store,frame.local(local::offset),0u);
    kept.candidate=frame.candidate_address(memory.load32(candidates_again,frame.local(argument::candidates)));
    for(bool first_entry=true;;first_entry=false) {
        const auto definition_value=first_entry?actions_address:memory.load32(definition_loop,frame.local(local::definition));
        const auto entries=memory.load32(entries_load,at(definition_value,definition::entries));
        const auto offset=memory.load32(offset_load,frame.local(local::offset));
        const auto kind=memory.load32(kind_load,entries+offset+entry::kind);
        kept.held=frame.entry(entries,offset);
        const auto result=frame.looked_up(kind,kept);
        if(result) {
            const auto count=memory.load32(count_loop,frame.local(argument::count));
            const auto found=frame.found(result);
            if(frame.room_compared(count)) {
                frame.found_part(found);
                const auto second=memory.load32(second_loop,frame.local(argument::second));
                const auto first=memory.load32(first_loop,frame.local(argument::first));
                frame.pass(second);
                frame.pass(first);
                const auto index=memory.load32(index_loop,frame.local(local::index));
                frame.work_passed();
                const auto context=memory.load32(context_loop,frame.local(argument::context));
                frame.pass(index);
                frame.pass(context);
                if(frame.filled_entry(kept)) {
                    const auto weight=memory.load32(weight_load,at(kept.held,entry::weight));
                    const auto weighted=sse::multiply(weight,memory.load32(scale_loop,frame.local(local::scale)));
                    kept.held=memory.load32(scorer_loop,frame.local(argument::scorer));
                    frame.weight_room();
                    memory.store32(weight_store,frame.top(),frame.weighted(weighted));
                    if(frame.entry_scored(kept)) {
                        const auto count_now=memory.load32(count_increment_load,frame.local(argument::count));
                        memory.store32(count_increment_store,frame.local(argument::count),frame.incremented(count_now));
                        kept.candidate=frame.advanced(kept.candidate);
                    }
                }
            }
        }
        const auto offset_now=memory.load32(offset_next,frame.local(local::offset));
        const auto index_now=memory.load32(index_next,frame.local(local::index));
        memory.store32(offset_store_next,frame.local(local::offset),frame.offset_advanced(offset_now));
        const auto definition_last=memory.load32(definition_next,frame.local(local::definition));
        const auto count_value=memory.load32(count_next,at(definition_last,definition::count));
        const auto next=frame.index_advanced(index_now,count_value);
        memory.store32(index_store_next,frame.local(local::index),next);
        if(!(std::int32_t(next)<std::int32_t(count_value)))break;
    }
    const auto total=memory.load32(total_load,frame.local(argument::count));
    frame.restoring(true);
    return total;
}
} // namespace action_candidates
} // namespace halo2::native_action_candidates
