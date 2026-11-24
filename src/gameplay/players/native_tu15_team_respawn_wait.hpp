#pragma once
// Boundary adapter for the team respawn wait, 160360
// (native_tu15_team_respawn_wait_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the player (RET), the
// answer in AL; a 0x28-byte frame (the iterator's state, the team counts, the
// share, the player's index, the flag byte) below the saved EBP and EDI (ESI
// and EBX as the tests pass); 1A8E50 (ESI the iterator), 160570 (ECX a
// player), 1601B0 (EAX a player) and 164890 (ECX a team) direct, every
// register each returns coming back. Each access publishes its original
// instruction.
// HALO2_REFERENCE_TEAM_RESPAWN_WAIT=1 restores the translated body.
// HALO2_VERIFY_TEAM_RESPAWN_WAIT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/players/native_tu15_team_respawn_wait_systems.hpp"

namespace halo2::native_team_respawn_wait_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_team_respawn_wait;
using Access=logic::team_respawn_wait::Access;
using Kept=logic::team_respawn_wait::Kept;
using Outcome=logic::team_respawn_wait::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_TEAM_RESPAWN_WAIT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_TEAM_RESPAWN_WAIT");return v;}
enum Branch : unsigned {unbalanced, no_engine, no_teams, alive, even, full, queued, admitted, set_aside, waiting, tied, branch_count};
static_assert(unsigned(Outcome::admitted)==Branch::admitted);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Team respawn wait"};return v;}
// Each role's instruction (the counting pass, then the queue pass where the role is in both).
inline constexpr std::uint32_t sites[][2]{{0x160363u,0},{0x160369u,0},{0x160370u,0},{0x160379u,0},{0x160382u,0},{0x16039cu,0},
    {0x1603a2u,0},{0x1603a9u,0},{0x1603b8u,0},{0x1603c1u,0},{0x1603cbu,0},{0x1603dcu,0},{0x1603e4u,0},{0x1603e9u,0},
    {0x1603edu,0x1604b5u},{0x1603f1u,0x1604b9u},{0x1603f5u,0x1604bdu},{0x160402u,0x1604d0u},{0x160406u,0},{0x16040fu,0},{0x160414u,0},
    {0x160431u,0},{0x160438u,0},{0x160442u,0},{0x160456u,0},{0x16046bu,0},{0x160470u,0},{0x160476u,0},{0x160480u,0},{0x16048du,0},
    {0x160491u,0},{0x160498u,0},{0x16049du,0},{0x1604adu,0},{0x1604b1u,0},{0x1604d4u,0},{0x1604dau,0},{0x1604e6u,0},{0x1604ecu,0},
    {0x160502u,0},{0x160502u,0},{0x160508u,0},{0x16050eu,0},{0x16051au,0},{0x160528u,0},{0x160528u,0},{0x160539u,0},{0x16053du,0},
    {0x160545u,0},{0x160555u,0}};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::flag_load)+1u);
