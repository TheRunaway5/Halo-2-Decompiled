#pragma once
// Boundary adapter for the respawn wait, 1601B0
// (native_tu15_respawn_wait_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the player (RET), the answer
// in AL; a 0x20-byte frame (the iterator's state, the counts, the player's
// index, the team flag) below the saved EBX, ESI and EDI (and EBP past the
// unit test); 1A8E50 (ESI the iterator) and 160570 (ECX a player) direct,
// every register each returns coming back. Each access publishes its
// original instruction.
// HALO2_REFERENCE_RESPAWN_WAIT=1 restores the translated body.
// HALO2_VERIFY_RESPAWN_WAIT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/players/native_tu15_respawn_wait_systems.hpp"

namespace halo2::native_respawn_wait_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_respawn_wait;
using Access=logic::respawn_wait::Access;
using Kept=logic::respawn_wait::Kept;
using Outcome=logic::respawn_wait::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RESPAWN_WAIT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RESPAWN_WAIT");return v;}
enum Branch : unsigned {unlimited, alive, full, queued, admitted, team_game, set_aside, tied, branch_count};
static_assert(unsigned(Outcome::admitted)==Branch::admitted);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Respawn wait"};return v;}
// Each role's instruction: the living pass, then the dead pass (0 where a role is in one only).
inline constexpr std::uint32_t sites[][2]{{0x1601b3u,0},{0x1601b9u,0},{0x1601c0u,0},{0x1601c7u,0},{0x1601d3u,0},{0x1601e9u,0},
    {0x1601feu,0x16027du},{0x160202u,0x160296u},{0x160206u,0x16029au},{0x160213u,0x1602b0u},{0x160219u,0x1602b5u},{0x16021fu,0x1602bbu},
    {0x160228u,0x1602c4u},{0x16022eu,0x1602cau},{0x160234u,0x1602d0u},{0x16023du,0x1602d8u},{0x160243u,0x1602deu},{0x160249u,0x1602e4u},
    {0x160251u,0x1602ecu},{0x160269u,0},{0x16026fu,0},{0,0x16028au},{0,0x16028eu},{0,0x1602f5u},{0,0x160304u},{0,0x16030au},{0,0x160316u},
    {0,0x160324u},{0,0x160324u},{0,0x160339u},{0,0x16033du}};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::ahead_final)+1u);
