#pragma once

namespace halo2::source_objective_native_tu15_detail {
// Whole configuration callbacks. Register/flag operations here describe the
// compatibility boundary; family and eligibility rules use typed kernels.
inline bool family(WorldState& s) {
    if(!admitted(s))return false;
    cursor(s,0x249440u);s.gp[1]=r(s.memory,0x4f3218u,4);
    cursor(s,0x249446u);s.gp[2]=r(s.memory,s.gp[1]+0x180u,4);
    s.gp[0]=std::uint32_t(source_objective_family(std::bit_cast<std::int32_t>(s.gp[2])));
    s.flags.minus(s.gp[2],9,4);cursor(s,0x24945bu);s.pop();
    ++source_objective_native_tu15_stats.owners[0];return true;
}
template<class State,class Dispatch>bool selected_team(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    cursor(s,0x24a6c0u);s.gp[0]=r(s.memory,0x4ecc20u,4);
    cursor(s,0x24a6c5u);const auto flags=r(s.memory,s.gp[0]+0x12u,1);
    s.flags.logic(flags&8u,1);
    if(flags&8u) {
        s.gp[1]=0xffffffffu;cursor(s,0x24a6d0u);
        dispatch(s,0x164930u,0x24a6d0u,true);
    } else {
        cursor(s,0x24a6d5u);s.gp[1]=r(s.memory,0x52d710u,4);
        cursor(s,0x24a6dbu);s.gp[0]=r(s.memory,s.gp[1]+0x1f4u,4);
        cursor(s,0x24a6e1u);s.pop();
    }
    ++source_objective_native_tu15_stats.owners[1];return true;
}
inline bool team_available(WorldState& s) {
    if(!admitted(s))return false;
    cursor(s,0x24b390u);s.gp[0]=r(s.memory,0x52d710u,4);
    cursor(s,0x24b395u);s.gp[1]=r(s.memory,s.gp[4]+4u,4);s.gp[2]=0xffffffffu;
    cursor(s,0x24b39cu);const auto spawn=signed_half(r(s.memory,s.gp[0]+2u+s.gp[1]*2u,2));
    s.flags.minus(std::uint16_t(spawn),0xffffu,2);
    auto goal=std::int32_t(-1);
    if(spawn==-1) {
        cursor(s,0x24b3a3u);goal=signed_half(r(s.memory,s.gp[0]+0x14u+s.gp[1]*2u,2));
        s.flags.minus(std::uint16_t(goal),0xffffu,2);
    }
    const auto available=source_objective_team_has_marker(std::int16_t(spawn),std::int16_t(goal));
    s.gp[0]=(s.gp[0]&0xffffff00u)|std::uint32_t(available);
    if(!available)s.flags.logic(0,1);
    cursor(s,available?0x24b3b1u:0x24b3acu);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[2];return true;
}
inline bool default_relationship(WorldState& s) {
    if(!admitted(s))return false;
    cursor(s,0x24b2f0u);s.gp[1]=r(s.memory,s.gp[4]+4u,4);
    s.gp[0]=(s.gp[0]&0xffffff00u)|std::uint32_t(
        source_objective_uses_default_relationship(std::bit_cast<std::int32_t>(s.gp[1])));
    s.flags.logic(s.gp[1],4);cursor(s,0x24b2fcu);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[3];return true;
}
template<class State,class Dispatch>bool team_eligible(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24cd70u,s.gp[3]);s.gp[3]&=0xffffff00u;
    const auto team=std::bit_cast<std::int32_t>(s.gp[6]);s.flags.logic(s.gp[6],4);
    const auto finish=[&](std::uint32_t restore,std::uint32_t ret) {
        cursor(s,restore);s.gp[3]=s.pop();cursor(s,ret);s.pop();
        ++source_objective_native_tu15_stats.owners[9];return true;
    };
    if(team<0 || team>=8) {
        s.flags.minus(s.gp[6],8,4);
        bool eligible{};
        if(team==8) {
            s.gp[2]=read(s,0x24cddcu,0x4f3218u);const auto kind=read(s,0x24cde2u,s.gp[2]+0x240u);
            s.flags.minus(kind,2,4);eligible=source_objective_team_eligible(team,std::bit_cast<std::int32_t>(kind),0,false,0);
        }
        s.gp[0]=(s.gp[0]&0xffffff00u)|std::uint32_t(eligible);
        return finish(team==8?0x24cdecu:0x24cdf0u,team==8?0x24cdedu:0x24cdf1u);
    }
    s.flags.minus(s.gp[6],8,4);push(s,0x24cd7cu,s.gp[5]);s.gp[5]=read(s,0x24cd7du,0x4f3218u);
    push(s,0x24cd83u,s.gp[7]);s.gp[7]=read(s,0x24cd84u,s.gp[5]+0x240u);
    const auto kind=std::bit_cast<std::int32_t>(s.gp[7]);s.flags.minus(s.gp[7],2,4);
    bool active{};
    if(kind!=2) {
        s.gp[1]=s.gp[6];call(s,dispatch,0x164890u,0x24cd91u,0x24cd96u);
        active=(s.gp[0]&255u)!=0;s.flags.logic(s.gp[0]&255u,1);
    }
    if(kind==2 || !active) {
        cursor(s,0x24cdf2u);s.gp[7]=s.pop();cursor(s,0x24cdf3u);s.gp[5]=s.pop();s.gp[0]&=0xffffff00u;
        return finish(0x24cdf6u,0x24cdf7u);
    }
    s.flags.minus(s.gp[7],1,4);
    if(kind!=1) {
        cursor(s,0x24cdd1u);s.gp[7]=s.pop();cursor(s,0x24cdd2u);s.gp[5]=s.pop();
        s.gp[0]=(s.gp[0]&0xffffff00u)|1u;return finish(0x24cdd5u,0x24cdd6u);
    }
    const auto engine=read(s,0x24cd9fu,s.gp[5]+0x180u);s.flags.minus(engine,9,4);
    std::uint32_t selected{};
    if(engine==9) {
        s.gp[0]=read(s,0x24cda8u,0x52d710u);s.gp[1]=read(s,0x24cdadu,s.gp[0]+0x1f4u);selected=s.gp[1];
        cursor(s,0x24cdb3u);s.gp[7]=s.pop();s.flags.minus(s.gp[6],selected,4);
        cursor(s,0x24cdb6u);s.gp[5]=s.pop();
    } else {
        s.gp[1]=read(s,0x24cdbcu,0x52d710u);s.gp[0]=read(s,0x24cdc2u,s.gp[1]+0x1f4u);selected=s.gp[0];
        cursor(s,0x24cdc8u);s.gp[7]=s.pop();s.flags.minus(s.gp[6],selected,4);
        cursor(s,0x24cdcbu);s.gp[5]=s.pop();
    }
    s.gp[0]=(s.gp[0]&0xffffff00u)|std::uint32_t(source_objective_team_eligible(
        team,kind,std::bit_cast<std::int32_t>(engine),active,std::bit_cast<std::int32_t>(selected)));
    return finish(engine==9?0x24cdbau:0x24cdcfu,engine==9?0x24cdbbu:0x24cdd0u);
}
} // namespace halo2::source_objective_native_tu15_detail
