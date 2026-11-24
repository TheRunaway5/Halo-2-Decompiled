#pragma once
// Boundary adapter for the flagged object visit, entered at 102910
// (native_tu15_flagged_objects_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: 20 bytes of stack for the
// record (SUB ESP,14 ... ADD ESP,14); BCEE0 gets the iterator's address and
// 102980 the handle pushed (each pops it), both through the unit's dispatch,
// and every register each returns comes back; EAX (the last answer), ECX
// and EDX as the original leaves them; the flags of the ADD; RET. Each
// access publishes its original instruction.
// HALO2_REFERENCE_FLAGGED_OBJECTS=1 restores the translated body.
// HALO2_VERIFY_FLAGGED_OBJECTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_flagged_objects_systems.hpp"

namespace halo2::native_flagged_objects_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_flagged_objects;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_FLAGGED_OBJECTS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_FLAGGED_OBJECTS");return p && *p;}();
    return value;
}
enum Branch : unsigned {empty, unflagged, visited, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Flagged objects verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Route> struct FlaggedObjects : Port<State> {
    using Access=logic::flagged_objects::Access;
    Route &route;
    std::uint32_t depth{},nexts{};
    FlaggedObjects(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case signature:return 0x102918u;
        case mask:return 0x102920u;
        case kind:return 0x102928u;
        case word:return 0x10292du;
        case handle:return 0x102934u;
        case object_store:return nexts==1?0x102943u:0x10296fu;
        case flag:return 0x102950u;
        case handle_read:return 0x102959u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        if(a==flag){this->test8(v&logic::flag_bit);this->reached((v&logic::flag_bit)?Branch::visited:Branch::unflagged);}   // TEST byte [EAX+16C],1
        if(a==handle_read)gp[ecx]=v;
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // (LEA EAX,[ESP+4]; PUSH EAX) or (LEA EDX,[ESP+4]; PUSH EDX); CALL BCEE0; TEST EAX,EAX.
    std::uint32_t next() {
        auto &gp=this->gp;
        if(nexts==0) {
            this->call_routed(route,0x10293cu,0x102941u,0xbcee0u);
        } else {
            gp[edx]=gp[esp]+logic::local::mask;this->push(0x102967u,gp[edx]);
            this->call_routed(route,0x102968u,0x10296du,0xbcee0u);
        }
        ++nexts;depth=0u;this->test32(gp[eax]);
        return gp[eax];
    }
    // PUSH ECX; CALL 102980.
    void visit(std::uint32_t handle) {this->push(0x10295du,handle);this->call_routed(route,0x10295eu,0x102963u,0x102980u);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        // SUB ESP,14; LEA EAX,[ESP+4]; PUSH EAX.
        gp[esp]=fl.minus(gp[esp],logic::local::size,4);gp[eax]=gp[esp]+logic::local::mask;this->push(0x102917u,gp[eax]);depth=4u;
        Memory<FlaggedObjects> memory{*this};
        logic::flagged_objects::visit(memory,*this);
        if(nexts==1)this->reached(Branch::empty);
        gp[esp]=fl.plus(gp[esp],logic::local::size,4);this->ret(0x102977u,0);this->publish(0x102977u);   // ADD ESP,14
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x102910u,0x102977u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return FlaggedObjects<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_flagged_objects_detail
