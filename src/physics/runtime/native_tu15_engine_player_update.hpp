#pragma once
// Boundary adapter for one player's engine slot update
// (native_tu15_engine_player_update_systems.hpp), entered at 1EEE80 in the
// multiplayer engines unit. Its caller and its children (1EFD60, 1F0670 and
// 1F1030, entered as the unit's direct bodies, and 1494F0 through the unit's
// dispatch) are still translated TU1.5 code, so this adapter owns that ABI
// and nothing else:
//   - the argument: AX the player handle; no result;
//   - the stack frame: SUB ESP,0C makes three locals (the record index, the
//     award pass's slot cursor, the record's offset); EBX, EBP, ESI and EDI
//     are saved below them. Frame::local gives a local's address as the
//     original addresses it, from the stack pointer the last child left;
//   - the original's register file at every child call: each phase's walk
//     (the scan: EDI record, EBX slot, ESI timer, EBP current; the current
//     slot: EDI record, EBP slot, ESI entry, EBX definition; the award pass:
//     EDI record, ESI slot, EBX definition offset), handed back after each
//     call as the callee returns it; the register arguments (EAX, EDX) and
//     what the original leaves in scratch registers (the slot state in ECX,
//     the mask sum in EBP, ...); the flags of the instruction that decides
//     each call (the idle slot's CMP chain, the state's CMP ECX,4, the tick
//     count's INC, the award's range CMP, the previous child's TEST AL,AL);
//   - the original's two case tables: the state's jump table (1EF260) and
//     the award pass's byte and jump tables (1EF27C, 1EF274), read in order
//     and mapped through the unit's canonical branch mapping, so unmapped and
//     unexpected targets raise the translated body's faults;
//   - the stack arguments, return addresses, the exit flags (ADD ESP,0C) and
//     the RET;
//   - the diagnostic cursor: each access publishes its original instruction
//     (the role -> instruction table below), and the RET publishes the exit;
//   - every store goes through the unit's store helper.
// Delete the adapter when the caller and the children go native.
// HALO2_REFERENCE_ENGINE_PLAYER_UPDATE=1 restores the translated body.
// HALO2_VERIFY_ENGINE_PLAYER_UPDATE=1 records the native run (children
// genuine, once) and replays the unmodified translated body against it, as
// in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_engine_player_update_systems.hpp"

namespace halo2::native_engine_player_update_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx16;using native_object_tick_detail::si;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_multiplayer_engine;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ENGINE_PLAYER_UPDATE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ENGINE_PLAYER_UPDATE");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    campaign_globals, campaign_suppressed, no_player, engine_off, suppressed,
    count_195, count_196, clear_197, count_198, count_199, count_19a, count_19b,
    empty_unowned, empty_beyond, empty_last, empty_kept, slot_picked, slot_declined, timed_kept, timed_expired, slot_other,
    no_slot, roster_record, no_roster, slot_same, slot_changed, slot_first,
    state_over, state0_none, state0_query, state1_award, state1_promote_wait, state1_promote, state1_reset,
    state2_award, state2_held, state2_none, state2_timer, state3_award, state3_promote, state3_wait, state3_clear,
    state4_counting, state4_settled,
    award_owner, award_both, award_range, award_table_skip, award_declined, award_no_roster, award_negative,
    award_beyond, award_indexed, branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Engine player update verify: %llu calls; %llu ordered accesses (%llu stores) and %llu child "
            "boundaries identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// ---- 1EEE80 -------------------------------------------------------------------
