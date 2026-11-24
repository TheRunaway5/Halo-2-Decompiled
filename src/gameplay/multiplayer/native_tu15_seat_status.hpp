#pragma once
// Boundary adapter for the seat status, 161610
// (native_tu15_seat_status_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: no arguments; EBX saved, and EBP,
// ESI and EDI once a seat has a player; 1618B0 gets the handle pushed (RET 4),
// every register it returns coming back. Each access publishes its original
// instruction.
// HALO2_REFERENCE_SEAT_STATUS=1 restores the translated body.
// HALO2_VERIFY_SEAT_STATUS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_seat_status_systems.hpp"

namespace halo2::native_seat_status_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_seat_status;
using Access=logic::seat_status::Access;
using Kept=logic::seat_status::Kept;
using Outcome=logic::seat_status::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SEAT_STATUS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SEAT_STATUS");return v;}
// The outcomes, then each seat's: its status unchanged, restarted, matched.
enum Branch : unsigned {no_player, visited, unchanged, restarted, matched, branch_count};
static_assert(unsigned(Outcome::visited)==Branch::visited);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Seat status"};return v;}
template<class State,class Status> struct Adapter : Port<State> {
    Status &status_child;
    bool saved{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Status &st):Port<State>(s,c,t,{}),status_child(st){}
    // A seat's scan step: CMP [ECX],-1; then MOV found,EAX or INC EAX; ADD ECX,4; CMP EAX,4.
    void scanned(std::uint32_t v,unsigned found) {
        auto &gp=this->gp;auto &fl=this->flags;
        fl.minus(v,logic::none,4);
        if(v!=logic::none){gp[found]=gp[eax];return;}
        gp[eax]=fl.increment(gp[eax],4,false);gp[ecx]=fl.plus(gp[ecx],4u,4);fl.minus(gp[eax],logic::seat_count,4);
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case seats:return 0x161610u;
        case first_handle:return 0x161620u;
        case seats_again:return 0x161649u;
        case seat_player:return 0x16164eu;
        case previous_code:gp[ecx]=fl.shift(3u*gp[ebx],2u,4);return 0x161663u;                         // LEA ECX,[EBX+EBX*2]; SHL ECX,2
        case game_globals:return 0x16166bu;
        case multiplayer:return 0x161671u;
        case instances:return 0x161677u;
        case multiplayer_data:gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);return 0x161686u;    // AND EDX,FFFF; SHL EDX,4
        case runtime_address:return 0x16168au;
        case code_store:return 0x16168fu;
        case timer_store:return 0x161695u;
        case index_store:return 0x16169bu;
        case count:return 0x1616a5u;
        case statuses:return 0x1616afu;
        case status_value:return 0x1616b8u;
        case matched_store:this->reached(Branch::matched);return 0x1616c9u;
        case seats_next:return 0x1616e3u;
        case next_handle:return 0x1616f0u;
        }
        return 0u;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case seats:                                                                                     // PUSH EBX; OR EBX,-1; XOR EAX,EAX; ADD ECX,C
            gp[ecx]=v;this->push(0x161616u,gp[ebx]);gp[ebx]=fl.logic(0xffffffffu,4);gp[eax]=fl.logic(0u,4);
            gp[ecx]=fl.plus(gp[ecx],logic::seat_players,4);break;
        case first_handle:scanned(v,ebx);break;
        case seats_again:gp[eax]=v;break;
        case seat_player:gp[eax]=v;fl.minus(v,logic::none,4);break;
        case previous_code:fl.minus(gp[eax],v,4);this->reached(gp[eax]==v?Branch::unchanged:Branch::restarted);break;   // CMP EAX,[ECX+4F63D8]
        case game_globals:case multiplayer:case multiplayer_data:gp[edx]=v;break;
        case instances:gp[esi]=v;break;
        case runtime_address:gp[esi]=v;gp[edx]=fl.logic(0u,4);break;                                   // XOR EDX,EDX
        case count:gp[edi]=v;fl.minus(v,gp[edx],4);break;                                              // CMP EDI,EDX
        case statuses:gp[esi]=fl.plus(v,logic::status_code,4);break;                                   // ADD ESI,4
        case status_value:                                                                             // MOVSX EBP,[ESI]; CMP EBP,EAX
            gp[ebp]=logic::widen16(v);fl.minus(gp[ebp],gp[eax],4);
            if(gp[ebp]!=gp[eax]){gp[edx]=fl.increment(gp[edx],4,false);gp[esi]=fl.plus(gp[esi],logic::status_size,4);fl.minus(gp[edx],gp[edi],4);}
            break;
        case seats_next:gp[ecx]=v+4u*gp[eax]+logic::seat_players;break;                                // LEA ECX,[ECX+EAX*4+C]
        case next_handle:scanned(v,edx);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // CMP EBX,-1; PUSH EBP; PUSH ESI; PUSH EDI.
    void visiting() {
        auto &gp=this->gp;
        this->flags.minus(gp[ebx],logic::none,4);
        this->push(0x16163bu,gp[ebp]);this->push(0x16163cu,gp[esi]);this->push(0x16163du,gp[edi]);saved=true;
    }
    std::uint32_t status(std::uint32_t,Kept &kept) {
        this->push(0x161657u,this->gp[eax]);
        this->call(0x161658u,0x16165du,0x1618b0u,status_child);
        kept.seat=this->gp[ebx];
        return this->gp[eax];
    }
    // OR EDX,-1; CMP EBX,-1; XOR EAX,EAX or LEA EAX,[EBX+1]; CMP EAX,4.
    void advancing(const Kept &kept) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.logic(0xffffffffu,4);fl.minus(kept.seat,logic::none,4);
        gp[eax]=kept.seat==logic::none?fl.logic(0u,4):kept.seat+1u;fl.minus(gp[eax],logic::seat_count,4);
    }
    // CMP EDX,-1; MOV EBX,EDX.
    void chosen(const Kept &kept){this->flags.minus(kept.seat,logic::none,4);this->gp[ebx]=kept.seat;}
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::seat_status::update(memory,*this);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::no_player)this->flags.minus(gp[ebx],logic::none,4);                    // CMP EBX,-1
        if(saved){gp[edi]=this->pop(0x16170du);gp[esi]=this->pop(0x16170eu);gp[ebp]=this->pop(0x16170fu);}
        gp[ebx]=this->pop(0x161710u);
        this->ret(0x161711u,0u);this->publish(0x161711u);
    }
};
template<class State,class Reference,class Status> bool bridge(State &s,Reference reference,Status status) {
    return run_owner(s,0x161610u,0x161711u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Status>(v,c,t,status);});
}
} // namespace halo2::native_seat_status_detail
