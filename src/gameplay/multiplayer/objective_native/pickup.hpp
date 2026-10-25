#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool pickup_allowed(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24a810u,s.gp[1]);s.gp[0]=read(s,0x24a811u,0x4f63b8u);
    s.gp[1]=read(s,0x24a816u,s.gp[0]+0xc14u);push(s,0x24a81cu,s.gp[5]);push(s,0x24a81du,s.gp[6]);
    s.gp[6]=read(s,0x24a81eu,0x56dfa0u+s.gp[1]*4u);s.flags.logic(s.gp[6],4);
    s.gp[2]=(s.gp[2]&0xffffff00u)|1u;store(s,0x24a829u,s.gp[4]+0xbu,1,1);
    const auto finish=[&](bool early) {
        cursor(s,early?0x24a84du:0x24a908u);s.gp[6]=s.pop();
        s.gp[0]=(s.gp[0]&0xffffff00u)|(s.gp[2]&255u);
        cursor(s,early?0x24a850u:0x24a90bu);s.gp[5]=s.pop();
        cursor(s,early?0x24a851u:0x24a90cu);s.gp[1]=s.pop();
        cursor(s,early?0x24a852u:0x24a90du);s.pop();s.gp[4]+=8u;
        ++source_objective_native_tu15_stats.owners[10];return true;
    };
    bool active=s.gp[6]!=0;
    if(active) {const auto phase=read(s,0x24a82fu,s.gp[0]+0x6cu,2);s.flags.minus(phase,1,2);active=phase==1;}
    if(active) {
        s.gp[5]=read(s,0x24a836u,0x4f3218u);const auto mode=read(s,0x24a83cu,s.gp[5]+0xcu,1);
        s.flags.minus(mode,4,1);
        if(mode!=4) {const auto phase=read(s,0x24a842u,s.gp[0]+0xc04u);s.flags.minus(phase,1,4);active=phase==1;}
    }
    if(!active) {s.gp[2]&=0xffffff00u;s.flags.logic(0,1);return finish(true);}
    s.gp[1]=read(s,0x24a855u,s.gp[4]+0x10u);s.flags.minus(s.gp[1],0xffffffffu,4);
    if(s.gp[1]==0xffffffffu)return finish(false);
    s.gp[0]=read(s,0x24a862u,s.gp[4]+0x14u);s.flags.minus(s.gp[0],0xffffffffu,4);
    if(s.gp[0]==0xffffffffu)return finish(false);
    s.gp[2]=read(s,0x24a86fu,0x4f54f4u);s.gp[6]=read(s,0x24a875u,s.gp[2]+0x44u);
    s.gp[2]=read(s,0x24a878u,0x4ecbd0u);s.gp[2]=read(s,0x24a87eu,s.gp[2]+0x44u);
    s.gp[1]=(s.gp[1]&65535u)*0x21cu;s.gp[1]=s.flags.plus(s.gp[1],s.gp[6],4);
    push(s,0x24a88fu,s.gp[7]);s.gp[7]=s.gp[1];s.gp[1]=s.gp[0]&65535u;
    s.flags.logic(s.gp[1],4);s.gp[1]*=3u;s.gp[1]=read(s,0x24a89du,s.gp[2]+8u+s.gp[1]*4u);
    s.gp[6]=std::uint32_t(signed_half(read(s,0x24a8a1u,s.gp[1]+0x17eu,2)));
    call(s,dispatch,0x106350u,0x24a8a8u,0x24a8adu);
    const auto classification=std::uint16_t(s.gp[0]);s.flags.minus(classification,1,2);
    const auto owner=std::int16_t(signed_half(s.gp[6]));bool allowed=true;
    if(classification==1) {
        s.flags.minus(s.gp[6],0xffffffffu,4);
        if(owner!=-1) {
            s.gp[0]=read(s,0x24a8b8u,s.gp[5]+0x180u);const auto engine=std::bit_cast<std::int32_t>(s.gp[0]);
            s.flags.minus(s.gp[0],9,4);std::uint8_t item_flags{};
            if(engine==9) {
                s.gp[2]=read(s,0x24a8c3u,0x52d710u);
                item_flags=std::uint8_t(read(s,0x24a8c9u,s.gp[2]+0x170u+s.gp[6],1));s.flags.logic(item_flags&2u,1);
            }
            if(engine==9 && (item_flags&2u))allowed=false;
            else {
                s.flags.minus(s.gp[6],8,4);
                if(owner!=8) {
                    s.flags.minus(s.gp[0],9,4);std::int8_t team{};
                    if(engine!=9) {
                        const auto raw=read(s,0x24a8e8u,s.gp[7]+0xc0u,1);
                        team=std::bit_cast<std::int8_t>(std::uint8_t(raw));s.gp[1]=std::uint32_t(std::int32_t(team));
                        s.flags.minus(s.gp[6],s.gp[1],4);
                        if(owner!=team)s.flags.minus(s.gp[0],9,4);
                    } else {
                        const auto raw=read(s,0x24a8f8u,s.gp[7]+0xc0u,1);
                        team=std::bit_cast<std::int8_t>(std::uint8_t(raw));s.gp[2]=std::uint32_t(std::int32_t(team));
                        s.flags.minus(s.gp[6],s.gp[2],4);
                    }
                    allowed=source_objective_pickup_allowed(engine,classification,owner,team,item_flags);
                }
            }
        }
    }
    if(allowed) {
        s.gp[2]=(s.gp[2]&0xffffff00u)|read(s,0x24a903u,s.gp[4]+0xfu,1);
        cursor(s,0x24a907u);s.gp[7]=s.pop();return finish(false);
    }
    cursor(s,0x24a8d3u);s.gp[7]=s.pop();s.gp[2]&=0xffffff00u;s.flags.logic(0,1);
    cursor(s,0x24a8d6u);s.gp[6]=s.pop();s.gp[0]&=0xffffff00u;
    cursor(s,0x24a8d9u);s.gp[5]=s.pop();cursor(s,0x24a8dau);s.gp[1]=s.pop();
    cursor(s,0x24a8dbu);s.pop();s.gp[4]+=8u;++source_objective_native_tu15_stats.owners[10];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
