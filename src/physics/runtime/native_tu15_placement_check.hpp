#pragma once
// Boundary adapter for the placement check, entered at 175300
// (native_tu15_placement_check_systems.hpp). Its callers and the checks keep
// the original ABI, which the adapter owns: EBX the record (kept across the
// checks), the result in AL (MOV AL,1 or XOR AL,AL; the rest of EAX as the
// last check left it); B1720 gets ESI +0x2C and EDI +0x38 (both saved
// around it), 1751B0 and B16C0 get EAX the part, the scalar checks XMM0
// (MOVSS); every register a check returns comes back; the flags of the last
// instruction that sets them; RET. Each access publishes its original
// instruction.
// HALO2_REFERENCE_PLACEMENT_CHECK=1 restores the translated body.
// HALO2_VERIFY_PLACEMENT_CHECK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_placement_check_systems.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"

namespace halo2::native_placement_check_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_placement_check;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PLACEMENT_CHECK");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PLACEMENT_CHECK");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_record, unchecked, unit_vectors_failed, first_point_failed, second_point_failed, finite_vector_failed,
    first_scalar_failed, second_scalar_failed, third_scalar_failed, sane, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Placement check verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// Each check's call site (CALL, return address) and child.
struct Site {std::uint32_t call,next,target;};
inline constexpr Site sites[]{
    {0x175311u,0x175316u,0xb1720u},{0x17531fu,0x175324u,0x1751b0u},{0x17532bu,0x175330u,0x1751b0u},
    {0x175337u,0x17533cu,0xb16c0u},{0x175345u,0x17534au,0x175280u},{0x175353u,0x175358u,0x175240u},
    {0x175364u,0x175369u,0x1752c0u},
};
template<class State,class Route> struct Placement : Port<State> {
    using Access=logic::placement_check::Access;
    using Check=logic::placement_check::Check;
    using Kept=logic::placement_check::Kept;
    Route &route;
    Placement(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case flags:return 0x175304u;
        case first_scalar:return 0x175340u;
        case second_scalar:return 0x17534eu;
        case third_scalar:return 0x17535cu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        if(a==Access::flags){this->test8(v&logic::record::checked_bit);if(!(v&logic::record::checked_bit))this->reached(Branch::unchecked);}
        else this->s.xmm[0]=native_aim_clusters::sse::scalar(v);                       // MOVSS XMM0
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    bool present(std::uint32_t record) {
        this->test32(record);
        if(!record)this->reached(Branch::no_record);
        return record!=0;
    }
    bool check(Kept &kept,Check c,std::uint32_t part) {
        auto &gp=this->gp;
        const auto &site=sites[unsigned(c)];
        if(c==Check::unit_vectors) {
            // PUSH ESI; PUSH EDI; LEA EDI,[EBX+38]; LEA ESI,[EBX+2C].
            this->push(0x175309u,gp[esi]);this->push(0x17530au,gp[edi]);
            gp[edi]=part+logic::record::second_vector;gp[esi]=part+logic::record::first_vector;
        } else if(c==Check::first_point||c==Check::second_point||c==Check::finite_vector)gp[eax]=part;   // LEA EAX
        this->call(site.call,site.next,site.target,[&](State &v){route(v,site.target);});
        this->test8(gp[eax]);                                                                     // TEST AL,AL
        const bool passed=(gp[eax]&0xffu)!=0;
        if(c==Check::unit_vectors){gp[edi]=this->pop(0x175318u);gp[esi]=this->pop(0x175319u);}  // POP EDI; POP ESI
        kept.record=gp[ebx];
        if(!passed)this->reached(Branch(unsigned(Branch::unit_vectors_failed)+unsigned(c)));
        else if(c==Check::third_scalar)this->reached(Branch::sane);
        return passed;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Placement> memory{*this};
        const bool result=logic::placement_check::sane(memory,*this,gp[ebx]);
        if(result) {
            set_low_byte(gp[eax],1u);                                                             // MOV AL,1
            this->ret(0x17536fu,0);this->publish(0x17536fu);
        } else {
            set_low_byte(gp[eax],this->flags.logic(0u,1));                                       // XOR AL,AL
            this->ret(0x175372u,0);this->publish(0x175372u);
        }
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x175300u,0x175372u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Placement<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_placement_check_detail
