#pragma once
// Boundary adapter for the unit countdowns, entered at C8C20
// (native_tu15_unit_countdowns_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the unit handle; ESI
// and EDI saved; 1D1F30 gets EAX the handle, EDX the pending handle and the
// pending word, float, 0 and 1 pushed (and pops them); D2500 is reached
// through the unit's dispatch; every register each returns comes back; the
// result in AL (MOV AL,1 or MOV AL,DL, DL set once a word countdown ticks)
// above the rest of EAX; ECX (a countdown word) and EDX as the original
// leaves them; XMM0 zero once a value is cleared; the flags of the last
// instruction that sets them; RET. Each access publishes its original
// instruction.
// HALO2_REFERENCE_UNIT_COUNTDOWNS=1 restores the translated body.
// HALO2_VERIFY_UNIT_COUNTDOWNS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_unit_countdowns_systems.hpp"

namespace halo2::native_unit_countdowns_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_unit_countdowns;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNIT_COUNTDOWNS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNIT_COUNTDOWNS");return p && *p;}();
    return value;
}
enum Branch : unsigned {idle, first_expired, second_expired, counted, finished, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Unit countdowns verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class HandOver,class Finish> struct UnitCountdowns : Port<State> {
    using Access=logic::unit_countdowns::Access;
    using Kept=logic::unit_countdowns::Kept;
    HandOver &hand_over_child;
    Finish &finish_route;
    UnitCountdowns(State &s,std::uint32_t *c,Trace<State> *t,HandOver &h,Finish &f)
        :Port<State>(s,c,t,{}),hand_over_child(h),finish_route(f){}
    void decrement_word(){auto &gp=this->gp;gp[eax]=gp[ecx]-1u;this->flags.logic(gp[eax]&0xffffu,2);}   // LEA EAX,[ECX-1]; TEST AX,AX
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:return 0xc8c20u;
        case header_datums:return 0xc8c26u;
        case unit_address:return 0xc8c35u;
        case first:return 0xc8c3bu;
        case first_store:decrement_word();return 0xc8c4fu;
        case first_clear:this->s.xmm[0]=_mm_setzero_ps();this->reached(Branch::first_expired);return 0xc8c5bu;   // XORPS XMM0,XMM0
        case second:gp[ecx]=fl.logic(0u,4);return 0xc8c67u;                                        // XOR ECX,ECX
        case second_store:decrement_word();return 0xc8c79u;
        case pending_value:return 0xc8c82u;
        case pending_handle:return 0xc8c88u;
        case pending_word:gp[ecx]=fl.logic(0u,4);return 0xc8c90u;                                  // XOR ECX,ECX
        case word_clear:this->s.xmm[0]=_mm_setzero_ps();return 0xc8ca7u;                           // XORPS XMM0,XMM0
        case handle_clear:return 0xc8cb0u;
        case value_clear:return 0xc8cbau;
        case countdown:return 0xc8cc4u;
        case countdown_store:set_low_byte(gp[eax],fl.increment(gp[eax]&0xffu,1,true));return 0xc8cd0u;   // DEC AL
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; PUSH ESI; PUSH EDI; MOV EDI,EAX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case header_datums:gp[edx]=v;this->push(0xc8c29u,gp[esi]);this->push(0xc8c2au,gp[edi]);gp[edi]=gp[eax];
            gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;
        case unit_address:gp[esi]=v;gp[ecx]=fl.logic(0u,4);break;                                  // MOV ESI; XOR ECX,ECX
        // MOV CX,[ESI+2E8]; XOR DL,DL; TEST CX,CX.
        case first:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);set_low_byte(gp[edx],fl.logic(0u,1));fl.logic(v&0xffffu,2);break;
        case second:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);fl.logic(v&0xffffu,2);break;      // MOV CX; TEST CX,CX
        case pending_value:gp[eax]=v;break;
        case pending_handle:gp[edx]=v;break;
        case pending_word:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);break;
        case countdown:set_low_byte(gp[eax],v);this->test8(v);break;                               // MOV AL; TEST AL,AL
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void counted(){set_low_byte(this->gp[edx],1u);this->reached(Branch::counted);}               // MOV DL,1
    // PUSH 1; PUSH 0; PUSH EAX; MOV EAX,EDI; PUSH ECX; CALL 1D1F30.
    void hand_over(Kept &kept,std::uint32_t word,std::uint32_t value,std::uint32_t target) {
        auto &gp=this->gp;
        gp[edx]=target;this->push(0xc8c97u,1u);this->push(0xc8c99u,0u);this->push(0xc8c9bu,value);
        gp[eax]=kept.handle;this->push(0xc8c9eu,word);
        this->call(0xc8c9fu,0xc8ca4u,0x1d1f30u,hand_over_child);
        kept={gp[esi],gp[edi]};this->reached(Branch::second_expired);
    }
    // CALL D2500 (dispatched).
    void finish(Kept &kept) {
        auto &gp=this->gp;
        this->call_routed(finish_route,0xc8cd8u,0xc8cddu,0xd2500u);
        kept={gp[esi],gp[edi]};this->reached(Branch::finished);
    }
    void run() {
        auto &gp=this->gp;
        Memory<UnitCountdowns> memory{*this};
        const auto result=logic::unit_countdowns::tick(memory,*this,gp[eax]);
        if(!result.counted_down && !result.ticked)this->reached(Branch::idle);
        if(result.counted_down) {
            gp[edi]=this->pop(0xc8cddu);set_low_byte(gp[eax],1u);gp[esi]=this->pop(0xc8ce0u);          // POP EDI; MOV AL,1; POP ESI
            this->ret(0xc8ce1u,0);this->publish(0xc8ce1u);return;
        }
        gp[edi]=this->pop(0xc8ce2u);set_low_byte(gp[eax],result.ticked?1u:0u);gp[esi]=this->pop(0xc8ce5u);   // MOV AL,DL
        this->ret(0xc8ce6u,0);this->publish(0xc8ce6u);
    }
};
template<class State,class Reference,class HandOver,class Finish> bool bridge(State &s,Reference reference,HandOver hand_over,Finish finish) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc8c20u,0xc8ce6u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return UnitCountdowns<State,HandOver,Finish>(v,c,t,hand_over,finish);});
    return true;
}
} // namespace halo2::native_unit_countdowns_detail
