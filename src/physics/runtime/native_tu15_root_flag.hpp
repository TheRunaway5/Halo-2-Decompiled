#pragma once
// Boundary adapter for the root flag, entered at BBD50
// (native_tu15_object_root_systems.hpp, root_flag). Its callers keep the
// original ABI, which the adapter owns: EAX the object handle, the bit in
// EAX; ESI saved (the headers); ECX (the root's index, tripled) and EDX (the
// datums) as the original leaves them; the flags of the final AND; RET.
// Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_ROOT=1 restores the translated body.
// HALO2_VERIFY_OBJECT_ROOT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_root_systems.hpp"

namespace halo2::native_root_flag_detail {
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
        if(calls)std::fprintf(stderr,"Root flag verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct RootFlag : Port<State> {
    using Access=logic::root_flag::Access;
    unsigned steps{};
    RootFlag(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xbbd57u;
        case header_datums:return 0xbbd5fu;
        case chain_object:return 0xbbd6cu;
        case chain_parent:return 0xbbd70u;
        case datums_again:return 0xbbd78u;
        case root_flags:return 0xbbd86u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[esi]=v;break;
        case header_datums:gp[edx]=v;break;
        case chain_object:gp[eax]=v;break;
        case chain_parent:gp[eax]=v;fl.minus(v,logic::none,4);break;                               // CMP EAX,-1
        // MOV EDX,[ESI+44]; AND ECX,FFFF; XOR EAX,EAX; LEA ECX,[ECX+ECX*2].
        case datums_again:gp[edx]=v;gp[ecx]=fl.logic(gp[ecx]&0xffffu,4)*3u;gp[eax]=fl.logic(0u,4);break;
        // MOV AL,[...]; POP ESI; SHR EAX,6; AND EAX,1.
        case root_flags:
            set_low_byte(gp[eax],v);gp[esi]=this->pop(0xbbd8au);
            gp[eax]=fl.logic(fl.right(gp[eax],logic::root_flag_bit,4,false)&1u,4);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV ECX,EAX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
    void stepped(std::uint32_t current) {
        auto &gp=this->gp;
        gp[ecx]=current;gp[eax]=this->flags.logic(current&0xffffu,4)*3u;
        if(steps++)this->reached(Branch::chained);
    }
    void walked(std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        // OR ECX,-1; CMP EAX,-1; PUSH ESI.
        gp[ecx]=this->flags.logic(gp[ecx]|logic::none,4);this->flags.minus(gp[eax],logic::none,4);this->push(0xbbd56u,gp[esi]);
        this->reached(gp[eax]==logic::none?Branch::no_handle:Branch::rooted);
        Memory<RootFlag> memory{*this};
        const auto bit=logic::root_flag::of(memory,*this,gp[eax]);
        gp[eax]=bit;this->ret(0xbbd91u,0);this->publish(0xbbd91u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbbd50u,0xbbd91u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return RootFlag<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_root_flag_detail
