#pragma once
// Boundary adapter for the object type callbacks, entered at 10E1F0
// (native_tu15_object_type_callbacks_systems.hpp). Its callers and the
// callbacks keep the original ABI, which the adapter owns: EBX the handle;
// two stack arguments, RET 8; EBP, ESI and EDI saved (EBP holds the second
// argument); the arguments are addressed from the live ESP, so a callback
// that leaves the stack unbalanced moves them as it does for the original;
// each callback (through the unit's route, at the address the type names)
// gets (EBX, first, second), EAX its own address, ECX the chain entry, and
// every register it returns comes back; the flags of the last TEST; each
// access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_TYPE_CALLBACKS=1 restores the translated body.
// HALO2_VERIFY_OBJECT_TYPE_CALLBACKS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_type_callbacks_systems.hpp"

namespace halo2::native_object_type_callbacks_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_type_callbacks;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_TYPE_CALLBACKS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_TYPE_CALLBACKS");return p && *p;}();
    return value;
}
enum Branch : unsigned {empty_chain, no_callback, called, next_type, chain_end, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object type callbacks verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge callbacks_edges[]{
    {0x10e21cu,0x10e258u,empty_chain},{0x10e232u,0x10e246u,no_callback},{0x10e232u,0x10e239u,called},
    {0x10e246u,0x10e230u,next_type},{0x10e246u,0x10e258u,chain_end},
};
template<class State,class Route> struct Callbacks : Port<State> {
    using Access=logic::object_type_callbacks::Access;
    using Local=logic::object_type_callbacks::Local;
    using Kept=logic::object_type_callbacks::Kept;
    Route &route;
    // The bytes pushed below the return address: it sits at ESP+depth.
    std::uint32_t depth{};
    Callbacks(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,callbacks_edges),route(r){}
    std::uint32_t local(Local l)const{return this->gp[esp]+depth+(l==Local::first?4u:8u);}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:return 0x10e1f0u;
        case object_datums:return 0x10e1f6u;
        case object_address:return 0x10e203u;
        case type:return 0x10e207u;
        case second_argument:return 0x10e20fu;
        case definition:return 0x10e215u;
        case chain_first:return 0x10e21cu;
        case chain_entry:return 0x10e230u;
        case callback:return 0x10e232u;
        case first_argument:return 0x10e239u;
        // INC ESI; MOVSX EAX,SI.
        case chain_next:gp[esi]=this->flags.increment(gp[esi],4,false);gp[eax]=logic::widen16(gp[esi]);return 0x10e246u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:gp[ecx]=v;break;
        // MOV EAX,EBX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case object_datums:gp[edx]=v;gp[eax]=this->flags.logic(gp[ebx]&0xffffu,4)*3u;break;
        case object_address:case chain_entry:gp[eax]=v;break;
        case type:gp[ecx]=std::uint32_t(std::int32_t(std::int8_t(v)));save(0x10e20eu,gp[ebp]);break;   // PUSH EBP
        case second_argument:gp[ebp]=v;save(0x10e213u,gp[esi]);save(0x10e214u,gp[edi]);break;           // PUSH ESI; PUSH EDI
        case definition:gp[edi]=v;break;
        // LEA EAX,[EDI+84]; XOR ESI,ESI; TEST ECX,ECX.
        case chain_first:gp[ecx]=v;gp[eax]=gp[edi]+0x84u;gp[esi]=0;this->test32(v);break;
        case callback:gp[eax]=v;this->test32(v);break;
        // PUSH EBP; PUSH EDX; PUSH EBX.
        case first_argument:gp[edx]=v;save(0x10e23du,gp[ebp]);save(0x10e23eu,gp[edx]);save(0x10e23fu,gp[ebx]);break;
        // TEST ECX,ECX; LEA EAX,[EDI+EAX*4+84].
        case chain_next:gp[ecx]=v;this->test32(v);gp[eax]=gp[edi]+0x84u+gp[eax]*4u;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void call(std::uint32_t entry,Kept &kept) {
        this->call_routed(route,0x10e240u,0x10e242u,entry);depth-=12u;
        kept={this->gp[edi],this->gp[esi]};
    }
    void run() {
        auto &gp=this->gp;
        Memory<Callbacks> memory{*this};
        logic::object_type_callbacks::run(memory,*this,gp[ebx]);
        gp[edi]=this->pop(0x10e258u);gp[esi]=this->pop(0x10e259u);gp[ebp]=this->pop(0x10e25au);
        this->ret(0x10e25bu,8u);this->publish(0x10e25bu);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x10e1f0u,0x10e25bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Callbacks<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_object_type_callbacks_detail
