#pragma once
// Boundary adapter for the unit override query, entered at E3310
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the unit handle, the result in AL (XOR AL,AL, MOV AL,1) above the rest of EAX as the original leaves it; ECX and EDX as the original leaves them; the flags of the last instruction that
// sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_SMALL_QUERIES=1 restores the translated body.
// HALO2_VERIFY_SMALL_QUERIES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_small_queries_systems.hpp"

namespace halo2::native_unit_override_detail {
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
enum Branch : unsigned {other_state, inactive, clear, on, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"unit override query verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Query : Port<State> {
    using Access=logic::unit_override::Access;
    Query(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xe3310u;case header_datums:return 0xe3316u;case unit_address:return 0xe3321u;
        case state:return 0xe3325u;case active:return 0xe3332u;case bits:return 0xe333cu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case unit_address:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;               // AND EAX; LEA
        // MOV DL; XOR AL,AL; CMP DL,6.
        case state:set_low_byte(gp[edx],v);set_low_byte(gp[eax],fl.logic(0u,1));fl.minus(v&0xffu,logic::unit_override::override_state,1);
            if((v&0xffu)!=logic::unit_override::override_state)this->reached(Branch::other_state);break;
        case active:set_low_byte(gp[edx],v);this->test8(v);if(!(v&0xffu))this->reached(Branch::inactive);break;   // MOV DL; TEST DL,DL
        // MOVZX ECX,word; SHR ECX,D; TEST CL,1.
        case bits:gp[ecx]=fl.right(v&0xffffu,logic::unit_override::override_bit,4,false);this->test8(gp[ecx]&1u);
            this->reached((gp[ecx]&1u)?Branch::on:Branch::clear);break;
        }
    }
    void run() {
        Memory<Query> memory{*this};
        if(logic::unit_override::on(memory,this->gp[eax]))set_low_byte(this->gp[eax],1u);           // MOV AL,1
        this->ret(0xe334du,0);this->publish(0xe334du);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xE3310u,0xe334du,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_unit_override_detail
