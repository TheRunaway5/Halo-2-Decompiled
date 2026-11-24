#pragma once
// Boundary adapter for the player directors, entered at 157FE0
// (native_tu15_player_directors_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns:
//   - the seconds at [EBP+8], RET 4; PUSH EBP; MOV EBP,ESP; AND ESP,-8;
//     SUB ESP,EC make the frame (the director offset, the handle offset and
//     the slot at [ESP+10], [ESP+14] and [ESP+18], the request at [ESP+1C]
//     and the state at [ESP+48] once EBX, ESI and EDI are saved); the
//     locals are addressed from the live ESP and the seconds from the live
//     EBP, so a child that leaves the stack unbalanced or changes EBP moves
//     them as it does for the original;
//   - 158790 gets AL the mode change and ECX the slot; 1588B0 (through the
//     unit's route) CL the mode change and EAX the slot; the callback
//     (through the route, at the address the director names) the target,
//     the request and the state, and pops them; 171FF0 ECX the slot; every
//     register a child returns comes back;
//   - XMM0 and XMM1 as the original leaves them; the flags of the last
//     instruction that sets them (UCOMISS's LAHF in AH);
//   - the direction flag clear (a set one is refused at admission);
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_PLAYER_DIRECTORS=1 restores the translated body.
// HALO2_VERIFY_PLAYER_DIRECTORS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/vehicles/native_tu15_player_directors_systems.hpp"
#include <emmintrin.h>
#include <stdexcept>

