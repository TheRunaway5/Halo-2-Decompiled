#pragma once
// Boundary adapter for the rider check, 1AE6F0
// (native_tu15_rider_check_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ECX the vehicle, the context and
// the result's address on the stack (RET 8), the answer in AL; a 0x10-byte
// frame (the chain cursor) below the saved EBX, EBP, ESI and EDI; D2B00 gets
// ESI the cursor; 1E7810 SI the team and ECX the context (through the unit's
// dispatch, as D2B00 is); every register each returns coming back. Each
// access publishes its original instruction.
// HALO2_REFERENCE_RIDER_CHECK=1 restores the translated body.
// HALO2_VERIFY_RIDER_CHECK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_rider_check_systems.hpp"

namespace halo2::native_rider_check_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_rider_check;
using Access=logic::rider_check::Access;
using Kept=logic::rider_check::Kept;
using Outcome=logic::rider_check::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RIDER_CHECK");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RIDER_CHECK");return v;}
enum Branch : unsigned {none_hostile, hostile, passed, branch_count};
static_assert(unsigned(Outcome::passed)==Branch::passed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Rider check"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x1ae6f8u,0x1ae704u,0x1ae70au,0x1ae713u,0x1ae717u,0x1ae722u,0x1ae726u,0x1ae72au,0x1ae733u,
    0x1ae735u,0x1ae739u,0x1ae73eu,0x1ae741u,0x1ae745u,0x1ae74cu,0x1ae770u,0x1ae774u,0x1ae777u,0x1ae788u,0x1ae78eu,0x1ae795u,
    0x1ae7a2u,0x1ae7b1u,0x1ae7bcu,0x1ae7c5u,0x1ae765u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::headers_again)+1u);
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    std::uint32_t depth{};      // bytes below the return address
    unsigned steps{};           // the chain steps taken
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t cursor(std::uint32_t offset)const{return this->gp[esp]+depth-0x10u+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case vehicle_address:gp[eax]=fl.shift(3u*gp[eax],2u,4);break;                                // LEA EAX,[EAX+EAX*2]; SHL EAX,2
        case definition:gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);break;                    // AND EDX,FFFF; SHL EDX,4
        case seat_flags:gp[eax]=fl.product(logic::widen16(gp[edi]),logic::seat_size);break;          // MOVSX EAX,DI; IMUL EAX,EAX,B0
        case result_store:                                                                           // POP EDI; POP ESI; MOV AL,1; POP EBP
            gp[edi]=take(0x1ae7c0u);gp[esi]=take(0x1ae7c1u);set_low_byte(gp[eax],1u);gp[ebp]=take(0x1ae7c4u);break;
        default:break;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case instances:gp[esi]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4);put(0x1ae703u,gp[edi]);break;   // AND EAX,FFFF; PUSH EDI
        case headers:case headers_again:gp[edi]=v;break;
        case datums:case vehicle_address:case vehicle_tag:case result_argument:case vehicle_again:case unit_datums:gp[edx]=v;break;
        case definition:gp[ebp]=v;break;
        case datums_again:case seats:case context_argument:case result_again:gp[ecx]=v;break;
        case first_child:case unit_handle:case unit_address:gp[eax]=v;break;
        case seat:                                                                                   // MOV DI,[ESP+18]; AND EAX,FFFF; CMP DI,-1; LEA ECX,[EAX+EAX*2]
            set_low_word(gp[edi],v);gp[eax]=fl.logic(gp[eax]&0xffffu,4);fl.minus(v&0xffffu,logic::no_seat,2);gp[ecx]=3u*gp[eax];break;
        case unit_team:set_low_word(gp[esi],v);break;
        case seat_flags:gp[edx]=fl.right(v,logic::open_seat_bit,4,false);this->test8(gp[edx]&1u);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        if(a==Access::cursor_vehicle){gp[ecx]=fl.logic(0xffffffffu,4);set_low_byte(gp[ebx],fl.logic(0u,1));}   // OR ECX,-1; XOR BL,BL
    }
    void refresh(Kept &kept){kept={this->gp[edi],this->gp[ebp],this->gp[ebx]&0xffu};}
    // LEA ESI,[ESP+10]; CALL D2B00; TEST EAX,EAX.
    std::uint32_t next_unit(Kept &kept) {
        auto &gp=this->gp;
        gp[esi]=cursor(0u);
        this->call_routed(route,steps?0x1ae7d4u:0x1ae750u,steps?0x1ae7d9u:0x1ae755u,0xd2b00u);++steps;
        this->test32(gp[eax]);refresh(kept);
        return gp[eax];
    }
    std::uint32_t hostile(std::uint32_t,std::uint32_t,Kept &kept) {
        this->call_routed(route,0x1ae799u,0x1ae79eu,0x1e7810u);this->test8(this->gp[eax]&0xffu);refresh(kept);
        return this->gp[eax];
    }
    void marked(const Kept &kept){set_low_byte(this->gp[ebx],kept.answer);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x10u,4);depth=0x10u;                                               // SUB ESP,10
        put(0x1ae6f3u,gp[ebx]);put(0x1ae6f4u,gp[ebp]);gp[eax]=gp[ecx];put(0x1ae6f7u,gp[esi]);
        Memory<Adapter> memory{*this};
        const auto outcome=logic::rider_check::check(memory,*this,gp[ecx]);
        this->reached(unsigned(outcome));
        std::uint32_t exit;
        if(outcome==Outcome::passed){gp[ebx]=take(0x1ae7c7u);exit=0x1ae7c8u;}
        else {
            const std::uint32_t at=steps>1u?0x1ae7ddu:0x1ae759u;
            gp[edi]=take(at);gp[esi]=take(at+1u);gp[ebp]=take(at+2u);set_low_byte(gp[eax],gp[ebx]);gp[ebx]=take(at+5u);
            exit=at+6u;
        }
        gp[esp]=fl.plus(gp[esp],0x10u,4);                                                            // ADD ESP,10
        this->ret(exit+3u,8u);this->publish(exit+3u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x1ae6f0u,0x1ae7e6u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_rider_check_detail
