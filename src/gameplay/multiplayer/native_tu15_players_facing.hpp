#pragma once
// Boundary adapter for the players facing check, 164750
// (native_tu15_players_facing_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the caller's registers in
// (BBE00 reads them), AL the answer (RET); 0x40 bytes of frame with EBX and
// ESI saved below them, then EDI, read from the ESP each child left; BBE00
// (ESI the output), 1A8F10 (ESI the iterator) and 11F7E0 (ECX the offset,
// EDX the aim; XMM0 the angle) through the unit's dispatch, every register
// each returns coming back. XMM0 to XMM2 hold what the original's scalar
// moves and arithmetic leave in them. Each access publishes its original
// instruction.
// HALO2_REFERENCE_PLAYERS_FACING=1 restores the translated body.
// HALO2_VERIFY_PLAYERS_FACING=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_players_facing_systems.hpp"

namespace halo2::native_players_facing_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_players_facing;
using Access=logic::players_facing::Access;
using Kept=logic::players_facing::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLAYERS_FACING");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLAYERS_FACING");return v;}
enum Branch : unsigned {nobody, near, aiming, facing, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Players facing"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x164760u,0x164765u,0x164770u,0x164774u,0x164789u,0x16478fu,0x164795u,0x164799u,0x1647a7u,0x1647aau,
    0x1647b4u,0x1647b6u,0x1647bau,0x1647bdu,0x1647c4u,0x1647c8u,0x1647d4u,0x1647dau,0x1647e0u,0x1647e6u,0x1647ecu,0x1647f9u,0x164806u,
    0x16480cu,0x16481du,0x16482au,0x16483bu,0x164851u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::facing_load)+1u);
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below EBX and ESI
    bool first_next{true};
    std::uint32_t exit{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case players_load:case unit_load:gp[eax]=v;break;
        case headers_load:gp[edi]=v;break;
        case datum_load:case datums_load:case unit_address:case aim_z:gp[ecx]=v;break;
        case aim_x:case aim_y:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // SUB ESP,40; PUSH EBX; PUSH ESI; LEA ESI,[ESP+20]; XOR BL,BL
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);
        this->push(0x164753u,gp[ebx]);this->push(0x164754u,gp[esi]);depth=0u;
        gp[esi]=local(logic::local::point);set_low_byte(gp[ebx],this->flags.logic(0u,1));
    }
    void point_found(Kept &kept) {                                                                       // CALL BBE00
        auto child=[&](State &v){dispatch(v,0xbbe00u,0x16475bu);};
        this->call(0x16475bu,0x164760u,0xbbe00u,child);
        refresh(kept);
    }
    void iterated() {                                                                                    // OR EAX,-1; LEA ESI,[ESP+38]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(gp[eax]|logic::none,4);gp[esi]=local(logic::local::iterator);
    }
    bool next(Kept &kept) {                                                                              // (LEA ESI,[ESP+3C]); CALL 1A8F10; TEST AL,AL
        auto &gp=this->gp;
        const std::uint32_t pc=first_next?0x164778u:0x164862u;
        if(!first_next)gp[esi]=local(logic::local::iterator);
        auto child=[&](State &v){dispatch(v,0x1a8f10u,pc);};
        this->call(pc,pc+5u,0x1a8f10u,child);
        refresh(kept);
        this->test8(gp[eax]);
        if(first_next && !(gp[eax]&0xffu))this->reached(Branch::nobody);
        first_next=false;
        return gp[eax]&0xffu;
    }
    void looping() {                                                                                     // XORPS XMM0; PUSH EDI
        this->s.xmm[0]=_mm_setzero_ps();this->push(0x164788u,this->gp[edi]);depth=4u;
    }
    std::uint32_t indexed(std::uint32_t handle) {                                                       // MOV ECX,EAX; AND ECX,FFFF; LEA EDX,[ECX+ECX*2]
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(handle&logic::handle_mask,4);gp[edx]=3u*gp[ecx];
        return gp[ecx];
    }
    std::uint32_t aimed(std::uint32_t unit_address){return this->gp[ecx]=this->flags.plus(unit_address,logic::object::aim,4);}   // ADD ECX,88
    void position_address(){this->gp[esi]=local(logic::local::position);}                              // LEA ESI,[ESP+30]
    void positioned(Kept &kept) {                                                                        // CALL BBE00
        auto child=[&](State &v){dispatch(v,0xbbe00u,0x1647ccu);};
        this->call(0x1647ccu,0x1647d1u,0xbbe00u,child);
        refresh(kept);
    }
    void distance(std::uint32_t,std::uint32_t dx2,std::uint32_t range,std::uint32_t distance_value) {  // ...; MOVSS XMM0,range; ADDSS; COMISS XMM0,XMM2
        auto &x=this->s.xmm;
        x[0]=lane0(range);x[1]=lane0(dx2);x[2]=lane0(distance_value);
        this->flags.scalar_compare(x[0],x[2],false);
        if(!logic::sse::not_above(range,distance_value))this->reached(Branch::near);
    }
    void aim_length(std::uint32_t ax,std::uint32_t ax2,std::uint32_t length,std::uint32_t shortest) {   // ...; COMISS XMM1,[46A19C]
        auto &x=this->s.xmm;
        x[0]=lane0(ax);x[1]=lane0(length);x[2]=lane0(ax2);
        this->flags.scalar_compare(x[1],lane0(shortest),false);
        if(!logic::sse::not_above(length,shortest))this->reached(Branch::aiming);
    }
    std::uint32_t angle(Kept &kept) {                                                                    // LEA ECX,[ESP+18]; LEA EDX,[ESP+C]; CALL 11F7E0
        auto &gp=this->gp;
        gp[ecx]=local(logic::local::offset);gp[edx]=local(logic::local::aim);
        auto child=[&](State &v){dispatch(v,0x11f7e0u,0x16484cu);};
        this->call(0x16484cu,0x164851u,0x11f7e0u,child);
        refresh(kept);
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[0])));
    }
    void facing_compared(std::uint32_t facing_value,std::uint32_t) {                                    // MOVSS XMM1,[46A1EC]; COMISS XMM1,XMM0
        auto &x=this->s.xmm;
        x[1]=lane0(facing_value);
        this->flags.scalar_compare(x[1],x[0],false);
    }
    void restoring(unsigned n) {
        auto &gp=this->gp;
        static constexpr std::uint32_t pops[3][4]{{0u,0x164881u,0x164884u,0x164888u},{0x16486fu,0x164870u,0x164873u,0x164877u},
            {0x164878u,0x164879u,0x16487cu,0x164880u}};
        const auto &p=pops[n];
        if(p[0])gp[edi]=this->pop(p[0]);
        gp[esi]=this->pop(p[1]);
        if(n==2u){set_low_byte(gp[eax],1u);this->reached(Branch::facing);}else set_low_byte(gp[eax],gp[ebx]);   // MOV AL,1 / MOV AL,BL
        gp[ebx]=this->pop(p[2]);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);                                         // ADD ESP,40
        exit=p[3];
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::players_facing::any(memory,*this,Kept{gp[ebx],gp[edi]});
        this->ret(exit,0u);this->publish(exit);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x164750u,0x164888u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_players_facing_detail
