#pragma once
// Native TU1.5 action allowed, entered at 1EFD60 (multiplayer engines unit):
// whether a local player may perform an action now.
//
//   action_allowed::allowed   1EFD60(EDX action; local player) -> AL; RET 4
//
// The local player's controls (0x1B0 bytes at [52BE64]) and player handle
// ([4F54F0] +0xC table) name the player (0x21C bytes in [4F54F4]) and its
// unit ([4ECBD0], 12-byte entries, the address at +8). The action's
// definition (20 bytes from [51DF94] +0x424, +0x420 of them) must exist;
// one flagged (+0x10 bit 0) is refused in game mode 2 (+8 of [4F3218]).
// Each action (0 to 0x1F, through the original's jump table at 1F05E4)
// then asks its own questions of the unit, its seat (+0x1FC), its parent
// (+0x14), its weapons (CE230, EAX the unit, CX the slot byte +0x212 or
// +0x213), the controls' last action (+0x80) and state, and the children
// below; a granted action of the first kind clears the controls' pending
// byte (+0x1A0).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_action_allowed.hpp) maps the role to the original
// instruction, owns the ABI of the children and leaves through the
// original's epilogue for each case. The values the original keeps in
// callee-saved registers across the children are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_action_allowed {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t controls=0x52be64u,local_players=0x4f54f0u,actions=0x51df94u,game=0x4f3218u,
    players=0x4f54f4u,objects=0x4ecbd0u,definitions=0x4f0414u,jump_table=0x1f05e4u,fresh=0x46a130u;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,none=0xffffffffu,no_seat=0xffffu,controls_size=0x1b0u,
    player_size=0x21cu,action_size=20u,last_action=0x1fu,checked_mode=2u;
namespace local_players {inline constexpr std::uint32_t handles=0xcu;}
namespace actions {inline constexpr std::uint32_t count=0x420u,array=0x424u;}
namespace action {inline constexpr std::uint32_t flags=0x10u;}
namespace game {inline constexpr std::uint32_t mode=8u;}
namespace player {inline constexpr std::uint32_t object=0x24u,unit=0x2cu;}
namespace entry {inline constexpr std::uint32_t size=12u,address=8u;}
namespace unit {inline constexpr std::uint32_t parent=0x14u,timer=0xf0u,flags=0x10au,seat=0x1fcu,slot=0x212u,next_slot=0x213u;
    inline constexpr unsigned driving_bit=9u;}
namespace controls {inline constexpr std::uint32_t last=0x80u,mode=0x140u,item=0x144u,state=0x148u,bits=0x14cu,flags=0x194u,
    count=0x197u,grenades=0x19bu,pending=0x1a0u;}
namespace definition_entry {inline constexpr std::uint32_t size=16u,address=8u;}
namespace definition {inline constexpr std::uint32_t seats=0x1ccu,ammo=0x21au;}
namespace seat {inline constexpr std::uint32_t size=0xb0u;inline constexpr unsigned driver_bit=2u,gunner_bit=3u,closed_bit=11u;}
// The frame (offsets from the ESP after EBX, ESI, EDI and EBP are pushed):
// the answer byte, the unit's handle, the rider iterator, the weapon
// report, and the argument.
namespace local {inline constexpr std::uint32_t answer=0x13u,unit=0x14u,iterator=0x18u,rider=0x1cu,rider_seat=0x20u,report=0x28u,
    rounds=0x4cu,loaded=0x50u,maximum=0x52u,reserve=0x54u,total=0x56u,player=0x68u;}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}

