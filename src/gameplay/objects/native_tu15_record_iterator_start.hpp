#pragma once
// Boundary adapter for the record iterator start, entered at 1D3F90
// (native_tu15_record_iterator_start_systems.hpp). Its callers and 1E5250
// keep the original ABI, which the adapter owns: EAX the source, ECX the
// iterator; EBX and ESI saved (the definition, the iterator); 1E5250 gets
// EAX the source, EBX the definition and ESI the iterator, and every
// register it returns comes back; RET. Each access publishes its original
// instruction.
// HALO2_REFERENCE_RECORD_ITERATOR_START=1 restores the translated body.
// HALO2_VERIFY_RECORD_ITERATOR_START=1 records the native run and replays
// the unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_record_iterator_start_systems.hpp"

namespace halo2::native_record_iterator_start_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_record_iterator_start;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_RECORD_ITERATOR_START");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_RECORD_ITERATOR_START");return p && *p;}();
    return value;
}
enum Branch : unsigned {started, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Record iterator start verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Route> struct IteratorStart : Port<State> {
    using Access=logic::record_iterator_start::Access;
    Route &route;
    IteratorStart(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case instances:return 0x1d3f90u;
        case tag:return 0x1d3f9au;
        case definition:return 0x1d3fa6u;
        case first_read:return 0x1d3faau;
        case first_store:return 0x1d3fadu;
        case second_read:return 0x1d3fb0u;
        case second_store:return 0x1d3fb3u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV EDX,[4F0414]; PUSH EBX; PUSH ESI; MOV ESI,ECX.
        case instances:gp[edx]=v;this->push(0x1d3f96u,gp[ebx]);this->push(0x1d3f97u,gp[esi]);gp[esi]=gp[ecx];break;
        case tag:gp[ecx]=fl.shift(fl.logic(v&0xffffu,4),logic::tag_instance_shift,4);break;      // AND ECX,FFFF; SHL ECX,4
        case definition:gp[ebx]=v;break;
        case first_read:gp[ecx]=v;break;
        case second_read:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void step(std::uint32_t){this->call_routed(route,0x1d3fb6u,0x1d3fbbu,0x1e5250u);}            // CALL 1E5250
    void run() {
        auto &gp=this->gp;
        Memory<IteratorStart> memory{*this};
        logic::record_iterator_start::start(memory,*this,gp[eax],gp[ecx]);
        this->reached(Branch::started);
        gp[esi]=this->pop(0x1d3fbbu);gp[ebx]=this->pop(0x1d3fbcu);
        this->ret(0x1d3fbdu,0);this->publish(0x1d3fbdu);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d3f90u,0x1d3fbdu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return IteratorStart<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_record_iterator_start_detail