template<class State,class Next,class Aside> struct Adapter : Port<State> {
    Next &next_child;
    Aside &aside_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned pass{},calls[2]{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Next &n,Aside &a):Port<State>(s,c,t,{}),next_child(n),aside_child(a){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        // The stores to the iterator start the dead pass: its array is stored before its first call.
        if(a==Access::free_store)pass=1u;
        return sites[unsigned(a)][a==Access::array_store?(calls[0]?1u:0u):pass];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        const bool dead=pass==1u;
        switch(a) {
        case game:gp[ecx]=v;break;
        case limit:case unit:case free_load:gp[edx]=v;break;
        case players_array:gp[ebx]=v;break;
        case players_data:gp[esi]=v;break;
        case engine_globals:gp[dead?eax:edx]=v;break;
        case engine_index:gp[dead?ecx:eax]=v;break;
        case engine:gp[dead?eax:ecx]=v;break;
        case current:gp[dead?esi:eax]=v;break;
        case game_again:gp[dead?edx:ecx]=v;break;
        case variant_flags:set_low_byte(gp[dead?eax:edx],v);break;
        case own_team:set_low_byte(gp[ecx],v);break;
        case game_final:gp[edx]=v;break;
        case limit_final:case other_order:case own_index:gp[eax]=v;break;
        case handle:gp[ebp]=v;gp[ecx]=v;break;                                                          // MOV EBP,[ESP+28]; MOV ECX,EBP
        case own_order:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(unsigned n) {
        auto &gp=this->gp;
        switch(n) {
        case 0u:put(0x1601bfu,gp[ebx]);break;                                                         // PUSH EBX
        case 1u:put(0x1601c6u,gp[esi]);break;                                                         // PUSH ESI
        case 2u:put(0x1601cau,gp[edi]);gp[edi]=this->flags.logic(gp[eax]&0xffffu,4);break;            // PUSH EDI; MOV EDI,EAX; AND
        default:put(0x1601f7u,gp[ebp]);gp[ebp]=this->flags.logic(0u,4);break;                          // PUSH EBP; XOR EBP,EBP
        }
    }
    void limit_tested(std::uint32_t index,std::uint32_t data,std::uint32_t limit_value) {              // IMUL EDI,21C; ADD EDI,ESI; XOR AL,AL; TEST EDX,EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edi]=fl.plus(fl.product(index,logic::player_size),data,4);
        set_low_byte(gp[eax],fl.logic(0u,1));this->test32(limit_value);
    }
    void unit_tested(std::uint32_t unit_value){this->gp[ecx]=this->flags.logic(logic::none,4);this->compare32(unit_value,logic::none);}   // OR ECX,-1; CMP
    std::uint32_t next(unsigned which,Kept &kept) {                                                    // LEA ESI,[ESP+20]; CALL 1A8E50; TEST AL,AL
        auto &gp=this->gp;
        pass=which;
        static constexpr std::uint32_t at[2][2]{{0x16020au,0x160260u},{0x16029eu,0x16032cu}};
        const auto site=at[which][calls[which]++?1u:0u];
        gp[esi]=local(logic::local::state);
        this->call(site,site+5u,0x1a8e50u,next_child);
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax];
    }
    void engine_tested(std::uint32_t engine_value){this->test32(engine_value);}
    void teams_tested(std::uint32_t teams) {                                                           // AND DL,1 (AL)
        set_low_byte(this->gp[pass?eax:edx],this->flags.logic(teams,1));
        if(teams)this->reached(Branch::team_game);
    }
    void teams_compared(std::uint32_t own,std::uint32_t other){this->flags.minus(own&0xffu,other&0xffu,1);}   // CMP CL,[..+C0]
    void unit_compared(std::uint32_t unit_value,std::uint32_t against){this->compare32(unit_value,against);}
    void self_compared(std::uint32_t current_address,std::uint32_t own){this->compare32(current_address,own);}   // CMP EAX(ESI),EDI
    void counted(const Kept &kept){this->gp[ebp]=this->flags.increment(kept.count-1u,4,false);}       // INC EBP
    void full_tested(std::uint32_t limit_value,std::uint32_t living){this->compare32(limit_value,living);}   // CMP EAX,EBP
    void freeing(std::uint32_t limit_value,const Kept &kept) {                                         // SUB EAX,EBP; OR EBX,-1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.minus(limit_value,kept.count,4);gp[ebx]=fl.logic(kept.held,4);
    }
    std::uint32_t set_aside(std::uint32_t handle_value,Kept &kept) {                                   // MOV ECX,EBP; CALL 160570; TEST AL,AL
        auto &gp=this->gp;
        gp[ebp]=handle_value;gp[ecx]=handle_value;
        this->call(0x1602fbu,0x160300u,0x160570u,aside_child);
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::set_aside);
        return gp[eax];
    }
    void orders_compared(std::uint32_t other,std::uint32_t own){this->compare32(other,own);}         // CMP EAX,ECX
    void indices_compared(std::uint32_t own_index,std::uint32_t handle_value) {                        // AND EBP,FFFF; CMP EAX,EBP
        auto &gp=this->gp;
        gp[ebp]=this->flags.logic(handle_value&0xffffu,4);this->compare32(own_index,gp[ebp]);
        this->reached(Branch::tied);
    }
    std::uint32_t incremented(std::uint32_t ahead){return this->flags.increment(ahead,4,false);}       // INC dword [ESP+14]
    void ahead_compared(std::uint32_t ahead,std::uint32_t free_value){this->compare32(ahead,free_value);}   // CMP [ESP+14],EDX
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x20u,4);depth=0x20u;                                                   // SUB ESP,20
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::respawn_wait::decide(memory,*this,gp[eax],outcome);
        this->reached(unsigned(outcome));
        std::uint32_t at=0x160350u;
        if(outcome==Outcome::full || outcome==Outcome::queued)gp[ebp]=take(0x16034fu);                 // MOV AL,1; POP EBP
        if(outcome==Outcome::admitted) {                                                                 // POP EBP, EDI, ESI; XOR AL,AL; POP EBX
            gp[ebp]=take(0x160343u);gp[edi]=take(0x160344u);gp[esi]=take(0x160345u);
            set_low_byte(gp[eax],fl.logic(answer,1));gp[ebx]=take(0x160348u);at=0x160349u;
        } else {set_low_byte(gp[eax],answer);gp[edi]=take(at);gp[esi]=take(at+1u);gp[ebx]=take(at+2u);at+=3u;}
        gp[esp]=fl.plus(gp[esp],0x20u,4);                                                                // ADD ESP,20
        this->ret(at+3u,0u);this->publish(at+3u);
    }
};
template<class State,class Reference,class Next,class Aside> bool bridge(State &s,Reference reference,Next next,Aside aside) {
    return run_owner(s,0x1601b0u,0x160356u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Next,Aside>(v,c,t,next,aside);});
}
} // namespace halo2::native_respawn_wait_detail