namespace action_allowed {
enum class Access {
    player_argument,controls_load,answer_clear,local_players,player_handle,actions_load,action_count,actions_array,action_flags,
    game_load,game_mode,players_load,players_data,unit_load,unit_store,objects_load,objects_data,unit_address,action_case,seat_10,
    parent_10,unit_handle_10,last_action_10,answer_set_10,answer_10,pending_clear_10,seat_11,parent_11,unit_handle_11,
    last_action_11,answer_set_11,answer_11,pending_clear_11,parent_12,last_action_12,answer_set_12,answer_12,pending_clear_12,
    parent_13,last_action_13,answer_set_13,answer_13,pending_clear_13,player_14,unit_handle_14,player_2,parent_2,last_action_2,
    answer_set_2,answer_2,pending_clear_2,parent_4,seat_4,parent_object_4,parent_tag_4,definitions_4,definition_4,seats_4,
    seat_flags_4,unit_handle_4,rider_seat_4,rider_4,rider_seat_flags_4,answer_set_4,answer_4,unit_15,unit_address_15,slot_15,
    player_15,answer_set_15,answer_15,unit_18,unit_address_18,slot_18,player_18,flags_0,answer_0,flags_1,answer_1,flags_31,
    answer_31,count_17,seat_17,answer_set_17,answer_17,game_mode_30,player_30,player_object_30,player_16,objects_16,objects_data_16,
    unit_a_16,address_a_16,slot_a_16,unit_b_16,address_b_16,slot_b_16,unit_c_16,address_c_16,slot_c_16,unit_d_16,address_d_16,
    slot_d_16,last_action_16,answer_set_16,answer_16,pending_clear_16,player_20,seat_20,answer_set_20,answer_20,player_23,
    objects_23,objects_data_23,unit_a_23,address_a_23,slot_a_23,unit_b_23,address_b_23,slot_b_23,rounds_23,maximum_23,loaded_23,
    total_23,reserve_23,answer_set_23,answer_23,unit_a_9,address_a_9,slot_a_9,unit_b_9,address_b_9,slot_b_9,shared_unit_handle,
    shared_answer_set,shared_answer,threshold_22,timer_22,last_action_22,player_22,answer_set_22,answer_22,flags_21,threshold_21,
    timer_21,last_action_21,answer_set_21,answer_21,mode_5,state_5,mode_6,state_6,item_6,item_tag_6,definitions_6,definition_6,
    ammo_6,answer_set_6,answer_6,mode_7,bits_7,state_7,mode_8,bits_8,count_8,answer_true,answer_load};
// How the original leaves: before EBP is saved, through the shared
// epilogue, a granted action clearing the pending byte, a granted action,
// a granted action of the shared tail, or the controls' flag bit.
enum class Leave {early,common,pending,granted,shared,flag};
struct Exit {Leave leave;std::uint32_t action;};
// The original's EBX (the unit), EBP (the objects), ESI (the player) and EDI (the controls).
struct Kept {std::uint32_t unit_address,objects,player_address,controls_address;};

// Frame: see native_tu15_action_allowed.hpp.
template<class Memory,class Frame> Exit allowed(Memory &memory,Frame &frame,Kept kept,std::uint32_t action_index) {
    using enum Access;
    const auto refuse=[&] {
        frame.leaving();
        memory.load8(answer_load,frame.local(local::answer));
        return Exit{Leave::common,action_index};
    };
    const auto early=[&] {
        memory.load8(answer_load,frame.local(local::answer));
        return Exit{Leave::early,action_index};
    };
    // A granted action of the first kind: the answer, then the pending byte cleared.
    const auto granted_pending=[&](Access answer_role,Access clear_role) {
        memory.load8(answer_role,frame.local(local::answer));
        frame.popped_base();
        memory.store8(clear_role,at(kept.controls_address,controls::pending),0u);
        return Exit{Leave::pending,action_index};
    };
    // A granted action: EBP and EDI popped, the answer set and read.
    const auto granted=[&](Access set_role,Access answer_role) {
        frame.popped_two();
        memory.store8(set_role,frame.local(local::answer),1u);
        memory.load8(answer_role,frame.local(local::answer));
        return Exit{Leave::granted,action_index};
    };
    const auto shared_tail=[&](bool yes) {
        if(!yes)return refuse();
        frame.popped_two();
        memory.store8(shared_answer_set,frame.local(local::answer),1u);
        memory.load8(shared_answer,frame.local(local::answer));
        return Exit{Leave::shared,action_index};
    };
    const auto set_true=[&]{memory.store8(answer_true,frame.local(local::answer),1u);return refuse();};

    // The controls, the player and the action's definition.
    frame.framing();
    const auto index=memory.load32(player_argument,frame.local(local::player));
    frame.saving(1u);
    frame.saving(2u);
    const auto controls_base=memory.load32(controls_load,globals::controls);
    frame.saving(3u);
    kept.controls_address=frame.controls(index,controls_base);
    auto handle=frame.no_player();
    const bool local=frame.named(index);
    memory.store8(answer_clear,frame.local(local::answer),0u);
    if(local) {
        const auto table=memory.load32(local_players,globals::local_players);
        handle=memory.load32(player_handle,table+4u*index+local_players::handles);
    }
    const auto actions_header=memory.load32(actions_load,globals::actions);
    auto definition_address=frame.no_definition();
    if(frame.present(actions_header) && frame.non_negative(action_index) &&
       frame.below(action_index,memory.load32(action_count,at(actions_header,actions::count)))) {
        const auto array=memory.load32(actions_array,at(actions_header,actions::array));
        definition_address=frame.definition(array,action_index);
    }
    if(!frame.named(handle) || !frame.present(definition_address))return early();
    const bool flagged=frame.flagged(memory.load8(action_flags,at(definition_address,action::flags)));
    const auto game_address=memory.load32(game_load,globals::game);
    if(flagged && frame.mode_is(memory.load32(game_mode,at(game_address,game::mode)),checked_mode))return early();
    const auto players_header=memory.load32(players_load,globals::players);
    const auto player_offset=frame.player_offset(handle);
    frame.saving(4u);
    const auto players=memory.load32(players_data,at(players_header,datum_data));
    const auto unit_handle=memory.load32(unit_load,player_offset+players+player::unit);
    kept.player_address=frame.player_address(player_offset,players);
    const bool has_unit=frame.named(unit_handle);
    memory.store32(unit_store,frame.local(local::unit),unit_handle);
    if(!has_unit)return refuse();
    const auto objects_header=memory.load32(objects_load,globals::objects);
    kept.objects=memory.load32(objects_data,at(objects_header,datum_data));
    const auto unit_index=frame.unit_index(unit_handle,action_index);
    kept.unit_address=memory.load32(unit_address,kept.objects+entry::size*unit_index+entry::address);
    if(!frame.listed(action_index))return refuse();
    frame.jumped(memory.load32(action_case,globals::jump_table+4u*action_index));

    // The weapon in the unit's slot byte (CE230).
    const auto weapon=[&](Access unit_role,Access address_role,Access slot_role,std::uint32_t slot,std::uint32_t objects_base,
                          unsigned site,bool keep_handle) {
        const auto h=memory.load32(unit_role,at(kept.player_address,player::unit));
        if(keep_handle)kept.player_address=h;
        const auto address=memory.load32(address_role,objects_base+entry::size*frame.weapon_unit(h,site)+entry::address);
        memory.load8(slot_role,at(address,slot));
        return frame.weapon(kept,h,site);
    };

    switch(action_index) {
    case 10:case 11: {
        // On foot, without a parent, and 1F1160 agrees (-1, 1).
        const bool second=action_index==11;
        if(frame.busy(kept,second?1u:0u))return refuse();
        const auto none_word=frame.none_seat();
        if(!frame.word_differs(memory.load16(second?seat_11:seat_10,at(kept.unit_address,unit::seat)),none_word))return refuse();
        if(!frame.differs(memory.load32(second?parent_11:parent_10,at(kept.unit_address,unit::parent)),none))return refuse();
        const auto h=memory.load32(second?unit_handle_11:unit_handle_10,frame.local(local::unit));
        if(!frame.agrees(kept,h,second?1u:0u))return refuse();
        const bool repeated=frame.is(memory.load32(second?last_action_11:last_action_10,at(kept.controls_address,controls::last)),action_index);
        memory.store8(second?answer_set_11:answer_set_10,frame.local(local::answer),1u);
        if(repeated)return refuse();
        return granted_pending(second?answer_11:answer_10,second?pending_clear_11:pending_clear_10);
    }
    case 12:case 13: {
        // Without a parent.
        const bool second=action_index==13;
        if(frame.busy(kept,second?3u:2u))return refuse();
        if(frame.differs(memory.load32(second?parent_13:parent_12,at(kept.unit_address,unit::parent)),none))return refuse();
        const bool repeated=frame.is(memory.load32(second?last_action_13:last_action_12,at(kept.controls_address,controls::last)),action_index);
        memory.store8(second?answer_set_13:answer_set_12,frame.local(local::answer),1u);
        if(repeated)return refuse();
        return granted_pending(second?answer_13:answer_12,second?pending_clear_13:pending_clear_12);
    }
    case 14: {
        // 1EFD10 (0x10) does not answer and CAD40 names a seat.
        if(frame.nonzero(frame.asked(kept,memory.load32(player_14,frame.local(local::player)),0x10u)))return refuse();
        const auto h=memory.load32(unit_handle_14,frame.local(local::unit));
        return shared_tail(frame.seat_named(kept,h));
    }
    case 2: {
        if(frame.busy(kept,4u))return refuse();
        if(!frame.may(kept,memory.load32(player_2,frame.local(local::player)),0xcu,0u))return refuse();
        if(frame.differs(memory.load32(parent_2,at(kept.unit_address,unit::parent)),none))return refuse();
        const bool repeated=frame.is(memory.load32(last_action_2,at(kept.controls_address,controls::last)),2u);
        memory.store8(answer_set_2,frame.local(local::answer),1u);
        if(repeated)return refuse();
        return granted_pending(answer_2,pending_clear_2);
    }
    case 4: {
        // In a seat of a parent: the seat neither driver, gunner nor closed, and no other rider in a driver's seat.
        if(frame.busy(kept,5u))return refuse();
        const auto parent=memory.load32(parent_4,at(kept.unit_address,unit::parent));
        if(!frame.named(parent))return refuse();
        const auto seat_index=memory.load16(seat_4,at(kept.unit_address,unit::seat));
        if(!frame.seat_held(seat_index))return refuse();
        const auto parent_address=memory.load32(parent_object_4,kept.objects+entry::size*frame.parent_index(parent)+entry::address);
        const auto tag=memory.load32(parent_tag_4,parent_address);
        const auto definitions=memory.load32(definitions_4,globals::definitions);
        const auto definition_value=memory.load32(definition_4,definitions+definition_entry::size*frame.tag_index(tag)+definition_entry::address);
        const auto seats=memory.load32(seats_4,at(definition_value,definition::seats));
        const auto flags_value=memory.load32(seat_flags_4,seats+seat::size*frame.seat_offset(seat_index,seats));
        if(frame.seat_bit(flags_value,seat::driver_bit,0u) || frame.seat_bit(flags_value,seat::gunner_bit,1u) ||
           frame.seat_bit(flags_value,seat::closed_bit,2u))return refuse();
        frame.riders_start(kept,parent);
        if(!frame.next_rider(kept,false))return set_true();
        const auto self=memory.load32(unit_handle_4,frame.local(local::unit));
        for(;;) {
            const auto rider_seat=memory.load16(rider_seat_4,frame.local(local::rider_seat));
            if(frame.word_differs(rider_seat,no_seat) && frame.differs(memory.load32(rider_4,frame.local(local::rider)),self) &&
               frame.seat_bit(memory.load32(rider_seat_flags_4,seats+seat::size*frame.rider_offset(rider_seat)),seat::driver_bit,3u))
                return refuse();
            if(!frame.next_rider(kept,true))break;
        }
        return granted(answer_set_4,answer_4);
    }
    case 15:case 18: {
        // A weapon in the slot, and 1AC79B does not answer.
        const bool first=action_index==15;
        if(!frame.named(weapon(first?unit_15:unit_18,first?unit_address_15:unit_address_18,first?slot_15:slot_18,unit::slot,
                              kept.objects,first?0u:1u,false)))return refuse();
        const auto p=memory.load32(first?player_15:player_18,frame.local(local::player));
        if(!first)kept.player_address=p;
        if(frame.answers(kept,p,first?0u:1u))return refuse();
        if(first)return granted(answer_set_15,answer_15);
        // ECX from ESI as 1AC79B returned it.
        return shared_tail(frame.may(kept,kept.player_address,0xfu,1u));
    }
    case 0:case 1:case 31: {
        // The controls' flag bit 0, 1 or 2.
        static constexpr Access flags_roles[]{flags_0,flags_1,flags_31};
        static constexpr Access answer_roles[]{answer_0,answer_1,answer_31};
        const unsigned k=action_index==0?0u:action_index==1?1u:2u;
        const auto flags_value=memory.load8(flags_roles[k],at(kept.controls_address,controls::flags));
        frame.popped_two();
        const auto bit=frame.flag_bit(flags_value,k);
        frame.popped_player();
        memory.store8(answer_roles[k],frame.local(local::answer),bit);
        return {Leave::flag,action_index};
    }
    case 17: {
        // Grenades left, and on foot.
        if(!frame.some(memory.load8(count_17,at(kept.controls_address,controls::grenades))))return refuse();
        if(frame.word_differs(memory.load16(seat_17,at(kept.unit_address,unit::seat)),no_seat))return refuse();
        return granted(answer_set_17,answer_17);
    }
    case 30: {
        // In game mode 2: 161530, 165A50, and the player's object's 536E0.
        if(!frame.mode_is(memory.load32(game_mode_30,at(game_address,game::mode)),checked_mode))return refuse();
        if(!frame.ready(kept))return refuse();
        if(!frame.named(frame.player_slot(kept,memory.load32(player_30,frame.local(local::player)))))return refuse();
        const auto object_handle=memory.load32(player_object_30,at(kept.player_address,player::object));
        kept.player_address=object_handle;
        if(!frame.named(object_handle))return refuse();
        return shared_tail(frame.object_ready(kept,object_handle));
    }
    case 16: {
        // A weapon in the slot, none in the next, 1060A0 and 1060E0 agree.
        if(frame.answers(kept,memory.load32(player_16,frame.local(local::player)),2u))return refuse();
        const auto header=memory.load32(objects_16,globals::objects);
        // The objects in EBX from here, as each child returns it.
        kept.unit_address=memory.load32(objects_data_16,at(header,datum_data));
        if(!frame.named(weapon(unit_a_16,address_a_16,slot_a_16,unit::slot,kept.unit_address,2u,false)))return refuse();
        if(frame.named(weapon(unit_b_16,address_b_16,slot_b_16,unit::next_slot,kept.unit_address,3u,false)))return refuse();
        weapon(unit_c_16,address_c_16,slot_c_16,unit::slot,kept.unit_address,4u,false);
        if(!frame.reloadable(kept))return refuse();
        weapon(unit_d_16,address_d_16,slot_d_16,unit::slot,kept.unit_address,5u,true);
        if(!frame.word_differs(frame.rounds(kept),no_seat))return refuse();
        const bool repeated=frame.is(memory.load32(last_action_16,at(kept.controls_address,controls::last)),0x10u);
        memory.store8(answer_set_16,frame.local(local::answer),1u);
        if(repeated)return refuse();
        return granted_pending(answer_16,pending_clear_16);
    }
    case 20: {
        // 1AC595 answers, and on foot.
        if(!frame.answers(kept,memory.load32(player_20,frame.local(local::player)),3u))return refuse();
        if(frame.word_differs(memory.load16(seat_20,at(kept.unit_address,unit::seat)),no_seat))return refuse();
        return granted(answer_set_20,answer_20);
    }
    case 23: {
        // A weapon whose report (1055F0) has rounds, a magazine at least twice loaded, and some in reserve.
        if(frame.answers(kept,memory.load32(player_23,frame.local(local::player)),4u))return refuse();
        const auto header=memory.load32(objects_23,globals::objects);
        // The objects in EDI from here, as each child returns it.
        kept.controls_address=memory.load32(objects_data_23,at(header,datum_data));
        if(!frame.named(weapon(unit_a_23,address_a_23,slot_a_23,unit::slot,kept.controls_address,6u,false)))return refuse();
        frame.reported(kept,weapon(unit_b_23,address_b_23,slot_b_23,unit::slot,kept.controls_address,7u,true));
        if(!frame.word_positive(memory.load16(rounds_23,frame.local(local::rounds))))return refuse();
        const auto maximum=memory.load16(maximum_23,frame.local(local::maximum));
        if(!frame.word_nonzero(maximum))return refuse();
        if(frame.too_loaded(signed_word(memory.load16(loaded_23,frame.local(local::loaded))),maximum))return refuse();
        if(!frame.word_compared_nonzero(memory.load16(total_23,frame.local(local::total))))return refuse();
        if(!frame.word_positive(memory.load16(reserve_23,frame.local(local::reserve))))return refuse();
        return granted(answer_set_23,answer_23);
    }
    case 9: {
        // A weapon in the next slot, and 105FD0 agrees.
        if(!frame.named(weapon(unit_a_9,address_a_9,slot_a_9,unit::next_slot,kept.objects,8u,false)))return refuse();
        weapon(unit_b_9,address_b_9,slot_b_9,unit::next_slot,kept.objects,9u,true);
        return shared_tail(frame.switchable(kept));
    }
    case 24:case 25:case 26:case 27:case 28:case 29: {
        // 1F1160 (0, the mode) agrees.
        const auto mode=action_index==24?1u:(action_index==25||action_index==28)?3u:4u;
        frame.agreement_pushed(mode);
        const auto h=memory.load32(shared_unit_handle,frame.local(local::unit));
        return shared_tail(frame.agrees(kept,h,6u));
    }
    case 22:case 21: {
        // A fresh unit (its timer below [46A130]) is granted at once; otherwise the last action must be the other.
        const bool driving=action_index==21;
        if(driving) {
            frame.flags_cleared();
            if(!frame.driving(memory.load16(flags_21,at(kept.unit_address,unit::flags))))return refuse();
        }
        const auto threshold=memory.load32(driving?threshold_21:threshold_22,globals::fresh);
        if(frame.fresh(threshold,memory.load32(driving?timer_21:timer_22,at(kept.unit_address,unit::timer))))return set_true();
        if(!frame.is(memory.load32(driving?last_action_21:last_action_22,at(kept.controls_address,controls::last)),driving?0x15u:0x16u))
            return refuse();
        if(!driving && !frame.may(kept,memory.load32(player_22,frame.local(local::player)),0x15u,2u))return refuse();
        return granted(driving?answer_set_21:answer_set_22,driving?answer_21:answer_22);
    }
    case 5: {
        frame.three();
        if(!frame.word_is(memory.load16(mode_5,at(kept.controls_address,controls::mode)),3u))return refuse();
        if(frame.word_below(memory.load16(state_5,at(kept.controls_address,controls::state)),3u))return refuse();
        return set_true();
    }
    case 6: {
        // An equipment item (BCDF0, 2) with ammunition.
        if(!frame.word_is(memory.load16(mode_6,at(kept.controls_address,controls::mode)),5u))return refuse();
        if(frame.word_below(memory.load16(state_6,at(kept.controls_address,controls::state)),3u))return refuse();
        const auto item=frame.item(kept,memory.load32(item_6,at(kept.controls_address,controls::item)));
        if(!frame.present(item))return refuse();
        const auto tag=memory.load32(item_tag_6,item);
        const auto definitions=memory.load32(definitions_6,globals::definitions);
        const auto definition_value=memory.load32(definition_6,definitions+definition_entry::size*frame.tag_index(tag)+definition_entry::address);
        if(!frame.word_compared_nonzero(memory.load16(ammo_6,at(definition_value,definition::ammo))))return refuse();
        return granted(answer_set_6,answer_6);
    }
    case 7: {
        if(!frame.word_is(memory.load16(mode_7,at(kept.controls_address,controls::mode)),1u))return refuse();
        const auto bits=memory.load16(bits_7,at(kept.controls_address,controls::bits));
        if(!frame.low_bits(bits))return refuse();
        if(frame.word_below(memory.load16(state_7,at(kept.controls_address,controls::state)),3u))return refuse();
        return set_true();
    }
    case 8: {
        if(!frame.word_is(memory.load16(mode_8,at(kept.controls_address,controls::mode)),1u))return refuse();
        if(!frame.high_bits(memory.load8(bits_8,at(kept.controls_address,controls::bits))))return refuse();
        if(frame.byte_below(memory.load8(count_8,at(kept.controls_address,controls::count)),3u))return refuse();
        return set_true();
    }
    default:return refuse();
    }
}
} // namespace action_allowed
} // namespace halo2::native_action_allowed
