#pragma once
// Boundary adapter for the team active query, 164890
// (native_tu15_team_active_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: ECX the team (RET), the answer in AL; ECX (its top
// byte the team flag), ESI and EDI saved. Each access publishes its original
// instruction.
// HALO2_REFERENCE_TEAM_ACTIVE=1 restores the translated body.
// HALO2_VERIFY_TEAM_ACTIVE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_team_active_systems.hpp"

namespace halo2::native_team_active_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_team_active;
using Access=logic::team_active::Access;
using Outcome=logic::team_active::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_TEAM_ACTIVE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_TEAM_ACTIVE");return v;}
enum Branch : unsigned {no_engine, no_teams, out_of_range, answered, active, branch_count};
static_assert(unsigned(Outcome::answered)==Branch::answered);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Team active"};return v;}
inline constexpr std::uint32_t sites[]{0x164892u,0x164898u,0x16489fu,0x1648acu,0x1648b2u,0x1648bbu,0x1648cau};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::team_mask)+1u);
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case engine_globals:gp[esi]=v;break;
        case engine_index:case game:gp[edx]=v;break;
        case engine:gp[edi]=v;break;
        case variant_flags:set_low_byte(gp[edx],v);break;
        case team_mask:gp[eax]=v&0xffffu;break;                                                         // MOVZX EAX,word [ESI+C]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){put(0x164890u,gp[ecx]);put(0x164891u,gp[esi]);}                                      // PUSH ECX; PUSH ESI
        else put(0x16489eu,gp[edi]);                                                                     // PUSH EDI
    }
    void engine_tested(std::uint32_t engine_value){set_low_byte(this->gp[eax],this->flags.logic(0u,1));this->test32(engine_value);}   // XOR AL,AL; TEST EDI,EDI
    void teams_tested(std::uint32_t teams){set_low_byte(this->gp[edx],this->flags.logic(teams,1));}   // AND DL,1
    void range_tested(std::uint32_t team,bool high) {                                                  // TEST ECX,ECX; CMP ECX,8
        if(high)this->compare32(team,logic::team_count);else this->test32(team);
    }
    std::uint32_t masked(std::uint32_t team,std::uint32_t mask) {                                      // MOV EDX,1; SHL EDX,CL; TEST EDX,EAX; SETNZ AL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.shift(1u,team&0xffu,4);
        const auto active_bit=fl.logic(gp[edx]&mask,4)!=0u;
        if(active_bit)this->reached(Branch::active);
        return active_bit?1u:0u;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::team_active::query(memory,*this,gp[ecx],outcome);
        this->reached(unsigned(outcome));
        set_low_byte(gp[eax],answer);
        gp[edi]=take(0x1648dau);gp[esi]=take(0x1648dbu);gp[ecx]=take(0x1648dcu);
        this->ret(0x1648ddu,0u);this->publish(0x1648ddu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x164890u,0x1648ddu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_team_active_detail
