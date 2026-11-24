#pragma once
// Native TU1.5 channel events, entered at 1CF700 (physics live support unit;
// TU1.1 1CAE40): the events, sounds and effects of an animation that a
// channel's step passes, handed to a callback.
//
//   channel_events::fire   1CF700(EBX = callback; definition, animation, options, from, to, context) -> EAX mask, RET 18
//
// The step's range runs from the smaller of from and to to the larger (each
// kept in the frame); an entry is passed when its frame (int16 +2) is at
// least the start and below the end. Three passes, each skipped under an
// option bit:
//   - events (option bit 7; a count +0x4C, 4-byte events at +0x50): each
//     passed event sets its bit (int16 +0) in the answer mask, and with a
//     callback is handed on as a record of type 0;
//   - sounds (option bit 8; a count +0x54, 8-byte sounds at +0x58) and
//     effects (option bit 9; a count +0x5C, 4-byte effects at +0x60): a
//     passed entry naming a reference (int16 +0, not NONE) of the
//     definition's sounds (+0x18) or effects (+0x20), 12 bytes each, with a
//     tag (+4, not NONE), is handed on as a record of type 1 or 2 when its
//     flags (word +8) allow: bit 1 needs option bit 13, bit 2 option bit 12,
//     bit 4 a forward step (from not above to), bit 5 a backward one.
// The callback (stdcall) gets the context, the entry's frame and a 20-byte
// record in the frame: the type (word +0), the entry's frame (word +2), the
// tag (+4; NONE for events), the event's bit (+8; NONE for the others), the
// flags (+0xC; 0 for events), and +0x10: the sound's +4, else 0x600008A.
// The range's ends are read again after each callback.
//
// The single-precision compares are the original's scalar SSE, in its order,
// on the live MXCSR. This is the game logic only. The game's memory is the
// retained TU1.5 bank, reached through a Memory object with typed loads and
// stores; every access names its role (Access), including the original's
// frame (the mask, the range, the entry's frame, the record) and its
// argument slots; the boundary adapter (native_tu15_channel_events.hpp) maps
// the role to the original instruction and owns the ABI of the callback. The
// callback and the loop registers the original keeps across it are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_channel_events {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
inline constexpr std::uint32_t reference_size=12u,no_event_name=0x600008au;
namespace animation {inline constexpr std::uint32_t event_count=0x4cu,events=0x50u,sound_count=0x54u,sounds=0x58u,effect_count=0x5cu,
    effects=0x60u;}
inline constexpr std::uint32_t event_size=4u,sound_size=8u,effect_size=4u;
namespace entry {inline constexpr std::uint32_t name=0u,frame=2u,sound_extra=4u;}
namespace definition {inline constexpr std::uint32_t sounds=0x18u,effects=0x20u;}
namespace reference {inline constexpr std::uint32_t tag=4u,flags=8u;}
namespace option {inline constexpr std::uint32_t no_events=0x80u,no_sounds=0x100u,no_effects=0x200u,second_gate=0x1000u,first_gate=0x2000u;}
namespace gate {inline constexpr std::uint32_t first=2u,second=4u,forward_only=0x10u,backward_only=0x20u;}
namespace argument {inline constexpr std::uint32_t definition=0u,animation=1u,options=2u,from=3u,to=4u,context=5u;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t mask=-0x28,start=-0x24,end=-0x20,entry_frame=-0x1c,sound_count=-0x18,effect_frame=-0x18,
    record=-0x14;}
namespace record {inline constexpr std::uint32_t type=0u,frame=2u,tag=4u,bit=8u,flags=0xcu,extra=0x10u;}