template<class State,class Next,class Aside,class Waits,class Admits> struct Adapter : Port<State> {
    Next &next_child;
    Aside &aside_child;
    Waits &waits_child;
    Admits &admits_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned passes{},calls[2]{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Next &n,Aside &a,Waits &w,Admits &m)
        :Port<State>(s,c,t,{}),next_child(n),aside_child(a),waits_child(w),admits_child(m){}
    unsigned pass()const{return passes>1u?1u:0u;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){const auto &row=sites[unsigned(a)];return row[1] && pass()?row[1]:row[0];}
    void loaded(Access a,std::uint32_t address,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case game:case unit:case share_again:gp[ecx]=v;break;
        case variant_flags:case engine_globals:case engine_index:gp[edx]=v;break;
        case players_array:case teammate_unit:gp[ebp]=v;break;
        case players_data:case own_order:case own_index:case share_final:gp[eax]=v;break;
        case engine:case current:case other_order:gp[esi]=v;break;
        case teams_flags:set_low_byte(gp[ecx],v);break;
        case handle:gp[ebx]=v;gp[ecx]=v;break;                                                          // MOV EBX,[ESP+30]; MOV ECX,EBX
        case teammate_handle:gp[ebx]=v;break;
        case count_team:case team_size:case own_team:gp[eax]=logic::widen8(v);break;                   // MOVSX EAX,byte
        case count_load:set_low_byte(gp[ecx],v);gp[eax]=address;break;                                 // MOV CL,[ESP+EAX+1C]; LEA EAX
        case own_count:gp[ecx]=logic::widen8(v);break;
        case own_team_again:set_low_byte(gp[edx],v);break;
        case flag_load:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(unsigned n) {
        auto &gp=this->gp;
        switch(n) {
        case 0u:put(0x16036fu,gp[ebp]);break;                                                         // PUSH EBP
        case 1u:put(0x160376u,gp[edi]);gp[edi]=gp[eax];break;                                         // PUSH EDI; MOV EDI,EAX
        case 2u:put(0x1603a8u,gp[esi]);break;                                                         // PUSH ESI
        default:put(0x1603ceu,gp[ebx]);break;                                                         // PUSH EBX
        }
    }
    void indexing(std::uint32_t index){this->gp[edi]=this->flags.logic(index,4);}                      // AND EDI,FFFF
    void balance_tested(std::uint32_t index,std::uint32_t data,std::uint32_t flags) {                  // IMUL; ADD; SHR EDX,E; XOR AL,AL; TEST DL,1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edi]=fl.plus(fl.product(index,logic::player_size),data,4);
        gp[edx]=fl.right(flags,logic::game::balanced_bit,4,false);
        set_low_byte(gp[eax],fl.logic(0u,1));fl.logic(gp[edx]&1u,1);
    }
    void engine_tested(std::uint32_t engine_value){this->test32(engine_value);}
    void teams_tested(std::uint32_t teams){set_low_byte(this->gp[ecx],this->flags.logic(teams,1));}   // AND CL,1
    void unit_tested(std::uint32_t unit_value,const Kept &kept) {                                      // OR EBX,-1; CMP ECX,EBX
        this->gp[ebx]=this->flags.logic(kept.held,4);this->compare32(unit_value,kept.held);
    }
    void counting(const Kept &) {                                                                      // XOR EDX,EDX (XOR EAX,EAX)
        this->gp[passes++?eax:edx]=this->flags.logic(0u,4);
    }
    std::uint32_t next(unsigned which,Kept &kept) {                                                    // LEA ESI,[ESP+28]; CALL 1A8E50; TEST AL,AL
        auto &gp=this->gp;
        static constexpr std::uint32_t at[2][2]{{0x1603f9u,0x16044du},{0x1604c1u,0x160530u}};
        const auto site=at[which][calls[which]++?1u:0u];
        gp[esi]=local(logic::local::state);
        this->call(site,site+5u,0x1a8e50u,next_child);
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax];
    }
    void team_tested(std::uint32_t team){this->flags.minus(team&0xffu,logic::no_team,1);}             // CMP byte [ESI+C0],FF
    void unit_compared(std::uint32_t unit_value,std::uint32_t against){this->compare32(unit_value,against);}
    std::uint32_t set_aside(std::uint32_t handle_value,Kept &kept) {                                   // MOV ECX,EBX; CALL 160570; TEST AL,AL
        auto &gp=this->gp;
        gp[ebx]=handle_value;gp[ecx]=handle_value;
        const auto site=pass()?0x1604f4u:0x16041au;
        this->call(site,site+5u,0x160570u,aside_child);
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::set_aside);
        return gp[eax];
    }
    std::uint32_t waits(std::uint32_t handle_value,Kept &kept) {                                       // MOV EAX,EBX; CALL 1601B0; TEST AL,AL
        auto &gp=this->gp;
        gp[eax]=handle_value;
        this->call(0x160425u,0x16042au,0x1601b0u,waits_child);
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::waiting);
        return gp[eax];
    }
    void released(const Kept &kept){this->gp[ebx]=this->flags.logic(kept.held,4);}                    // OR EBX,-1
    std::uint32_t bumped(std::uint32_t count) {                                                        // INC CL
        const auto v=this->flags.increment(count&0xffu,1,false);set_low_byte(this->gp[ecx],v);return v;
    }
    void scanning(const Kept &kept){this->gp[esi]=this->flags.logic(kept.current,4);}                 // XOR ESI,ESI
    std::uint32_t admitted(std::uint32_t team,Kept &kept) {                                            // MOV ECX,ESI; CALL 164890; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=team;
        this->call(0x160462u,0x160467u,0x164890u,admits_child);
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax];
    }
    void share_compared(std::uint32_t share,std::uint32_t count){this->compare32(share,count);}       // CMP [ESP+14],EAX
    void advanced(const Kept &kept) {                                                                  // INC ESI; CMP ESI,8
        this->gp[esi]=this->flags.increment(kept.current-1u,4,false);this->compare32(kept.current,logic::team_count);
    }
    void floored(std::uint32_t share){this->gp[eax]=logic::share_floor;this->compare32(share,logic::share_floor);}   // MOV EAX,2; CMP ECX,EAX
    void even_compared(std::uint32_t share,std::uint32_t count){this->compare32(share,count);}        // CMP [ESP+14],ECX
    void teams_compared(std::uint32_t own,std::uint32_t other){this->flags.minus(own&0xffu,other&0xffu,1);}   // CMP DL,[ESI+C0]
    void self_compared(std::uint32_t current_address,std::uint32_t own){this->compare32(current_address,own);}   // CMP ESI,EDI
    void living_compared(std::uint32_t unit_value){this->compare32(unit_value,logic::none);}          // CMP EBP,-1
    void orders_compared(std::uint32_t other,std::uint32_t own){this->compare32(other,own);}         // CMP ESI,EAX
    void indices_compared(std::uint32_t own_index,std::uint32_t handle_value) {                        // AND EBX,FFFF; CMP EAX,EBX
        auto &gp=this->gp;
        gp[ebx]=this->flags.logic(handle_value&0xffffu,4);this->compare32(own_index,gp[ebx]);
        this->reached(Branch::tied);
    }
    std::uint32_t incremented(std::uint32_t v){return this->flags.increment(v,4,false);}               // INC dword
    std::uint32_t free_tested(std::uint32_t share,std::uint32_t living) {                              // SUB EAX,[ESP+18]; TEST EAX,EAX
        auto &gp=this->gp;
        gp[eax]=this->flags.minus(share,living,4);this->test32(gp[eax]);return gp[eax];
    }
    void ahead_compared(std::uint32_t ahead,std::uint32_t free_value){this->compare32(ahead,free_value);}   // CMP [ESP+1C],EAX
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x28u,4);depth=0x28u;                                                   // SUB ESP,28
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::team_respawn_wait::decide(memory,*this,gp[eax],outcome);
        this->reached(unsigned(outcome));
        std::uint32_t at;
        if(outcome==Outcome::even || outcome==Outcome::admitted) {                                       // POP EBX, ESI, EDI; XOR AL,AL; POP EBP
            gp[ebx]=take(0x16054bu);gp[esi]=take(0x16054cu);gp[edi]=take(0x16054du);
            set_low_byte(gp[eax],fl.logic(answer,1));gp[ebp]=take(0x160550u);at=0x160551u;
        } else {
            set_low_byte(gp[eax],answer);
            if(outcome!=Outcome::unbalanced && outcome!=Outcome::no_engine && outcome!=Outcome::no_teams)gp[ebx]=take(0x160559u);
            if(outcome!=Outcome::unbalanced)gp[esi]=take(0x16055au);
            gp[edi]=take(0x16055bu);gp[ebp]=take(0x16055cu);at=0x16055du;
        }
        gp[esp]=fl.plus(gp[esp],0x28u,4);                                                                // ADD ESP,28
        this->ret(at+3u,0u);this->publish(at+3u);
    }
};
template<class State,class Reference,class Next,class Aside,class Waits,class Admits>
bool bridge(State &s,Reference reference,Next next,Aside aside,Waits waits,Admits admits) {
    return run_owner(s,0x160360u,0x160560u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Next,Aside,Waits,Admits>(v,c,t,next,aside,waits,admits);});
}
} // namespace halo2::native_team_respawn_wait_detail