namespace halo2::native_player_directors_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_player_directors;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PLAYER_DIRECTORS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PLAYER_DIRECTORS");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_player, present, mode_out_of_table, tell_158790, unchanged, tell_1588b0, reset_skipped, reset,
    no_callback, other_callback, first_player_callback, not_first_slot, inactive, timer_zero, quick_end, longest_raised,
    longest_kept, floored, next_director, last_director, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Player directors verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge directors_edges[]{
    {0x158037u,0x1581e7u,no_player},{0x158037u,0x158051u,present},{0x15806eu,0x158097u,mode_out_of_table},
    {0x158076u,0x15807du,tell_158790},{0x158076u,0x158097u,unchanged},{0x158076u,0x1580d0u,tell_1588b0},
    {0x1580dfu,0x158097u,reset_skipped},{0x1580dfu,0x1580ebu,reset},
    {0x1580a0u,0x158132u,no_callback},{0x1580a0u,0x158122u,other_callback},{0x1580a0u,0x1580b2u,first_player_callback},
    {0x1580c0u,0x158132u,not_first_slot},
    {0x158132u,0x1581d5u,inactive},{0x15813du,0x1581cdu,timer_zero},{0x158159u,0x158162u,quick_end},
    {0x158194u,0x15819au,longest_raised},{0x158194u,0x1581a8u,longest_kept},
    {0x15820du,0x158020u,next_director},{0x15820du,0x158217u,last_director},
};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available && native_record_fade::scalar::available;
template<class State,class Route,class Tell,class Update> struct Directors : Port<State> {
    using Access=logic::player_directors::Access;
    using Local=logic::player_directors::Local;
    using Kept=logic::player_directors::Kept;
    Route &route;Tell &tell_child;Update &update_child;
    // The bytes pushed below the aligned frame: it starts at ESP+pushed.
    std::uint32_t pushed{};
    Directors(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Tell &k,Update &u)
        :Port<State>(s,c,t,directors_edges),route(r),tell_child(k),update_child(u){}
    std::uint32_t local(Local l)const {
        const auto frame=this->gp[esp]+pushed;
        switch(l) {
        case Local::seconds:return this->gp[ebp]+8u;
        case Local::director_offset:return frame+4u;
        case Local::handle_offset:return frame+8u;
        case Local::slot:return frame+0xcu;
        case Local::request:return frame+0x10u;
        case Local::state:return frame+0x3cu;
        }
        return 0;
    }
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);pushed+=4u;}
    std::uint32_t director()const{return this->gp[ebx]-logic::director_cursor;}
    std::uint32_t instruction(Access a,std::uint32_t address) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case seconds_argument:return 0x157fecu;
        case seconds_store:return 0x157ff6u;
        case slot_init:return 0x157ffeu;
        case handle_offset_init:return 0x158002u;
        case offset_init:gp[ebx]=logic::globals::directors+logic::director_cursor;return 0x15800fu;   // MOV EBX,4F556A
        case director_offset:return 0x158020u;
        case players:return 0x15802eu;
        case handle_offset:return 0x158033u;
        case player_handle:return 0x158037u;
        // XOR EAX,EAX; MOV ECX,B; LEA EDI,[ESP+1C]; REP STOSD.
        case request_clear:
            if(address==local(Local::request)){gp[eax]=this->flags.logic(0u,4);gp[ecx]=logic::request_words;gp[edi]=address;}
            return 0x158051u;
        case flag_a_clear:return 0x158053u;
        case flag_b_clear:return 0x158057u;
        case mode:return 0x15805au;
        case seconds_global:return 0x158062u;
        case request_slot:return 0x15806au;
        case request_seconds:return 0x15806eu;
        case mode_case:return 0x158076u;
        case mode_changed_for_158790:return 0x15807du;
        case mode_changed_for_1588b0:return 0x1580d0u;
        case mode_changed_for_reset:return 0x1580dfu;
        case one:x[1]=_mm_setzero_ps();return 0x1580ebu;                                           // XORPS XMM1
        case target_clear:return 0x1580f3u;
        case target_second_clear:return 0x1580f8u;
        case callback_store:return 0x1580fdu;
        case fade_store:return 0x158104u;
        case fade_flag_clear:return 0x15810cu;
        case mode_changed_clear:return 0x158097u;
        case state_clear:return 0x15809eu;
        case callback:return 0x1580a0u;
        case first_players:return 0x1580b2u;
        case first_probe:return 0x1580c0u;
        // LEA EDX,[ESP+48]; PUSH EDX; LEA EAX,[ESP+20]; PUSH EAX; LEA ECX,[EBX-46]; PUSH ECX.
        case callback_call:
            gp[edx]=local(Local::state);save(0x158122u,gp[edx]);gp[eax]=local(Local::request);save(0x158127u,gp[eax]);
            gp[ecx]=director()+logic::director::target;save(0x15812bu,gp[ecx]);return 0x15812cu;
        case state_flags:return 0x158132u;
        case timer:return 0x15813du;
        case quick_end:return 0x15814bu;
        case timer_quick:return 0x158153u;
        case callback_again:return 0x158159u;
        case timer_end:return 0x158162u;
        case first_fade_end:return 0x158167u;
        case first_fade_kind:return 0x158170u;
        case second_fade_end:return 0x158178u;
        case second_fade_kind:return 0x158181u;
        case longest:return 0x15818bu;
        case timer_longest:return 0x158194u;
        case timer_kept:return 0x15819au;
        case longest_store:return 0x15819fu;
        case timer_now:return 0x1581a8u;
        case seconds_again:return 0x1581adu;
        case timer_store:return 0x1581bau;
        // LEA EAX,[EBX+6]; MOV ECX,2B; LEA ESI,[ESP+48]; MOV EDI,EAX; REP MOVSD.
        case state_word:case state_word_store:
            if(a==state_word && address==local(Local::state)) {
                gp[eax]=director()+logic::director::state;gp[ecx]=logic::state_words;gp[esi]=address;gp[edi]=gp[eax];
            }
            return 0x1581cdu;
        case slot_reload:return 0x1581cfu;
        case inactive_read:return 0x1581d5u;
        case inactive_store:return 0x1581deu;
        case offset_read:return 0x1581e7u;
        case handle_offset_read:return 0x1581ebu;
        case slot_store:return 0x158205u;
        case offset_store:return 0x158209u;
        case handle_offset_store:return 0x15820du;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        // PUSH EBX; PUSH ESI; XOR ESI,ESI; PUSH EDI.
        case seconds_argument:
            x[0]=lane0_only(v);save(0x157ff1u,gp[ebx]);save(0x157ff2u,gp[esi]);gp[esi]=this->flags.logic(0u,4);
            save(0x157ff5u,gp[edi]);break;
        case director_offset:this->compare32(v,logic::null_director);break;
        case players:gp[eax]=v;break;
        case handle_offset:gp[ecx]=v;break;
        // CMP [ECX+EAX],-1; SETNE AL; TEST AL,AL.
        case player_handle:this->compare32(v,logic::none);set_low_byte(gp[eax],v!=logic::none?1u:0u);this->test8(gp[eax]);break;
        case mode:gp[eax]=v;this->compare32(v,4u);break;                                          // CMP EAX,4
        case seconds_global:case one:case quick_end:case longest:case timer_kept:case timer_now:x[0]=lane0_only(v);break;
        case mode_changed_for_158790:set_low_byte(gp[eax],v);gp[ecx]=gp[esi];break;               // MOV ECX,ESI
        case mode_changed_for_1588b0:set_low_byte(gp[ecx],v);gp[eax]=gp[esi];break;               // MOV EAX,ESI
        case mode_changed_for_reset:set_low_byte(gp[eax],v);this->test8(v);break;
        // TEST EAX,EAX; CMP EAX,16F6A0.
        case callback:
            gp[eax]=v;this->test32(v);if(v)this->compare32(v,logic::first_player_callback);break;
        // OR EDX,-1; XOR EAX,EAX; ADD ECX,C.
        case first_players:
            gp[ecx]=v;gp[edx]=this->flags.logic(gp[edx]|logic::none,4);gp[eax]=this->flags.logic(0u,4);
            gp[ecx]=this->flags.plus(gp[ecx],logic::local_handles,4);break;
        // CMP [ECX],-1; (INC EAX; ADD ECX,4; CMP EAX,4).
        case first_probe:
            this->compare32(v,logic::none);
            if(v==logic::none) {
                gp[eax]=this->flags.increment(gp[eax],4,false);gp[ecx]=this->flags.plus(gp[ecx],4u,4);
                this->compare32(gp[eax],logic::slot_count);
            }
            break;
        case state_flags:this->test8(v&1u);break;
        // MOVSS XMM0; UCOMISS XMM0,XMM1; LAHF; TEST AH,44.
        case timer: {
            x[0]=lane0_only(v);this->flags.scalar_compare(x[0],x[1],true);
            const auto ah=(this->flags.bits()|2u)&0xffu;
            gp[eax]=(gp[eax]&0xffff00ffu)|(ah<<8);this->test8(ah&0x44u);break;
        }
        case timer_quick:case timer_longest:this->flags.scalar_compare(x[0],lane0_only(v),false);break;   // COMISS XMM0,m32
        case callback_again:this->compare32(v,logic::default_callback);break;
        case slot_reload:gp[esi]=v;break;
        // LEA EAX,[EBX+6]; AND ECX,-2.
        case inactive_read:gp[ecx]=v;gp[eax]=director()+logic::director::state;gp[ecx]=this->flags.logic(v&~1u,4);break;
        case offset_read:gp[edi]=v;break;
        // INC ESI; ADD EDI,140; ADD EDX,4; ADD EBX,140; CMP EBX,4F5A6A.
        case handle_offset_read:
            gp[edx]=v;gp[esi]=this->flags.increment(gp[esi],4,false);gp[edi]=this->flags.plus(gp[edi],logic::director_size,4);
            gp[edx]=this->flags.plus(gp[edx],4u,4);gp[ebx]=this->flags.plus(gp[ebx],logic::director_size,4);
            this->compare32(gp[ebx],logic::globals::directors+logic::director_cursor+logic::slot_count*logic::director_size);
            break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case request_clear:case state_clear:gp[edi]+=4u;--gp[ecx];break;
        case state_word_store:gp[esi]+=4u;gp[edi]+=4u;--gp[ecx];break;
        default:break;
        }
    }
    void returned(Kept &kept)const{kept={this->gp[ebx],this->gp[esi]};}
    void tell_158790(std::uint32_t,Kept &kept){this->call(0x158084u,0x158089u,0x158790u,tell_child);returned(kept);}
    void tell_1588b0(std::uint32_t,Kept &kept){this->call_routed(route,0x1580d8u,0x1580ddu,0x1588b0u);returned(kept);}
    [[noreturn]] void unknown_case(){throw std::runtime_error("Unbound TU1.5 contact dispatch target");}
    // (XORPS XMM1;) XOR EAX,EAX; MOV ECX,2B; LEA EDI,[ESP+48].
    void cases_done(bool reset) {
        auto &gp=this->gp;
        if(!reset)this->s.xmm[1]=_mm_setzero_ps();
        gp[eax]=this->flags.logic(0u,4);gp[ecx]=logic::state_words;gp[edi]=local(Local::state);
    }
    // (MOV EDX,EAX;) CMP ESI,EDX.
    void first_slot_found(std::uint32_t found) {
        if(found!=logic::none)this->gp[edx]=this->gp[eax];
        this->compare32(this->gp[esi],this->gp[edx]);
    }
    void run_callback(std::uint32_t entry,std::uint32_t,Kept &kept) {
        this->call_routed(route,0x15812cu,0x15812fu,entry);pushed-=12u;
        this->s.xmm[1]=_mm_setzero_ps();                                                          // XORPS XMM1
        returned(kept);
    }
    // COMISS XMM1,XMM0; (MOVAPS XMM0,XMM1).
    void timer_left(std::uint32_t left) {
        auto &x=this->s.xmm;
        x[0]=lane0_only(left);this->flags.scalar_compare(x[1],x[0],false);
        if(!logic::sse::not_above(0u,left)){x[0]=x[1];this->reached(Branch::floored);}
    }
    // MOV ECX,ESI; CALL 171FF0.
    void update_slot(Kept &kept){this->gp[ecx]=this->gp[esi];this->call(0x1581e2u,0x1581e7u,0x171ff0u,update_child);returned(kept);}
    void run() {
        auto &gp=this->gp;
        // PUSH EBP; MOV EBP,ESP; AND ESP,-8; SUB ESP,EC.
        this->push(0x157fe0u,gp[ebp]);gp[ebp]=gp[esp];
        gp[esp]=this->flags.minus(this->flags.logic(gp[esp]&0xfffffff8u,4),0xecu,4);
        Memory<Directors> memory{*this};
        logic::player_directors::update(memory,*this);
        gp[edi]=this->pop(0x158217u);gp[esi]=this->pop(0x158218u);gp[ebx]=this->pop(0x158219u);
        gp[esp]=gp[ebp];gp[ebp]=this->pop(0x15821cu);                                             // MOV ESP,EBP; POP EBP
        this->ret(0x15821du,4u);this->publish(0x15821du);
    }
};
template<class State,class Reference,class Route,class Tell,class Update>
bool bridge(State &s,Reference reference,Route route,Tell tell,Update update) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.other_flags&0x400u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x157fe0u,0x15821du,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Directors<State,Route,Tell,Update>(v,c,t,route,tell,update);});
    return true;
}
} // namespace halo2::native_player_directors_detail
