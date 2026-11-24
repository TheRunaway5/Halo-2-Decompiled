#pragma once
// Boundary adapter for the player relation, entered at 1902E0
// (native_tu15_player_relation_systems.hpp). The callers and the children
// keep the original ABI, which the adapter owns: [ESP+4] the object, [ESP+8]
// the flag, [ESP+C] the index output; RET C; EBP/EDI/EBX/ESI saved once the
// flag is 1; 158290 (the unit's route) gets EAX the slot; BCFE0 (the unit's
// route) EAX/ESI as the two checks set them; every register a child
// returns comes back; the flags of the last test; each access publishes its
// original instruction.
// HALO2_REFERENCE_PLAYER_RELATION=1 restores the translated body.
// HALO2_VERIFY_PLAYER_RELATION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_player_relation_systems.hpp"

namespace halo2::native_player_relation_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_player_relation;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PLAYER_RELATION");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PLAYER_RELATION");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_output, output, unflagged, flagged, absent, other_type, no_unit, within, contains, unrelated,
    branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Player relation verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge relation_edges[]{
    {0x1902e0u,0x1902eeu,no_output},{0x1902e0u,0x1902e8u,output},
    {0x1902f2u,0x190399u,unflagged},{0x1902f2u,0x1902fbu,flagged},
    {0x190315u,0x190315u,absent},{0x190315u,0x19037du,absent},
    {0x190324u,0x190315u,other_type},{0x190324u,0x19037du,other_type},
    {0x19034fu,0x190315u,no_unit},{0x19034fu,0x19037du,no_unit},
    {0x19035eu,0x190384u,within},{0x19036bu,0x190384u,contains},
    {0x19036bu,0x190315u,unrelated},{0x19036bu,0x19037du,unrelated},
};
template<class State,class Route> struct Relation : Port<State> {
    using Access=logic::player_relation::Access;
    using Local=logic::player_relation::Local;
    using Kept=logic::player_relation::Kept;
    Route &route;
    // E: where the return address sits; ESP is E-10 inside the search.
    std::uint32_t frame{};
    Relation(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,relation_edges),route(r){}
    std::uint32_t local(Local l)const{return frame+4u+4u*std::uint32_t(l);}
    void refresh(Kept &kept) {
        auto &gp=this->gp;
        frame=gp[esp]+0x10u;
        kept={gp[edi],gp[ebp],gp[ebx],gp[esi]};
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index_out_argument:return 0x1902e0u;
        case index_clear:return 0x1902e8u;
        case flag_argument:return 0x1902eeu;
        case flag_value:return 0x1902f2u;
        case local_players:this->push(0x1902fbu,gp[ebp]);return 0x1902fcu;
        case player_handle:return 0x190315u;
        case players_table:return 0x19033bu;
        case players_data:return 0x190340u;
        // AND ESI,FFFF; IMUL ESI,ESI,21C.
        case player_unit:gp[esi]=this->flags.product(this->flags.logic(gp[esi]&0xffffu,4),logic::player_size);return 0x19034fu;
        case object_argument:return 0x190358u;
        case index_out_reload:return 0x190384u;
        case index_store:return 0x19038cu;
        case flag_reload:return 0x19038eu;
        case flag_clear:return 0x190395u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index_out_argument:case index_out_reload:case flag_argument:case players_table:
            gp[eax]=v;if(a!=flag_argument && a!=players_table)this->test32(v);break;
        case flag_value:this->flags.minus(v&0xffu,1u,1);break;
        // MOV EBP,[4F54F0]; PUSH EDI; PUSH EBX; XOR EDI,EDI; ADD EBP,C; PUSH ESI.
        case local_players:
            gp[ebp]=v;this->push(0x190302u,gp[edi]);this->push(0x190303u,gp[ebx]);gp[edi]=this->flags.logic(0,4);
            gp[ebp]=this->flags.plus(gp[ebp],logic::local_player_handles,4);this->push(0x190309u,gp[esi]);break;
        case player_handle:gp[esi]=v;break;
        case players_data:gp[ecx]=v;break;
        case player_unit:gp[esi]=v;this->compare32(v,logic::none);break;
        case object_argument:gp[ebx]=v;break;
        // MOV EDX,[flag]; POP ESI; POP EBX; POP EDI.
        case flag_reload:gp[edx]=v;gp[esi]=this->pop(0x190392u);gp[ebx]=this->pop(0x190393u);gp[edi]=this->pop(0x190394u);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void slot_check(Kept &){this->compare32(this->gp[edi],logic::none);}
    // CMP ESI,-1; SETNZ BL; TEST BL,BL.
    void present(Kept &kept,std::uint32_t) {
        auto &gp=this->gp;
        this->compare32(gp[esi],logic::none);
        gp[ebx]=(gp[ebx]&0xffffff00u)|(gp[esi]!=logic::none?1u:0u);this->test8(gp[ebx]);
        kept.present=gp[ebx];
    }
    // MOV EAX,EDI; CALL 158290; MOV EDX,1; MOV ECX,EAX; SHL EDX,CL; TEST
    // DL,3; (TEST BL,BL).
    std::uint32_t player_type(Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=gp[edi];
        this->call_routed(route,0x190324u,0x190329u,0x158290u);
        refresh(kept);
        gp[ecx]=gp[eax];gp[edx]=this->flags.shift(1u,gp[ecx]&0xffu,4);this->test8(gp[edx]&logic::related_types);
        if(gp[edx]&0xffu&logic::related_types)this->test8(gp[ebx]);
        return gp[eax];
    }
    // MOV EAX,EBX; CALL BCFE0; TEST AL,AL.
    std::uint32_t within(Kept &kept,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        gp[eax]=gp[ebx];
        this->call_routed(route,0x19035eu,0x190363u,0xbcfe0u);
        refresh(kept);this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    // MOV EAX,ESI; MOV ESI,EBX; CALL BCFE0; TEST AL,AL.
    std::uint32_t contains(Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=gp[esi];gp[esi]=gp[ebx];
        this->call_routed(route,0x19036bu,0x190370u,0xbcfe0u);
        refresh(kept);this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    // INC EDI; ADD EBP,4; CMP EDI,4.
    void advance(Kept &kept) {
        auto &gp=this->gp;
        gp[edi]=this->flags.increment(gp[edi],4,false);gp[ebp]=this->flags.plus(gp[ebp],4u,4);
        this->compare32(gp[edi],logic::local_player_count);
        kept.slot=gp[edi];kept.handles=gp[ebp];
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        Memory<Relation> memory{*this};
        const auto exit=logic::player_relation::find(memory,*this);
        if(exit==logic::player_relation::Exit::unrelated) {
            gp[esi]=this->pop(0x19037du);gp[ebx]=this->pop(0x19037eu);gp[edi]=this->pop(0x19037fu);gp[ebp]=this->pop(0x190380u);
            this->ret(0x190381u,0xcu);this->publish(0x190381u);
            return;
        }
        if(exit==logic::player_relation::Exit::related)gp[ebp]=this->pop(0x190398u);
        this->ret(0x190399u,0xcu);this->publish(0x190399u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1902e0u,0x190399u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Relation<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_player_relation_detail
