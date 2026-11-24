#pragma once
// Boundary adapter for the first-person unit, 3D2B0
// (native_tu15_first_person_unit_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: no arguments; ESI saved;
// BCDF0 gets EAX the unit and the kind pushed (RET 4), EDI saved around it;
// 158290 EAX the view; 168DED the view, the unit, the capacity and the
// models' address pushed (RET 10), every register each returns coming back.
// Each access publishes its original instruction.
// HALO2_REFERENCE_FIRST_PERSON_UNIT=1 restores the translated body.
// HALO2_VERIFY_FIRST_PERSON_UNIT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/animation/native_tu15_first_person_unit_systems.hpp"

namespace halo2::native_first_person_unit_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_first_person_unit;
using Access=logic::first_person_unit::Access;
using Kept=logic::first_person_unit::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_FIRST_PERSON_UNIT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_FIRST_PERSON_UNIT");return v;}
enum Branch : unsigned {overridden, set, no_view, claimed, gathered, branch_count};
static_assert(unsigned(logic::first_person_unit::Outcome::gathered)==Branch::gathered);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"First-person unit"};return v;}
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    bool player_found{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case record:return 0x3d2b0u;
        case unit_clear:return 0x3d2b5u;
        case count_clear:return 0x3d2bfu;
        case active:return 0x3d2c9u;
        case mode:return 0x3d2d3u;
        case override_unit:this->push(0x3d2dau,gp[edi]);return 0x3d2dbu;                            // PUSH EDI
        case unit_store:return 0x3d2f0u;
        case set_unit:return 0x3d2f8u;
        case set_store:return 0x3d2ffu;
        case view:return 0x3d306u;
        case players:return 0x3d31cu;
        case player:return 0x3d321u;
        case player_array:return 0x3d32fu;
        case player_data:return 0x3d335u;
        case player_unit:
            gp[eax]=fl.product(fl.logic(gp[eax]&0xffffu,4),logic::player_record::size);                       // AND EAX,FFFF; IMUL EAX,EAX,21C
            return 0x3d343u;
        case gathered_store:return 0x3d355u;
        case count_store:return 0x3d35fu;
        }
        return 0u;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case record:gp[eax]=v;break;
        case active:
            set_low_byte(gp[ecx],v);
            this->push(0x3d2cbu,gp[esi]);gp[esi]=fl.logic(gp[esi]|0xffffffffu,4);                    // PUSH ESI; OR ESI,-1
            this->test8(v);break;
        case mode:fl.minus(v&0xffffu,logic::override_mode,2);break;
        case override_unit:gp[edi]=v;break;
        case set_unit:gp[eax]=v;fl.minus(v,gp[esi],4);break;                                          // CMP EAX,ESI
        case view:gp[esi]=v;fl.minus(v,logic::none,4);break;
        case players:gp[eax]=v;break;
        case player:
            gp[eax]=v;fl.minus(v,logic::none,4);
            set_low_byte(gp[ecx],v!=logic::none?1u:0u);this->test8(gp[ecx]);break;                      // SETNZ CL; TEST CL,CL
        case player_array:gp[ecx]=v;break;
        case player_data:gp[edx]=v;break;
        case player_unit:gp[eax]=v;player_found=true;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        // CMOVNZ ESI,EDI; POP EDI; CMP ESI,-1; MOV [4CE3F0],ESI.
        if(a==Access::unit_store){this->gp[esi]=v;this->flags.minus(v,logic::none,4);}
    }
    // PUSH 3; MOV EAX,EDI; CALL BCDF0; TEST EAX,EAX.
    bool find(std::uint32_t,std::uint32_t kind,Kept &kept) {
        auto &gp=this->gp;
        this->push(0x3d2deu,kind);gp[eax]=gp[edi];
        this->call_routed(route,0x3d2e2u,0x3d2e7u,0xbcdf0u);
        this->test32(gp[eax]);
        kept.unit=gp[esi];kept.candidate=gp[edi];
        const bool found=gp[eax]!=0u;
        gp[edi]=this->pop(0x3d2ecu);
        return found;
    }
    // MOV EAX,ESI; CALL 158290; TEST EAX,EAX.
    bool claims(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=gp[esi];
        this->call_routed(route,0x3d313u,0x3d318u,0x158290u);
        this->test32(gp[eax]);
        kept.view=gp[esi];
        return gp[eax]!=0u;
    }
    // OR EAX,-1 without a player; PUSH 4CE3F8; PUSH 4; PUSH EAX; PUSH ESI.
    void arguments(std::uint32_t,std::uint32_t,std::uint32_t capacity,std::uint32_t models) {
        auto &gp=this->gp;
        if(!player_found)gp[eax]=this->flags.logic(gp[eax]|0xffffffffu,4);
        this->push(0x3d34cu,models);this->push(0x3d351u,capacity);this->push(0x3d353u,gp[eax]);this->push(0x3d354u,gp[esi]);
    }
    std::uint32_t gather() {
        this->call_routed(route,0x3d35au,0x3d35fu,0x168dedu);
        return this->gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::first_person_unit::choose(memory,*this)));
        gp[esi]=this->pop(0x3d364u);
        this->ret(0x3d365u,0u);this->publish(0x3d365u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x3d2b0u,0x3d365u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_first_person_unit_detail
