#pragma once
// Boundary adapter for the root words, entered at BC330
// (native_tu15_object_root_systems.hpp, root_words). Its callers keep the
// original ABI, which the adapter owns: ECX the object handle, EAX the out
// pointer; EDI and (along the chain) ESI saved (the headers, the datums);
// ECX and EDX as the original leaves them (the second word, the root's
// first); the flags of the last instruction that sets them; RET. Each access
// publishes its original instruction.
// HALO2_REFERENCE_OBJECT_ROOT=1 restores the translated body.
// HALO2_VERIFY_OBJECT_ROOT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_root_systems.hpp"

namespace halo2::native_root_words_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_root;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_ROOT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_ROOT");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_handle, rooted, chained, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Root words verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct RootWords : Port<State> {
    using Access=logic::root_words::Access;
    unsigned steps{};bool chained_walk{};
    RootWords(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xbc337u;
        case header_datums:this->push(0xbc33fu,this->gp[esi]);chained_walk=true;return 0xbc340u;   // PUSH ESI
        case chain_object:return 0xbc34eu;
        case chain_parent:return 0xbc352u;
        case datums_again:return 0xbc35bu;
        case root_object:return 0xbc367u;
        case first:return 0xbc36bu;
        case first_store:return 0xbc36eu;
        case second:return 0xbc370u;
        case second_store:return 0xbc373u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[edi]=v;break;
        case header_datums:gp[esi]=v;break;
        case chain_object:case first:gp[ecx]=v;break;
        case chain_parent:gp[ecx]=v;fl.minus(v,logic::none,4);break;                               // CMP ECX,-1
        // MOV ECX,[EDI+44]; AND EDX,FFFF; LEA EDX,[EDX+EDX*2].
        case datums_again:gp[ecx]=v;gp[edx]=fl.logic(gp[edx]&0xffffu,4)*3u;break;
        case root_object:case second:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV EDX,ECX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2].
    void stepped(std::uint32_t current) {
        auto &gp=this->gp;
        gp[edx]=current;gp[ecx]=this->flags.logic(current&0xffffu,4)*3u;
        if(steps++)this->reached(Branch::chained);
    }
    void walked(std::uint32_t){if(chained_walk)this->gp[esi]=this->pop(0xbc35au);}                // POP ESI
    void run() {
        auto &gp=this->gp;
        // OR EDX,-1; CMP ECX,-1; PUSH EDI.
        gp[edx]=this->flags.logic(gp[edx]|logic::none,4);this->flags.minus(gp[ecx],logic::none,4);this->push(0xbc336u,gp[edi]);
        this->reached(gp[ecx]==logic::none?Branch::no_handle:Branch::rooted);
        Memory<RootWords> memory{*this};
        logic::root_words::copy(memory,*this,gp[ecx],gp[eax]);
        gp[edi]=this->pop(0xbc376u);this->ret(0xbc377u,0);this->publish(0xbc377u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbc330u,0xbc377u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return RootWords<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_root_words_detail
