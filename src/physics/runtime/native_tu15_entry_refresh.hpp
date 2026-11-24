#pragma once
// Boundary adapter for the entry refresh, entered at 1e5d60
// (native_tu15_small_calls_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: the registers and stack as each hook
// below states (the original's own instructions); every register the child
// returns comes back; the flags of the last instruction that sets them. Each
// access publishes its original instruction.
// HALO2_REFERENCE_SMALL_CALLS=1 restores the translated body.
// HALO2_VERIFY_SMALL_CALLS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_small_calls_systems.hpp"

namespace halo2::native_entry_refresh_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_small_calls;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SMALL_CALLS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SMALL_CALLS");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_entries, unmarked, refreshed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Entry refresh verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Child> struct Query : Port<State> {
    Child &child;
    Query(State &s,std::uint32_t *c,Trace<State> *t,Child &k):Port<State>(s,c,t,{}),child(k){}
    using Access=logic::entry_refresh::Access;
    using Kept=logic::entry_refresh::Kept;
    bool walked{};
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case count:return 0x1e5d60u;case entries:return 0x1e5d70u;case marked:return 0x1e5d76u;case count_again:return 0x1e5d87u;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV EAX,[EDI+AC]; PUSH EBX; XOR EBX,EBX; TEST EAX,EAX.
        case count:gp[eax]=v;this->push(0x1e5d66u,gp[ebx]);gp[ebx]=fl.logic(0u,4);this->test32(v);
            if(logic::signed_value(v)<=0)this->reached(Branch::no_entries);break;
        case entries:gp[eax]=v;break;
        // MOV CL,[EAX+ESI+10]; ADD EAX,ESI; TEST CL,CL.
        case marked:set_low_byte(gp[ecx],v);gp[eax]=fl.plus(gp[eax],gp[esi],4);this->test8(v);
            if(!(v&0xffu))this->reached(Branch::unmarked);break;
        case count_again:gp[eax]=v;break;
        }
    }
    void walking(){auto &gp=this->gp;this->push(0x1e5d6du,gp[esi]);gp[esi]=this->flags.logic(0u,4);walked=true;}   // PUSH ESI; XOR ESI,ESI
    // XOR CL,CL; CALL 126200.
    void refresh(Kept &kept,std::uint32_t) {
        auto &gp=this->gp;
        set_low_byte(gp[ecx],this->flags.logic(0u,1));this->call(0x1e5d82u,0x1e5d87u,0x126200u,child);
        kept={gp[edi],gp[ebx],gp[esi]};this->reached(Branch::refreshed);
    }
    // INC EBX; ADD ESI,14; CMP EBX,EAX: the comparison sets every flag the other two do.
    void stepped(const Kept &kept,std::uint32_t limit) {
        auto &gp=this->gp;
        gp[ebx]=kept.index;gp[esi]=kept.offset;this->flags.minus(kept.index,limit,4);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Query> memory{*this};
        logic::entry_refresh::refresh(memory,*this,gp[edi]);
        if(walked)gp[esi]=this->pop(0x1e5d95u);                                                    // POP ESI
        gp[ebx]=this->pop(0x1e5d96u);this->ret(0x1e5d97u,0);this->publish(0x1e5d97u);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference,class Child> bool bridge(State &s,Reference reference,Child child) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e5d60u,0x1e5d97u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State,Child>(v,c,t,child);});
    return true;
}
} // namespace halo2::native_entry_refresh_detail
