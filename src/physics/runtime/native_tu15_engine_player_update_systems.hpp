#pragma once
// Native TU1.5 multiplayer game engine: one player's slot update, entered at
// 1EEE80 (multiplayer engines unit).
//
//   player_slots::update    1EEE80  ticks a local player's engine record
//
// Each local player with an engine record ([52BE64], 0x1B0 bytes per record)
// has 32 slots, each a state and a timer, of which at most one is current,
// plus seven countdown bytes. The slot definitions ([51DF94]) describe the
// slots. One update:
//   1. ticks the countdown bytes down (the third is cleared when the second
//      runs out);
//   2. scans the slots: a cooling slot's timer runs down until the slot is
//      idle again; an idle slot is offered as the current slot (1EFD60) when
//      there is none, when it comes before the current one, or when it is the
//      last slot. The last slot accepted becomes current;
//   3. advances the current slot (a changed current slot resets the previous
//      one and the current slot's tick count):
//        idle      becomes starting; its timer comes from 1494F0 with its
//                  definition's start value (0 without a definition);
//        starting  finished (1F0670): awarded (1F1030), no current slot.
//                  Still wanted (1EFD60): its timer runs down, then it is
//                  active. Otherwise it is idle again, no current slot;
//        active    counts ticks; finished: awarded and reset. Still wanted:
//                  unchanged. Otherwise ending, for its definition's hold time
//                  in seconds (0 without a definition);
//        ending    counts ticks; finished: awarded and reset. Still wanted:
//                  active again. Otherwise its timer runs down, then it is
//                  idle, no current slot;
//        cooling   its timer runs down, then it is idle;
//   4. awards every other slot from 15 to 30 that the original's case table
//      selects, unless both of its mask bits are set, when it is finished.
// Nothing happens for a player without an engine record, when the engine
// records are disabled, or in a campaign whose option word is 2 or 3. The
// state names describe the transitions; the game meaning of the slots is not
// established.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (the Access enum); the boundary adapter
// (native_tu15_engine_player_update.hpp) maps the role to the original
// instruction for the diagnostic cursor. The calls into translated code, the
// original's case tables, registers and flags belong to that adapter, reached
// through a Frame object. The update's locals live in the original's stack
// frame, so they are read and written as memory, with roles, in the
// original's order; Frame::local gives their addresses.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (the slot
//     timers are decremented in place and read again; locals are read again
//     after the calls);
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - the values the original keeps in callee-saved registers across a call
//     are each phase's walk (Scan, CurrentSlot, Award). A conforming callee
//     returns them unchanged; the adapter hands back what the callee actually
//     returned, so the update continues exactly as the original does even
//     when a callee does not preserve them.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_multiplayer_engine {
using native_object_update::DataArray;
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t players=0x4f54f4u;          // DataArray* of Player
inline constexpr std::uint32_t game_globals=0x4f3218u;     // GameGlobals*
inline constexpr std::uint32_t engine_players=0x52be64u;   // EnginePlayers*
inline constexpr std::uint32_t slot_definitions=0x51df94u; // SlotDefinitions*, or null
inline constexpr std::uint32_t game_time=0x51df54u;        // GameTime*
}

// A player datum (0x21C bytes): the field this system reads.
struct Player {
    std::uint8_t unused_here_00[0x28];
    std::int16_t engine_record;       // +0x28: index of the player's EnginePlayer, or -1
};
static_assert(offsetof(Player,engine_record)==0x28);
inline constexpr std::uint32_t player_size=0x21cu;

// The game globals: the fields this system reads.
struct GameGlobals {
    std::uint8_t unused_here_00[8];
    std::int32_t game_mode;           // +0x08: 1 in a campaign
    std::uint8_t unused_here_0c[0x126];
    std::int16_t campaign_option;     // +0x132: 2 and 3 suppress the update (the game meaning is not established)
};
static_assert(offsetof(GameGlobals,game_mode)==8 && offsetof(GameGlobals,campaign_option)==0x132);
inline constexpr std::int32_t campaign_mode=1;

// The game time globals: the field this system reads.
struct GameTime {
    std::uint16_t unused_here_00;
    std::int16_t ticks_per_second;    // +2
};
static_assert(offsetof(GameTime,ticks_per_second)==2);

// One slot of an engine record.
struct Slot {
    std::int16_t state;               // slot_state
    std::int16_t timer;               // ticks left in the state
};
static_assert(sizeof(Slot)==4 && offsetof(Slot,timer)==2);
namespace slot_state {
inline constexpr std::uint32_t idle=0,starting=1,active=2,ending=3,cooling=4;
}
inline constexpr std::uint32_t slot_count=32u;
inline constexpr std::uint32_t last_slot=slot_count-1u;
// The slots the award pass may award: 15 to 30.
inline constexpr std::uint32_t first_awarded_slot=15u,awarded_slot_span=15u;

