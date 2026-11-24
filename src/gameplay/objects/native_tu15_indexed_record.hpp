#pragma once
// Boundary adapter for the indexed record, entered at 172200
// (native_tu15_object_leaves_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: ECX the index (scaled and offset to
// the record on return); the record or 0 in EAX (XOR EAX,EAX first); the
// flags of the last instruction that sets them; RET.
// HALO2_REFERENCE_OBJECT_LEAVES=1 restores the translated body.
// HALO2_VERIFY_OBJECT_LEAVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_object_leaves_systems.hpp"

namespace halo2::native_indexed_record_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_leaves;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_LEAVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_LEAVES");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_index, no_bsp, record, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object leaves (indexed record) verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Leaf : Port<State> {
    Leaf(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    using Access=logic::indexed_record::Access;
    std::uint32_t instruction(Access,std::uint32_t){return 0x172207u;}
    void loaded(Access,std::uint32_t,std::uint32_t v){this->flags.minus(v&0xffffu,0xffffu,2);}   // CMP word [474C24],-1
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.logic(0u,4);fl.minus(gp[ecx],logic::none,4);                                     // XOR EAX,EAX; CMP ECX,-1
        Memory<Leaf> memory{*this};
        const auto result=logic::indexed_record::address(memory,gp[ecx]);
        this->reached(unsigned(result.outcome));
        if(result.outcome==logic::indexed_record::Outcome::record) {
            // IMUL ECX,ECX,358; ADD ECX,4F655C; MOV EAX,ECX.
            gp[ecx]=fl.plus(fl.product(gp[ecx],logic::indexed_record::record_size),logic::globals::records,4);
            gp[eax]=result.address;
        }
        this->ret(0x17221fu,0);this->publish(0x17221fu);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x172200u,0x17221fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Leaf<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_indexed_record_detail
