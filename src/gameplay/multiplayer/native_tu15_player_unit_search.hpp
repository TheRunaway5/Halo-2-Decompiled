#pragma once
// Boundary adapter for the player unit search, 153360
// (native_tu15_player_unit_search_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the player on the stack
// (RET 4); a 0x40-byte frame (the found list) below the saved EBX, EBP and
// ESI (and EDI while the list is walked); BD080 gets its seven arguments
// pushed (RET 1C), the radius stored into the slot pushed for it; 1538B0 the
// player and the handle pushed (RET 8); every register each returns coming
// back. Each access publishes its original instruction.
// HALO2_REFERENCE_PLAYER_UNIT_SEARCH=1 restores the translated body.
// HALO2_VERIFY_PLAYER_UNIT_SEARCH=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_player_unit_search_systems.hpp"

namespace halo2::native_player_unit_search_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_player_unit_search;
using Access=logic::player_unit_search::Access;
using Kept=logic::player_unit_search::Kept;
using Outcome=logic::player_unit_search::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLAYER_UNIT_SEARCH");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLAYER_UNIT_SEARCH");return v;}
// The outcomes, then: a unit offered, a child appended.
enum Branch : unsigned {no_unit, riding, searched, offered, appended, branch_count};
static_assert(unsigned(Outcome::searched)==Branch::searched);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Player unit search"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x153360u,0x153364u,0x15336au,0x15337au,0x15338fu,0x153398u,0x15339bu,0x15339fu,0x1533a9u,
    0x1533aeu,0x1533c0u,0x1533e3u,0x1533e7u,0x1533f4u,0x1533f8u,0x153409u,0x153414u,0x15341au,0x153428u,0x153435u,0x153438u,
    0x153444u,0x153448u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::child_sibling)+1u);
