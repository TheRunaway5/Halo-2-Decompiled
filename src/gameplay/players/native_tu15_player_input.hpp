#pragma once
// Boundary adapter for the player input, entered at 1EEDB0
// (native_tu15_player_input_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: EAX the player and the input at
// [ESP+4], RET 4; EBP and ESI saved (EBP the input), EBX and EDI once the
// player has a local slot (EBX the record's current input); the copies run
// as the original's REP MOVSD (the direction flag clear: a set one is
// refused at admission); 1F0F90 gets EAX the record's state and ECX the
// input, and every register it returns comes back; the flags of the last
// instruction that sets them; each access publishes its original
// instruction.
// HALO2_REFERENCE_PLAYER_INPUT=1 restores the translated body.
// HALO2_VERIFY_PLAYER_INPUT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/players/native_tu15_player_input_systems.hpp"

namespace halo2::native_player_input_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_player_input;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PLAYER_INPUT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PLAYER_INPUT");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_slot, no_unit, pending, no_choice, same_choice, changed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Player input verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge input_edges[]{
    {0x1eedcau,0x1eee6eu,no_slot},{0x1eee0au,0x1eee67u,no_unit},{0x1eee27u,0x1eee67u,pending},
    {0x1eee31u,0x1eee67u,no_choice},{0x1eee39u,0x1eee67u,same_choice},{0x1eee39u,0x1eee41u,changed},
};
template<class State,class Update> struct Input : Port<State> {
    using Access=logic::player_input::Access;
    using Local=logic::player_input::Local;
    Update &update_child;
    std::uint32_t depth{};
    Input(State &s,std::uint32_t *c,Trace<State> *t,Update &u):Port<State>(s,c,t,input_edges),update_child(u){}
    std::uint32_t local(Local)const{return this->gp[esp]+depth+4u;}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case players:return 0x1eedb0u;
        case input_argument:save(0x1eedc1u,gp[ebp]);return 0x1eedc2u;                              // PUSH EBP
        case player_data:return 0x1eedc7u;
        case local_slot:return 0x1eedcau;
        case locals:return 0x1eeddau;
        case previous_read:case previous_store:return 0x1eedfdu;
        case input_read:case input_store:return 0x1eee08u;
        case unit:return 0x1eee0au;
        case object_table:return 0x1eee12u;
        case object_datums:return 0x1eee18u;
        case object_address:gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;return 0x1eee23u;   // AND; LEA
        case pending:return 0x1eee27u;
        case choice:return 0x1eee31u;
        case unit_choice:return 0x1eee39u;
        case changes:return 0x1eee41u;
        case game_time:return 0x1eee47u;
        case changes_store:return 0x1eee4eu;
        case ticks:return 0x1eee54u;
        case timer_store:return 0x1eee59u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // AND EAX,FFFF; IMUL EAX,EAX,21C.
        case players:gp[ecx]=v;gp[eax]=this->flags.product(this->flags.logic(gp[eax]&0xffffu,4),logic::player_size);break;
        case input_argument:gp[ebp]=v;save(0x1eedc6u,gp[esi]);break;                              // PUSH ESI
        case player_data:gp[esi]=v;break;
        // ADD EAX,ESI; CMP ECX,-1.
        case local_slot:
            gp[ecx]=logic::widen16(v);gp[eax]=this->flags.plus(gp[eax],gp[esi],4);this->compare32(gp[ecx],logic::none);
            if(gp[ecx]==logic::none)this->reached(Branch::no_slot);
            break;
        // IMUL ECX,ECX,1B0; ADD EDX,ECX; PUSH EBX; PUSH EDI.
        case locals:
            gp[ecx]=this->flags.product(gp[ecx],logic::local_size);gp[edx]=this->flags.plus(v,gp[ecx],4);
            save(0x1eede8u,gp[ebx]);save(0x1eede9u,gp[edi]);break;
        case unit:gp[eax]=v;this->compare32(v,logic::none);break;
        case object_table:case object_datums:gp[ecx]=v;break;
        case object_address:gp[eax]=v;break;
        case pending:this->flags.minus(v&0xffffu,0xffffu,2);break;
        case choice:set_low_byte(gp[ecx],v);this->flags.minus(v&0xffu,logic::no_choice,1);break;   // CMP CL,FF
        case unit_choice:this->flags.minus(gp[ecx]&0xffu,v&0xffu,1);break;                       // CMP CL,byte
        case changes:set_low_byte(gp[ecx],v);break;
        case game_time:gp[eax]=v;set_low_byte(gp[ecx],this->flags.increment(gp[ecx]&0xffu,1,false));break;   // INC CL
        case ticks:set_low_byte(gp[ecx],this->flags.shift(v&0xffu,1u,1));break;                  // SHL CL,1
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        using enum Access;
        if(a==previous_store || a==input_store){this->gp[esi]+=4u;this->gp[edi]+=4u;--this->gp[ecx];}
    }
    // LEA EBX,[EDX+88]; LEA EDI,[EDX+E4]; MOV ECX,17; MOV ESI,EBX.
    void copying() {
        auto &gp=this->gp;
        gp[ebx]=gp[edx]+logic::record::input;gp[edi]=gp[edx]+logic::record::previous;gp[ecx]=logic::input_words;gp[esi]=gp[ebx];
    }
    // MOV ECX,17; MOV ESI,EBP; MOV EDI,EBX.
    void copied(){auto &gp=this->gp;gp[ecx]=logic::input_words;gp[esi]=gp[ebp];gp[edi]=gp[ebx];}
    // LEA EAX,[EDX+140]; MOV ECX,EBP; CALL 1F0F90.
    void update() {
        auto &gp=this->gp;
        gp[eax]=gp[edx]+logic::record::state;gp[ecx]=gp[ebp];this->call(0x1eee67u,0x1eee6cu,0x1f0f90u,update_child);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Input> memory{*this};
        if(logic::player_input::accept(memory,*this,gp[eax])){gp[edi]=this->pop(0x1eee6cu);gp[ebx]=this->pop(0x1eee6du);}
        gp[esi]=this->pop(0x1eee6eu);gp[ebp]=this->pop(0x1eee6fu);this->ret(0x1eee70u,4u);this->publish(0x1eee70u);
    }
};
template<class State,class Reference,class Update> bool bridge(State &s,Reference reference,Update update) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.other_flags&0x400u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1eedb0u,0x1eee70u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Input<State,Update>(v,c,t,update);});
    return true;
}
} // namespace halo2::native_player_input_detail
