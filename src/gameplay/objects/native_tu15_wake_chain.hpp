#pragma once
// Boundary adapter for the wake chain, entered at B9390
// (native_tu15_wake_chain_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the handle; EBX and ESI saved (ESI the header
// table); EAX, ECX and EDX as the original leaves them; the flags of the
// last TEST or CMP; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_WAKE_CHAIN=1 restores the translated body.
// HALO2_VERIFY_WAKE_CHAIN=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_wake_chain_systems.hpp"

namespace halo2::native_wake_chain_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_wake_chain;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_WAKE_CHAIN");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_WAKE_CHAIN");return p && *p;}();
    return value;
}
enum Branch : unsigned {awake, no_object, woken, next_parent, last_parent, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Wake chain verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge wake_edges[]{
    {0xb93a6u,0xb93bdu,woken},{0xb93d1u,0xb93bdu,next_parent},{0xb93d1u,0xb93d9u,last_parent},
};
template<class State> struct Wake : Port<State> {
    using Access=logic::wake_chain::Access;
    Wake(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,wake_edges){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // PUSH EBX; MOV EDX,EAX; PUSH ESI.
        case headers:this->push(0xb9390u,gp[ebx]);gp[edx]=gp[eax];this->push(0xb9393u,gp[esi]);return 0xb9394u;
        case datums:return 0xb939au;
        // AND EDX,FFFF; LEA EDX,[EDX+EDX*2].
        case first_flags:gp[edx]=this->flags.logic(gp[edx]&0xffffu,4)*3u;return 0xb93a6u;
        // AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; SHL EAX,2.
        case flags:gp[eax]=this->flags.shift(this->flags.logic(gp[eax]&0xffffu,4)*3u,2u,4);return 0xb93bdu;
        case flags_store:return 0xb93c8u;
        case datums_again:return 0xb93cau;
        case object:return 0xb93cdu;
        case parent:return 0xb93d1u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:gp[esi]=v;break;
        case datums:case datums_again:gp[ecx]=v;break;
        // TEST byte,2; (CMP EAX,-1).
        case first_flags:
            this->test8(v&logic::header_flags::awake);
            if(v&logic::header_flags::awake)this->reached(Branch::awake);
            else{this->compare32(gp[eax],logic::none);if(gp[eax]==logic::none)this->reached(Branch::no_object);}
            break;
        // MOV BL; LEA ECX,[EAX+ECX+2]; OR BL,2.
        case flags:gp[ecx]=gp[eax]+gp[ecx]+2u;set_low_byte(gp[ebx],this->flags.logic((v|logic::header_flags::awake)&0xffu,1));break;
        case object:gp[eax]=v;break;
        case parent:gp[eax]=v;this->compare32(v,logic::none);break;                               // CMP EAX,-1
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<Wake> memory{*this};
        logic::wake_chain::wake(memory,gp[eax]);
        gp[esi]=this->pop(0xb93d9u);gp[ebx]=this->pop(0xb93dau);this->ret(0xb93dbu,0);this->publish(0xb93dbu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xb9390u,0xb93dbu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Wake<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_wake_chain_detail
