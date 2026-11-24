#pragma once
// Native TU1.5 unit weapon controls, entered at C7DD0 (physics live support
// unit): a unit's per-tick weapon, grenade and zoom state, and the control
// word each hand's weapon gets.
//
//   unit_weapon_controls::update   C7DD0(unit handle) -> AL changed; RET 4
//
// Unless the unit's definition (+0xBC) has bit 11 set:
//  * three timers count down: the signed byte +0x1F5 while positive
//    (reaching 0 requests the actions 0x13 and 9), the bytes +0x1FA and
//    +0x1FB while nonzero; each marks the unit changed;
//  * with the unit's state (+0x134) bit 1, the stance: control (+0x148) bit
//    27, or bit 14 for a biped (the object type +0xAA is 0), asks 1164D0 and
//    otherwise 116480; when that answers no, 116440 or 116400 gets 0.5 and
//    the unit is changed.
// Unless the definition's bit 10 or the unit's +0x10A bit 2 is set:
//  * a unit with +0x19 bit 3 requests the actions 0x13 and 9 (E8E10, ECX the
//    action, the unit pushed); otherwise each hand's weapon (the signed
//    bytes +0x212/+0x216, current and desired) stamps the game time (+8 of
//    [51DF54]) into the current weapon's +0x228 slot and, when they differ,
//    requests 8 (the first hand) or 0x12 (the second);
//  * control bit 28 requests the actions 0 and 0xA;
//  * a desired grenade (+0x23D) other than the current one (+0x23C), while
//    113190 answers no, goes to CE3A0 (CX the grenade, EBX 0), whose answer
//    other than 0xFFFF becomes the current grenade;
//  * the zoom: the requested level (+0x241) when CAAA0 answers, otherwise
//    -1, replaces the current one (+0x240) when it differs (-1 also clears
//    +0x26C); the first hand's weapon, when its definition has zoom levels
//    (+0x1FE), names the zoom-in (+0x270) or zoom-out (+0x278) sound,
//    otherwise the global player information (+0x134 of [4ECC1C]) does
//    (+0xC8, +0xD0, its levels +0xB8); zooming in with more than one level
//    scales the sound by level / (levels - 1), otherwise by [46A0D0]; a sound
//    goes to 18C000 (EAX 0x4000095, ESI the unit, EDI the request {NONE,
//    sound, scale, 0}).
// Unless the definition's bit 11 is set, each hand with a weapon index gets
// 105500(weapon, control word, scale; RET C): the control bits 16-20 and 30
// (the first hand) or 21-25 and 31 (the second) become the word's bits 1-4,
// 0 and 7, control bit 29 its bit 8, a zoom other than -1 its bit 6, and
// its bit 5 is set when the hand is blocked: 113190's answer (cleared while
// +0x1EC is positive), or the unit's actions (+4 of the block at the signed
// word +0x346) 0x16, 0x1A, 0x1B or the hand's own weapon change (8 or 0x12)
// active; the scale is +0x1C0 (the first hand) or +0x1C4. D3A80 (the seat
// pair check) gets the unit last.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_unit_weapon_controls.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>
#include <stdexcept>

