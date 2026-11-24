#pragma once
// Boundary adapter for the seat mode, entered at 158340
// (native_tu15_seat_mode_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns: EBX the unit, the mode's address on
// the stack (RET 4); the saved ECX's slot as the frame's word; ESI, EDI (and
// EBP past the record) saved; BCDF0 (through the unit's dispatch) gets EAX
// the unit and the kind pushed (RET 4), 111CD0 EAX the unit, C9550 ECX the
// unit (direct or through the dispatch: the bridge's), every register each
// returns coming back; AX the answer; the flags of the last test. Each
// access publishes its original instruction.
// HALO2_REFERENCE_SEAT_MODE=1 restores the translated body.
// HALO2_VERIFY_SEAT_MODE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_seat_mode_systems.hpp"

namespace halo2::native_seat_mode_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_seat_mode;
using Access=logic::seat_mode::Access;
using Kept=logic::seat_mode::Kept;
using Outcome=logic::seat_mode::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SEAT_MODE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SEAT_MODE");return v;}
enum Branch : unsigned {unseated, driving, passenger, other, gunner, branch_count};
static_assert(unsigned(Outcome::other)==Branch::other);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Seat mode"};return v;}
template<class State,class Route,class Name,class Accepts> struct Adapter : Port<State> {
    Route &route;
    Name &name_child;
    Accepts &accepts_child;
    std::uint32_t pushed{};
    bool driving_path{},passenger_path{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Name &n,Accepts &a)
        :Port<State>(s,c,t,{}),route(r),name_child(n),accepts_child(a){}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);pushed+=4u;}
    std::uint32_t local()const{return this->gp[esp]+pushed-4u;}
    std::uint32_t argument()const{return this->gp[esp]+pushed+4u;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[edi],this->gp[esi],this->gp[ebp]};}
    Outcome path()const{return driving_path?Outcome::driving:passenger_path?Outcome::passenger:Outcome::other;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case local_clear:put(0x158345u,logic::record_kind);gp[eax]=gp[ebx];return 0x158349u;         // PUSH 3; MOV EAX,EBX
        case mode_slot:return 0x158356u;
        case mode_clear:return 0x15835au;
        case vehicle:return 0x158363u;
        case objects:return 0x15836fu;
        case object_data:gp[eax]=fl.logic(gp[eax]&0xffffu,4);gp[ecx]=3u*gp[eax];return 0x15837du;      // AND EAX,FFFF; LEA ECX
        case object:return 0x158380u;
        case type:return 0x15838cu;
        case tag:return 0x1583a4u;
        case tags:return 0x1583a6u;
        case definition:gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);return 0x1583b4u;           // AND EAX,FFFF; SHL EAX,4
        case seat_index:return 0x1583b8u;
        case seats:return 0x1583bfu;
        case seat_flags:gp[eax]=fl.plus(fl.product(gp[eax],logic::seat_size),gp[esi],4);return 0x1583cdu;   // IMUL EAX,B0; ADD EAX,ESI
        case local_gunner:this->reached(Branch::gunner);return 0x1583deu;
        case mode_slot_again:{
            const auto p=path();
            return p==Outcome::driving?0x1583f5u:p==Outcome::passenger?0x158412u:0x158427u;
        }
        case local:return 0x15842bu;
        case mode_store:{
            const auto p=path();
            const auto pops=p==Outcome::driving?0x1583f9u:p==Outcome::passenger?0x158416u:0x158430u;
            gp[ebp]=this->pop(pops);gp[edi]=this->pop(pops+1u);pushed-=8u;                            // POP EBP; POP EDI
            return p==Outcome::driving?0x1583fbu:p==Outcome::passenger?0x158418u:0x158432u;
        }
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case mode_slot:gp[eax]=v;break;
        case vehicle:gp[eax]=v;this->compare32(v,logic::none);break;                                   // CMP EAX,-1
        case objects:gp[edx]=v;break;
        case object_data:gp[eax]=v;break;
        case object:gp[esi]=v;break;
        // MOV CL,[ESI+AA]; MOV EDX,1; SHL EDX,CL; MOV EBP,EAX; TEST DL,3.
        case type:set_low_byte(gp[ecx],v);gp[edx]=1u<<(v&31u);gp[ebp]=gp[eax];this->test8(gp[edx]&logic::seated_types);break;
        case tag:gp[eax]=v;break;
        case tags:gp[ecx]=v;break;
        case definition:gp[edx]=v;break;
        case seat_index:gp[eax]=native_object_update::widen16(v&0xffffu);break;
        case seats:gp[esi]=v;break;
        // MOV EAX,[EAX]; MOV ECX,EAX; SHR ECX,6; SHR EAX,4; AND CL,1; TEST AL,1.
        case seat_flags:
            gp[ecx]=v>>6;gp[eax]=v>>4;set_low_byte(gp[ecx],fl.logic(gp[ecx]&1u,1));this->test8(gp[eax]&1u);
            if(!(v&logic::seat::driver))this->test8(gp[ecx]);                                             // TEST CL,CL
            break;
        case mode_slot_again:{const auto p=path();gp[p==Outcome::driving?eax:p==Outcome::passenger?ecx:edx]=v;break;}
        case local:gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t record(Kept &kept) {
        auto &gp=this->gp;
        this->call_routed(route,0x15834du,0x158352u,0xbcdf0u);pushed-=4u;
        gp[edi]=gp[eax];this->test32(gp[edi]);                                                         // MOV EDI,EAX; TEST EDI,EDI
        refresh(kept);
        return gp[eax];
    }
    // PUSH EBP; MOV EAX,EBX; CALL 111CD0.
    std::uint32_t name(Kept &kept) {
        auto &gp=this->gp;
        put(0x158384u,gp[ebp]);gp[eax]=kept.unit;
        this->call(0x158387u,0x15838cu,0x111cd0u,name_child);
        refresh(kept);kept.name=gp[eax];
        return gp[eax];
    }
    // TEST CL,CL; MOV ECX,EBX; CALL C9550; TEST AL,AL; (CMP EBP,400004A).
    std::uint32_t accepts(Kept &kept) {
        auto &gp=this->gp;
        this->test8(gp[ecx]);gp[ecx]=kept.unit;
        this->call(0x1583ecu,0x1583f1u,0xc9550u,accepts_child);
        this->test8(gp[eax]);
        refresh(kept);
        driving_path=gp[eax]&0xffu;
        if(!driving_path){this->compare32(gp[ebp],logic::passenger_name);passenger_path=gp[ebp]==logic::passenger_name;}
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        put(0x158340u,gp[ecx]);put(0x158341u,gp[esi]);put(0x158342u,gp[edi]);gp[esi]=this->flags.logic(0u,4);   // XOR ESI,ESI
        Kept kept{gp[ebx],0u,0u,0u};
        Memory<Adapter> memory{*this};
        std::uint32_t answer{};
        const auto outcome=logic::seat_mode::of(memory,*this,kept,answer);
        this->reached(unsigned(outcome));
        std::uint32_t exit;
        if(outcome==Outcome::unseated) {
            gp[edi]=this->pop(0x15843cu);gp[eax]=(gp[eax]&0xffff0000u)|(answer&0xffffu);                 // MOV AX,SI
            gp[esi]=this->pop(0x158440u);gp[ecx]=this->pop(0x158441u);exit=0x158442u;
        } else if(outcome==Outcome::other) {
            gp[esi]=this->pop(0x158437u);gp[ecx]=this->pop(0x158438u);exit=0x158439u;
        } else {
            const bool drives=outcome==Outcome::driving;
            gp[eax]=1u;gp[esi]=this->pop(drives?0x158405u:0x158422u);gp[ecx]=this->pop(drives?0x158406u:0x158423u);
            exit=drives?0x158407u:0x158424u;
        }
        this->ret(exit,4u);this->publish(exit);
    }
};
template<class State,class Reference,class Route,class Name,class Accepts>
bool bridge(State &s,Reference reference,Route route,Name name,Accepts accepts) {
    return run_owner(s,0x158340u,0x158442u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route,Name,Accepts>(v,c,t,route,name,accepts);});
}
} // namespace halo2::native_seat_mode_detail
