#pragma once
// Boundary adapter for the round end, 15B740
// (native_tu15_round_end_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns: EDI the winner's output and
// teams_down on the stack (RET 4), the answer in AL; a 0x10-byte frame below
// the saved EBX and EBP (ESI saved around the counts); 15B4D0 and 15B620 (the
// three outputs pushed, RET C) direct, 161530 through the dispatch, every
// register each returns coming back. Each access publishes its original
// instruction.
// HALO2_REFERENCE_ROUND_END=1 restores the translated body.
// HALO2_VERIFY_ROUND_END=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_round_end_systems.hpp"

namespace halo2::native_round_end_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_round_end;
using Access=logic::round_end::Access;
using Kept=logic::round_end::Kept;
using Outcome=logic::round_end::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ROUND_END");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ROUND_END");return v;}
enum Branch : unsigned {no_variant, not_down, not_single, no_last, player_wins, team_down, other_mode, no_option, other_team, team_none,
    many, few, out_written, branch_count};
static_assert(unsigned(Outcome::team_none)==Branch::team_none);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Round end"};return v;}
inline constexpr std::uint32_t sites[]{0x15b745u,0x15b74bu,0x15b751u,0x15b75cu,0x15b768u,0x15b78fu,0x15b793u,0x15b79cu,0x15b7b5u,
    0x15b7c2u,0x15b7d4u,0x15b7dau,0x15b7ebu,0x15b7f3u,0x15b7f7u,0x15b7fdu,0x15b80du,0x15b816u,0x15b81eu,0x15b826u,0x15b829u,
    0x15b831u,0x15b833u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::answer_final)+1u);
template<class State,class Players,class Count,class Dispatch> struct Adapter : Port<State> {
    Players &players_child;
    Count &count_child;
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    bool many_players{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Players &p,Count &k,Dispatch &d)
        :Port<State>(s,c,t,{}),players_child(p),count_child(k),dispatch(d){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx]&0xffu,this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::answer_store)return many_players?0x15b79cu:0x15b7abu;
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case session_address:gp[ebp]=v;break;
        case variant_index:case living_load:case out_argument:gp[eax]=v;break;
        case variant_entry:case players_header:case game_mode:gp[ecx]=v;break;
        case teams_load:this->compare32(gp[esi],logic::many_players);gp[ecx]=v;break;                  // CMP ESI,1; MOV ECX,[ESP+14]
        case answer_load:set_low_byte(gp[ecx],v);break;
        case last_load:gp[esi]=v;break;
        case player_datums:case game_address:gp[edx]=v;break;
        case player_team:gp[eax]=logic::widen8(v);break;
        case team_option:fl.minus(v,1u,4);break;                                                       // CMP [EDX+240],1
        case session_team:fl.minus(v,gp[eax],4);break;                                                 // CMP [EBP+2F0],EAX
        case answer_final:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t){if(a==Access::out_store)this->reached(Branch::out_written);}
    void cleared(const Kept &kept){set_low_byte(this->gp[ebx],this->flags.logic(kept.teams_down,1));}  // XOR BL,BL
    void variant_tested(std::uint32_t record){this->test32(record);}                                   // TEST ECX,ECX
    void saving(){put(0x15b767u,this->gp[esi]);}
    void restoring(){this->gp[esi]=take(0x15b828u);}
    std::uint32_t players(Kept &kept) {
        this->call(0x15b770u,0x15b775u,0x15b4d0u,players_child);
        refresh(kept);
        return this->gp[eax];
    }
    // LEA ECX,[ESP+14]; PUSH ECX; LEA EDX,[ESP+14]; MOVSX ESI,AX; PUSH EDX; LEA EAX,[ESP+20]; PUSH EAX; CALL 15B620.
    void count(std::uint32_t last,std::uint32_t living,std::uint32_t teams,Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=teams;put(0x15b779u,gp[ecx]);gp[edx]=living;gp[esi]=kept.held;put(0x15b781u,gp[edx]);gp[eax]=last;put(0x15b786u,gp[eax]);
        this->call(0x15b787u,0x15b78cu,0x15b620u,count_child);depth-=0xcu;
        refresh(kept);
    }
    void answered(std::uint32_t players_count,std::uint32_t living,std::uint32_t) {                   // CMP EAX,2 or TEST EAX,EAX
        many_players=logic::signed_value(players_count)>logic::signed_value(logic::many_players);
        this->reached(many_players?Branch::many:Branch::few);
        if(many_players)this->compare32(living,logic::sides);else this->test32(living);
    }
    void teams_tested(std::uint32_t,std::uint32_t teams,const Kept &kept) {                            // CMP ECX,2 or TEST ECX,ECX; SETcc BL
        if(many_players)this->compare32(teams,logic::sides);else this->test32(teams);
        set_low_byte(this->gp[ebx],kept.teams_down);
    }
    void answer_tested(std::uint32_t answer,std::uint32_t living){this->test8(answer&0xffu);if(answer&0xffu)this->compare32(living,1u);}
    void last_tested(std::uint32_t last){this->compare32(last,logic::none);}
    std::uint32_t by_team(Kept &kept) {
        auto child=[&](State &v){dispatch(v,0x161530u,0x15b7cbu);};
        this->call(0x15b7cbu,0x15b7d0u,0x161530u,child);
        refresh(kept);
        this->test8(this->gp[eax]&0xffu);                                                              // TEST AL,AL
        return this->gp[eax];
    }
    void team_read(const Kept &kept) {                                                                 // AND ESI,FFFF; IMUL ESI,21C; TEST BL,BL
        auto &fl=this->flags;
        this->gp[esi]=fl.product(fl.logic(kept.held&0xffffu,4),logic::player_size);this->test8(kept.teams_down&0xffu);
    }
    void mode_tested(std::uint32_t mode){this->compare32(mode,logic::mode_a);if(mode!=logic::mode_a)this->compare32(mode,logic::mode_b);}
    void out_tested(std::uint32_t out){this->test32(out);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x10u,4);depth=0x10u;                                                 // SUB ESP,10
        put(0x15b743u,gp[ebx]);put(0x15b744u,gp[ebp]);
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::round_end::check(memory,*this,Kept{gp[ebx]&0xffu,gp[ebp],gp[esi],gp[edi]})));
        gp[ebp]=take(0x15b837u);gp[ebx]=take(0x15b838u);
        gp[esp]=fl.plus(gp[esp],0x10u,4);                                                              // ADD ESP,10
        this->ret(0x15b83cu,4u);this->publish(0x15b83cu);
    }
};
template<class State,class Reference,class Players,class Count,class Dispatch>
bool bridge(State &s,Reference reference,Players players,Count count,Dispatch dispatch) {
    return run_owner(s,0x15b740u,0x15b83cu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Players,Count,Dispatch>(v,c,t,players,count,dispatch);});
}
} // namespace halo2::native_round_end_detail
