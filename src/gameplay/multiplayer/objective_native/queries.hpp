#pragma once

namespace halo2::source_objective_native_tu15_detail {
inline bool marker_owner(WorldState& s) {
    if(!admitted(s))return false;
    s.gp[1]=read(s,0x24be50u,0x52d710u);s.gp[0]=0;
    s.gp[1]=s.flags.plus(s.gp[1],0x26u,4);push(s,0x24be5bu,s.gp[6]);
    const auto bank=s.gp[1]-0x26u;
    const auto team=source_objective_marker_team(std::bit_cast<std::int32_t>(s.gp[2]),
        [&](std::int32_t index,bool secondary) {
            s.gp[0]=std::uint32_t(index);s.gp[1]=bank+0x26u+std::uint32_t(index)*2u;
            s.gp[6]=std::uint32_t(signed_half(read(s,secondary?0x24be68u:0x24be60u,
                s.gp[1]-(secondary?0u:0x12u),2)));
            s.flags.minus(s.gp[6],s.gp[2],4);
            return std::bit_cast<std::int32_t>(s.gp[6]);
        });
    if(team<0) {
        s.gp[1]=bank+0x38u;s.gp[0]=0xffffffffu;s.flags.logic(s.gp[0],4);
        cursor(s,0x24be7bu);s.gp[6]=s.pop();cursor(s,0x24be7cu);s.pop();
    } else {
        s.flags.minus(std::uint32_t(team),9,4);s.gp[1]=read(s,0x24be82u,0x4f63b8u);
        s.gp[0]=std::uint32_t(signed_half(read(s,0x24be88u,s.gp[1]+0x10u+std::uint32_t(team)*2u,2)));
        cursor(s,0x24be8du);s.gp[6]=s.pop();cursor(s,0x24be8eu);s.pop();
    }
    ++source_objective_native_tu15_stats.owners[4];return true;
}
template<class State,class Dispatch>bool home_marker(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24b4c0u,s.gp[6]);s.gp[6]=0xffffffffu;
    s.flags.minus(s.gp[0],8,4);
    if(s.gp[0]==8) {
        s.gp[0]=read(s,0x24b4c9u,0x52d710u);s.gp[0]=read(s,0x24b4ceu,s.gp[0]+0x38u);
        cursor(s,0x24b4d1u);s.gp[6]=s.pop();cursor(s,0x24b4d2u);s.pop();
    } else {
        const auto team=std::bit_cast<std::int32_t>(s.gp[0]);
        s.flags.logic(s.gp[0],4);
        if(team>=0) s.flags.minus(s.gp[0],8,4);
        bool available=team>=0 && team<8;
        if(available) {
            push(s,0x24b4dcu,s.gp[0]);call(s,dispatch,0x15b3b0u,0x24b4ddu,0x24b4e2u);
            s.gp[0]=std::uint32_t(signed_half(s.gp[0]));s.flags.minus(s.gp[0],0xffffffffu,4);
            available=s.gp[0]!=0xffffffffu;
        }
        if(available) {
            s.gp[1]=read(s,0x24b4eau,0x52d710u);
            s.gp[0]=std::uint32_t(signed_half(read(s,0x24b4f0u,s.gp[1]+2u+s.gp[0]*2u,2)));
            cursor(s,0x24b4f5u);s.gp[6]=s.pop();cursor(s,0x24b4f6u);s.pop();
        } else {
            s.gp[0]=s.gp[6];cursor(s,0x24b4f9u);s.gp[6]=s.pop();cursor(s,0x24b4fau);s.pop();
        }
    }
    ++source_objective_native_tu15_stats.owners[5];return true;
}
inline bool registered_carrier(WorldState& s) {
    if(!admitted(s))return false;
    s.gp[1]=read(s,0x24a180u,s.gp[4]+4u)&65535u;s.gp[0]=s.gp[1]*3u;
    s.gp[1]=read(s,0x24a18du,0x4ecbd0u);s.gp[2]=read(s,0x24a193u,s.gp[1]+0x44u);
    s.gp[2]=read(s,0x24a196u,s.gp[2]+8u+s.gp[0]*4u);
    const auto object_class=read(s,0x24a19au,s.gp[2]+0xaau,1);
    s.gp[1]=(s.gp[1]&0xffffff00u)|object_class;s.gp[0]=1u<<(object_class&31u);
    push(s,0x24a1a7u,s.gp[6]);s.gp[6]=0xffffffffu;s.flags.logic(s.gp[0]&4u,1);
    bool available=source_objective_item_class(std::uint8_t(object_class));
    if(available) {
        s.gp[2]=(s.gp[2]&0xffff0000u)|read(s,0x24a1afu,s.gp[2]+0x17eu,2);
        const auto team=signed_half(s.gp[2]);s.flags.logic(std::uint16_t(team),2);
        if(team>=0)s.flags.minus(std::uint16_t(team),9,2);
        available=source_objective_team_slot(team);
        if(available) {
            s.gp[1]=std::uint32_t(team);s.gp[2]=read(s,0x24a1c4u,0x52d710u);
            s.gp[0]=read(s,0x24a1cau,s.gp[2]+0x14cu+s.gp[1]*4u);
        }
    }
    if(!available)s.gp[0]=s.gp[6];
    cursor(s,available?0x24a1d1u:0x24a1d7u);s.gp[6]=s.pop();
    cursor(s,available?0x24a1d2u:0x24a1d8u);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[6];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
