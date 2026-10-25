#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool player_spawn(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[1]=read(s,0x249600u,0x4f54f4u);s.gp[2]=read(s,0x249606u,s.gp[1]+0x44u);
    s.gp[4]=s.flags.minus(s.gp[4],0x24u,4);push(s,0x24960cu,s.gp[6]);s.gp[6]=read(s,0x24960du,s.gp[4]+0x2cu);
    s.gp[0]=s.flags.plus((s.gp[6]&65535u)*0x21cu,s.gp[2],4);s.gp[2]=read(s,0x249620u,0x4f3218u);
    const auto mode=read(s,0x249626u,s.gp[2]+0xcu,1);s.flags.minus(mode,4,1);
    if(mode!=4) {
        push(s,0x24962cu,s.gp[3]);s.gp[3]=(s.gp[3]&0xffffff00u)|read(s,0x24962du,s.gp[0]+0xc0u,1);
        s.gp[1]=0xffffffffu;s.flags.minus(s.gp[3]&255u,255u,1);const bool playing=(s.gp[3]&255u)!=255u;
        cursor(s,0x24963au);s.gp[3]=s.pop();
        if(playing) {
            s.gp[0]=0;push(s,0x24963fu,s.gp[7]);const auto engine=read(s,0x249640u,s.gp[2]+0x180u);s.flags.minus(engine,9,4);
            s.gp[0]=engine==9?0u:1u;s.gp[2]=s.gp[4]+8u;
            for(unsigned n=0;n<4;++n)store(s,0x24964eu+n*4u,s.gp[4]+0x14u+n*4u,4,s.gp[1]);
            store(s,0x24965eu,s.gp[4]+0x28u,2,s.gp[1]);
            s.gp[0]=s.flags.plus((s.gp[0]-1u)&7u,3,4);store(s,0x24966au,s.gp[4]+8u,4,s.gp[0]);
            s.gp[0]=0;s.flags.logic(0,4);store(s,0x249670u,s.gp[4]+0xcu,4,0);store(s,0x249674u,s.gp[4]+0x24u,4,0);
            store(s,0x249678u,s.gp[4]+0x10u,4,s.gp[6]);call(s,dispatch,0xb1e80u,0x24967cu,0x249681u);
            s.gp[7]=s.gp[4]+8u;call(s,dispatch,0x1a8740u,0x249685u,0x24968au);
            s.gp[1]=read(s,0x24968au,0x4f3218u);const auto kind=read(s,0x249690u,s.gp[1]+0x240u);s.flags.minus(kind,1,4);
            cursor(s,0x249697u);s.gp[7]=s.pop();
            if(kind==1) {push(s,0x24969au,s.gp[6]);call(s,dispatch,0x24c540u,0x24969bu,0x2496a0u);}
        }
    }
    cursor(s,0x2496a0u);s.gp[6]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x24u,4);cursor(s,0x2496a4u);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[28];return true;
}
template<class State,class Dispatch>bool spawn_filter(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24a6f0u,s.gp[1]);s.gp[0]=read(s,0x24a6f1u,s.gp[4]+0xcu);s.gp[0]=read(s,0x24a6f5u,s.gp[0]+0xcu);
    s.gp[1]=read(s,0x24a6f8u,0x4f3218u);push(s,0x24a6feu,s.gp[3]);push(s,0x24a6ffu,s.gp[6]);
    s.gp[3]=1;s.gp[6]=s.gp[0]&1u;s.flags.logic(s.gp[6],4);push(s,0x24a709u,s.gp[7]);
    store(s,0x24a70au,s.gp[4]+0xfu,1,1);store(s,0x24a70eu,s.gp[4]+0x18u,4,s.gp[0]);
    bool first=s.gp[6]!=0,resolve=true;
    if(!first) {
        s.flags.logic(s.gp[0]&2u,1);s.gp[2]=9;
        if(s.gp[0]&2u) {const auto engine=read(s,0x24a721u,s.gp[1]+0x180u);s.flags.minus(engine,9,4);first=engine!=9;}
    }
    if(!first) {
        s.gp[7]=s.gp[0]&4u;s.flags.logic(s.gp[7],4);
        if(!s.gp[7]) {
            s.flags.logic(s.gp[0]&8u,1);resolve=(s.gp[0]&8u)!=0;
            if(resolve) {const auto engine=read(s,0x24a738u,s.gp[1]+0x180u);s.flags.minus(engine,9,4);resolve=engine==9;}
        }
    }
    if(resolve) {
        const auto kind=read(s,first?0x24a79eu:0x24a744u,s.gp[1]+0x240u);s.flags.minus(kind,1,4);
        const auto bank_register=first?7u:6u;s.gp[bank_register]=read(s,first?0x24a7a4u:0x24a74au,0x52d710u);
        if(kind==1)s.gp[3]=read(s,first?0x24a7acu:0x24a752u,s.gp[bank_register]+0x1f4u);
        else {
            s.gp[1]=read(s,first?0x24a7b4u:0x24a75au,s.gp[4]+0x14u);s.gp[2]=read(s,first?0x24a7b8u:0x24a75eu,0x4f54f4u);
            s.gp[0]=read(s,first?0x24a7beu:0x24a764u,s.gp[2]+0x44u);s.gp[1]&=65535u;
            s.flags.logic(s.gp[1],4);s.gp[1]=s.flags.product(s.gp[1],0x21cu);
            s.gp[3]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(s,first?0x24a7cdu:0x24a773u,s.gp[0]+0xc0u+s.gp[1],1)))));
        }
        call(s,dispatch,0x160e50u,first?0x24a7d5u:0x24a77bu,first?0x24a7dau:0x24a780u);s.flags.logic(s.gp[0],4);
        if(s.gp[0]) {
            const auto item=read(s,first?0x24a7deu:0x24a784u,s.gp[bank_register]+0x170u+s.gp[3],1);
            s.gp[0]=(s.gp[0]&0xffffff00u)|((~item)&1u);s.flags.logic(s.gp[0]&255u,1);
            const auto home_register=first?6u:7u;const bool require_home=s.gp[home_register]!=0;
            s.flags.logic(s.gp[home_register],4);const bool away=(s.gp[0]&255u)!=0;
            if(require_home)s.flags.logic(s.gp[0]&255u,1);
            bool allowed{};
            if(require_home && away)allowed=source_objective_spawn_filter(true,false,true);
            else {
                const auto flags=read(s,first?0x24a7f1u:0x24a797u,s.gp[4]+0x18u,1);
                const auto away_mask=first?2u:8u;s.flags.logic(flags&away_mask,1);
                if(flags&away_mask)s.flags.logic(s.gp[0]&255u,1);
                allowed=source_objective_spawn_filter(require_home,(flags&away_mask)!=0,away);
            }
            if(!allowed)store(s,0x24a7fcu,s.gp[4]+0xfu,1,0);
        }
    }
    s.gp[0]=(s.gp[0]&0xffffff00u)|read(s,0x24a801u,s.gp[4]+0xfu,1);
    cursor(s,0x24a805u);s.gp[7]=s.pop();cursor(s,0x24a806u);s.gp[6]=s.pop();cursor(s,0x24a807u);s.gp[3]=s.pop();
    cursor(s,0x24a808u);s.gp[1]=s.pop();cursor(s,0x24a809u);s.pop();s.gp[4]+=8u;
    ++source_objective_native_tu15_stats.owners[29];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
