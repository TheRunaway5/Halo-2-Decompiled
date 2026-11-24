#pragma once
// Boundary adapter for the header unanswered query, entered at 17c330
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

namespace halo2::native_header_unanswered_detail {
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
enum Branch : unsigned {inactive, answered, unanswered, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Header unanswered verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Child> struct Query : Port<State> {
    Child &child;
    Query(State &s,std::uint32_t *c,Trace<State> *t,Child &k):Port<State>(s,c,t,{}),child(k){}
    using Access=logic::header_unanswered::Access;
    // [ESP+8] after PUSH EBX.
    std::uint32_t argument()const{return this->gp[esp]+8u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case headers:return 0x17c330u;case header_datums:return 0x17c336u;case header_flags:return 0x17c341u;case argument:return 0x17c34du;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;                 // AND EAX,FFFF; LEA
        // MOV CL,[...]; PUSH EBX; XOR BL,BL; TEST CL,1.
        case header_flags:set_low_byte(gp[ecx],v);this->push(0x17c345u,gp[ebx]);set_low_byte(gp[ebx],fl.logic(0u,1));
            this->test8(v&1u);if(!(v&1u))this->reached(Branch::inactive);break;
        case argument:gp[ecx]=v;break;
        }
    }
    // CALL BBD10 (dispatched); TEST AL,AL.
    std::uint32_t ask(std::uint32_t) {
        this->call_routed(child,0x17c351u,0x17c356u,0xbbd10u);this->test8(this->gp[eax]);
        this->reached((this->gp[eax]&0xffu)?Branch::answered:Branch::unanswered);
        return this->gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Query> memory{*this};
        // MOV AL,1 when unanswered, else MOV AL,BL.
        set_low_byte(gp[eax],logic::header_unanswered::test(memory,*this,gp[eax])?1u:gp[ebx]);
        gp[ebx]=this->pop(0x17c35eu);this->ret(0x17c35fu,4u);this->publish(0x17c35fu);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference,class Child> bool bridge(State &s,Reference reference,Child child) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x17c330u,0x17c35fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State,Child>(v,c,t,child);});
    return true;
}
} // namespace halo2::native_header_unanswered_detail
