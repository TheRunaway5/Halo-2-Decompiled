#pragma once
// Boundary adapter for the item notice, entered at 1D9F10
// (native_tu15_item_notice_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: ESI the record (RET); EBX and EDI
// saved; 315CB0 (through the unit's dispatch) gets ECX the item, every
// register it returns coming back; EAX the flags past bit 5 or the count;
// the flags of the last test. Each access publishes its original
// instruction.
// HALO2_REFERENCE_ITEM_NOTICE=1 restores the translated body.
// HALO2_VERIFY_ITEM_NOTICE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_item_notice_systems.hpp"

namespace halo2::native_item_notice_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_item_notice;
using Access=logic::item_notice::Access;
using Kept=logic::item_notice::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ITEM_NOTICE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ITEM_NOTICE");return v;}
enum Branch : unsigned {quiet, empty, noticed, branch_count};
static_assert(unsigned(logic::item_notice::Outcome::noticed)==Branch::noticed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Item notice"};return v;}
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    unsigned saves{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case flags:return 0x1d9f10u;case count:return 0x1d9f1au;case items:return 0x1d9f30u;
            case item:return 0x1d9f33u;case count_again:return 0x1d9f3cu;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case flags:gp[eax]=v>>5;this->test8(gp[eax]&1u);break;                                       // SHR EAX,5; TEST AL,1
        // MOV EAX,[ESI+74]; PUSH EDI; XOR EDI,EDI; TEST EAX,EAX; (PUSH EBX; XOR EBX,EBX).
        case count:
            gp[eax]=v;this->push(0x1d9f1du,gp[edi]);gp[edi]=fl.logic(0u,4);this->test32(v);saves=1u;
            if(std::int32_t(v)>0){this->push(0x1d9f24u,gp[ebx]);gp[ebx]=fl.logic(0u,4);saves=2u;}
            break;
        case items:case item:gp[ecx]=v;break;
        // MOV EAX,[ESI+74]; INC EDI; ADD EBX,60; CMP EDI,EAX.
        case count_again:
            gp[eax]=v;gp[edi]=fl.increment(gp[edi],4,false);gp[ebx]=fl.plus(gp[ebx],logic::item_size,4);this->compare32(gp[edi],v);
            break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void each(Kept &kept,std::uint32_t) {
        this->call_routed(route,0x1d9f37u,0x1d9f3cu,0x315cb0u);
        kept={this->gp[esi],this->gp[edi],this->gp[ebx]};
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::item_notice::notify(memory,*this,gp[esi])));
        if(saves==2u)gp[ebx]=this->pop(0x1d9f47u);
        if(saves)gp[edi]=this->pop(0x1d9f48u);
        this->ret(0x1d9f49u,0u);this->publish(0x1d9f49u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x1d9f10u,0x1d9f49u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_item_notice_detail