// A player's engine record (0x1B0 bytes).
struct EnginePlayer {
    Slot slots[32];                   // +0x00
    std::int32_t current;             // +0x80: the current slot, or -1
    std::uint32_t current_ticks;      // +0x84: ticks the current slot has been active or ending
    std::uint8_t unused_here_88[0xe8];
    std::uint32_t award_mask_a;       // +0x170: a slot with its bit set here and in
    std::uint32_t award_mask_b;       // +0x174: award_mask_b is not awarded
    std::uint8_t unused_here_178[0x1d];
    std::uint8_t countdowns[7];       // +0x195: ticked down each update; [2] is cleared when [1] runs out
    std::uint8_t unused_here_19c[0x14];
};
static_assert(sizeof(EnginePlayer)==0x1b0 && offsetof(EnginePlayer,current)==0x80 && offsetof(EnginePlayer,current_ticks)==0x84 &&
              offsetof(EnginePlayer,award_mask_a)==0x170 && offsetof(EnginePlayer,award_mask_b)==0x174 &&
              offsetof(EnginePlayer,countdowns)==0x195);
// The engine records, then the byte that enables them.
struct EnginePlayers {
    EnginePlayer players[4];
    std::uint8_t unused_here_6c0[5];
    std::uint8_t enabled;             // +0x6C5
};
static_assert(offsetof(EnginePlayers,enabled)==0x6c5);

// The slot definitions: a list of 0x14-byte entries, one per slot.
struct SlotDefinitions {
    std::uint8_t unused_here_00[0x420];
    std::int32_t count;               // +0x420
    std::uint32_t entries;            // +0x424: SlotDefinition
};
static_assert(offsetof(SlotDefinitions,count)==0x420 && offsetof(SlotDefinitions,entries)==0x424);
struct SlotDefinition {
    std::uint8_t unused_here_00[8];
    std::uint16_t hold_seconds;       // +0x08: how long the slot stays ending
    std::uint16_t unused_here_0a;
    std::uint32_t start;              // +0x0C: handed to 1494F0, whose result starts the slot's timer
    std::uint32_t unused_here_10;
};
static_assert(sizeof(SlotDefinition)==0x14 && offsetof(SlotDefinition,hold_seconds)==8 && offsetof(SlotDefinition,start)==0xc);

