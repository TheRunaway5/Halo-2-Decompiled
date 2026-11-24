#pragma once
// Boundary adapter for the score lead, 15BE80
// (native_tu15_score_lead_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns: the player and the two outputs on
// the stack (RET C); a 0x14-byte frame (the tie, the team-game byte, the
// iterator) below the saved EBX, EBP, ESI and EDI; 15B210 (EDI the team),
// 1648E0 (ECX the team) and 1A8E50 (ESI the iterator) through the dispatch,
// every register each returns coming back. Each access publishes its
// original instruction.
// HALO2_REFERENCE_SCORE_LEAD=1 restores the translated body.
// HALO2_VERIFY_SCORE_LEAD=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_score_lead_systems.hpp"

namespace halo2::native_score_lead_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_score_lead;
using Access=logic::score_lead::Access;
using Kept=logic::score_lead::Kept;
using Outcome=logic::score_lead::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SCORE_LEAD");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SCORE_LEAD");return v;}
enum Branch : unsigned {team_lead, team_trailing, player_lead, player_trailing, tie, branch_count};
static_assert(unsigned(Outcome::player_trailing)==Branch::player_trailing);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Score lead"};return v;}
inline constexpr std::uint32_t sites[]{0x15be83u,0x15be89u,0x15be9eu,0x15bea1u,0x15bea7u,0x15beadu,0x15beb6u,0x15bebdu,0x15bec3u,
    0x15beccu,0x15bed2u,0x15bee2u,0u,0x15bf44u,0x15bf50u,0x15bf54u,0x15bf58u,0x15bf65u,0x15bf69u,0x15bf6fu,0x15bf75u,0x15bf94u};
