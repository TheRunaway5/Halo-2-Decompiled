#pragma once
// Boundary adapter for the type hooks, entered at 10DB40
// (native_tu15_type_hooks_systems.hpp). Its callers and the hooks keep the
// original ABI, which the adapter owns: EBX the handle, two arguments (RET
// 8); EBP, ESI and EDI saved; each hook function (through the unit's
// dispatch) gets the handle, the first and the second pushed (it pops them)
// and answers AL, every register it returns coming back; AL the answer,
// ECX the last slot, EDX the last hook read; the flags of the closing TEST.
// Each access publishes its original instruction.
// HALO2_REFERENCE_TYPE_HOOKS=1 restores the translated body.
// HALO2_VERIFY_TYPE_HOOKS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_type_hooks_systems.hpp"

namespace halo2::native_type_hooks_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_type_hooks;
using Access=logic::type_hooks::Access;
using Kept=logic::type_hooks::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_TYPE_HOOKS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_TYPE_HOOKS");return v;}
enum Branch : unsigned {no_hooks, agreed, refused, skipped, branch_count};
static_assert(unsigned(logic::type_hooks::Outcome::refused)==Branch::refused);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Type hooks"};return v;}
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    std::uint32_t depth{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects:return 0x10db40u;
        case object_data:return 0x10db46u;
        case object:gp[eax]=3u*this->flags.logic(gp[ebx]&0xffffu,4);return 0x10db53u;                 // MOV EAX,EBX; AND; LEA
        case type:return 0x10db57u;
        case second:this->push(0x10db5eu,gp[ebp]);depth=4u;return 0x10db5fu;                           // PUSH EBP
        case definition:this->push(0x10db63u,gp[esi]);depth=8u;return 0x10db64u;                       // PUSH ESI
        case first_hook:return 0x10db6bu;
        case hook:return 0x10db80u;
        case function:return 0x10db82u;
        case first:return 0x10db8du;
        case next_hook:gp[edi]=this->flags.increment(gp[edi],4,false);gp[ecx]=native_object_update::widen16(gp[edi]&0xffffu);
            return 0x10dba4u;                                                                           // INC EDI; MOVSX ECX,DI
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects:gp[ecx]=v;break;
        case object_data:gp[edx]=v;break;
        case object:gp[eax]=v;break;
        case type:gp[ecx]=logic::widen8(v&0xffu);break;
        case second:gp[ebp]=v;break;
        case definition:gp[esi]=v;break;
        // MOV EDX,[ESI+84]; LEA ECX,[ESI+84]; PUSH EDI; XOR EDI,EDI; TEST EDX,EDX; MOV AL,1.
        case first_hook:
            gp[edx]=v;gp[ecx]=gp[esi]+logic::hooks;this->push(0x10db77u,gp[edi]);depth=12u;gp[edi]=this->flags.logic(0u,4);
            this->test32(v);set_low_byte(gp[eax],1u);break;
        case hook:gp[ecx]=v;break;
        case function:gp[ecx]=v;this->test32(v);break;
        case first:gp[edx]=v;break;
        // MOV EDX,[ESI+ECX*4+84]; TEST EDX,EDX; LEA ECX,[ESI+ECX*4+84].
        case next_hook:gp[edx]=v;this->test32(v);gp[ecx]=gp[esi]+4u*gp[ecx]+logic::hooks;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EBP; PUSH EDX; PUSH EBX; CALL ECX; TEST AL,AL; MOV AL,1 or XOR AL,AL.
    std::uint32_t ask(Kept &kept,std::uint32_t function_value,std::uint32_t first_value) {
        auto &gp=this->gp;
        this->test8(gp[eax]);
        this->push(0x10db91u,kept.second);this->push(0x10db92u,first_value);this->push(0x10db93u,kept.handle);
        this->call_routed(route,0x10db94u,0x10db96u,function_value);
        kept={gp[ebx],gp[ebp],gp[esi],gp[edi]};
        const bool yes=gp[eax]&0xffu;
        this->test8(gp[eax]);
        set_low_byte(gp[eax],yes?1u:this->flags.logic(0u,1));
        return yes?1u:0u;
    }
    // TEST AL,AL; XOR AL,AL.
    void refused(){this->test8(this->gp[eax]);set_low_byte(this->gp[eax],this->flags.logic(0u,1));this->reached(Branch::skipped);}
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::type_hooks::agree(memory,*this,gp[ebx])));
        gp[edi]=this->pop(0x10dbb6u);gp[esi]=this->pop(0x10dbb7u);gp[ebp]=this->pop(0x10dbb8u);
        this->ret(0x10dbb9u,8u);this->publish(0x10dbb9u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x10db40u,0x10dbb9u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_type_hooks_detail
