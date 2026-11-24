#pragma once
// Native TU1.5 animation busy check, entered at 113190 (physics live
// support unit): whether a unit's current animations keep it busy.
//
//   animation_busy::check   113190(unit handle) -> AL busy; RET 4
//
// A unit with a player (+0x13C) first works out whether its weapons are
// idle: the first hand's weapon (the signed byte +0x212 among the weapons
// +0x218) has its timer (the word +0x17C) run out (not positive) and the
// second hand's (+0x213), if any, has too; and whether its animation state
// (the block at the signed word +0x12A) has the pose flag (+0x14 bit 4). A
// unit without a player has neither. The unit is busy when C9590 answers (EAX
// the unit); otherwise when its state's animation (+0x7C) is a pose
// animation and the pose flag is clear, or a melee or throw animation and
// the weapons are not idle. Its channel animation (the block at the signed
// word +0x33E, its +0xA4) then decides: a melee or throw animation makes it
// busy exactly when the weapons are not idle, a smash animation exactly
// when the pose flag is clear. The original searches the animation names
// (string ids, the length in the top byte) as compiled binary searches;
// they are sets here, and the adapter maps the leaf an unlisted channel
// animation reaches to the original's exit.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_animation_busy.hpp) maps the role to the original instruction
// and owns the ABI of C9590. The weapons' idleness (BL) and the state's
// animation (ESI) the original keeps across it are a Kept, refreshed from
// the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_animation_busy {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_address=8u,handle_mask=0xffffu,none=0xffffffffu;
namespace unit {inline constexpr std::uint32_t state=0x12au,player=0x13cu,first_weapon=0x212u,second_weapon=0x213u,weapons=0x218u,
    channels=0x33eu;}
namespace state {inline constexpr std::uint32_t pose_flags=0x14u,animation=0x7cu,pose_bit=0x10u;}
namespace channels {inline constexpr std::uint32_t animation=0xa4u;}
namespace weapon {inline constexpr std::uint32_t timer=0x17cu;}
// The frame below the saved registers (offsets from the ESP after all four
// are pushed): the busy byte, a byte left 0, the pose flag, the channel
// animation and the unit's handle.
namespace local {inline constexpr std::uint32_t busy=0x11u,spare=0x12u,posed=0x13u,channel=0x14u,argument=0x1cu,size=8u;}

// Animations busy unless the state's pose flag is set (the first search's first class).
inline constexpr std::uint32_t pose_animations[]{
    0x0400004au,0x0400076cu,0x0400076du,0x050000c3u /*board*/,0x05000768u,0x05000769u,0x0500076au,0x0500076bu,0x0600005fu /*posing*/,
    0x0600008cu,0x060000ccu,0x060000cdu,0x060006acu,0x0700002cu,0x07000543u,0x0700076eu,0x080000c4u,0x0800076fu,0x09000011u,0x09000012u,
    0x0a000010u,0x0a000013u,0x0a00002du,0x0a00003eu,0x0a00022du /*smash_left*/,0x0a000767u,0x0b00002eu,0x0b00022eu /*smash_right*/,
    0x0b0005b8u,0x0c000043u,0x0c0005b7u,0x0d00002bu,0x0d000042u,0x0d00022cu,0x0e00002au,0x0e00003bu,0x0e0000c3u,0x0f00003au,0x1000003cu};
// Melee and throw animations: busy unless the weapons are idle.
inline constexpr std::uint32_t weapon_animations[]{
    0x0500000au /*melee*/,0x05000024u /*ready*/,0x0a000040u,0x0a0005b9u,0x0b0005b2u /*melee_lunge*/,0x0c000073u /*melee_1sthit*/,
    0x0c000075u /*melee_2ndhit*/,0x0c000077u /*melee_3rdhit*/,0x0c0006b3u,0x0c0006cdu,0x0d000021u /*throw_grenade*/,0x0e000038u,
    0x0e00067du,0x1000006cu /*throw_overheated*/,0x11000074u /*melee_1sthit2idle*/,0x11000076u /*melee_2ndhit2idle*/,
    0x140005b3u /*melee_lunge_airborne*/};
// The channel animations searched second: weapon ones, then pose ones.
inline constexpr std::uint32_t channel_weapon_animations[]{
    0x0500000au /*melee*/,0x0a000040u,0x0a0005b9u,0x0b0005b2u /*melee_lunge*/,0x0c000073u /*melee_1sthit*/,0x0c000075u /*melee_2ndhit*/,
    0x0c000077u /*melee_3rdhit*/,0x0c0006b3u,0x0c0006cdu,0x0d000021u /*throw_grenade*/,0x0e000038u,0x0e00067du,
    0x1000006cu /*throw_overheated*/,0x11000074u /*melee_1sthit2idle*/,0x11000076u /*melee_2ndhit2idle*/,
    0x140005b3u /*melee_lunge_airborne*/};
inline constexpr std::uint32_t channel_pose_animations[]{
    0x0a00022du /*smash_left*/,0x0b00022eu /*smash_right*/,0x0d00022cu};
