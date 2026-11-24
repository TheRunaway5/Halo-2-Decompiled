#pragma once
// Boundary adapter for the block ready, entered at 116480
// (native_tu15_tiny_leaves_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: EAX the object handle, and 1 or 0
// (MOV EAX,1; XOR EAX,EAX) on return; ECX (the object, its low byte the
// last byte tested) and EDX as the original leaves them; the flags of the
// last instruction that sets them; RET.
// HALO2_REFERENCE_TINY_LEAVES=1 restores the translated body.
// HALO2_VERIFY_TINY_LEAVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_tiny_leaves_systems.hpp"

namespace halo2::native_block_ready_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_tiny_leaves;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TINY_LEAVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TINY_LEAVES");return p && *p;}();
    return value;
}
enum Branch : unsigned {open, blocked, ready, waiting, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Tiny leaves (block ready) verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Leaf : Port<State> {
    Leaf(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    using Access=logic::block_ready::Access;
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case headers:return 0x116480u;case header_datums:return 0x116486u;case object_address:return 0x116491u;
            case block:return 0x116495u;case gate:return 0x11649eu;case state:return 0x1164acu;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        namespace bits=logic::block_ready;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;                 // AND EAX,FFFF; LEA
        case object_address:gp[ecx]=v;break;
        case block:gp[eax]=fl.plus(logic::widen16(v),gp[ecx],4);break;                             // MOVSX EAX; ADD EAX,ECX
        // MOV CL,[EAX+89]; TEST CL,CL; and again while set.
        case gate:set_low_byte(gp[ecx],v);this->test8(v);if(v&0xffu)this->test8(v);break;
        // MOV CL,[EAX+8B]; TEST CL,1; then TEST CL,2.
        case state:set_low_byte(gp[ecx],v);this->test8(v&bits::blocked_bit);if(!(v&bits::blocked_bit))this->test8(v&bits::ready_bit);break;
        }
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::block_ready::Outcome;
        Memory<Leaf> memory{*this};
        const auto outcome=logic::block_ready::test(memory,gp[eax]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::open || outcome==Outcome::ready){gp[eax]=1u;this->ret(0x1164c4u,0);this->publish(0x1164c4u);return;}
        gp[eax]=this->flags.logic(0u,4);this->ret(0x1164beu,0);this->publish(0x1164beu);            // XOR EAX,EAX
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x116480u,0x1164c4u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Leaf<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_block_ready_detail
