#pragma once
// Boundary adapter for the datum visit, entered at 118760
// (native_tu15_datum_visit_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: the quiet word and the handle on the
// stack (RET 8); ESI and EDI saved around 47860, which gets ESI the list,
// EDI the visitor and the handle and the first entry pushed (RET 8), every
// register it returns coming back; the flags of the quiet word's CMP where
// quiet. Each access publishes its original instruction.
// HALO2_REFERENCE_DATUM_VISIT=1 restores the translated body.
// HALO2_VERIFY_DATUM_VISIT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/native_tu15_datum_visit_systems.hpp"

namespace halo2::native_datum_visit_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_datum_visit;
using Access=logic::datum_visit::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_DATUM_VISIT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_DATUM_VISIT");return v;}
enum Branch : unsigned {quiet, walked, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Datum visit"};return v;}
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    unsigned depth{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case quiet:return 0x118760u;
        case handle:return 0x118768u;
        case datums:return 0x11876cu;
        // MOV EAX,ECX; AND EAX,FFFF; IMUL EAX,EAX,2BC; PUSH ESI.
        case data:gp[eax]=fl.product(fl.logic(gp[ecx]&0xffffu,4),logic::datum_size);this->push(0x11877fu,gp[esi]);depth=4u;
            return 0x118780u;
        case first:return 0x118789u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case quiet:fl.minus(v,0u,2);break;                                                            // CMP word [ESP+4],0
        case handle:gp[ecx]=v;break;
        case datums:gp[edx]=v;break;
        // MOV ESI,[EDX+44]; PUSH EDI; ADD EAX,ESI; LEA ESI,[EAX+10].
        case data:gp[esi]=v;this->push(0x118783u,gp[edi]);depth=8u;gp[eax]=fl.plus(gp[eax],v,4);gp[esi]=gp[eax]+logic::list;break;
        case first:gp[eax]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH ECX; PUSH EAX; MOV EDI,8FDB0; CALL 47860.
    void walk(std::uint32_t handle_value,std::uint32_t first_value,std::uint32_t list_address) {
        auto &gp=this->gp;
        this->push(0x11878cu,handle_value);this->push(0x11878du,first_value);gp[edi]=logic::visitor;gp[esi]=list_address;
        this->call_routed(route,0x118793u,0x118798u,0x47860u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const bool walked=logic::datum_visit::visit(memory,*this);
        this->reached(walked?Branch::walked:Branch::quiet);
        if(walked){gp[edi]=this->pop(0x118798u);gp[esi]=this->pop(0x118799u);}
        this->ret(0x11879au,8u);this->publish(0x11879au);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x118760u,0x11879au,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_datum_visit_detail
