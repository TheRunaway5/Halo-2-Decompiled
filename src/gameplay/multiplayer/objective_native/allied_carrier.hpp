#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool allied_carrier(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x14u,4);push(s,0x24abf3u,s.gp[3]);push(s,0x24abf4u,s.gp[5]);
    s.gp[5]=read(s,0x24abf5u,0x4f54f4u);s.gp[1]=read(s,0x24abfbu,s.gp[5]+0x44u);push(s,0x24abfeu,s.gp[6]);push(s,0x24abffu,s.gp[7]);
    s.gp[7]=read(s,0x24ac00u,s.gp[4]+0x28u)&65535u;s.flags.logic(s.gp[7],4);s.gp[7]=s.flags.product(s.gp[7],0x21cu);
    s.gp[6]=read(s,0x24ac10u,s.gp[1]+s.gp[7]+0x2cu);s.gp[7]=s.flags.plus(s.gp[7],s.gp[1],4);s.set(3,s.flags.logic(0,1),1,0);
    s.flags.minus(s.gp[6],0xffffffffu,4);
    if(s.gp[6]!=0xffffffffu) {
        s.gp[1]=s.gp[6];call(s,dispatch,0xbcfb0u,0x24ac1fu,0x24ac24u);s.flags.minus(s.gp[0],s.gp[6],4);store(s,0x24ac26u,s.gp[4]+0x10u,4,s.gp[0]);
        if(s.gp[0]!=s.gp[6]) {
            store(s,0x24ac2cu,s.gp[4]+0x18u,4,s.gp[5]);s.gp[5]=s.flags.logic(0xffffffffu,4);s.gp[6]=s.gp[4]+0x14u;
            store(s,0x24ac37u,s.gp[4]+0x20u,4,s.gp[5]);store(s,0x24ac3bu,s.gp[4]+0x1cu,4,s.gp[5]);
            call(s,dispatch,0x1a8f10u,0x24ac3fu,0x24ac44u);s.flags.logic(s.reg(0,1,0),1);
            while(s.reg(0,1,0)) {
                s.gp[0]=read(s,0x24ac50u,s.gp[4]+0x1cu);const auto self=read(s,0x24ac54u,s.gp[4]+0x28u);s.flags.minus(s.gp[0],self,4);
                if(s.gp[0]!=self) {
                    s.gp[6]=read(s,0x24ac5au,s.gp[4]+0x14u);s.set(1,read(s,0x24ac5eu,s.gp[6]+0xc0u,1),1,0);
                    const auto team=read(s,0x24ac64u,s.gp[7]+0xc0u,1);s.flags.minus(s.reg(1,1,0),team,1);
                    if(s.reg(1,1,0)==team) {
                        push(s,0x24ac6cu,1);call(s,dispatch,0x1a8fd0u,0x24ac6eu,0x24ac73u);s.flags.minus(s.gp[0],s.gp[5],4);
                        if(s.gp[0]!=s.gp[5]) {
                            s.gp[1]=read(s,0x24ac77u,s.gp[6]+0x2cu);s.flags.minus(s.gp[1],s.gp[5],4);
                            if(s.gp[1]!=s.gp[5]) {
                                call(s,dispatch,0xbcfb0u,0x24ac7eu,0x24ac83u);const auto root=read(s,0x24ac83u,s.gp[4]+0x10u);
                                s.flags.minus(s.gp[0],root,4);if(s.gp[0]==root)s.set(3,1,1,0);
                            }
                        }
                    }
                }
                s.gp[6]=s.gp[4]+0x14u;call(s,dispatch,0x1a8f10u,0x24ac8fu,0x24ac94u);s.flags.logic(s.reg(0,1,0),1);
            }
        }
    }
    cursor(s,0x24ac98u);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.set(0,s.reg(3,1,0),1,0);s.gp[3]=s.pop();
    s.gp[4]=s.flags.plus(s.gp[4],0x14u,4);s.pop();s.gp[4]+=4u;++source_objective_native_tu15_stats.owners[34];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
