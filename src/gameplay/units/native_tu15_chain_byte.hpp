#pragma once
// Boundary adapter for the chain byte propagation, entered at 17FAF0
// (native_tu15_chain_byte_systems.hpp). The callers and the child 17FB60
// keep the original ABI, which the adapter owns: EDX the handle; RET; EDI
// saved first, ESI before the head's next is read; 17FB60 gets ECX the head
// record, EDX the handle, EDI the byte, ESI the next; the walk keeps EAX the
// record, ECX the byte shifted to the top, EDX the array; the flags of the
// last CMP; each access publishes its original instruction.
// HALO2_REFERENCE_CHAIN_BYTE=1 restores the translated body.
// HALO2_VERIFY_CHAIN_BYTE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_chain_byte_systems.hpp"

namespace halo2::native_chain_byte_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_chain_byte;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CHAIN_BYTE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CHAIN_BYTE");return p && *p;}();
    return value;
}
enum Branch : unsigned {not_head, head, accepted, declined, no_chain, chain, more, last, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Chain byte verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge chain_edges[]{
    {0x17fb04u,0x17fb57u,not_head},{0x17fb04u,0x17fb0eu,head},
    {0x17fb16u,0x17fb56u,accepted},{0x17fb16u,0x17fb24u,declined},
    {0x17fb51u,0x17fb32u,more},{0x17fb51u,0x17fb56u,last},
};
template<class State,class Accept> struct Chain : Port<State> {
    using Access=logic::chain_byte::Access;
    using Kept=logic::chain_byte::Kept;
    Accept &accept_child;bool saved_esi{};
    Chain(State &s,std::uint32_t *c,Trace<State> *t,Accept &a):Port<State>(s,c,t,chain_edges),accept_child(a){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case array:return 0x17faf0u;
        case data:return 0x17fafeu;
        case head:return 0x17fb04u;
        case group:return 0x17fb0eu;
        case next:saved_esi=true;this->push(0x17fb12u,gp[esi]);return 0x17fb13u;
        case array_again:return 0x17fb24u;
        case data_reload:gp[eax]=gp[esi];return 0x17fb32u;                                        // MOV EAX,ESI
        case element_word:return 0x17fb3du;
        case element_next:return 0x17fb43u;
        case element_store:return 0x17fb51u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV EAX,[4F7220]; MOV ECX,EDX; AND ECX,FFFF; PUSH EDI.
        case array:gp[eax]=v;gp[ecx]=this->flags.logic(gp[edx]&0xffffu,4);this->push(0x17fafdu,gp[edi]);break;
        case data:gp[edi]=v;gp[ecx]=this->flags.shift(gp[ecx],logic::record_shift,4);break;          // SHL ECX,6
        case head:gp[eax]=v;gp[ecx]=this->flags.plus(gp[ecx],gp[edi],4);this->compare32(v,gp[edx]);break;
        case group:gp[edi]=v;break;
        case next:gp[esi]=v;break;
        // MOV EDX,[4F7220]; MOV ECX,EDI; SHL ECX,18.
        case array_again:gp[edx]=v;gp[ecx]=this->flags.shift(gp[edi],24u,4);break;
        // MOV ESI,[EDX+44]; AND EAX,FFFF; SHL EAX,6.
        case data_reload:
            gp[esi]=v;gp[eax]=this->flags.shift(this->flags.logic(gp[eax]&0xffffu,4),logic::record_shift,4);break;
        case element_word:gp[edi]=v;gp[eax]=this->flags.plus(gp[eax],gp[esi],4);break;
        // MOV ESI,[EAX+3C]; AND EDI,FFFFFF; OR EDI,ECX; CMP ESI,-1.
        case element_next:
            gp[esi]=v;gp[edi]=this->flags.logic(this->flags.logic(gp[edi]&0xffffffu,4)|gp[ecx],4);
            this->compare32(v,logic::none);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // CALL 17FB60; TEST AL,AL; (CMP ESI,-1).
    std::uint32_t accept(Kept &kept) {
        auto &gp=this->gp;
        this->call(0x17fb16u,0x17fb1bu,0x17fb60u,accept_child);
        this->test8(gp[eax]);
        if(!(gp[eax]&0xffu)){this->compare32(gp[esi],logic::none);this->reached(gp[esi]==logic::none?no_chain:chain);}
        kept={gp[esi],gp[edi]};
        return gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Chain> memory{*this};
        logic::chain_byte::propagate(memory,*this,gp[edx]);
        if(saved_esi)gp[esi]=this->pop(0x17fb56u);
        gp[edi]=this->pop(0x17fb57u);
        this->ret(0x17fb58u,0);this->publish(0x17fb58u);
    }
};
template<class State,class Reference,class Accept> bool bridge(State &s,Reference reference,Accept accept) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x17faf0u,0x17fb58u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Chain<State,Accept>(v,c,t,accept);});
    return true;
}
} // namespace halo2::native_chain_byte_detail
