#pragma once
// Boundary adapter for the mode action, entered at 1582F0
// (native_tu15_mode_action_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the unit, two arguments
// (RET 8); EBX, EBP, ESI and EDI saved; 158340 (through the unit's dispatch)
// gets EAX the second argument's slot, EBX the unit and the slot pushed
// (RET 4), 244F80 ECX the unit and 245D10 EAX the mode, each the second and
// the first pushed (RET 8), every register each returns coming back; AX the
// mode over the last child's EAX; the flags as the last child leaves them.
// Each access publishes its original instruction.
// HALO2_REFERENCE_MODE_ACTION=1 restores the translated body.
// HALO2_VERIFY_MODE_ACTION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_mode_action_systems.hpp"

namespace halo2::native_mode_action_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_mode_action;
using Access=logic::mode_action::Access;
using Kept=logic::mode_action::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_MODE_ACTION");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_MODE_ACTION");return v;}
enum Branch : unsigned {still, moving, branch_count};
static_assert(unsigned(logic::mode_action::Outcome::moving)==Branch::moving);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Mode action"};return v;}
template<class State,class Route,class Still,class Moving> struct Adapter : Port<State> {
    Route &route;
    Still &still_child;
    Moving &moving_child;
    std::uint32_t depth{};
    bool moving_path{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Still &st,Moving &mv)
        :Port<State>(s,c,t,{}),route(r),still_child(st),moving_child(mv){}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u*k;}
    void refresh(Kept &kept){kept={this->gp[edi],this->gp[ebp],this->gp[esi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::second){put(0x1582f0u,this->gp[ebx]);put(0x1582f1u,this->gp[ebp]);return 0x1582f2u;}   // PUSH EBX; PUSH EBP
        return moving_path?0x158324u:0x15830eu;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        if(a==Access::second){gp[ebp]=v;return;}
        if(moving_path)gp[edx]=v;else gp[ecx]=v;
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH ESI; PUSH EDI; MOV EDI,EAX; LEA EAX,[ESP+18]; PUSH EAX; MOV EBX,EDI; CALL 158340;
    // PUSH EBP; MOV ESI,EAX; TEST SI,SI.
    std::uint32_t mode(Kept &kept) {
        auto &gp=this->gp;
        put(0x1582f6u,gp[esi]);put(0x1582f7u,gp[edi]);gp[edi]=gp[eax];
        gp[eax]=gp[esp]+0x18u;put(0x1582feu,gp[eax]);gp[ebx]=gp[edi];
        this->call_routed(route,0x158301u,0x158306u,0x158340u);depth-=4u;
        put(0x158306u,gp[ebp]);gp[esi]=gp[eax];this->flags.logic(gp[esi]&0xffffu,2);
        refresh(kept);
        moving_path=(gp[eax]&0xffffu)!=0u;
        return gp[eax];
    }
    // PUSH ECX (or EDX); (MOV ECX,EDI;) CALL 244F80 (or 245D10).
    void act(Kept &kept,bool moving_mode,std::uint32_t first_value) {
        auto &gp=this->gp;
        if(!moving_mode) {
            put(0x158312u,first_value);gp[ecx]=kept.unit;
            this->call(0x158315u,0x15831au,0x244f80u,still_child);
        } else {
            put(0x158328u,first_value);
            this->call(0x158329u,0x15832eu,0x245d10u,moving_child);
        }
        depth-=8u;
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;
        Kept kept{gp[eax],0u,0u};
        Memory<Adapter> memory{*this};
        const auto outcome=logic::mode_action::act(memory,*this,kept);
        this->reached(unsigned(outcome));
        const auto pop=moving_path?0x15832eu:0x15831au;
        gp[edi]=this->pop(pop);gp[eax]=(gp[eax]&0xffff0000u)|(gp[esi]&0xffffu);                   // MOV AX,SI
        gp[esi]=this->pop(pop+4u);gp[ebp]=this->pop(pop+5u);gp[ebx]=this->pop(pop+6u);
        this->ret(pop+7u,8u);this->publish(pop+7u);
    }
};
template<class State,class Reference,class Route,class Still,class Moving>
bool bridge(State &s,Reference reference,Route route,Still still,Moving moving) {
    return run_owner(s,0x1582f0u,0x158335u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route,Still,Moving>(v,c,t,route,still,moving);});
}
} // namespace halo2::native_mode_action_detail
