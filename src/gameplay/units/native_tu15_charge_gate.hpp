#pragma once
// Boundary adapter for the charge gate, entered at 105FD0
// (native_tu15_charge_gate_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the object handle; ESI saved around the
// count; the result in AL (MOV AL,1; XOR AL,AL) above the tag index scaled;
// ECX the object, EDX the definition or its entries; XMM0 the charge, its
// upper lanes zero; the flags of the last instruction that sets them; RET.
// Each access publishes its original instruction.
// HALO2_REFERENCE_CHARGE_GATE=1 restores the translated body.
// HALO2_VERIFY_CHARGE_GATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_charge_gate_systems.hpp"

namespace halo2::native_charge_gate_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_charge_gate;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CHARGE_GATE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CHARGE_GATE");return p && *p;}();
    return value;
}
enum Branch : unsigned {charged, no_entries, empty_entry, second_set, first_set, closed, branch_count};
static_assert(unsigned(logic::charge_gate::Outcome::closed)==Branch::closed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Charge gate verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct ChargeGate : Port<State> {
    using Access=logic::charge_gate::Access;
    std::uint32_t charge_value{};
    ChargeGate(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x105fd0u;
        case header_datums:return 0x105fd6u;
        case object_address:return 0x105fe1u;
        case tag:return 0x105fe5u;
        case tags:return 0x105fe7u;
        case charge:return 0x105fedu;
        case one:return 0x105ffdu;
        case definition:return 0x106004u;
        case count:this->push(0x10600cu,this->gp[esi]);return 0x10600du;                            // PUSH ESI
        case entries:return 0x106018u;
        case entry_word:return 0x10601eu;
        case second:return 0x106025u;
        case first:return 0x10602fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;                 // AND EAX,FFFF; LEA
        case object_address:gp[ecx]=v;break;
        case tag:gp[eax]=v;break;
        case tags:gp[edx]=v;break;
        // MOVSS XMM0,[ECX+184]; AND EAX,FFFF; SHL EAX,4.
        case charge:charge_value=v;this->s.xmm[0]=logic::sse::scalar(v);gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);break;
        case one:fl.scalar_compare(logic::sse::scalar(charge_value),logic::sse::scalar(v),false);break;   // COMISS
        case definition:gp[edx]=v;set_low_byte(gp[eax],1u);break;                                   // MOV EDX; MOV AL,1
        // MOV ESI,[EDX+2C0]; TEST ESI,ESI; POP ESI.
        case count:gp[esi]=v;this->test32(v);gp[esi]=this->pop(0x106015u);break;
        case entries:gp[edx]=v;break;
        case entry_word:case second:case first:fl.minus(v&0xffffu,0u,2);break;                      // CMP word,0
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<ChargeGate> memory{*this};
        const auto result=logic::charge_gate::open(memory,gp[eax]);
        this->reached(unsigned(result.outcome));
        if(!result.open)set_low_byte(gp[eax],this->flags.logic(0u,1));                               // XOR AL,AL
        this->ret(0x10603bu,0);this->publish(0x10603bu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x105fd0u,0x10603bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ChargeGate<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_charge_gate_detail
