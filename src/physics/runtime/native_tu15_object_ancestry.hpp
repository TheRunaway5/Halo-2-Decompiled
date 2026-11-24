#pragma once
// Boundary adapter for the object ancestry, entered at BCFE0
// (native_tu15_object_ancestry_systems.hpp). The callers keep the original
// ABI, which the adapter owns: EAX the object, ESI the ancestor, EAX the
// result; EDI saved around the walk; ECX the last object visited, EDX the
// header table, the flags of the CMP that decided (or XOR EAX,EAX); each
// access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_ANCESTRY=1 restores the translated body.
// HALO2_VERIFY_OBJECT_ANCESTRY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_ancestry_systems.hpp"

namespace halo2::native_object_ancestry_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_ancestry;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_ANCESTRY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_ANCESTRY");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_object, walked, reached, parent, top, within, not_within, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object ancestry verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge ancestry_edges[]{
    {0xbd005u,0xbcff4u,parent},
};
template<class State> struct Ancestry : Port<State> {
    using Access=logic::object_ancestry::Access;
    bool saved{};
    Ancestry(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,ancestry_edges){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:return 0xbcfe8u;
        // CMP ECX,ESI; MOV EDI,[EDX+44].
        case rows:this->compare32(gp[ecx],gp[esi]);return 0xbcff4u;
        // MOV ECX,EAX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case object_data:gp[ecx]=gp[eax];gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;return 0xbd001u;
        case parent:return 0xbd005u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:gp[edx]=v;saved=true;this->push(0xbcfeeu,gp[edi]);break;                    // PUSH EDI
        case rows:gp[edi]=v;break;
        case object_data:gp[eax]=v;break;
        case parent:gp[eax]=v;this->compare32(v,logic::none);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<Ancestry> memory{*this};
        // OR ECX,-1; CMP EAX,-1.
        gp[ecx]=this->flags.logic(gp[ecx]|0xffffffffu,4);
        this->compare32(gp[eax],logic::none);
        const auto walk=logic::object_ancestry::within(memory,gp[eax],gp[esi]);
        // The walk ends at CMP ECX,ESI or CMP EAX,-1; CMP ESI,-1 follows.
        if(saved){gp[edi]=this->pop(0xbd00du);this->reached(walked);this->reached(walk.stopped?reached:top);}
        else this->reached(no_object);
        // CMP ESI,-1; CMP ECX,ESI; MOV EAX,1 or XOR EAX,EAX.
        this->compare32(gp[esi],logic::none);
        if(gp[esi]!=logic::none)this->compare32(gp[ecx],gp[esi]);
        if(walk.within){gp[eax]=1u;this->reached(within);this->ret(0xbd01cu,0);this->publish(0xbd01cu);}
        else {gp[eax]=this->flags.logic(0,4);this->reached(not_within);this->ret(0xbd01fu,0);this->publish(0xbd01fu);}
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbcfe0u,0xbd01fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Ancestry<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_object_ancestry_detail
