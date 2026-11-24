#pragma once
// Boundary adapter for the player mark counters, entered at 165860
// (native_tu15_player_marks_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns:
//   - no arguments, RET; SUB ESP,18 makes the frame (the slots processed at
//     [ESP+14], the players counted at [ESP+10] and the iterator at
//     [ESP+18] once ESI, EDI, EBX and EBP are saved); the locals are
//     addressed from the live ESP, so a child that leaves the stack
//     unbalanced moves them as it does for the original;
//   - 1A8E50 (through the unit's route) gets ESI the iterator; 53780 gets
//     ESI the other player's index and EDI the slot player's; every
//     register a child returns comes back;
//   - the flags of the last instruction that sets them (ADD ESP,18 at the
//     exit);
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_PLAYER_MARKS=1 restores the translated body.
// HALO2_VERIFY_PLAYER_MARKS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/multiplayer/native_tu15_player_marks_systems.hpp"

namespace halo2::native_player_marks_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_player_marks;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PLAYER_MARKS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PLAYER_MARKS");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_players, first_slot, walked, nothing_to_walk, own_player, other_player, word_set, not_applying, not_ready,
    refused, disabled, enabled, unmarked, marked, walk_end, counter_reset, next_slot, last_slot, slot_cleared,
    branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Player marks verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge marks_edges[]{
    {0x165880u,0x165a01u,no_players},{0x165880u,0x1658a8u,first_slot},
    {0x1658a8u,0x1658b4u,walked},
    {0x1658d1u,0x165996u,nothing_to_walk},{0x1658d1u,0x1658e0u,other_player},{0x165983u,0x1658e0u,other_player},
    {0x1658e0u,0x165983u,own_player},{0x1658f0u,0x165983u,word_set},{0x1658fbu,0x165983u,not_applying},
    {0x165912u,0x165983u,not_ready},{0x16591bu,0x165983u,refused},{0x165924u,0x165983u,disabled},
    {0x165924u,0x165930u,enabled},{0x16593du,0x165983u,unmarked},{0x16593du,0x16595fu,marked},
    {0x165983u,0x165990u,walk_end},{0x165996u,0x1659b2u,counter_reset},
    {0x1659c9u,0x1659e6u,next_slot},{0x1659c9u,0x165a01u,last_slot},
    {0x165a01u,0x165a1fu,slot_cleared},{0x165a33u,0x165a1fu,slot_cleared},
};
template<class State,class Route,class Allowed> struct Marks : Port<State> {
    using Access=logic::player_marks::Access;
    using Local=logic::player_marks::Local;
    using Kept=logic::player_marks::Kept;
    Route &route;Allowed &allowed_child;
    // The frame's bytes below the return address: it sits at ESP+depth.
    std::uint32_t depth{};
    Marks(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Allowed &a)
        :Port<State>(s,c,t,marks_edges),route(r),allowed_child(a){}
    std::uint32_t local(Local l)const {
        const auto entry=this->gp[esp]+depth;
        switch(l) {
        case Local::processed:return entry-0x14u;
        case Local::counted:return entry-0x18u;
        case Local::iterator:return entry-0x10u;
        }
        return 0;
    }
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t address) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // SUB ESP,18; PUSH ESI; PUSH EDI.
        case players:
            gp[esp]=this->flags.minus(gp[esp],0x18u,4);depth=0x18u;save(0x165863u,gp[esi]);save(0x165864u,gp[edi]);
            return 0x165865u;
        case processed_init:return 0x165871u;
        case first_probe:return 0x165880u;
        // CMP EBP,ESI (EBP the slot found); MOV EBX,[EDI+EBP*4+C].
        case slot_player:gp[ebp]=gp[edx];this->compare32(gp[ebp],gp[esi]);return 0x1658a8u;
        case player_array:return 0x1658b4u;
        case iterator_index:return 0x1658b9u;
        case iterator_handle:return 0x1658bdu;
        case counted_init:gp[esi]=local(Local::iterator);return 0x1658c5u;                      // LEA ESI,[ESP+18]
        case iterator_array:return 0x1658cdu;
        case other_handle:return 0x1658e0u;
        case other_record:return 0x1658ecu;
        case other_word:return 0x1658f0u;
        case marks_apply:return 0x1658fbu;
        case marks_ready:return 0x165912u;
        case marks_enabled:return 0x165924u;
        case first_marks:return 0x165930u;
        // XOR EAX,EAX; TEST DL,DL.
        case second_marks:gp[eax]=0;this->test8(gp[edx]);return 0x16593du;
        // MOV EAX,EBP; SHL EAX,4; ADD EAX,ESI.
        case counter_read:case counter_store:
            if(a==counter_read)gp[eax]=this->flags.plus(this->flags.shift(gp[ebp],4u,4),gp[esi],4);
            return 0x16595fu;
        case counted_read:return 0x16596eu;
        case counted_store:return 0x16597bu;
        case players_again:return 0x165990u;
        case counted_final:return 0x165996u;
        case counter_reset:return 0x1659b2u;
        case processed_read:case processed_store:return 0x1659c9u;
        case next_probe:return 0x1659e6u;
        // CMP EDX,ESI; MOV EBP,EDX.
        case processed_final:this->compare32(gp[edx],gp[esi]);gp[ebp]=gp[edx];return 0x165a01u;
        case counter_clear: {
            static constexpr std::uint32_t pcs[]{0x165a1fu,0x165a21u,0x165a24u,0x165a27u,0x165a2au,0x165a2du,0x165a30u,0x165a33u};
            return pcs[((address-logic::globals::counters)&0x1fu)>>2];
        }
        }
        return 0;
    }
    // CMP [ECX],ESI; (INC EAX; ADD ECX,4; CMP EAX,4).
    void probed(std::uint32_t v) {
        auto &gp=this->gp;
        this->compare32(v,gp[esi]);
        if(v==gp[esi]) {
            gp[eax]=this->flags.increment(gp[eax],4,false);gp[ecx]=this->flags.plus(gp[ecx],4u,4);
            this->compare32(gp[eax],logic::slot_count);
        }
    }
    // MOV ESI,1; SHL ESI,CL; TEST value,ESI.
    void bit_test(std::uint32_t value) {
        auto &gp=this->gp;
        gp[esi]=this->flags.shift(1u,gp[ecx]&0xffu,4);this->test32(value&gp[esi]);
    }
    void loaded(Access a,std::uint32_t address,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // XOR EAX,EAX; OR ESI,-1; PUSH EBX.
        case players:gp[edi]=v;gp[eax]=0;gp[esi]=this->flags.logic(gp[esi]|logic::none,4);save(0x165870u,gp[ebx]);break;
        case first_probe:case next_probe:probed(v);break;
        // CMP EBX,ESI.
        case slot_player:gp[ebx]=v;this->compare32(v,gp[esi]);break;
        case player_array:gp[eax]=v;break;
        // CMP ESI,EBX.
        case other_handle:gp[esi]=v;this->compare32(v,gp[ebx]);break;
        case other_record:gp[ecx]=v;break;
        case other_word:this->flags.minus(v&0xffffu,0xffffu,2);break;
        // MOV EDI,EBX; AND ESI,FFFF; AND EDI,FFFF; TEST AL,AL.
        case marks_apply:
            set_low_byte(gp[eax],v);gp[edi]=gp[ebx];gp[esi]=this->flags.logic(gp[esi]&0xffffu,4);
            gp[edi]=this->flags.logic(gp[edi]&0xffffu,4);this->test8(v);break;
        case marks_ready:set_low_byte(gp[eax],v);this->test8(v);break;
        // XOR ECX,ECX; TEST DL,DL.
        case marks_enabled:set_low_byte(gp[edx],v);gp[ecx]=0;this->test8(v);break;
        case first_marks:gp[ecx]=v;break;
        case second_marks:gp[eax]=v;break;
        case counter_read:this->flags.increment(v,2,false);break;
        // LEA EAX,[4F6408+EAX*2]; MOV EAX,[ESP+10]; MOV EDX,1; SHL EDX,CL; OR EAX,EDX.
        case counted_read:gp[eax]=this->flags.logic(v|this->flags.shift(1u,gp[ecx]&0xffu,4),4);gp[edx]=1u<<(gp[ecx]&31u);break;
        case players_again:gp[edi]=v;break;
        // MOVZX EAX; XOR EDX,EDX; XOR ECX,ECX.
        case counted_final:gp[eax]=v&0xffffu;gp[edx]=0;gp[ecx]=this->flags.logic(0u,4);break;
        // MOV ECX,EBP; MOV EAX,1; SHL EAX,CL; OR [ESP+14],EAX (the reset loop's registers are overwritten).
        case processed_read:gp[ecx]=gp[ebp];gp[eax]=this->flags.shift(1u,gp[ecx]&0xffu,4);this->flags.logic(v|gp[eax],4);break;
        // POP EBP; XOR ECX,ECX; MOV EAX,4F6408; POP EBX.
        case processed_final:
            gp[edx]=v&0xffffu;gp[ebp]=this->pop(0x165a06u);gp[ecx]=this->flags.logic(0u,4);
            gp[eax]=logic::globals::counters;gp[ebx]=this->pop(0x165a0eu);break;
        default:break;
        }
        (void)address;
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV EDX,ESI; LEA ECX,[EDI+C]; PUSH EBP.
        case processed_init:gp[edx]=gp[esi];gp[ecx]=gp[edi]+logic::local_handles;save(0x16587au,gp[ebp]);break;
        // OR ESI,-1.
        case processed_store:gp[esi]=this->flags.logic(gp[esi]|logic::none,4);break;
        default:break;
        }
    }
    void returned(Kept &kept)const {
        kept={this->gp[edi],this->gp[edi],this->gp[ebp],this->gp[ebx],this->gp[esi]};
    }
    std::uint32_t player(Kept &kept,std::uint32_t pc) {
        this->call_routed(route,pc,pc+5u,0x1a8e50u);
        returned(kept);this->test8(this->gp[eax]);
        return this->gp[eax];
    }
    std::uint32_t first_player(Kept &kept){return player(kept,0x1658d1u);}
    // LEA ESI,[ESP+18].
    std::uint32_t next_player(Kept &kept){this->gp[esi]=local(Local::iterator);return player(kept,0x165983u);}
    std::uint32_t marks_allowed(Kept &kept) {
        this->call(0x16591bu,0x165920u,0x53780u,allowed_child);
        returned(kept);this->test8(this->gp[eax]);
        return this->gp[eax];
    }
    // (XOR EAX,EAX;) OR EAX,ECX; MOV EDX,1; MOV ECX,ESI; SHL EDX,CL; TEST EDX,EAX; SETNE AL; TEST AL,AL.
    void marks_tested(bool enabled) {
        auto &gp=this->gp;
        if(!enabled)gp[eax]=0;
        gp[eax]=this->flags.logic(gp[eax]|gp[ecx],4);gp[ecx]=gp[esi];
        gp[edx]=this->flags.shift(1u,gp[ecx]&0xffu,4);
        this->test32(gp[edx]&gp[eax]);set_low_byte(gp[eax],(gp[edx]&gp[eax])!=0?1u:0u);this->test8(gp[eax]);
    }
    // CMP EBP,ESI; MOV EDX,ESI; XOR EAX,EAX or LEA EAX,[EBP+1]; CMP EAX,4; LEA ECX,[EDI+EAX*4+C].
    void next_search(std::uint32_t start) {
        auto &gp=this->gp;
        this->compare32(gp[ebp],gp[esi]);gp[edx]=gp[esi];
        if(gp[ebp]==gp[esi])gp[eax]=this->flags.logic(0u,4);else gp[eax]=gp[ebp]+1u;
        this->compare32(gp[eax],logic::slot_count);
        if(std::int32_t(start)<std::int32_t(logic::slot_count))gp[ecx]=gp[edi]+logic::local_handles+gp[eax]*4u;
    }
    void found(std::uint32_t){this->gp[edx]=this->gp[eax];}                                    // MOV EDX,EAX
    void run() {
        auto &gp=this->gp;
        Memory<Marks> memory{*this};
        logic::player_marks::update(memory,*this);
        // The clearing loop leaves ECX at 4 and EAX past the counters.
        gp[ecx]=logic::slot_count;gp[eax]=logic::globals::counters+logic::slot_count*2u*logic::counters_per_slot;
        gp[edi]=this->pop(0x165a3fu);gp[esi]=this->pop(0x165a40u);
        gp[esp]=this->flags.plus(gp[esp],0x18u,4);
        this->ret(0x165a44u,0);this->publish(0x165a44u);
    }
};
template<class State,class Reference,class Route,class Allowed>
bool bridge(State &s,Reference reference,Route route,Allowed allowed) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x165860u,0x165a44u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Marks<State,Route,Allowed>(v,c,t,route,allowed);});
    return true;
}
} // namespace halo2::native_player_marks_detail
