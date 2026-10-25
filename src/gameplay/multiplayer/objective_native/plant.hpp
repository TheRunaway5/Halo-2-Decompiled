#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool plant_bomb(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x38u,4);s.gp[1]=read(s,0x24be93u,0x4ecbd0u);s.gp[2]=read(s,0x24be99u,s.gp[1]+0x44u);push(s,0x24be9cu,s.gp[5]);s.gp[5]=read(s,0x24be9du,s.gp[4]+0x44u);
    push(s,0x24bea1u,s.gp[7]);s.gp[7]=s.gp[0];s.gp[0]=read(s,0x24bea4u,s.gp[4]+0x44u)&65535u;s.flags.logic(s.gp[0],4);s.gp[0]=s.flags.shift(s.gp[0]*3u,2,4);store(s,0x24beb3u,s.gp[4]+0xcu,4,s.gp[0]);
    s.gp[0]=read(s,0x24beb7u,s.gp[2]+s.gp[0]+8u);s.gp[0]=extend(read(s,0x24bebbu,s.gp[0]+0x17eu,2),2);s.flags.minus(s.gp[0],0xffffffffu,4);store(s,0x24bec5u,s.gp[4]+0x48u,4,s.gp[0]);
    if(s.gp[0]!=0xffffffffu) {
        push(s,0x24becfu,s.gp[3]);push(s,0x24bed0u,s.gp[6]);s.gp[2]=s.gp[7];call(s,dispatch,0x24be50u,0x24bed3u,0x24bed8u);s.gp[1]=read(s,0x24bed8u,0x4f3218u);store(s,0x24bedeu,s.gp[4]+0x10u,4,s.gp[0]);
        const auto engine=read(s,0x24bee2u,s.gp[1]+0x180u);s.flags.minus(engine,9,4);s.gp[3]=1;
        if(engine==9) {
            s.gp[0]=read(s,0x24bef0u,0x4f54f4u);s.gp[1]=read(s,0x24bef5u,s.gp[0]+0x44u);s.gp[6]=s.gp[5]&65535u;s.flags.logic(s.gp[6],4);s.gp[2]=s.flags.product(s.gp[6],0x21cu);s.gp[2]=extend(read(s,0x24bf08u,s.gp[1]+s.gp[2]+0xc0u,1),1);
            push(s,0x24bf10u,s.gp[3]);s.gp[1]=19;s.gp[0]=s.gp[6];call(s,dispatch,0x19b1d0u,0x24bf18u,0x24bf1du);s.gp[0]=21;s.gp[1]=s.gp[6];call(s,dispatch,0x19baa0u,0x24bf24u,0x24bf29u);
        }
        s.flags.minus(s.gp[7],0xffffffffu,4);
        if(s.gp[7]!=0xffffffffu) {
            s.gp[2]=read(s,0x24bf32u,0x4ecc20u);s.gp[0]=read(s,0x24bf38u,s.gp[2]+0x11cu);s.gp[6]=read(s,0x24bf3eu,s.gp[4]+0x4cu);s.gp[7]=s.flags.shift(s.gp[7],5,4);
            s.gp[1]=read(s,0x24bf45u,s.gp[0]+s.gp[7]+4u);s.gp[2]=read(s,0x24bf49u,s.gp[0]+s.gp[7]+8u);s.gp[7]=s.flags.plus(s.gp[7],s.gp[0],4);s.gp[0]=read(s,0x24bf4fu,s.gp[7]);store(s,0x24bf51u,s.gp[4]+0x18u,4,s.gp[0]);
            s.gp[0]=read(s,0x24bf55u,0x52d710u);store(s,0x24bf5au,s.gp[0]+0x204u,2,s.flags.logic(read(s,0x24bf5au,s.gp[0]+0x204u,2)|s.reg(3,2,0),2));s.gp[0]=s.gp[4]+0x18u;push(s,0x24bf65u,s.gp[0]);store(s,0x24bf66u,s.gp[4]+0x20u,4,s.gp[1]);store(s,0x24bf6au,s.gp[4]+0x24u,4,s.gp[2]);
            call(s,dispatch,0x160da0u,0x24bf6eu,0x24bf73u);s.gp[6]=read(s,0x24bf73u,0x52d710u);store(s,0x24bf79u,s.gp[6]+0x204u,1,s.flags.logic(read(s,0x24bf79u,s.gp[6]+0x204u,1)&0xfeu,1));
            s.gp[7]=read(s,0x24bf80u,s.gp[4]+0x50u);s.gp[3]=read(s,0x24bf84u,0x4f63b8u);store(s,0x24bf8au,s.gp[6]+s.gp[7]*4u+0x17cu,4,s.gp[5]);
            s.gp[1]=read(s,0x24bf91u,s.gp[3]+0xc14u);s.gp[0]=read(s,0x24bf97u,0x56dfa0u+s.gp[1]*4u);s.gp[1]=s.flags.logic(0xffffffffu,4);s.flags.logic(s.gp[0],4);
            if(s.gp[0]) {s.gp[0]=read(s,0x24bfa5u,s.gp[3]+0x24u);s.flags.minus(s.gp[0],s.gp[1],4);if(s.gp[0]!=s.gp[1]) {push(s,0x24bfacu,0x400u);push(s,0x24bfb1u,s.gp[0]);call(s,dispatch,0xb7e50u,0x24bfb2u,0x24bfb7u);s.gp[1]=s.flags.logic(0xffffffffu,4);}}
            s.gp[2]=read(s,0x24bfbau,0x4f3218u);s.set(0,read(s,0x24bfc0u,s.gp[2]+0x246u,2),2,0);s.flags.logic(s.reg(0,2,0),2);store(s,0x24bfcau,s.gp[6]+s.gp[7]*2u+0x114u,2,s.reg(0,2,0));
            if(!s.reg(0,2,0))store(s,0x24bfd4u,s.gp[6]+s.gp[7]*2u+0x114u,2,10);
            s.gp[0]=read(s,0x24bfdeu,0x51df54u);s.set(2,read(s,0x24bfe3u,s.gp[0]+2u,2),2,0);store(s,0x24bfe7u,s.gp[6]+s.gp[7]*2u+0x126u,2,s.reg(2,2,0));
            s.gp[0]=read(s,0x24bfefu,s.gp[3]+0xc14u);s.gp[2]=read(s,0x24bff5u,0x56dfa0u+s.gp[0]*4u);s.flags.logic(s.gp[2],4);
            if(s.gp[2]) {s.gp[0]=read(s,0x24c000u,s.gp[3]+0x24u);s.flags.minus(s.gp[0],s.gp[1],4);if(s.gp[0]!=s.gp[1]) {push(s,0x24c007u,0x80u);push(s,0x24c00cu,s.gp[0]);call(s,dispatch,0xb7e50u,0x24c00du,0x24c012u);s.gp[1]=s.flags.logic(0xffffffffu,4);}}
            const auto team_flags=s.gp[6]+s.gp[7]+0x170u;store(s,0x24c015u,team_flags,1,s.flags.logic(read(s,0x24c015u,team_flags,1)|2u,1));s.gp[2]=read(s,0x24c01du,s.gp[3]+0xc14u);s.gp[0]=read(s,0x24c023u,0x56dfa0u+s.gp[2]*4u);s.flags.logic(s.gp[0],4);
            if(s.gp[0]) {s.gp[3]=read(s,0x24c02eu,s.gp[3]+0x24u);s.flags.minus(s.gp[3],s.gp[1],4);if(s.gp[3]!=s.gp[1]) {push(s,0x24c035u,0x200u);push(s,0x24c03au,s.gp[3]);call(s,dispatch,0xb7e50u,0x24c03bu,0x24c040u);s.gp[1]=s.flags.logic(0xffffffffu,4);}}
            s.gp[0]=read(s,0x24c043u,0x4f54f4u);s.gp[2]=read(s,0x24c048u,s.gp[0]+0x44u);store(s,0x24c04bu,s.gp[4]+0x30u,4,s.gp[5]);s.gp[5]&=65535u;s.flags.logic(s.gp[5],4);s.gp[5]=s.flags.product(s.gp[5],0x21cu);
            store(s,0x24c05bu,s.gp[4]+0x34u,4,s.gp[1]);store(s,0x24c05fu,s.gp[4]+0x3cu,4,s.gp[1]);store(s,0x24c063u,s.gp[4]+0x24u,4,10);store(s,0x24c06bu,s.gp[4]+0x28u,4,13);store(s,0x24c073u,s.gp[4]+0x2cu,4,s.gp[1]);store(s,0x24c077u,s.gp[4]+0x38u,4,s.gp[1]);store(s,0x24c07bu,s.gp[4]+0x40u,4,0);store(s,0x24c083u,s.gp[4]+0x44u,2,s.reg(1,2,0));
            s.gp[0]=extend(read(s,0x24c088u,s.gp[2]+s.gp[5]+0xc0u,1),1);s.gp[2]=read(s,0x24c090u,s.gp[4]+0x10u);store(s,0x24c094u,s.gp[4]+0x34u,4,s.gp[0]);s.gp[0]=read(s,0x24c098u,0x4f3218u);store(s,0x24c09du,s.gp[4]+0x3cu,4,s.gp[2]);
            const auto mode=read(s,0x24c0a1u,s.gp[0]+0xcu,1);s.flags.minus(mode,4,1);
            if(mode!=4) {s.gp[2]=s.gp[4]+0x24u;call(s,dispatch,0xb1e80u,0x24c0abu,0x24c0b0u);s.gp[7]=s.gp[4]+0x24u;call(s,dispatch,0x1a8740u,0x24c0b4u,0x24c0b9u);s.gp[1]=s.flags.logic(0xffffffffu,4);}
            s.gp[2]=read(s,0x24c0bcu,0x4ecbd0u);s.gp[0]=read(s,0x24c0c2u,s.gp[2]+0x44u);s.gp[2]=read(s,0x24c0c5u,s.gp[4]+0x14u);s.gp[0]=read(s,0x24c0c9u,s.gp[0]+s.gp[2]+8u);
            store(s,0x24c0cdu,s.gp[0]+0xc8u,4,s.gp[1]);store(s,0x24c0d3u,s.gp[0]+0xc4u,4,s.gp[1]);store(s,0x24c0d9u,s.gp[0]+0xc2u,2,s.reg(1,2,0));
        }
        cursor(s,0x24c0e0u);s.gp[6]=s.pop();s.gp[3]=s.pop();
    }
    cursor(s,0x24c0e2u);s.gp[7]=s.pop();s.gp[5]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x38u,4);s.pop();s.gp[4]+=8u;++source_objective_native_tu15_stats.owners[51];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
