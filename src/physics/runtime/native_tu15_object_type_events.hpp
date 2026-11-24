#pragma once
// Boundary adapter for the object type events, entered at 10E340
// (native_tu15_object_type_events_systems.hpp). Its callers and the
// callbacks keep the original ABI, which the adapter owns: EDI the handle
// and EBX the value; two stack arguments, RET 8; EBP saved (holding the
// second argument), ESI once the chain is walked (the chain slot); the
// arguments are addressed from the live ESP, so a callback that leaves the
// stack unbalanced moves them as it does for the original; each callback
// (through the unit's route, at the address the type names) gets (EDI,
// first, second, EBX), EAX its own address, and every register it returns
// comes back; the flags of the last TEST; each access publishes its
// original instruction.
// HALO2_REFERENCE_OBJECT_TYPE_EVENTS=1 restores the translated body.
// HALO2_VERIFY_OBJECT_TYPE_EVENTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_type_events_systems.hpp"

namespace halo2::native_object_type_events_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_type_events;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_TYPE_EVENTS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_TYPE_EVENTS");return p && *p;}();
    return value;
}
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object type events verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
enum Branch : unsigned {empty_chain, no_callback, called, next_type, chain_end, branch_count};
inline constexpr Edge events_edges[]{
    {0x10e373u,0x10e3a0u,empty_chain},{0x10e382u,0x10e393u,no_callback},{0x10e382u,0x10e389u,called},
    {0x10e393u,0x10e380u,next_type},{0x10e393u,0x10e39fu,chain_end},
};
template<class State,class Route> struct Events : Port<State> {
    using Access=logic::object_type_events::Access;
    using Local=logic::object_type_events::Local;
    using Kept=logic::object_type_events::Kept;
    Route &route;
    std::uint32_t depth{};bool walked{};
    Events(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,events_edges),route(r){}
    std::uint32_t local(Local l)const{return this->gp[esp]+depth+(l==Local::first?4u:8u);}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:return 0x10e340u;
        case object_datums:return 0x10e346u;
        // MOV EAX,EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case object_address:gp[eax]=this->flags.logic(gp[edi]&0xffffu,4)*3u;return 0x10e353u;
        case type:return 0x10e357u;
        case definition:return 0x10e35eu;
        case chain_first:return 0x10e365u;
        case second_argument:return 0x10e373u;
        // (PUSH ESI; MOV ESI,EAX the first time.)
        case chain_entry:if(!walked){walked=true;save(0x10e379u,gp[esi]);gp[esi]=gp[eax];}return 0x10e380u;
        case callback:return 0x10e382u;
        case first_argument:return 0x10e389u;
        case chain_next:return 0x10e393u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:gp[ecx]=v;break;
        case object_datums:gp[edx]=v;break;
        case object_address:case definition:case chain_entry:gp[eax]=v;break;
        case type:gp[ecx]=std::uint32_t(std::int32_t(std::int8_t(v)));break;
        // ADD EAX,84; TEST ECX,ECX; PUSH EBP.
        case chain_first:
            gp[ecx]=v;gp[eax]=this->flags.plus(gp[eax],0x84u,4);this->test32(v);save(0x10e372u,gp[ebp]);break;
        case second_argument:gp[ebp]=v;break;
        case callback:gp[eax]=v;this->test32(v);break;
        // PUSH EBX; PUSH EBP; PUSH EDX; PUSH EDI.
        case first_argument:
            gp[edx]=v;save(0x10e38du,gp[ebx]);save(0x10e38eu,gp[ebp]);save(0x10e38fu,gp[edx]);save(0x10e390u,gp[edi]);break;
        // ADD ESI,4; TEST ECX,ECX; MOV EAX,ESI.
        case chain_next:gp[ecx]=v;gp[esi]=this->flags.plus(gp[esi],4u,4);this->test32(v);gp[eax]=gp[esi];break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void call(std::uint32_t entry,Kept &kept) {
        this->call_routed(route,0x10e391u,0x10e393u,entry);depth-=16u;
        kept={this->gp[esi]};
    }
    void run() {
        auto &gp=this->gp;
        Memory<Events> memory{*this};
        logic::object_type_events::run(memory,*this,gp[edi]);
        if(walked)gp[esi]=this->pop(0x10e39fu);
        gp[ebp]=this->pop(0x10e3a0u);this->ret(0x10e3a1u,8u);this->publish(0x10e3a1u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x10e340u,0x10e3a1u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Events<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_object_type_events_detail
