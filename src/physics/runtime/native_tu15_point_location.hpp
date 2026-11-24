#pragma once
// Boundary adapter for the point location, entered at 11E600
// (native_tu15_point_location_systems.hpp). Its callers and 14CDB0 keep the
// original ABI, which the adapter owns: ESI the location, the point in the
// registers 14CDB0 reads (passed through); EDI saved (its low word the BSP
// index meanwhile); 14CDB0 gets ECX the collision BSP and EAX zero, and
// every register it returns comes back; EAX (the leaf, then the cluster
// widened or NONE), ECX and EDX as the original leaves them; the flags of
// the last instruction that sets them; RET. Each access publishes its
// original instruction.
// HALO2_REFERENCE_POINT_LOCATION=1 restores the translated body.
// HALO2_VERIFY_POINT_LOCATION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_point_location_systems.hpp"

namespace halo2::native_point_location_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_point_location;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_POINT_LOCATION");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_POINT_LOCATION");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_bsp, located, outside, branch_count};
static_assert(unsigned(logic::point_location::Outcome::outside)==Branch::outside);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Point location verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Leaf> struct PointLocation : Port<State> {
    using Access=logic::point_location::Access;
    using Kept=logic::point_location::Kept;
    Leaf &leaf_child;
    PointLocation(State &s,std::uint32_t *c,Trace<State> *t,Leaf &l):Port<State>(s,c,t,{}),leaf_child(l){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case bsp_index:this->push(0x11e600u,this->gp[edi]);return 0x11e601u;                       // PUSH EDI
        case empty_cluster:return 0x11e60eu;
        case empty_bsp:return 0x11e612u;
        case empty_leaf:return 0x11e616u;
        case collision:return 0x11e61eu;
        case leaf_store:return 0x11e62eu;
        case structure:return 0x11e632u;
        case leaves:return 0x11e638u;
        case cluster:return 0x11e63bu;
        case bsp_store:return 0x11e63fu;
        case cluster_store:return 0x11e643u;
        case outside_bsp_store:this->gp[eax]=this->flags.logic(this->gp[eax]|logic::none,4);return 0x11e64cu;   // OR EAX,-1
        case outside_cluster_store:return 0x11e650u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV DI,[474C24]; CMP DI,-1.
        case bsp_index:gp[edi]=(gp[edi]&0xffff0000u)|(v&0xffffu);fl.minus(v&0xffffu,logic::no_word,2);break;
        case collision:gp[ecx]=v;gp[eax]=fl.logic(0u,4);break;                                      // XOR EAX,EAX
        case structure:gp[ecx]=v;break;
        case leaves:gp[edx]=v;break;
        case cluster:gp[eax]=logic::widen16(v);break;                                               // MOVSX EAX,word
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // CALL 14CDB0; CMP EAX,-1 (the MOV storing the leaf sets no flags).
    std::uint32_t leaf(Kept &kept,std::uint32_t) {
        auto &gp=this->gp;
        this->call(0x11e626u,0x11e62bu,0x14cdb0u,leaf_child);this->flags.minus(gp[eax],logic::none,4);
        kept={gp[esi],gp[edi]&logic::no_word};
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        Memory<PointLocation> memory{*this};
        const auto outcome=logic::point_location::locate(memory,*this,gp[esi]);
        this->reached(unsigned(outcome));
        static constexpr std::uint32_t pops[]{0x11e61cu,0x11e647u,0x11e654u};
        gp[edi]=this->pop(pops[unsigned(outcome)]);
        this->ret(pops[unsigned(outcome)]+1u,0);this->publish(pops[unsigned(outcome)]+1u);
    }
};
template<class State,class Reference,class Leaf> bool bridge(State &s,Reference reference,Leaf leaf) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x11e600u,0x11e655u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return PointLocation<State,Leaf>(v,c,t,leaf);});
    return true;
}
} // namespace halo2::native_point_location_detail