// ------------------------------------------------------------ player slots
// 1EEE80. Frame:
//   local(Local)                 the address of one of the update's locals
//   scan_pick(scan, index)       1EFD60: does the scanned idle slot want to be current?
//   state_case(state)            the case of the original's state table the state selects
//   start(slot, value)           1494F0: the starting timer (AX)
//   finished(slot, index)        1F0670: is the current slot finished?
//   wanted(slot, index)          1EFD60: does the current slot still want to be current?
//   award(slot)                  1F1030: awards the current slot
//   award_case(slot)             whether the original's award case table selects the slot
//   award_finished(award, index) 1F0670: is the slot finished?
//   award_slot(award, definition, entry)  1F1030: awards the slot
namespace player_slots {
enum class Local {
    engine_record,   // the player's engine record index
    slot_cursor,     // the award pass's Slot
    record_offset    // the engine record's offset in EnginePlayers
};
enum class Access {
    player_table,player_rows,game,engine_record,game_mode,engine_record_store,campaign_option,
    engines,enabled,
    countdown_0,current_read,record_offset_store,countdown_0_store,countdown_1,countdown_1_store,countdown_2_clear,
    countdown_3,countdown_3_store,countdown_4,countdown_4_store,countdown_5,countdown_5_store,countdown_6,countdown_6_store,
    scan_state,cooling_timer,cooling_timer_store,cooling_check,cooling_timer_clear,cooling_state_clear,scan_record_reload,
    definitions,definition_count,definition_list,previous_current,previous_timer_clear,previous_state_clear,
    current_store,current_ticks_clear,current_state,
    start_state,start_timer_none,start_value,start_timer,
    starting_award_clear,starting_record,starting_timer,starting_timer_store,starting_check,starting_promote_state,
    starting_promote_timer,starting_reset_state,starting_reset_timer,starting_reset_clear,
    active_ticks,active_ticks_store,active_record,active_ending_state,game_time,ticks_per_second,hold_seconds,active_ending_timer,
    award_clear,award_ticks_clear,
    ending_ticks,ending_ticks_store,ending_record,ending_active_state,ending_active_timer,ending_timer,ending_timer_store,
    ending_check,ending_idle_timer,ending_idle_state,ending_clear,ending_ticks_clear,
    cooling_current_timer,cooling_current_store,cooling_current_check,cooling_current_timer_clear,cooling_current_state_clear,
    cursor_init,award_current,award_offset,award_engines,award_mask_b,award_mask_a,award_record,award_definitions,
    award_count,award_list,award_cursor,cursor_read,cursor_store
};
enum class Exit {no_record,disabled,updated};
// The case of the state table that runs.
enum class Case {idle,starting,active,ending,cooling};
// Kept across the scan's calls.
struct Scan {
    std::uint32_t record;     // the EnginePlayer
    std::uint32_t slot;       // the slot scanned
    std::uint32_t timer;      // its Slot::timer (the scan walks the timers)
    std::uint32_t current;    // the current slot so far
};
// Kept across the current slot's calls.
struct CurrentSlot {
    std::uint32_t record;
    std::uint32_t slot;
    std::uint32_t entry;      // its Slot
    std::uint32_t definition; // its SlotDefinition, or 0
};
// Kept across the award pass's calls.
struct Award {
    std::uint32_t record;
    std::uint32_t slot;
    std::uint32_t definition_offset;   // the slot's offset in the definition list
};
constexpr std::uint32_t slot_of_timer(std::uint32_t timer){return timer-std::uint32_t(offsetof(Slot,timer));}

// A countdown byte ticks down to zero; returns the new value, or none if it
// was already zero.
template<class Memory> std::uint32_t tick(Memory &memory,Access read,Access store,std::uint32_t address) {
    const auto ticks=memory.load8(read,address);
    if(!ticks)return none;
    memory.store8(store,address,ticks-1u);
    return ticks-1u;
}
// A timer runs down in place (a decrement, then the timer read again): true
// while it is still above zero.
template<class Memory> bool run_down(Memory &memory,Access read,Access store,Access check,std::uint32_t timer) {
    memory.store16(store,timer,memory.load16(read,timer)-1u);
    return std::int16_t(memory.load16(check,timer))>0;
}

// The current slot (step 3), in a state the original's state table covers.
template<class Memory,class Frame>
void advance(Memory &memory,Frame &frame,CurrentSlot &slot,std::uint32_t state,std::uint32_t record_index) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto state_of=[&]{return at(slot.entry,offsetof(Slot,state));};
    const auto timer_of=[&]{return at(slot.entry,offsetof(Slot,timer));};
    // Finished active or ending slots are awarded, and nothing is current.
    const auto award_and_reset=[&] {
        frame.award(slot);
        memory.store32(award_clear,at(slot.record,offsetof(EnginePlayer,current)),none);
        memory.store32(award_ticks_clear,at(slot.record,offsetof(EnginePlayer,current_ticks)),0);
    };
    const auto count_tick=[&](Access read,Access store) {
        const auto address=at(slot.record,offsetof(EnginePlayer,current_ticks));
        memory.store32(store,address,memory.load32(read,address)+1u);
    };
    switch(frame.state_case(state)) {
    case Case::idle: {
        memory.store16(start_state,state_of(),slot_state::starting);
        if(!slot.definition){memory.store16(start_timer_none,timer_of(),0);return;}
        const auto value=memory.load32(start_value,at(slot.definition,offsetof(SlotDefinition,start)));
        const auto ticks=frame.start(slot,value);
        memory.store16(start_timer,timer_of(),ticks);
        return;
    }
    case Case::starting:
        if(frame.finished(slot,record_index)) {
            frame.award(slot);
            memory.store32(starting_award_clear,at(slot.record,offsetof(EnginePlayer,current)),none);
            return;
        }
        if(frame.wanted(slot,memory.load32(starting_record,local(Local::engine_record)))) {
            if(run_down(memory,starting_timer,starting_timer_store,starting_check,timer_of()))return;
            memory.store16(starting_promote_state,state_of(),slot_state::active);
            memory.store16(starting_promote_timer,timer_of(),0);
            return;
        }
        memory.store16(starting_reset_state,state_of(),slot_state::idle);
        memory.store16(starting_reset_timer,timer_of(),0);
        memory.store32(starting_reset_clear,at(slot.record,offsetof(EnginePlayer,current)),none);
        return;
    case Case::active: {
        count_tick(active_ticks,active_ticks_store);
        if(frame.finished(slot,record_index)){award_and_reset();return;}
        if(frame.wanted(slot,memory.load32(active_record,local(Local::engine_record))))return;
        memory.store16(active_ending_state,state_of(),slot_state::ending);
        if(!slot.definition){memory.store16(start_timer_none,timer_of(),0);return;}
        const auto time=memory.load32(game_time,globals::game_time);
        const auto per_second=widen16(memory.load16(ticks_per_second,at(time,offsetof(GameTime,ticks_per_second))));
        const auto seconds=memory.load16(hold_seconds,at(slot.definition,offsetof(SlotDefinition,hold_seconds)));
        memory.store16(active_ending_timer,timer_of(),per_second*seconds);
        return;
    }
    case Case::ending:
        count_tick(ending_ticks,ending_ticks_store);
        if(frame.finished(slot,record_index)){award_and_reset();return;}
        if(frame.wanted(slot,memory.load32(ending_record,local(Local::engine_record)))) {
            memory.store16(ending_active_state,state_of(),slot_state::active);
            memory.store16(ending_active_timer,timer_of(),0);
            return;
        }
        if(run_down(memory,ending_timer,ending_timer_store,ending_check,timer_of()))return;
        memory.store16(ending_idle_timer,timer_of(),0);
        memory.store16(ending_idle_state,state_of(),slot_state::idle);
        memory.store32(ending_clear,at(slot.record,offsetof(EnginePlayer,current)),none);
        memory.store32(ending_ticks_clear,at(slot.record,offsetof(EnginePlayer,current_ticks)),0);
        return;
    case Case::cooling:
        if(run_down(memory,cooling_current_timer,cooling_current_store,cooling_current_check,timer_of()))return;
        memory.store16(cooling_current_timer_clear,timer_of(),0);
        memory.store16(cooling_current_state_clear,state_of(),slot_state::idle);
        return;
    }
}

