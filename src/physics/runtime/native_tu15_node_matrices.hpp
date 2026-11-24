#pragma once
// Boundary adapter for the node matrices, entered at BAC30
// (native_tu15_node_matrices_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the handle, the count pointer at
// [ESP+4]; EAX the result; ECX the object, EDX the count; the flags of the
// final ADD (MUL's carry and overflow before it); RET 4. Each access
// publishes its original instruction.
// HALO2_REFERENCE_NODE_MATRICES=1 restores the translated body.
// HALO2_VERIFY_NODE_MATRICES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_node_matrices_systems.hpp"

namespace halo2::native_node_matrices_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_node_matrices;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_NODE_MATRICES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_NODE_MATRICES");return p && *p;}();
    return value;
}
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Node matrices verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
enum Branch : unsigned {counted, branch_count};
template<class State> struct Matrices : Port<State> {
    using Access=logic::node_matrices::Access;
    using Local=logic::node_matrices::Local;
    Matrices(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(Local)const{return this->gp[esp]+4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:return 0xbac30u;
        case object_datums:return 0xbac36u;
        case object_address:gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;return 0xbac41u;   // AND; LEA
        case size:return 0xbac45u;
        case count_argument:return 0xbac53u;
        case count_store:return 0xbac5au;
        case offset:return 0xbac5cu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:case object_address:gp[ecx]=v;break;
        case object_datums:gp[edx]=v;break;
        // MOVSX EDX; MOV EAX,4EC4EC4F; MUL EDX.
        case size: {
            const auto product=std::uint64_t(logic::reciprocal)*logic::widen16(v);
            gp[eax]=std::uint32_t(product);gp[edx]=std::uint32_t(product>>32);
            this->flags.cf=this->flags.of=gp[edx]!=0;break;
        }
        case count_argument:gp[eax]=v;gp[edx]=this->flags.right(gp[edx],logic::reciprocal_shift,4,false);break;   // SHR EDX,4
        // MOVSX EAX; ADD EAX,ECX: the flags here, the sum (the logic's result) in run().
        case offset:(void)this->flags.plus(logic::widen16(v),gp[ecx],4);this->reached(Branch::counted);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        Memory<Matrices> memory{*this};
        this->gp[eax]=logic::node_matrices::of(memory,*this,this->gp[eax]);
        this->ret(0xbac65u,4u);this->publish(0xbac65u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbac30u,0xbac65u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Matrices<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_node_matrices_detail