namespace channel_events {
// Each role is one instruction per pass; the adapter maps (role, pass) to it.
enum class Access {to_load,from_load,mask_clear,start_store,start_from,end_store,end_to,options_load,pass_animation,pass_count,
    count_store,animation_load,list_load,entry_frame,start_load,entry_frame_store,end_load,event_bit,mask_load,mask_store,entry_name,
    definition_load,references,reference_tag,reference_flags,options_gate,options_gate_second,sound_extra,frame_load,record_bit,
    record_frame,record_type,record_flags,record_tag,record_extra,context_load,to_again,from_again,count_load,mask_answer};
// Which list a pass walks.
enum class Pass {events,sounds,effects};
// The original's EBX (the callback) and the pass's index and count registers across the callback.
struct Kept {std::uint32_t callback,index,count;};
// Frame:
//   argument(k), local(offset)                     the argument slots and the frame's slots now
//   saving(), saved()                              PUSH EBP; PUSH ESI; XOR ESI,ESI; (COMISS) PUSH EDI
//   ranged(backward)                               the range's second COMISS
//   passing(pass, options, skipped)                a pass's skip test (TEST AL,AL / TEST AH,1 / TEST AH,2)
//   counted(count, kept)                           the pass's count in its register and its test
//   sound_entry(address, frame), effect_entry(address, frame)   the entry's address and frame word in the pass's registers
//   converted(at_frame), entry(below_start, before_end)          the entry's CVTSI2SS, then the range's COMISS pair
//   marked(bit, mask, kept)                        MOV EAX,1; SHL EAX,CL; OR EBP,EAX; TEST EBX,EBX
//   named(name), referenced(reference, tag, tag_read)            the reference's checks (with TEST EBX,EBX in the sounds')
//   gate_set(flags, forward_ok), gated(forward_ok), callback_tested(kept), flags_word(flags)   the gates' CL and tests
//   restoring()                                    POP EDI; POP ESI; POP EBP
//   pushing(value), call_back(kept)                the callback's three arguments and CALL EBX, refreshing kept
//   advanced(index, count)                         the loop's INC and CMP
template<class Memory,class Frame> std::uint32_t fire(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The range: from the smaller end to the larger, in the frame.
    auto to=memory.load32(to_load,frame.argument(argument::to));
    auto from=memory.load32(from_load,frame.argument(argument::from));
    frame.saving();
    const bool backward=!sse::not_above(from,to);
    frame.saved();
    memory.store32(mask_clear,frame.local(local::mask),0u);
    memory.store32(start_store,frame.local(local::start),to);
    if(!backward)memory.store32(start_from,frame.local(local::start),from);
    const bool backward_again=!sse::not_above(from,to);
    frame.ranged(backward_again);
    memory.store32(end_store,frame.local(local::end),from);
    if(!backward_again)memory.store32(end_to,frame.local(local::end),to);
    // Whether an entry's frame lies in the range (the frame kept in the pass's slot).
    const auto in_range=[&](std::uint32_t frame_word,std::int32_t slot,std::uint32_t &at_frame) {
        at_frame=scalar::from_integer(widen16(frame_word));
        frame.converted(at_frame);
        const bool below_start=sse::below(at_frame,memory.load32(start_load,frame.local(local::start)));
        memory.store32(entry_frame_store,frame.local(slot),at_frame);
        bool before_end=false;
        if(!below_start)before_end=!sse::not_above(memory.load32(end_load,frame.local(local::end)),at_frame);
        frame.entry(below_start,before_end);
        return !below_start && before_end;
    };
    // A sound's or effect's reference flags against the options and the step's direction.
    const auto allowed=[&](std::uint32_t flags) {
        if((flags&gate::first) && !(memory.load32(options_gate,frame.argument(argument::options))&option::first_gate))return false;
        if((flags&gate::second) && !(memory.load32(options_gate_second,frame.argument(argument::options))&option::second_gate))return false;
        bool forward_ok=true;
        if(flags&gate::forward_only)forward_ok=sse::not_above(from,to);
        frame.gate_set(flags,forward_ok);
        if((flags&gate::backward_only) && !sse::not_above(to,from))return false;
        frame.gated(forward_ok);
        return forward_ok;
    };
    const auto reload=[&]() {
        to=memory.load32(to_again,frame.argument(argument::to));
        from=memory.load32(from_again,frame.argument(argument::from));
    };
    const auto record_address=[&]{return frame.local(local::record);};
    // The events: their bits in the mask, each handed on as a record of type 0.
    {
        const auto options_value=memory.load8(options_load,frame.argument(argument::options));
        const bool skipped=options_value&option::no_events;
        frame.passing(Pass::events,options_value,skipped);
        if(!skipped) {
            kept.count=memory.load32(pass_count,at(memory.load32(pass_animation,frame.argument(argument::animation)),animation::event_count));
            kept.index=0u;
            frame.counted(kept.count,kept);
            if(signed_value(kept.count)>0)do {
                const auto list=memory.load32(list_load,at(memory.load32(animation_load,frame.argument(argument::animation)),animation::events));
                const auto event_address=list+event_size*kept.index;
                const auto frame_word=memory.load16(entry_frame,at(event_address,entry::frame));
                std::uint32_t at_frame;
                if(in_range(frame_word,local::entry_frame,at_frame)) {
                    const auto bit=widen16(memory.load16(event_bit,at(event_address,entry::name)));
                    const auto mask=memory.load32(mask_load,frame.local(local::mask))|(1u<<(bit&31u));
                    frame.marked(bit,mask,kept);
                    memory.store32(mask_store,frame.local(local::mask),mask);
                    if(kept.callback) {
                        memory.store32(record_bit,at(record_address(),record::bit),bit);
                        memory.store16(record_frame,at(record_address(),record::frame),frame_word);
                        const auto frame_value=memory.load32(frame_load,frame.local(local::entry_frame));
                        frame.pushing(record_address());
                        memory.store16(record_type,at(record_address(),record::type),0u);
                        memory.store32(record_flags,at(record_address(),record::flags),0u);
                        const auto context=memory.load32(context_load,frame.argument(argument::context));
                        frame.pushing(frame_value);
                        frame.pushing(context);
                        memory.store32(record_tag,at(record_address(),record::tag),none);
                        memory.store32(record_extra,at(record_address(),record::extra),no_event_name);
                        frame.call_back(kept);
                        reload();
                    }
                }
                ++kept.index;
                frame.advanced(kept.index,kept.count);
            } while(signed_value(kept.index)<signed_value(kept.count));
        }
    }
    // The sounds: each passed and allowed one handed on as a record of type 1.
    {
        const auto options_value=memory.load32(options_load,frame.argument(argument::options));
        const bool skipped=options_value&option::no_sounds;
        frame.passing(Pass::sounds,options_value,skipped);
        if(!skipped) {
            const auto count=memory.load32(pass_count,at(memory.load32(pass_animation,frame.argument(argument::animation)),animation::sound_count));
            kept.index=0u;
            frame.counted(count,kept);
            memory.store32(count_store,frame.local(local::sound_count),count);
            if(signed_value(count)>0) {
                std::uint32_t count_now;
                do {
                    const auto list=memory.load32(list_load,at(memory.load32(animation_load,frame.argument(argument::animation)),animation::sounds));
                    const auto sound_address=list+sound_size*kept.index;
                    const auto frame_word=memory.load16(entry_frame,at(sound_address,entry::frame));
                    frame.sound_entry(sound_address,frame_word);
                    std::uint32_t at_frame;
                    if(in_range(frame_word,local::entry_frame,at_frame)) {
                        const auto name=memory.load16(entry_name,at(sound_address,entry::name));
                        frame.named(name);
                        if(name!=0xffffu) {
                            const auto references_address=memory.load32(references,at(memory.load32(definition_load,
                                frame.argument(argument::definition)),definition::sounds));
                            const auto reference_address=references_address+reference_size*widen16(name);
                            frame.referenced(reference_address,0u,false);
                            if(reference_address) {
                                const auto tag=memory.load32(reference_tag,at(reference_address,reference::tag));
                                frame.referenced(reference_address,tag,true);
                                if(tag!=none && kept.callback) {
                                    const auto flags=memory.load16(reference_flags,at(reference_address,reference::flags));
                                    if(allowed(flags)) {
                                        memory.store32(record_tag,at(record_address(),record::tag),tag);
                                        const auto extra=memory.load32(sound_extra,at(sound_address,entry::sound_extra));
                                        frame.flags_word(flags);
                                        memory.store32(record_extra,at(record_address(),record::extra),extra);
                                        const auto frame_value=memory.load32(frame_load,frame.local(local::entry_frame));
                                        frame.pushing(record_address());
                                        memory.store32(record_flags,at(record_address(),record::flags),flags);
                                        const auto context=memory.load32(context_load,frame.argument(argument::context));
                                        frame.pushing(frame_value);
                                        frame.pushing(context);
                                        memory.store16(record_type,at(record_address(),record::type),1u);
                                        memory.store32(record_bit,at(record_address(),record::bit),none);
                                        memory.store16(record_frame,at(record_address(),record::frame),frame_word);
                                        frame.call_back(kept);
                                        reload();
                                    }
                                }
                            }
                        }
                    }
                    count_now=memory.load32(count_load,frame.local(local::sound_count));
                    ++kept.index;
                    frame.advanced(kept.index,count_now);
                } while(signed_value(kept.index)<signed_value(count_now));
            }
        }
    }
    // The effects: each passed and allowed one handed on as a record of type 2.
    {
        const auto options_value=memory.load32(options_load,frame.argument(argument::options));
        const bool skipped=options_value&option::no_effects;
        frame.passing(Pass::effects,options_value,skipped);
        if(!skipped) {
            kept.count=memory.load32(pass_count,at(memory.load32(pass_animation,frame.argument(argument::animation)),animation::effect_count));
            kept.index=0u;
            frame.counted(kept.count,kept);
            if(signed_value(kept.count)>0)do {
                const auto list=memory.load32(list_load,at(memory.load32(animation_load,frame.argument(argument::animation)),animation::effects));
                const auto effect_address=list+effect_size*kept.index;
                const auto frame_word=memory.load16(entry_frame,at(effect_address,entry::frame));
                frame.effect_entry(effect_address,frame_word);
                std::uint32_t at_frame;
                if(in_range(frame_word,local::effect_frame,at_frame)) {
                    const auto name=memory.load16(entry_name,at(effect_address,entry::name));
                    frame.named(name);
                    if(name!=0xffffu) {
                        const auto references_address=memory.load32(references,at(memory.load32(definition_load,
                            frame.argument(argument::definition)),definition::effects));
                        const auto reference_address=references_address+reference_size*widen16(name);
                        frame.referenced(reference_address,0u,false);
                        if(reference_address) {
                            const auto tag=memory.load32(reference_tag,at(reference_address,reference::tag));
                            frame.referenced(reference_address,tag,true);
                            if(tag!=none) {
                                const auto flags=memory.load16(reference_flags,at(reference_address,reference::flags));
                                if(allowed(flags) && frame.callback_tested(kept)) {
                                    const auto frame_value=memory.load32(frame_load,frame.local(local::effect_frame));
                                    memory.store32(record_tag,at(record_address(),record::tag),tag);
                                    frame.flags_word(flags);
                                    frame.pushing(record_address());
                                    memory.store32(record_flags,at(record_address(),record::flags),flags);
                                    const auto context=memory.load32(context_load,frame.argument(argument::context));
                                    frame.pushing(frame_value);
                                    frame.pushing(context);
                                    memory.store16(record_type,at(record_address(),record::type),2u);
                                    memory.store32(record_bit,at(record_address(),record::bit),none);
                                    memory.store16(record_frame,at(record_address(),record::frame),frame_word);
                                    memory.store32(record_extra,at(record_address(),record::extra),no_event_name);
                                    frame.call_back(kept);
                                    reload();
                                }
                            }
                        }
                    }
                }
                ++kept.index;
                frame.advanced(kept.index,kept.count);
            } while(signed_value(kept.index)<signed_value(kept.count));
        }
    }
    const auto mask=memory.load32(mask_answer,frame.local(local::mask));
    frame.restoring();
    return mask;
}
} // namespace channel_events
} // namespace halo2::native_channel_events