template<class Memory,class Frame> Exit update(Memory &memory,Frame &frame,std::uint32_t player_handle) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};

    // The player's engine record; a campaign with option 2 or 3 suppresses it.
    const auto players=memory.load32(player_table,globals::players);
    const auto rows=memory.load32(player_rows,at(players,offsetof(DataArray,data)));
    const auto globals_data=memory.load32(game,globals::game_globals);
    const auto player=at(rows,handle_index(player_handle)*player_size);
    auto record_index=widen16(memory.load16(engine_record,at(player,offsetof(Player,engine_record))));
    const bool campaign=signed_value(memory.load32(game_mode,at(globals_data,offsetof(GameGlobals,game_mode))))==campaign_mode;
    memory.store32(engine_record_store,local(Local::engine_record),record_index);
    bool suppressed=false;
    if(campaign) {
        const auto option=memory.load16(campaign_option,at(globals_data,offsetof(GameGlobals,campaign_option)));
        suppressed=option==3u || option==2u;
    }
    if(record_index==none)return Exit::no_record;
    const auto engine_players=memory.load32(engines,globals::engine_players);
    if(!memory.load8(enabled,at(engine_players,offsetof(EnginePlayers,enabled))) || suppressed)return Exit::disabled;

    // 1. The countdown bytes.
    const auto offset=record_index*std::uint32_t(sizeof(EnginePlayer));
    const auto record=engine_players+offset;
    const auto countdown=[&](unsigned n){return at(record,offsetof(EnginePlayer,countdowns)+n);};
    const auto first=memory.load8(countdown_0,countdown(0));
    Scan scan{record,0,at(record,offsetof(EnginePlayer,slots[0].timer)),
              memory.load32(current_read,at(record,offsetof(EnginePlayer,current)))};
    memory.store32(record_offset_store,local(Local::record_offset),offset);
    if(first)memory.store8(countdown_0_store,countdown(0),first-1u);
    if(tick(memory,countdown_1,countdown_1_store,countdown(1))==0)memory.store8(countdown_2_clear,countdown(2),0);
    tick(memory,countdown_3,countdown_3_store,countdown(3));
    tick(memory,countdown_4,countdown_4_store,countdown(4));
    tick(memory,countdown_5,countdown_5_store,countdown(5));
    tick(memory,countdown_6,countdown_6_store,countdown(6));

    // 2. The scan.
    do {
        const auto state=memory.load16(scan_state,at(slot_of_timer(scan.timer),offsetof(Slot,state)));
        if(state==slot_state::idle) {
            if(scan.current==none || signed_value(scan.current)>signed_value(scan.slot) || scan.slot==last_slot) {
                const bool accepted=frame.scan_pick(scan,record_index);
                record_index=memory.load32(scan_record_reload,local(Local::engine_record));
                if(accepted)scan.current=scan.slot;
            }
        } else if(state==slot_state::cooling) {
            if(!run_down(memory,cooling_timer,cooling_timer_store,cooling_check,scan.timer)) {
                memory.store16(cooling_timer_clear,scan.timer,0);
                memory.store16(cooling_state_clear,at(slot_of_timer(scan.timer),offsetof(Slot,state)),slot_state::idle);
            }
        }
        ++scan.slot;
        scan.timer+=std::uint32_t(sizeof(Slot));
    } while(signed_value(scan.slot)<signed_value(slot_count));

    // 3. The current slot, with its definition when the list has one.
    auto pass_record=scan.record;
    if(scan.current!=none) {
        CurrentSlot slot{scan.record,scan.current,0,0};
        const auto definitions=memory.load32(Access::definitions,globals::slot_definitions);
        if(definitions && signed_value(slot.slot)>=0 &&
           signed_value(slot.slot)<signed_value(memory.load32(definition_count,at(definitions,offsetof(SlotDefinitions,count)))))
            slot.definition=element(memory.load32(definition_list,at(definitions,offsetof(SlotDefinitions,entries))),slot.slot,
                                    sizeof(SlotDefinition));
        const auto previous=memory.load32(previous_current,at(slot.record,offsetof(EnginePlayer,current)));
        slot.entry=element(at(slot.record,offsetof(EnginePlayer,slots)),slot.slot,sizeof(Slot));
        if(slot.slot!=previous) {
            // The previous current slot is reset, and the tick count restarts.
            if(previous!=none) {
                const auto old=element(at(slot.record,offsetof(EnginePlayer,slots)),previous,sizeof(Slot));
                memory.store16(previous_timer_clear,at(old,offsetof(Slot,timer)),0);
                memory.store16(previous_state_clear,at(old,offsetof(Slot,state)),0);
            }
            memory.store32(current_store,at(slot.record,offsetof(EnginePlayer,current)),slot.slot);
            memory.store32(current_ticks_clear,at(slot.record,offsetof(EnginePlayer,current_ticks)),0);
        }
        // States above cooling have no case.
        const auto state=memory.load16(current_state,at(slot.entry,offsetof(Slot,state)));
        if(state<=slot_state::cooling)advance(memory,frame,slot,state,record_index);
        pass_record=slot.record;
    }

    // 4. The award pass over every slot but the current one.
    Award pass{pass_record,0,0};
    memory.store32(cursor_init,local(Local::slot_cursor),pass.record);
    do {
        if(pass.slot!=memory.load32(award_current,at(pass.record,offsetof(EnginePlayer,current)))) {
            const auto mask_offset=memory.load32(award_offset,local(Local::record_offset));
            const auto masks=memory.load32(award_engines,globals::engine_players)+mask_offset;
            const auto bit=1u<<(pass.slot&31u);
            const bool masked_b=(memory.load32(award_mask_b,at(masks,offsetof(EnginePlayer,award_mask_b)))&bit)!=0;
            const bool masked_a=(memory.load32(award_mask_a,at(masks,offsetof(EnginePlayer,award_mask_a)))&bit)!=0;
            if(!(masked_a && masked_b) && pass.slot-first_awarded_slot<=awarded_slot_span && frame.award_case(pass.slot) &&
               frame.award_finished(pass,memory.load32(award_record,local(Local::engine_record)))) {
                const auto definitions=memory.load32(award_definitions,globals::slot_definitions);
                std::uint32_t definition=0;
                if(definitions && signed_value(pass.definition_offset)>=0 &&
                   signed_value(pass.slot)<signed_value(memory.load32(award_count,at(definitions,offsetof(SlotDefinitions,count)))))
                    definition=memory.load32(award_list,at(definitions,offsetof(SlotDefinitions,entries)))+pass.definition_offset;
                frame.award_slot(pass,definition,memory.load32(award_cursor,local(Local::slot_cursor)));
            }
        }
        const auto cursor=memory.load32(cursor_read,local(Local::slot_cursor));
        pass.definition_offset+=std::uint32_t(sizeof(SlotDefinition));
        ++pass.slot;
        memory.store32(cursor_store,local(Local::slot_cursor),cursor+std::uint32_t(sizeof(Slot)));
    } while(signed_value(pass.definition_offset)<signed_value(slot_count*std::uint32_t(sizeof(SlotDefinition))));
    return Exit::updated;
}
} // namespace player_slots
} // namespace halo2::native_multiplayer_engine