namespace halo2::native_unit_weapon_controls {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u,game_time=0x51df54u,
    default_zoom_scale=0x46a0d0u,game_globals=0x4ecc1cu;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_data=8u,time_now=8u,handle_mask=0xffffu,
    none=0xffffffffu,unnamed=0xffu;
inline constexpr std::uint32_t half=0x3f000000u,zoom_sound=0x4000095u;
namespace action {inline constexpr std::uint32_t expired=0x13u,expired_after=9u,first_weapon=8u,second_weapon=0x12u,trigger=0u,
    trigger_after=0xau;inline constexpr unsigned blocking[]{0x16u,0x1au,0x1bu};}
namespace unit {inline constexpr std::uint32_t tag=0u,stunned=0x19u,type=0xaau,object_flags=0x10au,state=0x134u,controls=0x148u,
    first_scale=0x1c0u,second_scale=0x1c4u,busy=0x1ecu,counter=0x1f5u,second_counter=0x1fau,third_counter=0x1fbu,weapon_index=0x212u,
    desired_weapon=0x216u,weapons=0x218u,weapon_times=0x228u,grenade=0x23cu,desired_grenade=0x23du,zoom=0x240u,requested_zoom=0x241u,
    zoom_motion=0x26cu,actions=0x346u,action_bits=4u;inline constexpr std::uint32_t stunned_bit=8u;
    inline constexpr unsigned state_bit=1u,still_bit=2u,trigger_bit=28u,stance_bit=27u,biped_stance_bit=14u,zoom_control_bit=29u;}
namespace definition {inline constexpr std::uint32_t flags=0xbcu;inline constexpr unsigned no_weapon_changes_bit=10u,passive_bit=11u;}
namespace weapon_definition {inline constexpr std::uint32_t zoom_levels=0x1feu,zoom_in_sound=0x270u,zoom_out_sound=0x278u;}
namespace player_information {inline constexpr std::uint32_t pointer=0x134u,zoom_levels=0xb8u,zoom_in_sound=0xc8u,zoom_out_sound=0xd0u;}
// The control word's bits for each hand: the control bits that become its
// bits 1-4, 0 and 7.
struct HandBits {unsigned first,second,third,fourth,zeroth,seventh;};
inline constexpr HandBits hand_bits[]{{16u,17u,18u,19u,20u,30u},{21u,22u,23u,24u,25u,31u}};
namespace word {inline constexpr std::uint32_t zeroth=1u,first=2u,second=4u,third=8u,fourth=0x10u,blocked=0x20u,zoomed=0x40u,seventh=0x80u,
    zoom_control=0x100u;}
// The frame below the saved registers (offsets from the ESP after all four
// are pushed): the changed byte, whether this is the first hand, the
// header offset, the hand, the scale, the control word, the definition
// (later the hand's weapon), the sound request and the unit's handle.
namespace local {inline constexpr std::uint32_t changed=0x12u,first=0x13u,offset=0x14u,hand=0x18u,scale=0x1cu,controls=0x20u,
    definition=0x24u,weapon=0x24u,request=0x28u,argument=0x3cu,size=0x28u;}

inline constexpr bool bit(std::uint32_t value,unsigned n){return ((value>>n)&1u)!=0u;}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

// CVTSI2SS on the live MXCSR.
namespace scalar {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline std::uint32_t from_integer(std::uint32_t x) {
    __m128 r=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(r):"r"(std::int32_t(x)));return sse::bits(r);
}
#else
inline std::uint32_t from_integer(std::uint32_t){throw std::runtime_error("Required TU1.5 CVTSI2SS backend unavailable");}
#endif
} // namespace scalar

