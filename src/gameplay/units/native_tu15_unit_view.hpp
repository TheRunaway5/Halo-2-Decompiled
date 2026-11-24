#pragma once
// Boundary adapter for the unit view, 244F80
// (native_tu15_unit_view_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns: ECX the unit, the position's and the
// forward's addresses on the stack (RET 8); a 0x70-byte frame (the marker's
// record) below the saved EBP, ESI and EDI; CD4A0 gets EAX the position and
// ECX the unit; BCDF0 EAX the vehicle and the kind pushed (RET 4); BAD60 EBX
// the record (EBX saved around it), CL 0 and the vehicle, the marker and 1
// pushed (RET C); every register each returns coming back. Each access
// publishes its original instruction.
// HALO2_REFERENCE_UNIT_VIEW=1 restores the translated body.
// HALO2_VERIFY_UNIT_VIEW=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_unit_view_systems.hpp"

namespace halo2::native_unit_view_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_unit_view;
using Access=logic::unit_view::Access;
using Kept=logic::unit_view::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_UNIT_VIEW");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_UNIT_VIEW");return v;}
enum Branch : unsigned {unseated, no_vehicle, own_view, no_marker, marker_view, branch_count};
static_assert(unsigned(logic::unit_view::Outcome::marker_view)==Branch::marker_view);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Unit view"};return v;}
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t marker_record(std::uint32_t offset)const{return this->gp[esp]+depth-0x70u+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case objects:return 0x244f80u;
        case object_data:return 0x244f86u;
        case forward_argument:return 0x244f8fu;
        case object_address:return 0x244f9cu;
        case position_argument:return 0x244fa0u;
        case aim_x:gp[eax]=gp[esi]+logic::unit::aim;return 0x244fb0u;                              // LEA EAX,[ESI+168]
        case aim_x_store:gp[ecx]=gp[ebp];return 0x244fb4u;                                         // MOV ECX,EBP
        case aim_y:return 0x244fb6u;
        case aim_y_store:return 0x244fb9u;
        case aim_z:return 0x244fbcu;
        case aim_z_store:return 0x244fbfu;
        case vehicle:return 0x244fc2u;
        case vehicle_tag:return 0x244fdbu;
        case instances:return 0x244fddu;
        case vehicle_definition:gp[ecx]=fl.shift(fl.logic(gp[ecx]&0xffffu,4),4u,4);return 0x244fecu;   // AND ECX,FFFF; SHL ECX,4
        case seat_index:return 0x244ff0u;
        case seats:return 0x244ff7u;
        case seat_flags:gp[ecx]=fl.product(gp[ecx],logic::seat_size);return 0x245003u;              // IMUL ECX,ECX,B0
        case position_output:return 0x245027u;
        case marker_x:return 0x24502eu;
        case marker_y:return 0x245032u;
        case position_x:return 0x245036u;
        case marker_z:return 0x245038u;
        case position_y:return 0x24503cu;
        case marker_forward_x:return 0x24503fu;
        case position_z:return 0x245043u;
        case marker_forward_y:return 0x245046u;
        case marker_forward_z:return 0x24504au;
        case forward_x:return 0x24504eu;
        case forward_y:return 0x245051u;
        case forward_z:return 0x245054u;
        }
        return 0u;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case objects:gp[edx]=v;break;
        case object_data:                                                                             // SUB ESP,70; MOV EAX,ECX; PUSH EBP
            gp[edx]=v;gp[esp]=fl.minus(gp[esp],0x70u,4);depth=0x70u;gp[eax]=gp[ecx];put(0x244f8eu,gp[ebp]);break;
        case forward_argument:                                                                        // AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; PUSH ESI
            gp[ebp]=v;gp[eax]=3u*fl.logic(gp[eax]&0xffffu,4);put(0x244f9bu,gp[esi]);break;
        case object_address:gp[esi]=v;break;
        case position_argument:gp[eax]=v;put(0x244fa4u,gp[edi]);break;                               // PUSH EDI
        case aim_x:case aim_y:gp[edx]=v;break;
        case aim_z:gp[eax]=v;break;
        case vehicle:gp[edi]=v;fl.minus(v,logic::none,4);break;
        case vehicle_tag:gp[ecx]=v;break;
        case instances:gp[edx]=v;break;
        case vehicle_definition:gp[eax]=v;break;
        case seat_index:gp[ecx]=logic::widen16(v);break;
        case seats:gp[edx]=v;break;
        case seat_flags:gp[eax]=fl.right(v,7u,4,false);this->test8(gp[eax]&1u);break;                // SHR EAX,7; TEST AL,1
        case position_output:gp[eax]=v;break;
        case marker_x:case marker_z:gp[ecx]=v;break;
        case marker_y:case marker_forward_x:gp[edx]=v;break;
        case marker_forward_y:gp[eax]=v;break;
        case marker_forward_z:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void refresh(Kept &kept){kept={this->gp[esi],this->gp[edi],this->gp[ebp]};}
    void eye(std::uint32_t,std::uint32_t,Kept &kept){this->call_routed(route,0x244fa5u,0x244faau,0xcd4a0u);refresh(kept);}
    // PUSH 2; MOV EAX,EDI; CALL BCDF0; TEST EAX,EAX.
    std::uint32_t find(std::uint32_t,std::uint32_t kind,Kept &kept) {
        auto &gp=this->gp;
        put(0x244fceu,kind);gp[eax]=gp[edi];
        this->call_routed(route,0x244fd2u,0x244fd7u,0xbcdf0u);depth-=4u;
        this->test32(gp[eax]);refresh(kept);
        return gp[eax];
    }
    // PUSH EBX; PUSH 1; PUSH F0000DB; PUSH EDI; XOR CL,CL; LEA EBX,[ESP+1C]; CALL BAD60; TEST AX,AX; POP EBX.
    bool marker(std::uint32_t,std::uint32_t name,Kept &kept) {
        auto &gp=this->gp;auto &fl=this->flags;
        put(0x24500du,gp[ebx]);put(0x24500eu,1u);put(0x245010u,name);put(0x245015u,gp[edi]);
        set_low_byte(gp[ecx],fl.logic(0u,1));gp[ebx]=gp[esp]+0x1cu;
        this->call_routed(route,0x24501cu,0x245021u,0xbad60u);depth-=12u;
        fl.logic(gp[eax]&0xffffu,2);
        gp[ebx]=take(0x245024u);refresh(kept);
        return (gp[eax]&0xffffu)!=0u;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::unit_view::locate(memory,*this,gp[ecx])));
        gp[edi]=take(0x245057u);gp[esi]=take(0x245058u);gp[ebp]=take(0x245059u);
        gp[esp]=this->flags.plus(gp[esp],0x70u,4);                                                   // ADD ESP,70
        this->ret(0x24505du,8u);this->publish(0x24505du);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x244f80u,0x24505du,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_unit_view_detail
