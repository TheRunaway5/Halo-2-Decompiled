#pragma once
// Boundary adapter for the object flag bit, entered at E9530
// (native_tu15_object_flag_bit_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the handle and ECX the bit; EAX the
// result (NEG; SBB; NEG), ECX the bit & 31 and EDX the word index; ESI and
// EDI saved; the flags of the final NEG; RET. Each access publishes its
// original instruction.
// HALO2_REFERENCE_OBJECT_FLAG_BIT=1 restores the translated body.
// HALO2_VERIFY_OBJECT_FLAG_BIT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_flag_bit_systems.hpp"

namespace halo2::native_object_flag_bit_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_flag_bit;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_FLAG_BIT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_FLAG_BIT");return p && *p;}();
    return value;
}
enum Branch : unsigned {bit_clear,bit_set,branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object flag bit verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct FlagBit : Port<State> {
    using Access=logic::object_flag_bit::Access;
    FlagBit(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case object_table:return 0xe9530u;
        case object_datums:return 0xe9536u;
        case object_address:return 0xe9541u;
        case bits_offset:return 0xe9546u;
        case bit_word:return 0xe955fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:gp[edx]=v;break;
        // AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case object_datums:gp[edx]=v;gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;break;
        case object_address:gp[eax]=v;this->push(0xe9545u,gp[esi]);break;                         // PUSH ESI
        // PUSH EDI; MOV EDX,ECX; ADD ESI,EAX; AND ECX,1F; MOV EDI,1; SHL EDI,CL; SAR EDX,5.
        case bits_offset:
            gp[esi]=logic::widen16(v);this->push(0xe954du,gp[edi]);gp[edx]=gp[ecx];
            gp[esi]=this->flags.plus(gp[esi],gp[eax],4);gp[ecx]=this->flags.logic(gp[ecx]&0x1fu,4);
            gp[edi]=this->flags.shift(1u,gp[ecx]&0xffu,4);gp[edx]=this->flags.right(gp[edx],5u,4,true);break;
        // AND EAX,EDI; NEG EAX; SBB EAX,EAX; POP EDI; NEG EAX; POP ESI.
        case bit_word:
            gp[eax]=this->flags.logic(v&gp[edi],4);gp[eax]=this->flags.minus(0u,gp[eax],4);
            gp[eax]=this->flags.borrow(gp[eax],gp[eax],4);gp[edi]=this->pop(0xe9569u);
            gp[eax]=this->flags.minus(0u,gp[eax],4);gp[esi]=this->pop(0xe956cu);
            this->reached(gp[eax]?bit_set:bit_clear);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        Memory<FlagBit> memory{*this};
        (void)logic::object_flag_bit::test(memory,this->gp[eax],this->gp[ecx]);
        this->ret(0xe956du,0);this->publish(0xe956du);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xe9530u,0xe956du,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return FlagBit<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_object_flag_bit_detail