namespace unit_weapon_controls {
enum class Access {headers_load,datums_load,instances_load,handle_load,unit_load,offset_store,unit_tag,definition_load,
    definition_flags,definition_store,changed_clear,first_counter,first_counter_store,changed_by_first,second_counter,
    second_counter_store,changed_by_second,third_counter,third_counter_store,changed_by_third,unit_state,stance_controls,object_type,
    changed_by_stance,weapon_flags,object_flags,stunned_test,hand_weapon,desired_weapon,time_global,time_load,weapon_time_store,
    trigger_test,desired_grenade,current_grenade,grenade_argument,desired_grenade_word,chosen_argument,current_grenade_store,
    zoom_argument,requested_zoom,current_zoom,current_zoom_store,zoom_motion_clear,zoom_headers,zoom_datums,zoom_offset,zoom_unit,
    zoom_slot,zoom_weapon,default_scale,weapon_datums,weapon_object,weapon_tag,weapon_instances,weapon_definition_load,
    zoom_levels_test,zoom_out_sound,zoom_in_sound,zoom_levels,game_globals_load,information_load,global_zoom_out,global_zoom_in,
    global_levels,sound_argument,request_handle,request_sound,request_scale,request_extra,definition_reload,hands_flags,hand_start,
    hand_slot,first_store,hand_headers,hand_datums,hand_offset,hand_unit,hand_slot_word,weapon_clear,hand_weapon_handle,weapon_store,
    hand_argument,busy_count,action_datums,action_offset,action_unit,block_first,bits_first,block_second,bits_second,block_third,
    bits_third,hand_action_block,hand_action_bits,hand_reload,hand_controls,zoom_test,first_load,controls_store,first_scale,
    second_scale,controls_word,weapon_reload,scale_store,scale_load,hand_next,pair_argument,changed_load};
// Where E8E10 is asked: the first timer running out, a stunned unit, a hand's
// weapon changing, control bit 28.
enum class Request {expired,stunned,weapon_changed,triggered};
inline constexpr Access action_blocks[]{Access::block_first,Access::block_second,Access::block_third};
inline constexpr Access action_words[]{Access::bits_first,Access::bits_second,Access::bits_third};
// The original's EBX (the unit's handle, later whether this is the first
// hand, in BL), EBP (the unit), ESI (the definition, later the hand) and
// EDI (the weapon hand, later the object headers).
struct Kept {std::uint32_t handle,first,unit,definition,hand,weapon_hand,headers;};

// Frame: see native_tu15_unit_weapon_controls.hpp.
template<class Memory,class Frame> void update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The unit and its definition.
    frame.framing();
    const auto headers=memory.load32(headers_load,globals::object_headers);
    const auto datums=memory.load32(datums_load,at(headers,datum_data));
    const auto instances=memory.load32(instances_load,globals::tag_instances);
    frame.saving(1u);
    kept.handle=memory.load32(handle_load,frame.local(local::argument));
    const auto offset=frame.header_offset(kept.handle);
    frame.saving(2u);
    kept.unit=memory.load32(unit_load,datums+offset+header_address);
    memory.store32(offset_store,frame.local(local::offset),offset);
    const auto tag=memory.load32(unit_tag,at(kept.unit,unit::tag));
    frame.saving(3u);
    kept.definition=memory.load32(definition_load,frame.tag_offset(tag)+instances+tag_data);
    const bool passive=frame.passive(memory.load32(definition_flags,at(kept.definition,definition::flags)));
    frame.saving(4u);
    memory.store32(definition_store,frame.local(local::definition),kept.definition);
    memory.store8(changed_clear,frame.local(local::changed),0u);
    if(!passive) {
        // The three timers.
        const auto counter=memory.load8(first_counter,at(kept.unit,unit::counter));
        if(frame.counting(counter)) {
            const auto next=frame.counted_down(counter);
            memory.store8(first_counter_store,at(kept.unit,unit::counter),next);
            if(next==0u) {
                frame.request(kept,Request::expired,action::expired);
                frame.request(kept,Request::expired,action::expired_after);
            }
            memory.store8(changed_by_first,frame.local(local::changed),1u);
        }
        const auto second=memory.load8(second_counter,at(kept.unit,unit::second_counter));
        if(frame.running(second)) {
            memory.store8(second_counter_store,at(kept.unit,unit::second_counter),frame.counted_down(second));
            memory.store8(changed_by_second,frame.local(local::changed),1u);
        }
        const auto third=memory.load8(third_counter,at(kept.unit,unit::third_counter));
        if(frame.running(third)) {
            memory.store8(third_counter_store,at(kept.unit,unit::third_counter),frame.counted_down(third));
            memory.store8(changed_by_third,frame.local(local::changed),1u);
        }
        // The stance.
        if(frame.state(memory.load32(unit_state,at(kept.unit,unit::state)))) {
            const auto controls=memory.load32(stance_controls,at(kept.unit,unit::controls));
            bool second_stance=frame.stance(controls);
            if(!second_stance && frame.biped(memory.load8(object_type,at(kept.unit,unit::type))))second_stance=frame.biped_stance(controls);
            if(!frame.in_stance(kept,second_stance)) {
                frame.blend_stance(kept,second_stance);
                memory.store8(changed_by_stance,frame.local(local::changed),1u);
            }
        }
    }
    // Weapon changes, grenades and zoom.
    if(!frame.weapons_fixed(memory.load32(weapon_flags,at(kept.definition,definition::flags))) &&
       !frame.still(memory.load8(object_flags,at(kept.unit,unit::object_flags)))) {
        if(frame.stunned(memory.load8(stunned_test,at(kept.unit,unit::stunned)))) {
            frame.request(kept,Request::stunned,action::expired);
            frame.request(kept,Request::stunned,action::expired_after);
        } else {
            kept.weapon_hand=frame.first_weapon_hand();
            do {
                const auto current=signed_byte(memory.load8(hand_weapon,kept.unit+kept.weapon_hand+unit::weapon_index));
                const bool holding=frame.holding(current);
                const auto desired=signed_byte(memory.load8(desired_weapon,kept.unit+kept.weapon_hand+unit::desired_weapon));
                if(holding) {
                    const auto clock=memory.load32(time_global,globals::game_time);
                    memory.store32(weapon_time_store,kept.unit+4u*current+unit::weapon_times,memory.load32(time_load,at(clock,time_now)));
                }
                if(frame.changing(desired,current)) {
                    if(frame.hand_is(kept.weapon_hand,0u))frame.request(kept,Request::weapon_changed,action::first_weapon);
                    else if(frame.hand_is(kept.weapon_hand,1u))frame.request(kept,Request::weapon_changed,action::second_weapon);
                }
                kept.weapon_hand=frame.next_weapon_hand(kept.weapon_hand);
            } while(std::int32_t(kept.weapon_hand)<2);
        }
        if(frame.triggered(memory.load32(trigger_test,at(kept.unit,unit::controls)))) {
            frame.request(kept,Request::triggered,action::trigger);
            frame.request(kept,Request::triggered,action::trigger_after);
        }
        // The grenade.
        const auto wanted=memory.load8(desired_grenade,at(kept.unit,unit::desired_grenade));
        if(frame.differ(wanted,memory.load8(current_grenade,at(kept.unit,unit::grenade))) &&
           !frame.busy(kept,memory.load32(grenade_argument,frame.local(local::argument)))) {
            memory.load8(desired_grenade_word,at(kept.unit,unit::desired_grenade));
            memory.load32(chosen_argument,frame.local(local::argument));
            const auto chosen=frame.chosen_grenade(kept);
            if(chosen!=0xffffu)memory.store8(current_grenade_store,at(kept.unit,unit::grenade),chosen);
        }
        // The zoom.
        memory.load32(zoom_argument,frame.local(local::argument));
        const auto requested=frame.can_zoom(kept)?memory.load8(requested_zoom,at(kept.unit,unit::requested_zoom)):frame.unzoomed();
        if(frame.zoom_changes(memory.load8(current_zoom,at(kept.unit,unit::zoom)),requested)) {
            const bool zooming_out=frame.zooming_out(requested);
            memory.store8(current_zoom_store,at(kept.unit,unit::zoom),requested);
            if(zooming_out)memory.store32(zoom_motion_clear,at(kept.unit,unit::zoom_motion),frame.cleared());
            // The zoom sound: the first hand's weapon's, else the global one.
            const auto headers_now=memory.load32(zoom_headers,globals::object_headers);
            const auto datums_now=memory.load32(zoom_datums,at(headers_now,datum_data));
            const auto unit_now=memory.load32(zoom_unit,datums_now+memory.load32(zoom_offset,frame.local(local::offset))+header_address);
            const auto slot=signed_byte(memory.load8(zoom_slot,at(unit_now,unit::weapon_index)));
            auto weapon=frame.unarmed(slot);
            if(slot!=none)weapon=memory.load32(zoom_weapon,unit_now+4u*frame.zoom_index(slot)+unit::weapons);
            const bool armed=frame.armed(weapon);
            auto scale=memory.load32(default_scale,globals::default_zoom_scale);
            std::uint32_t sound=none,levels=0u;
            bool scaled=false,from_weapon=false;
            if(armed) {
                const auto weapon_datums_now=memory.load32(weapon_datums,at(headers_now,datum_data));
                const auto object=memory.load32(weapon_object,weapon_datums_now+4u*frame.weapon_index(weapon)+header_address);
                const auto weapon_tag_now=memory.load32(weapon_tag,object);
                const auto instances_now=memory.load32(weapon_instances,globals::tag_instances);
                const auto definition_now=memory.load32(weapon_definition_load,frame.tag_offset(weapon_tag_now)+instances_now+tag_data);
                if(frame.has_levels(memory.load16(zoom_levels_test,at(definition_now,weapon_definition::zoom_levels)))) {
                    from_weapon=true;
                    if(frame.zooming_out(requested))sound=memory.load32(zoom_out_sound,at(definition_now,weapon_definition::zoom_out_sound));
                    else {
                        sound=memory.load32(zoom_in_sound,at(definition_now,weapon_definition::zoom_in_sound));
                        const auto weapon_levels=memory.load16(zoom_levels,at(definition_now,weapon_definition::zoom_levels));
                        if(frame.several(weapon_levels,2u)){levels=frame.widened(weapon_levels);scaled=true;}
                    }
                }
            }
            if(!from_weapon) {
                const bool out=frame.zooming_out(requested);
                const auto game=memory.load32(game_globals_load,globals::game_globals);
                const auto information=memory.load32(information_load,at(game,player_information::pointer));
                if(out)sound=memory.load32(global_zoom_out,at(information,player_information::zoom_out_sound));
                else {
                    sound=memory.load32(global_zoom_in,at(information,player_information::zoom_in_sound));
                    levels=memory.load32(global_levels,at(information,player_information::zoom_levels));
                    scaled=frame.several(levels,4u);
                }
            }
            if(scaled) {
                const auto level=scalar::from_integer(signed_byte(requested));
                const auto steps=scalar::from_integer(levels-1u);
                scale=sse::divide(level,steps);
                frame.scaled(level,steps,scale);
            }
            if(frame.sounded(sound)) {
                memory.load32(sound_argument,frame.local(local::argument));
                frame.requesting();
                memory.store32(request_handle,frame.local(local::request),none);
                memory.store32(request_sound,frame.local(local::request+4u),sound);
                memory.store32(request_scale,frame.local(local::request+8u),scale);
                memory.store32(request_extra,frame.local(local::request+12u),0u);
                frame.play(kept);
                kept.definition=memory.load32(definition_reload,frame.local(local::definition));
            }
        }
    }
    // Each hand's weapon controls.
    if(!frame.hands_passive(memory.load32(hands_flags,at(kept.definition,definition::flags)))) {
        kept.hand=frame.first_hand();
        memory.store32(hand_start,frame.local(local::hand),kept.hand);
        do {
            const auto slot=memory.load8(hand_slot,kept.unit+kept.hand+unit::weapon_index);
            kept.first=frame.is_first(kept.hand);
            const bool named=frame.named(slot);
            memory.store8(first_store,frame.local(local::first),kept.first);
            if(named) {
                // The hand's weapon.
                kept.headers=memory.load32(hand_headers,globals::object_headers);
                const auto datums_hand=memory.load32(hand_datums,at(kept.headers,datum_data));
                const auto unit_hand=memory.load32(hand_unit,datums_hand+memory.load32(hand_offset,frame.local(local::offset))+header_address);
                const auto index=signed_byte(memory.load8(hand_slot_word,unit_hand+kept.hand+unit::weapon_index));
                const bool indexed=frame.indexed(index);
                memory.store32(weapon_clear,frame.local(local::weapon),none);
                if(indexed) {
                    const auto weapon=memory.load32(hand_weapon_handle,unit_hand+4u*frame.hand_index(index)+unit::weapons);
                    memory.store32(weapon_store,frame.local(local::weapon),weapon);
                }
                // Whether the hand is blocked.
                bool blocked=frame.hand_busy(kept,memory.load32(hand_argument,frame.local(local::argument)));
                if(frame.counting_busy(memory.load32(busy_count,at(kept.unit,unit::busy))))blocked=frame.unblocked();
                const auto datums_actions=memory.load32(action_datums,at(kept.headers,datum_data));
                const auto unit_actions=memory.load32(action_unit,datums_actions+memory.load32(action_offset,frame.local(local::offset))+header_address);
                bool active=false;
                for(unsigned k=0;k<3u && !active;++k) {
                    const auto block=signed_word(memory.load16(action_blocks[k],at(unit_actions,unit::actions)));
                    active=frame.action_active(memory.load32(action_words[k],block+unit_actions+unit::action_bits),action::blocking[k]);
                }
                if(!active) {
                    const auto block=signed_word(memory.load16(hand_action_block,at(unit_actions,unit::actions)));
                    const auto change=frame.weapon_change(kept.first,block,unit_actions);
                    const auto bits=memory.load32(hand_action_bits,block+unit_actions+4u*(change>>5)+unit::action_bits);
                    kept.hand=memory.load32(hand_reload,frame.local(local::hand));
                    active=frame.change_active(bits,change);
                }
                if(active)blocked=frame.blocked();
                // The control word.
                const auto controls=memory.load32(hand_controls,at(kept.unit,unit::controls));
                const auto &b=hand_bits[frame.second_hand(kept.first)?1:0];
                std::uint32_t control_word=(bit(controls,b.first)?word::first:0u)|(bit(controls,b.second)?word::second:0u)|
                    (bit(controls,b.third)?word::third:0u)|(bit(controls,b.fourth)?word::fourth:0u)|(bit(controls,b.zeroth)?word::zeroth:0u)|
                    (bit(controls,b.seventh)?word::seventh:0u)|(bit(controls,unit::zoom_control_bit)?word::zoom_control:0u)|
                    (blocked?word::blocked:0u);
                if(memory.load8(zoom_test,at(kept.unit,unit::zoom))!=unnamed)control_word|=word::zoomed;
                frame.mapped(controls,b.seventh,control_word);
                // 105500 with the weapon, the word and the hand's scale.
                const bool first=frame.was_first(memory.load8(first_load,frame.local(local::first)));
                memory.store16(controls_store,frame.local(local::controls),control_word);
                const auto scale=first?memory.load32(first_scale,at(kept.unit,unit::first_scale)):
                    memory.load32(second_scale,at(kept.unit,unit::second_scale));
                const auto word_now=memory.load32(controls_word,frame.local(local::controls));
                const auto weapon_now=memory.load32(weapon_reload,frame.local(local::weapon));
                memory.store32(scale_store,frame.local(local::scale),scale);
                frame.control(kept,weapon_now,word_now,memory.load32(scale_load,frame.local(local::scale)));
            }
            kept.hand=frame.next_hand(kept.hand);
            memory.store32(hand_next,frame.local(local::hand),kept.hand);
        } while(std::int32_t(kept.hand)<2);
        frame.pair(kept,memory.load32(pair_argument,frame.local(local::argument)));
    }
    frame.finished(memory.load8(changed_load,frame.local(local::changed)));
}
} // namespace unit_weapon_controls
} // namespace halo2::native_unit_weapon_controls
