#pragma once
// Boundary adapter for the page scan, entered at 1F33C0
// (native_tu15_page_scan_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: EDI the cursor (RET); an 8-byte
// frame; ESI saved; 1F3460 (through the unit's dispatch) gets EAX the
// handle, ECX the frame's stride word and EDX the cursor's page, every
// register it returns coming back; EAX and ECX as the original leaves them;
// the flags of the closing ADD ESP,8. Each access publishes its original
// instruction.
// HALO2_REFERENCE_PAGE_SCAN=1 restores the translated body.
// HALO2_VERIFY_PAGE_SCAN=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_page_scan_systems.hpp"

namespace halo2::native_page_scan_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_page_scan;
using Access=logic::page_scan::Access;
using Kept=logic::page_scan::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PAGE_SCAN");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PAGE_SCAN");return v;}
enum Branch : unsigned {found, exhausted, next_page, branch_count};
static_assert(unsigned(logic::page_scan::Outcome::exhausted)==Branch::exhausted);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Page scan"};return v;}
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t local(std::uint32_t k)const{return this->gp[esp]+4u+4u*k;}                     // past the saved ESI
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case page_reset:return 0x1f33c7u;
        case handle:return 0x1f33ccu;
        case index_reset:return 0x1f33d4u;
        case index:return 0x1f33dfu;
        case count:return 0x1f33e3u;
        case stride:return 0x1f33e7u;
        case base:return 0x1f33ecu;
        case entry_store:return 0x1f33efu;
        case page:this->gp[eax]=this->flags.logic(0u,4);return 0x1f33f7u;                        // XOR EAX,EAX
        case page_store:this->gp[eax]=this->flags.increment(this->gp[eax],4,false);return 0x1f3401u;    // INC EAX
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case handle:gp[eax]=v;gp[ecx]=gp[esp]+4u;gp[edx]=gp[esi];break;                            // MOV EAX; LEA ECX; MOV EDX,ESI
        case index:gp[ecx]=native_object_update::widen16(v&0xffffu);break;
        case count:this->compare32(gp[ecx],v);break;                                               // CMP ECX,[EAX]
        case stride:gp[ecx]=fl.product(gp[ecx],v);break;                                           // IMUL ECX,[ESP+4]
        case base:gp[ecx]=fl.plus(gp[ecx],v,4);break;                                              // ADD ECX,[EAX+4]
        case page:gp[eax]=v&0xffffu;fl.minus(v,logic::last_page,2);break;                          // MOV AX; CMP AX,5
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        (void)v;
        if(a==Access::page_store)this->reached(Branch::next_page);
    }
    std::uint32_t table(Kept &kept,std::uint32_t) {
        this->call_routed(route,0x1f33dau,0x1f33dfu,0x1f3460u);
        kept={this->gp[edi],this->gp[esi]};
        return this->gp[eax];
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],8u,4);this->push(0x1f33c3u,gp[esi]);gp[esi]=gp[edi]+logic::cursor::page;   // SUB ESP,8; PUSH ESI; LEA ESI
        Kept kept{gp[edi],gp[esi]};
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::page_scan::scan(memory,*this,kept)));
        gp[esi]=this->pop(0x1f3406u);gp[esp]=fl.plus(gp[esp],8u,4);                                 // POP ESI; ADD ESP,8
        this->ret(0x1f340au,0u);this->publish(0x1f340au);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x1f33c0u,0x1f340au,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_page_scan_detail