template<class State,class Offer,class Route> struct Adapter : Port<State> {
    Offer &offer_child;
    Route &route;
    std::uint32_t depth{};      // bytes below the return address
    bool walked{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Offer &o,Route &r):Port<State>(s,c,t,{}),offer_child(o),route(r){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    std::uint32_t found(std::uint32_t k)const{return this->gp[esp]+depth-0x40u+4u*k;}
    std::uint32_t radius_slot()const{return this->gp[esp];}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case player_unit:gp[eax]=fl.plus(fl.product(fl.logic(gp[eax]&0xffffu,4),logic::player_size),gp[edx],4);break;   // AND; IMUL EAX,EAX,21C; ADD EAX,EDX
        case headers:gp[eax]=fl.logic(gp[eax]&0xffffu,4);put(0x15338eu,gp[ebx]);break;                 // AND EAX,FFFF; PUSH EBX
        case child_datums:fl.minus(gp[esi],logic::capacity,4);break;                                    // CMP ESI,10
        case child_object:gp[ecx]=3u*fl.logic(gp[eax]&0xffffu,4);gp[eax]&=0xffffu;break;              // AND EAX,FFFF; LEA ECX,[EAX+EAX*2]
        default:break;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case player_argument:case player_unit:case datums:case unit_object:case player_again:case first_child:case child_object:gp[eax]=v;break;
        case players:case found_handle:gp[ecx]=v;break;
        case player_data:case child_datums:gp[edx]=v;break;
        case headers:case headers_again:gp[ebx]=v;if(a==headers)gp[edx]=3u*gp[eax];break;           // LEA EDX,[EAX+EAX*2]
        case unit_parent:fl.minus(v,logic::none,4);break;
        case unit_radius:this->s.xmm[0]=lane0_only(v);break;
        case found_datums:gp[edx]=v;gp[eax]=3u*fl.logic(gp[ecx]&0xffffu,4);break;                    // MOV EAX,ECX; AND; LEA
        case found_object:gp[edi]=v;break;
        case found_type:                                                                              // MOVSX EAX,byte; CMP EAX,2; CMP EAX,3
            gp[eax]=logic::widen8(v);fl.minus(gp[eax],logic::first_unit_type,4);
            if(std::int32_t(gp[eax])>=std::int32_t(logic::first_unit_type))fl.minus(gp[eax],logic::last_unit_type,4);break;
        case type_again:                                                                              // MOV AL,[EDI+AA]; CMP AL,B; CMP AL,7
            set_low_byte(gp[eax],v);fl.minus(v&0xffu,logic::parent_type,1);
            if((v&0xffu)!=logic::parent_type)fl.minus(v&0xffu,logic::other_parent_type,1);break;
        case child_sibling:gp[eax]=v;gp[esi]=fl.increment(gp[esi],4,false);fl.minus(v,logic::none,4);this->reached(Branch::appended);break;   // INC ESI; CMP EAX,-1
        default:break;
        }
        if(a==first_child)fl.minus(v,logic::none,4);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framed(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],0x40u,4);depth=0x40u;this->flags.minus(gp[eax],logic::none,4);}   // SUB ESP,40; CMP EAX,-1
    // ADDSS (the radius in XMM0); PUSH EBP; PUSH ESI; PUSH 10; LEA ECX,[ESP+10]; PUSH ECX; PUSH ECX.
    void querying(std::uint32_t radius) {
        auto &gp=this->gp;
        this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0_only(radius));
        put(0x1533b6u,gp[ebp]);put(0x1533b7u,gp[esi]);put(0x1533b8u,logic::capacity);
        gp[ecx]=found(0u);put(0x1533beu,gp[ecx]);put(0x1533bfu,gp[ecx]);
    }
    // LEA EDX,[EAX+30]; PUSH EDX; ADD EAX,28; PUSH EAX; PUSH 89C; PUSH 0; CALL BD080; MOVSX ESI,AX; XOR EBP,EBP; TEST ESI,ESI.
    std::uint32_t query(std::uint32_t,std::uint32_t,Kept &kept) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=gp[eax]+logic::object::center;put(0x1533c8u,gp[edx]);gp[eax]=fl.plus(gp[eax],logic::object::origin,4);put(0x1533ccu,gp[eax]);
        put(0x1533cdu,logic::search_types);put(0x1533d2u,0u);
        this->call_routed(route,0x1533d4u,0x1533d9u,0xbd080u);depth-=28u;
        kept.headers=gp[ebx];
        gp[esi]=logic::widen16(gp[eax]&0xffffu);gp[ebp]=fl.logic(0u,4);this->test32(gp[esi]);
        return gp[eax]&0xffffu;
    }
    void saving_edi(){put(0x1533e2u,this->gp[edi]);walked=true;}
    // PUSH ECX; PUSH EAX; CALL 1538B0.
    void offer(std::uint32_t,std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        put(0x15340du,gp[ecx]);put(0x15340eu,gp[eax]);
        this->call(0x15340fu,0x153414u,0x1538b0u,offer_child);depth-=8u;this->reached(Branch::offered);
        kept={gp[ebx],gp[ebp],gp[esi],gp[edi]};
    }
    // INC EBP; CMP EBP,ESI.
    void next(const Kept &){auto &gp=this->gp;gp[ebp]=this->flags.increment(gp[ebp],4,false);this->flags.minus(gp[ebp],gp[esi],4);}
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::player_unit_search::run(memory,*this);
        this->reached(unsigned(outcome));
        if(outcome!=Outcome::no_unit) {
            if(outcome==Outcome::searched) {
                if(walked)gp[edi]=take(0x153456u);
                gp[esi]=take(0x153457u);gp[ebp]=take(0x153458u);
            }
            gp[ebx]=take(0x153459u);
        }
        gp[esp]=this->flags.plus(gp[esp],0x40u,4);                                                     // ADD ESP,40
        this->ret(0x15345du,4u);this->publish(0x15345du);
    }
};
template<class State,class Reference,class Offer,class Route> bool bridge(State &s,Reference reference,Offer offer,Route route) {
    return run_owner(s,0x153360u,0x15345du,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Offer,Route>(v,c,t,offer,route);});
}
} // namespace halo2::native_player_unit_search_detail
