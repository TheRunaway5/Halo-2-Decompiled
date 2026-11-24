#pragma once
// Boundary adapter for the lens broadcast, entered at 20700
// (native_tu15_lens_broadcast_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: EAX the handle (RET); ESI saved;
// 20E30 gets EAX the index, every register it returns coming back; EAX the
// view count once read, ECX the deferring byte or the views; the flags of
// the last test. Each access publishes its original instruction.
// HALO2_REFERENCE_LENS_BROADCAST=1 restores the translated body.
// HALO2_VERIFY_LENS_BROADCAST=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/effects/native_tu15_lens_broadcast_systems.hpp"

namespace halo2::native_lens_broadcast_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_lens_broadcast;
using Access=logic::lens_broadcast::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_LENS_BROADCAST");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_LENS_BROADCAST");return v;}
enum Branch : unsigned {nothing, kept, sent, branch_count};
static_assert(unsigned(logic::lens_broadcast::Outcome::sent)==Branch::sent);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Lens broadcast"};return v;}
template<class State,class Each> struct Adapter : Port<State> {
    Each &each_child;
    bool saved{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Each &e):Port<State>(s,c,t,{}),each_child(e){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case deferring:return 0x20705u;case views:return 0x2070fu;case kept_store:return 0x20719u;
            case count:return 0x2071fu;case count_again:return 0x20737u;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case deferring:set_low_byte(gp[ecx],v);this->test8(v);break;                                 // MOV CL; TEST CL,CL
        case views:gp[ecx]=v;this->test32(v);break;                                                  // MOV ECX; TEST ECX,ECX
        // MOV EAX,[531FBC]; PUSH ESI; XOR ESI,ESI; TEST EAX,EAX.
        case count:gp[eax]=v;this->push(0x20724u,gp[esi]);saved=true;gp[esi]=this->flags.logic(0u,4);this->test32(v);break;
        // MOV EAX,[531FBC]; INC ESI; CMP ESI,EAX.
        case count_again:gp[eax]=v;gp[esi]=this->flags.increment(gp[esi],4,false);this->compare32(gp[esi],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV EAX,ESI; CALL 20E30.
    std::uint32_t each(std::uint32_t index) {
        this->gp[eax]=index;
        this->call(0x20732u,0x20737u,0x20e30u,each_child);
        return this->gp[esi];
    }
    void run() {
        auto &gp=this->gp;
        this->compare32(gp[eax],logic::none);                                                       // CMP EAX,-1
        Memory<Adapter> memory{*this};
        const auto outcome=logic::lens_broadcast::send(memory,*this,gp[eax]);
        this->reached(unsigned(outcome));
        if(outcome==logic::lens_broadcast::Outcome::kept){this->ret(0x2071eu,0u);this->publish(0x2071eu);return;}
        if(saved)gp[esi]=this->pop(0x20741u);
        this->ret(0x20742u,0u);this->publish(0x20742u);
    }
};
template<class State,class Reference,class Each> bool bridge(State &s,Reference reference,Each each) {
    return run_owner(s,0x20700u,0x20742u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Each>(v,c,t,each);});
}
} // namespace halo2::native_lens_broadcast_detail
