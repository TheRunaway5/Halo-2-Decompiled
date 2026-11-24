#pragma once
// Boundary adapter for the director bind, entered at 171FF0
// (native_tu15_director_bind_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: EAX the object, ECX the index
// (RET); ESI and EDI saved; 175380 gets EAX and EDI the object, ECX the
// index and ESI the record, every register it returns coming back; EAX the
// started byte or zero, ECX the words' address where the original sets
// them; XMM0 zero where started; the flags of the started byte's TEST (or
// the flags' OR). Each access publishes its original instruction.
// HALO2_REFERENCE_DIRECTOR_BIND=1 restores the translated body.
// HALO2_VERIFY_DIRECTOR_BIND=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/vehicles/native_tu15_director_bind_systems.hpp"

namespace halo2::native_director_bind_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_director_bind;
using Access=logic::director_bind::Access;
using Kept=logic::director_bind::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_DIRECTOR_BIND");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_DIRECTOR_BIND");return v;}
enum Branch : unsigned {bound, started, branch_count};
static_assert(unsigned(logic::director_bind::Outcome::started)==Branch::started);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Director bind"};return v;}
inline constexpr std::uint32_t word_stores[]{0x17203eu,0x172040u,0x172043u,0x172046u,0x172049u,0x17204cu};
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    unsigned words{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    // PUSH ESI; MOV ESI,ECX; IMUL ESI,ESI,358; PUSH EDI; MOV EDI,EAX; ADD ESI,4F64A4; CALL 175380.
    void prepared(Kept &kept) {
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x171ff0u,gp[esi]);
        const auto offset=fl.product(gp[ecx],logic::record_size);
        this->push(0x171ff9u,gp[edi]);gp[edi]=kept.object;
        gp[esi]=fl.plus(offset,kept.record-offset,4);
        this->call_routed(route,0x172002u,0x172007u,0x175380u);
        kept={gp[esi],gp[edi]};
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case started:return 0x172007u;
        case object_store:return 0x17200fu;
        case pending_store:return 0x172012u;
        case started_store:this->s.xmm[0]=_mm_setzero_ps();return 0x17201eu;                       // XORPS XMM0,XMM0
        case value_store:return 0x172025u;
        case object:return 0x17202du;
        case flags:case flags_store:return 0x172030u;
        case object_again:return 0x172033u;
        // XOR EAX,EAX; ADD ECX,94 (before the first).
        case word_store:if(!words){gp[eax]=this->flags.logic(0u,4);gp[ecx]=this->flags.plus(gp[ecx],logic::object::words,4);}
            return word_stores[words++];
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case started:set_low_byte(gp[eax],v);this->test8(v);break;                                   // MOV AL; TEST AL,AL
        case object:gp[eax]=v;break;
        case object_again:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        if(a==Access::flags_store)this->flags.logic(v,4);                                            // OR [EAX],8
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::director_bind::bind(memory,*this,gp[eax],gp[ecx]);
        this->reached(unsigned(outcome));
        gp[edi]=this->pop(0x17204fu);gp[esi]=this->pop(0x172050u);
        this->ret(0x172051u,0u);this->publish(0x172051u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x171ff0u,0x172051u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_director_bind_detail