// Per exit (the teams' lead, the players' lead, trailing): the outputs' loads and stores, the pops.
inline constexpr std::uint32_t exit_sites[3][9]{
    {0x15bf16u,0x15bf1au,0x15bf1eu,0x15bf25u,0x15bf28u,0x15bf22u,0x15bf2au,0x15bf2eu,0u},
    {0x15bfb3u,0x15bfb7u,0x15bfbbu,0x15bfc2u,0x15bfc5u,0x15bfbfu,0x15bfc7u,0x15bfcbu,0u},
    {0x15bfceu,0u,0x15bfd2u,0x15bfd9u,0x15bfdcu,0x15bfd6u,0x15bfdfu,0x15bfe3u,0u}};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    unsigned exit{},scores_calls{},walks{};
    bool walking{},team_path{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case tied_store:return team_path?0x15bf01u:0x15bf9du;
        case leading_out:return exit_sites[exit][0];
        case tied_load:return exit_sites[exit][1];
        case tied_out:return exit_sites[exit][2];
        case leading_store:return exit_sites[exit][3];
        case tied_out_store:return exit_sites[exit][4];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case player_argument:case player_team_again:case handle_load:gp[eax]=a==player_team_again?logic::widen8(v):v;break;
        case players_array:gp[ebp]=v;break;
        case player_data:gp[esi]=fl.plus(gp[esi],v,4);break;                                          // ADD ESI,[EBP+44]
        case session_address:gp[edi]=v;break;
        case variant_index:case variant_entry:gp[ecx]=v;break;
        case game_address:gp[edx]=v;break;
        case mode_flags:set_low_byte(gp[edx],v);break;
        case player_team:gp[edi]=logic::widen8(v);break;
        case own_score:gp[ebx]=logic::widen16(v);break;
        case variant_index_again:gp[edx]=v;break;
        case variant_entry_again:gp[esi]=v;break;
        case other_score:gp[ecx]=logic::widen16(v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t){if(a==Access::tied_store)this->reached(Branch::tie);}
    void saving(unsigned step) {
        auto &gp=this->gp;auto &fl=this->flags;
        if(step==0u){put(0x15be87u,gp[ebx]);put(0x15be88u,gp[ebp]);}                                  // PUSH EBX; PUSH EBP
        else if(step==1u){put(0x15be8fu,gp[esi]);gp[eax]=fl.logic(gp[eax]&0xffffu,4);gp[esi]=fl.product(gp[eax],logic::player_size);}   // PUSH ESI; AND; IMUL
        else put(0x15be9du,gp[edi]);                                                                   // PUSH EDI
    }
    void variant_tested(std::uint32_t record){this->test32(record);}                                   // TEST ECX,ECX
    void team_tested(std::uint32_t flag_value){set_low_byte(this->gp[edx],this->flags.logic(flag_value,1));}   // AND DL,1
    std::uint32_t team_score(Kept &kept) {                                                             // CALL 15B210 (EDI the team)
        auto &gp=this->gp;
        team_path=true;
        gp[edi]=kept.walker;
        const std::uint32_t pc=scores_calls++?0x15bef8u:0x15bed9u;
        auto child=[&](State &v){dispatch(v,0x15b210u,pc);};
        this->call(pc,pc+5u,0x15b210u,child);
        refresh(kept);
        return gp[eax];
    }
    void teams_started(const Kept &kept){auto &gp=this->gp;gp[ebp]=kept.held;gp[edi]=this->flags.logic(kept.walker,4);}   // MOV EBP,EAX; XOR EDI,EDI
    void compared(std::uint32_t team,std::uint32_t own){this->compare32(team,own);}                   // CMP EDI,EAX
    std::uint32_t active(Kept &kept) {                                                                 // MOV ECX,EDI; CALL 1648E0; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=kept.walker;
        auto child=[&](State &v){dispatch(v,0x1648e0u,0x15beefu);};
        this->call(0x15beefu,0x15bef4u,0x1648e0u,child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);
        return gp[eax];
    }
    void difference(std::uint32_t diff,std::uint32_t base) {                                           // SUB EAX,EBP (TEST EAX) or SUB ECX,EBX; TEST ECX
        auto &gp=this->gp;
        const unsigned reg=team_path?eax:ecx;
        gp[reg]=this->flags.minus(diff+base,base,4);
        if(!team_path || diff)this->test32(diff);
    }
    void advanced(std::uint32_t team){this->gp[edi]=this->flags.increment(team-1u,4,false);this->compare32(team,logic::team_count);}
    // XOR EBX,EBX (or XOR ECX,ECX); TEST ECX,ECX (TEST ESI,ESI).
    void score_tested(std::uint32_t record) {
        auto &gp=this->gp;
        if(!walking)gp[ebx]=this->flags.logic(0u,4);else gp[ecx]=this->flags.logic(0u,4);
        this->test32(record);
    }
    void scores_tested(std::uint32_t scores,std::uint32_t index) {                                    // LEA ECX (EDX),[EDI+304]; TEST; (AND EAX,FFFF); IMUL EAX,1C
        auto &gp=this->gp;auto &fl=this->flags;
        gp[walking?edx:ecx]=scores;this->test32(scores);
        if(scores){if(walking)fl.logic(index,4);gp[eax]=fl.product(index,logic::score_size);}
    }
    void iterating(const Kept &){auto &gp=this->gp;gp[eax]=this->flags.logic(0xffffffffu,4);gp[esi]=local(logic::local::iterator);walking=true;}   // OR EAX,-1; LEA ESI
    std::uint32_t next(Kept &kept) {                                                                   // (LEA ESI,[ESP+14]); CALL 1A8E50; TEST AL,AL
        auto &gp=this->gp;
        const std::uint32_t pc=walks++?0x15bfaau:0x15bf5cu;
        gp[esi]=local(logic::local::iterator);
        auto child=[&](State &v){dispatch(v,0x1a8e50u,pc);};
        this->call(pc,pc+5u,0x1a8e50u,child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);
        return gp[eax];
    }
    void player_compared(std::uint32_t handle,std::uint32_t player_now){this->compare32(handle,player_now);}   // CMP EAX,[ESP+28]
    void leaving(unsigned which,std::uint32_t leading_address,std::uint32_t tied_address,std::uint32_t tie_value) {
        auto &gp=this->gp;
        exit=which;
        gp[eax]=leading_address;
        if(which<2u){set_low_byte(gp[ecx],tie_value);gp[edx]=tied_address;}else gp[ecx]=tied_address;
        gp[edi]=take(exit_sites[which][5]);gp[esi]=take(exit_sites[which][5]+1u);gp[ebp]=take(exit_sites[which][5]+2u);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x14u,4);depth=0x14u;                                                 // SUB ESP,14
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::score_lead::lead(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]})));
        gp[ebx]=take(exit_sites[exit][6]);
        gp[esp]=fl.plus(gp[esp],0x14u,4);                                                              // ADD ESP,14
        this->ret(exit_sites[exit][7],0xcu);this->publish(exit_sites[exit][7]);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x15be80u,0x15bfe3u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_score_lead_detail
