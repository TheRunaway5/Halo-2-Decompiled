#pragma once
// Boundary adapter for the cluster visibility query, entered at 11E780
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the header, the result in EAX (1 or 0); ECX the game globals and EDX the cluster's word index as the original leaves them; the flags of the last instruction that
// sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_SMALL_QUERIES=1 restores the translated body.
// HALO2_VERIFY_SMALL_QUERIES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_small_queries_systems.hpp"

namespace halo2::native_cluster_visible_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_small_queries;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SMALL_QUERIES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SMALL_QUERIES");return p && *p;}();
    return value;
}
enum Branch : unsigned {hidden, visible, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"cluster visibility query verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Query : Port<State> {
    using Access=logic::cluster_visible::Access;
    Query(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case cluster:return 0x11e780u;case game_state:return 0x11e790u;case bits:return 0x11e799u;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOVSX EDX; MOV ECX,EDX; AND ECX,1F; MOV EAX,1; SHL EAX,CL.
        case cluster:gp[edx]=logic::widen16(v);gp[ecx]=fl.logic(gp[edx]&0x1fu,4);gp[eax]=fl.shift(1u,gp[ecx]&0xffu,4);break;
        case game_state:gp[ecx]=v;gp[edx]=fl.right(gp[edx],5u,4,true);break;                    // MOV ECX; SAR EDX,5
        // AND EAX,[...]; NEG EAX; SBB EAX,EAX; NEG EAX.
        case bits:
            gp[eax]=fl.logic(gp[eax]&v,4);gp[eax]=fl.minus(0u,gp[eax],4);gp[eax]=fl.borrow(gp[eax],gp[eax],4);
            gp[eax]=fl.minus(0u,gp[eax],4);this->reached(gp[eax]?Branch::visible:Branch::hidden);break;
        }
    }
    void run() {
        Memory<Query> memory{*this};
        this->gp[eax]=logic::cluster_visible::visible(memory,this->gp[eax]);
        this->ret(0x11e7a6u,0);this->publish(0x11e7a6u);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x11E780u,0x11e7a6u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_cluster_visible_detail