// Coverage: the branch outcome is the original instruction that follows
// (the rest is read from the deciding compares, below).
inline constexpr Edge player_slot_edges[]{
    {0x1eef03u,0x1eef0fu,count_195},{0x1eef15u,0x1eef21u,count_196},{0x1eef21u,0x1eef29u,clear_197},
    {0x1eef30u,0x1eef3cu,count_198},{0x1eef42u,0x1eef4eu,count_199},{0x1eef54u,0x1eef60u,count_19a},{0x1eef66u,0x1eef72u,count_19b},
    {0x1eefd5u,0x1eeffcu,no_roster},{0x1eefe7u,0x1eeffcu,no_roster},{0x1eefe7u,0x1eefefu,roster_record},
    {0x1eeffcu,0x1ef023u,slot_same},{0x1eeffcu,0x1ef00eu,slot_changed},{0x1eeffcu,0x1ef017u,slot_first},
    {0x1ef023u,0x1ef19cu,state_over},
    {0x1ef038u,0x1ef10bu,state0_none},{0x1ef038u,0x1ef043u,state0_query},
    {0x1ef057u,0x1ef060u,state1_award},{0x1ef08cu,0x1ef19cu,state1_promote_wait},{0x1ef08cu,0x1ef097u,state1_promote},
    {0x1ef07fu,0x1ef0a7u,state1_reset},
    {0x1ef0c9u,0x1ef125u,state2_award},{0x1ef0d9u,0x1ef19cu,state2_held},{0x1ef0e8u,0x1ef10bu,state2_none},
    {0x1ef0e8u,0x1ef0efu,state2_timer},
    {0x1ef11cu,0x1ef125u,state3_award},{0x1ef14bu,0x1ef154u,state3_promote},{0x1ef165u,0x1ef19cu,state3_wait},
    {0x1ef165u,0x1ef16eu,state3_clear},
    {0x1ef18bu,0x1ef19cu,state4_counting},{0x1ef18bu,0x1ef191u,state4_settled},
    {0x1ef1a0u,0x1ef23au,award_owner},{0x1ef1f7u,0x1ef23au,award_table_skip},{0x1ef204u,0x1ef23au,award_declined},
    {0x1ef21du,0x1ef22du,award_beyond},{0x1ef225u,0x1ef22du,award_indexed},
};
// The three locals of SUB ESP,0C.
struct Locals {
    std::uint32_t engine_record;
    std::uint32_t slot_cursor;
    std::uint32_t record_offset;
};
template<class State,class Dispatch,class Branch_,class Pick,class Test,class Award> struct PlayerUpdate : Port<State> {
    using Access=logic::player_slots::Access;
    using Case=logic::player_slots::Case;
    Dispatch &dispatch;Branch_ &branch;Pick &pick;Test &test;Award &award_child;
    std::uint32_t locals{};      // the Locals as the original addresses them
    bool campaign{},scanning{},current_phase{};
    Case current_case{};
    PlayerUpdate(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d,Branch_ &b,Pick &p,Test &x,Award &a)
        :Port<State>(s,c,t,player_slot_edges),dispatch(d),branch(b),pick(p),test(x),award_child(a){}
    // The last scan step: INC EBX; ADD ESI,4; CMP EBX,20; then CMP EBP,-1.
    void end_scan() {
        auto &gp=this->gp;
        if(scanning){scanning=false;gp[ebx]=this->flags.increment(gp[ebx],4,false);gp[esi]+=4u;this->compare32(gp[ebx],0x20u);}
        this->compare32(gp[ebp],logic::none);
    }
    std::uint32_t instruction(Access a,std::uint32_t address) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case player_table:return 0x1eee83u;
        case player_rows:return 0x1eee89u;
        case game:return 0x1eee8cu;
        // AND EAX,FFFF; IMUL EAX,21C (the player's row).
        case engine_record:gp[eax]=this->flags.product(this->flags.logic(gp[eax]&0xffffu,4),logic::player_size);return 0x1eee9du;
        case game_mode:return 0x1eeea2u;
        case engine_record_store:return 0x1eeea6u;
        case campaign_option:return 0x1eeeabu;
        case engines:return 0x1eeecdu;
        case enabled:this->push(0x1eeed3u,gp[ebx]);return 0x1eeed4u;
        // PUSH EBP/ESI/EDI; IMUL EDI,EAX,1B0 (the record's offset).
        case countdown_0:
            this->push(0x1eeeeau,gp[ebp]);this->push(0x1eeeebu,gp[esi]);this->push(0x1eeeecu,gp[edi]);
            gp[edi]=this->flags.product(gp[eax],std::uint32_t(sizeof(logic::EnginePlayer)));
            return 0x1eeef5u;
        case current_read:return 0x1eeefcu;
        case record_offset_store:return 0x1eef03u;
        // DEC CL before each countdown store.
        case countdown_0_store:case countdown_1_store:case countdown_3_store:case countdown_4_store:case countdown_5_store:
        case countdown_6_store:
            set_low_byte(gp[ecx],this->flags.increment(low_byte(gp[ecx]),1,true));
            return a==countdown_0_store?0x1eef0fu:a==countdown_1_store?0x1eef21u:a==countdown_3_store?0x1eef3cu:
                   a==countdown_4_store?0x1eef4eu:a==countdown_5_store?0x1eef60u:0x1eef72u;
        case countdown_1:return 0x1eef15u;
        case countdown_2_clear:return 0x1eef29u;
        case countdown_3:return 0x1eef30u;
        case countdown_4:return 0x1eef42u;
        case countdown_5:return 0x1eef54u;
        case countdown_6:return 0x1eef66u;
        // XOR EBX,EBX; LEA ESI,[EDI+2] before the first slot; INC EBX; ADD
        // ESI,4 before the others. ESI is the scan's timer.
        case scan_state:
            if(scanning)gp[ebx]=this->flags.increment(gp[ebx],4,false);
            else {scanning=true;gp[ebx]=this->flags.logic(0,4);}
            gp[esi]=address+std::uint32_t(offsetof(logic::Slot,timer));
            return 0x1eef80u;
        case cooling_timer:case cooling_timer_store:return 0x1eef8du;
        case cooling_check:return 0x1eef90u;
        case cooling_timer_clear:return 0x1eef96u;
        case cooling_state_clear:return 0x1eef9bu;
        case scan_record_reload:return 0x1eefbbu;
        case definitions:end_scan();current_phase=true;return 0x1eefd5u;
        case definition_count:return 0x1eefe7u;
        case definition_list:return 0x1eefefu;
        case previous_current:return 0x1eeffcu;
        case previous_timer_clear:return 0x1ef00eu;
        case previous_state_clear:return 0x1ef013u;
        case current_store:return 0x1ef017u;
        case current_ticks_clear:return 0x1ef01du;
        case current_state:return 0x1ef023u;
        case start_state:this->compare32(gp[ebx],gp[edx]);return 0x1ef038u;            // CMP EBX,EDX (no definition)
        case start_timer_none:gp[eax]=this->flags.logic(0,4);return 0x1ef10bu;        // XOR EAX,EAX
        case start_value:return 0x1ef043u;
        case start_timer:return 0x1ef04cu;
        case starting_award_clear:return 0x1ef069u;
        case starting_record:return 0x1ef078u;
        case starting_timer:case starting_timer_store:return 0x1ef088u;
        case starting_check:return 0x1ef08cu;
        case starting_promote_state:return 0x1ef097u;
        case starting_promote_timer:return 0x1ef09cu;
        case starting_reset_state:return 0x1ef0a7u;
        case starting_reset_timer:return 0x1ef0acu;
        case starting_reset_clear:return 0x1ef0b2u;
        case active_ticks:case active_ticks_store:return 0x1ef0c1u;
        case active_record:return 0x1ef0d2u;
        case active_ending_state:this->test32(gp[ebx]);return 0x1ef0e8u;                // TEST EBX,EBX (no definition)
        case game_time:return 0x1ef0efu;
        case ticks_per_second:return 0x1ef0f5u;
        case hold_seconds:return 0x1ef0f9u;
        case active_ending_timer:gp[eax]=this->flags.product(gp[eax],gp[ecx]);return 0x1ef100u;   // IMUL EAX,ECX
        case award_clear:return 0x1ef12eu;
        case award_ticks_clear:return 0x1ef138u;
        case ending_ticks:case ending_ticks_store:return 0x1ef114u;
        case ending_record:return 0x1ef144u;
        case ending_active_state:return 0x1ef154u;
        case ending_active_timer:return 0x1ef159u;
        case ending_timer:case ending_timer_store:return 0x1ef161u;
        case ending_check:return 0x1ef165u;
        case ending_idle_timer:gp[eax]=this->flags.logic(0,4);return 0x1ef16eu;       // XOR EAX,EAX
        case ending_idle_state:return 0x1ef172u;
        case ending_clear:return 0x1ef175u;
        case ending_ticks_clear:return 0x1ef17fu;
        case cooling_current_timer:case cooling_current_store:return 0x1ef187u;
        case cooling_current_check:return 0x1ef18bu;
        case cooling_current_timer_clear:return 0x1ef191u;
        case cooling_current_state_clear:return 0x1ef195u;
        // XOR ESI,ESI; XOR EBX,EBX (the award pass's slot and offset).
        case cursor_init:
            if(!current_phase){end_scan();this->reached(no_slot);}
            gp[esi]=this->flags.logic(0,4);gp[ebx]=this->flags.logic(0,4);
            return 0x1ef19cu;
        case award_current:return 0x1ef1a0u;
        case award_offset:return 0x1ef1acu;
        case award_engines:return 0x1ef1b9u;
        case award_mask_b:return 0x1ef1bfu;
        case award_mask_a:return 0x1ef1ccu;
        case award_record:return 0x1ef1feu;
        case award_definitions:return 0x1ef20du;
        case award_count:return 0x1ef21du;
        case award_list:return 0x1ef225u;
        case award_cursor:return 0x1ef22du;
        case cursor_read:return 0x1ef23au;
        case cursor_store:return 0x1ef24bu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case player_table:case game:case cursor_read:case definitions:case previous_current:case current_state:case award_engines:
        case award_definitions:case hold_seconds:case active_record:
            gp[ecx]=v;
            break;
        case player_rows:case engines:case start_value:case game_time:case award_offset:case award_cursor:case ending_record:
            gp[edx]=v;
            break;
        case engine_record:case ticks_per_second:gp[eax]=sx16(v);break;
        case scan_record_reload:case starting_record:case award_record:gp[eax]=v;break;
        case current_read:gp[ebp]=v;break;
        case countdown_0:set_low_byte(gp[ecx],v);break;
        case countdown_1:case countdown_3:case countdown_4:case countdown_5:case countdown_6:set_low_byte(gp[ecx],v);this->test8(v);break;
        // CMP [ECX+8],1.
        case game_mode:this->compare32(v,std::uint32_t(logic::campaign_mode));campaign=v==std::uint32_t(logic::campaign_mode);break;
        // MOVSX ECX; CMP CX,3 / CMP CX,2; MOV CL,1 or XOR CL,CL; CMP EAX,-1.
        case campaign_option: {
            this->reached(campaign_globals);
            gp[ecx]=sx16(v);
            const bool suppress=(v&0xffffu)==3u || (v&0xffffu)==2u;
            if(suppress){set_low_byte(gp[ecx],1);this->reached(campaign_suppressed);}
            else {set_low_byte(gp[ecx],0);this->flags.logic(0,1);}
            this->compare32(gp[eax],logic::none);
            break;
        }
        // MOV BL; TEST BL,BL; then TEST CL,CL (the suppression).
        case enabled:set_low_byte(gp[ebx],v);this->test8(v);if(v&0xffu)this->test8(gp[ecx]);break;
        // MOVZX ECX,word; TEST ECX,ECX; an idle slot is offered after the
        // CMP chain EBP,-1 / EBP,EBX / EBX,1F; a cooling one is CMP ECX,4.
        case scan_state:
            gp[ecx]=v;this->test32(v);
            if(v==logic::slot_state::idle) {
                if(gp[ebp]==logic::none){this->compare32(gp[ebp],logic::none);this->reached(empty_unowned);}
                else {
                    this->compare32(gp[ebp],logic::none);this->compare32(gp[ebp],gp[ebx]);
                    if(si(gp[ebp])>si(gp[ebx]))this->reached(empty_beyond);
                    else {this->compare32(gp[ebx],logic::last_slot);this->reached(gp[ebx]==logic::last_slot?empty_last:empty_kept);}
                }
            } else {this->compare32(v,logic::slot_state::cooling);if(v!=logic::slot_state::cooling)this->reached(slot_other);}
            break;
        case cooling_check:this->flags.minus(v,0,2);this->reached(std::int16_t(v)>0?timed_kept:timed_expired);break;
        case starting_check:case ending_check:this->flags.minus(v,0,2);break;
        case cooling_current_check:this->flags.minus(v,gp[edx],2);break;                    // CMP word,DX (zero)
        // INC dword [EDI+84] (CF stays CMP ECX,4's).
        case active_ticks:case ending_ticks:this->flags.increment(v,4,false);break;
        default:break;
        }
        // The rest decide or set more than one register.
        switch(a) {
        // XOR EDX,EDX; XOR EBX,EBX; CMP ECX,EDX; CMP EBP,EDX.
        case definitions:
            gp[edx]=this->flags.logic(0,4);gp[ebx]=this->flags.logic(0,4);this->compare32(v,0);
            if(v)this->compare32(gp[ebp],0);
            break;
        case definition_count:this->compare32(gp[ebp],v);break;                              // CMP EBP,[ECX+420]
        // MOV ECX; LEA ESI,[EBP+EBP*4]; LEA EBX,[ECX+ESI*4] (the definition).
        case definition_list:gp[ecx]=v;gp[esi]=gp[ebp]*5u;gp[ebx]=v+gp[esi]*4u;break;
        // CMP EBP,ECX; LEA ESI,[EDI+EBP*4] (the entry); CMP ECX,-1.
        case previous_current:
            this->compare32(gp[ebp],v);gp[esi]=gp[edi]+gp[ebp]*4u;
            if(v!=gp[ebp])this->compare32(v,logic::none);
            break;
        case current_state:this->compare32(v,logic::slot_state::cooling);break;            // CMP ECX,4
        case award_current:this->compare32(gp[esi],v);break;                                // CMP ESI,[EDI+80]
        // MOV ECX,ESI; MOV EAX,1; SHL EAX,CL.
        case award_offset:gp[ecx]=gp[esi];gp[eax]=this->flags.shift(1u,gp[ecx]&0xffu,4);break;
        // AND EBP,EAX; NEG EBP; SBB EBP,EBP.
        case award_mask_b:gp[ebp]=(v&gp[eax])?0xffffffffu:0u;break;
        // AND EAX,[mask a]; AND EBP,2; NEG/SBB/NEG EAX; ADD EBP,EAX; CMP
        // EBP,3; SETE AL; TEST AL,AL; then LEA EAX,[ESI-F]; CMP EAX,F.
        case award_mask_a: {
            gp[eax]&=v;gp[ebp]&=2u;gp[eax]=gp[eax]?1u:0u;gp[ebp]+=gp[eax];
            const bool both=gp[ebp]==3u;
            this->compare32(gp[ebp],3u);set_low_byte(gp[eax],both?1u:0u);this->test8(both?1u:0u);
            if(both){this->reached(award_both);break;}
            gp[eax]=gp[esi]-logic::first_awarded_slot;this->compare32(gp[eax],logic::awarded_slot_span);
            if(gp[eax]>logic::awarded_slot_span)this->reached(award_range);
            break;
        }
        // XOR EAX,EAX; TEST ECX,ECX; TEST EBX,EBX (the offset's sign).
        case award_definitions:
            gp[eax]=this->flags.logic(0,4);this->test32(v);
            if(!v)this->reached(award_no_roster);
            else {this->test32(gp[ebx]);if(si(gp[ebx])<0)this->reached(award_negative);}
            break;
        case award_count:this->compare32(gp[esi],v);break;                                  // CMP ESI,[ECX+420]
        case award_list:gp[eax]=this->flags.plus(v,gp[ebx],4);break;                       // ADD EAX,EBX
        // ADD EBX,14; ADD ECX,4; INC ESI; CMP EBX,280.
        case cursor_read:
            gp[ebx]=this->flags.plus(gp[ebx],std::uint32_t(sizeof(logic::SlotDefinition)),4);
            gp[ecx]=this->flags.plus(gp[ecx],std::uint32_t(sizeof(logic::Slot)),4);
            gp[esi]=this->flags.increment(gp[esi],4,false);
            this->compare32(gp[ebx],logic::slot_count*std::uint32_t(sizeof(logic::SlotDefinition)));
            break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // Outside a campaign: XOR CL,CL; CMP EAX,-1.
        case engine_record_store:
            if(!campaign){set_low_byte(gp[ecx],0);this->flags.logic(0,1);this->compare32(gp[eax],logic::none);}
            break;
        // ADD EDI,EDX (the record); TEST CL,CL (the first countdown).
        case record_offset_store:gp[edi]=this->flags.plus(gp[edi],gp[edx],4);this->test8(gp[ecx]);break;
        default:break;
        }
    }
    // Frame.
    std::uint32_t local(logic::player_slots::Local l)const {
        switch(l) {
        case logic::player_slots::Local::engine_record:return locals+offsetof(Locals,engine_record);
        case logic::player_slots::Local::slot_cursor:return locals+offsetof(Locals,slot_cursor);
        case logic::player_slots::Local::record_offset:return locals+offsetof(Locals,record_offset);
        }
        return 0;
    }
    // After a call the four register saves are below the locals.
    void after_call(){locals=this->gp[esp]+0x10u;}
    template<class Child> void child(std::uint32_t pc,std::uint32_t next,std::uint32_t target,Child &body) {
        this->call(pc,next,target,body);after_call();
    }
    // The child's AL, tested (TEST AL,AL).
    bool al() {
        this->test8(this->gp[eax]);
        return low_byte(this->gp[eax])!=0;
    }
    // PUSH EAX (the record index); MOV EDX,EBX; 1EFD60. MOV EBP,EBX when accepted.
    bool scan_pick(logic::player_slots::Scan &w,std::uint32_t index) {
        auto &gp=this->gp;
        gp[edi]=w.record;gp[ebx]=w.slot;gp[esi]=w.timer;gp[ebp]=w.current;
        gp[eax]=index;this->push(0x1eefb1u,gp[eax]);gp[edx]=gp[ebx];
        child(0x1eefb4u,0x1eefb9u,0x1efd60u,pick);
        w.record=gp[edi];w.slot=gp[ebx];w.timer=gp[esi];w.current=gp[ebp];
        const bool accepted=al();
        this->reached(accepted?slot_picked:slot_declined);
        if(accepted)gp[ebp]=gp[ebx];
        return accepted;
    }
    // JMP [1EF260+ECX*4].
    Case state_case(std::uint32_t state) {
        const auto target=branch(this->read(0x1ef02fu,0x1ef260u+state*4u,4),this->s.image);
        switch(target) {
        case 0x1ef036u:current_case=Case::idle;break;
        case 0x1ef055u:current_case=Case::starting;break;
        case 0x1ef0c1u:current_case=Case::active;break;
        case 0x1ef114u:current_case=Case::ending;break;
        case 0x1ef187u:current_case=Case::cooling;break;
        default:throw std::runtime_error("Required original solver dispatch target unavailable");
        }
        return current_case;
    }
    // The current slot lives in EDI (record), EBP (slot), ESI (entry), EBX (definition).
    void keep(const logic::player_slots::CurrentSlot &w) {
        auto &gp=this->gp;gp[edi]=w.record;gp[ebp]=w.slot;gp[esi]=w.entry;gp[ebx]=w.definition;
    }
    void take(logic::player_slots::CurrentSlot &w)const {
        const auto &gp=this->gp;w.record=gp[edi];w.slot=gp[ebp];w.entry=gp[esi];w.definition=gp[ebx];
    }
    // MOV EDX,[EBX+C]; PUSH EDX; 1494F0 (dispatch). AX is the timer.
    std::uint32_t start(logic::player_slots::CurrentSlot &w,std::uint32_t value) {
        auto &gp=this->gp;
        keep(w);gp[edx]=value;this->push(0x1ef046u,gp[edx]);
        this->call_routed(dispatch,0x1ef047u,0x1ef04cu,0x1494f0u);after_call();
        take(w);
        return gp[eax];
    }
    // MOV EDX,EBP; 1F0670 (EAX: the record index).
    bool finished(logic::player_slots::CurrentSlot &w,std::uint32_t index) {
        auto &gp=this->gp;
        keep(w);gp[eax]=index;gp[edx]=w.slot;
        if(current_case==Case::starting)child(0x1ef057u,0x1ef05cu,0x1f0670u,test);
        else if(current_case==Case::active)child(0x1ef0c9u,0x1ef0ceu,0x1f0670u,test);
        else child(0x1ef11cu,0x1ef121u,0x1f0670u,test);
        take(w);
        return al();
    }
    // PUSH the record index (as loaded); MOV EDX,EBP; 1EFD60.
    bool wanted(logic::player_slots::CurrentSlot &w,std::uint32_t index) {
        auto &gp=this->gp;
        keep(w);
        if(current_case==Case::starting){this->push(0x1ef07cu,index);gp[edx]=w.slot;child(0x1ef07fu,0x1ef084u,0x1efd60u,pick);}
        else if(current_case==Case::active){this->push(0x1ef0d6u,index);gp[edx]=w.slot;child(0x1ef0d9u,0x1ef0deu,0x1efd60u,pick);}
        else {this->push(0x1ef148u,index);gp[edx]=w.slot;child(0x1ef14bu,0x1ef150u,0x1efd60u,pick);}
        take(w);
        return al();
    }
    // PUSH ESI (entry), EBX (definition), EDI (record), EBP (slot); 1F1030.
    void award(logic::player_slots::CurrentSlot &w) {
        auto &gp=this->gp;
        keep(w);
        const std::uint32_t site=current_case==Case::starting?0x1ef060u:0x1ef125u;
        this->push(site,gp[esi]);this->push(site+1u,gp[ebx]);this->push(site+2u,gp[edi]);this->push(site+3u,gp[ebp]);
        child(site+4u,site+9u,0x1f1030u,award_child);
        take(w);
    }
    // MOVZX ECX,byte [ESI+1EF26D]; JMP [1EF274+ECX*4].
    bool award_case(std::uint32_t slot) {
        auto &gp=this->gp;
        gp[ecx]=this->read(0x1ef1f0u,0x1ef26du+slot,1);
        const auto target=branch(this->read(0x1ef1f7u,0x1ef274u+gp[ecx]*4u,4),this->s.image);
        if(target==0x1ef23au)return false;
        if(target!=0x1ef1feu)throw std::runtime_error("Required original solver dispatch target unavailable");
        return true;
    }
    // The award pass lives in EDI (record), ESI (slot), EBX (definition offset).
    void keep(const logic::player_slots::Award &w){auto &gp=this->gp;gp[edi]=w.record;gp[esi]=w.slot;gp[ebx]=w.definition_offset;}
    void take(logic::player_slots::Award &w)const{const auto &gp=this->gp;w.record=gp[edi];w.slot=gp[esi];w.definition_offset=gp[ebx];}
    // MOV EDX,ESI; 1F0670 (EAX: the record index).
    bool award_finished(logic::player_slots::Award &w,std::uint32_t index) {
        auto &gp=this->gp;
        keep(w);gp[eax]=index;gp[edx]=w.slot;
        child(0x1ef204u,0x1ef209u,0x1f0670u,test);
        take(w);
        return al();
    }
    // PUSH the slot cursor, the definition (or 0), EDI, ESI; 1F1030.
    void award_slot(logic::player_slots::Award &w,std::uint32_t definition,std::uint32_t cursor) {
        auto &gp=this->gp;
        keep(w);gp[eax]=definition;gp[edx]=cursor;
        this->push(0x1ef231u,gp[edx]);this->push(0x1ef232u,gp[eax]);this->push(0x1ef233u,gp[edi]);this->push(0x1ef234u,gp[esi]);
        child(0x1ef235u,0x1ef23au,0x1f1030u,award_child);
        take(w);
    }
    void run() {
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],std::uint32_t(sizeof(Locals)),4);              // SUB ESP,0C
        locals=gp[esp];
        Memory<PlayerUpdate> memory{*this};
        switch(logic::player_slots::update(memory,*this,gp[eax])) {
        case logic::player_slots::Exit::no_record:this->reached(no_player);break;
        case logic::player_slots::Exit::disabled:
            this->reached(low_byte(gp[ebx])?suppressed:engine_off);
            gp[ebx]=this->pop(0x1ef258u);
            break;
        case logic::player_slots::Exit::updated:
            gp[edi]=this->pop(0x1ef255u);gp[esi]=this->pop(0x1ef256u);gp[ebp]=this->pop(0x1ef257u);gp[ebx]=this->pop(0x1ef258u);
            break;
        }
        gp[esp]=this->flags.plus(gp[esp],std::uint32_t(sizeof(Locals)),4);               // ADD ESP,0C
        this->ret(0x1ef25cu,0);this->publish(0x1ef25cu);
    }
};

template<class State,class Reference,class Dispatch,class Branch_,class Pick,class Test,class Award>
bool bridge(State &s,Reference reference,Dispatch dispatch,Branch_ branch,Pick pick,Test test,Award award) {
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1eee80u,0x1ef25cu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return PlayerUpdate<State,Dispatch,Branch_,Pick,Test,Award>(v,c,t,dispatch,branch,pick,test,award);});
    return true;
}
} // namespace halo2::native_engine_player_update_detail