template<std::size_t N> constexpr bool among(const std::uint32_t (&names)[N],std::uint32_t name) {
    for(const auto n:names)if(n==name)return true;
    return false;
}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace animation_busy {
enum class Access {argument_load,headers_load,datums_load,unit_load,channels_block,channel_animation,state_block,state_animation,
    channel_store,player_load,busy_clear,spare_clear,posed_clear,hand_unit,first_slot,first_weapon_load,first_object,first_timer,
    first_spare,second_datums,second_unit,second_slot,second_weapon_load,second_object,second_timer,pose_test,posed_set,
    no_player_spare,call_argument,busy_set,posed_load,busy_store,channel_load,channel_busy_store,channel_posed_load,
    channel_posed_store,channel_posed_result,result_load};
// The original's EBX (whether the weapons are idle, in BL) and ESI (the
// state's animation).
struct Kept {std::uint32_t idle,state_animation;};

// Frame: see native_tu15_animation_busy.hpp.
template<class Memory,class Frame> void check(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The unit, its channel animation and its state's.
    frame.framing();
    const auto handle=memory.load32(argument_load,frame.local(local::argument));
    const auto headers=memory.load32(headers_load,globals::object_headers);
    frame.saving(1u);
    const auto index=frame.indexed(handle);
    frame.saving(2u);
    const auto datums=memory.load32(datums_load,at(headers,datum_data));
    const auto offset=frame.header_offset(index);
    const auto unit_address=memory.load32(unit_load,datums+offset+header_address);
    const auto channels_offset=signed_word(memory.load16(channels_block,at(unit_address,unit::channels)));
    const auto channel=memory.load32(channel_animation,channels_offset+unit_address+channels::animation);
    frame.saving(3u);
    frame.saving(4u);
    const auto state_offset=signed_word(memory.load16(state_block,at(unit_address,unit::state)));
    kept.state_animation=memory.load32(state_animation,state_offset+unit_address+state::animation);
    memory.store32(channel_store,frame.local(local::channel),channel);
    const auto player=memory.load32(player_load,at(unit_address,unit::player));
    const auto state_address=frame.state_address(state_offset,unit_address);
    const bool played=frame.played(player);
    memory.store8(busy_clear,frame.local(local::busy),0u);
    memory.store8(spare_clear,frame.local(local::spare),0u);
    memory.store8(posed_clear,frame.local(local::posed),0u);
    if(!played)kept.idle=frame.idle(memory.load8(no_player_spare,frame.local(local::spare)));
    else {
        // The weapons are idle while the first's timer has run out and the second's has too.
        frame.datums_copied();
        const auto hands=memory.load32(hand_unit,datums+offset+header_address);
        bool timed=false;
        const auto first=signed_byte(memory.load8(first_slot,at(hands,unit::first_weapon)));
        if(frame.first_named(first)) {
            const auto weapon=memory.load32(first_weapon_load,hands+4u*frame.first_index(first)+unit::weapons);
            if(frame.armed(weapon)) {
                const auto object=memory.load32(first_object,datums+4u*frame.first_weapon_index(weapon)+header_address);
                kept.idle=frame.run_out(memory.load16(first_timer,at(object,weapon::timer)));
                timed=true;
            }
        }
        if(!timed)kept.idle=frame.idle(memory.load8(first_spare,frame.local(local::spare)));
        const auto datums_now=memory.load32(second_datums,at(headers,datum_data));
        const auto unit_now=memory.load32(second_unit,datums_now+offset+header_address);
        const auto second=signed_byte(memory.load8(second_slot,at(unit_now,unit::second_weapon)));
        if(frame.second_named(second)) {
            const auto weapon=memory.load32(second_weapon_load,unit_now+4u*frame.second_index(second)+unit::weapons);
            if(frame.armed(weapon)) {
                const auto object=memory.load32(second_object,datums+4u*frame.second_weapon_index(weapon)+header_address);
                if(frame.timing(memory.load16(second_timer,at(object,weapon::timer))))kept.idle=frame.not_idle();
            }
        }
        if(frame.posed(memory.load8(pose_test,at(state_address,state::pose_flags))))memory.store8(posed_set,frame.local(local::posed),1u);
    }
    // C9590, then the state's animation.
    memory.load32(call_argument,frame.local(local::argument));
    if(frame.answered(kept))memory.store8(busy_set,frame.local(local::busy),1u);
    else if(among(pose_animations,kept.state_animation))
        memory.store8(busy_store,frame.local(local::busy),frame.clear(memory.load8(posed_load,frame.local(local::posed))));
    else if(among(weapon_animations,kept.state_animation))
        memory.store8(busy_store,frame.local(local::busy),frame.clear(kept.idle));
    // The channel's animation.
    const auto channel_now=memory.load32(channel_load,frame.local(local::channel));
    if(among(channel_weapon_animations,channel_now)) {
        memory.store8(channel_busy_store,frame.local(local::busy),frame.clear(kept.idle));
        frame.leaving_matched();
    } else if(among(channel_pose_animations,channel_now)) {
        const bool clear=frame.unposed(memory.load8(channel_posed_load,frame.local(local::posed)));
        memory.store8(channel_posed_store,frame.local(local::busy),clear?1u:0u);
        frame.finished_posed(memory.load8(channel_posed_result,frame.local(local::busy)));
        return;
    } else frame.leaving(channel_now);
    frame.finished(memory.load8(result_load,frame.local(local::busy)));
}
} // namespace animation_busy
} // namespace halo2::native_animation_busy
