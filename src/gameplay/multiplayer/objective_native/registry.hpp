#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool select_carrier_leader(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x14u,4);s.gp[1]=read(s,0x249a43u,0x4ecbd0u);s.gp[2]=read(s,0x249a49u,s.gp[1]+0x44u);
    s.gp[1]=read(s,0x249a4cu,0x4f54f4u);s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);s.gp[0]*=3u;s.gp[0]=read(s,0x249a5au,s.gp[2]+s.gp[0]*4u+8u);
    push(s,0x249a5eu,s.gp[5]);s.gp[5]=extend(read(s,0x249a5fu,s.gp[0]+0x17eu,2),2);push(s,0x249a66u,s.gp[6]);
    s.gp[0]=s.flags.logic(0xffffffffu,4);push(s,0x249a6au,s.gp[7]);s.gp[6]=s.gp[4]+0x10u;
    store(s,0x249a6fu,s.gp[4]+0xcu,4,s.gp[0]);s.gp[7]=0x80000000u;store(s,0x249a78u,s.gp[4]+0x14u,4,s.gp[1]);
    store(s,0x249a7cu,s.gp[4]+0x1cu,4,s.gp[0]);store(s,0x249a80u,s.gp[4]+0x18u,4,s.gp[0]);
    call(s,dispatch,0x1a8e50u,0x249a84u,0x249a89u);s.flags.logic(s.reg(0,1,0),1);
    if(s.reg(0,1,0)) {
        push(s,0x249a8du,s.gp[3]);s.gp[3]=read(s,0x249a8eu,0x4f63b8u);
        do {
            s.flags.minus(s.gp[5],8,4);bool same_team=s.gp[5]==8;
            if(!same_team) {
                s.gp[2]=read(s,0x249a99u,s.gp[4]+0x14u);s.gp[0]=extend(read(s,0x249a9du,s.gp[2]+0xc0u,1),1);
                s.flags.minus(s.gp[0],s.gp[5],4);same_team=s.gp[0]==s.gp[5];
            }
            if(same_team) {
                s.gp[1]=read(s,0x249aa8u,s.gp[3]+0xc14u);s.gp[2]=read(s,0x249aaeu,0x56dfa0u+s.gp[1]*4u);
                s.gp[0]=0;s.flags.logic(0,4);s.flags.logic(s.gp[2],4);const auto registered=s.gp[2]!=0;s.gp[2]=read(s,0x249ab9u,s.gp[4]+0x1cu);
                if(registered) {
                    s.gp[1]=s.gp[3]+0x304u;s.flags.logic(s.gp[1],4);
                    if(s.gp[1]) {s.gp[0]=s.gp[2]&65535u;s.flags.logic(s.gp[0],4);s.gp[0]=s.flags.product(s.gp[0],0x1cu);s.gp[0]=extend(read(s,0x249ad3u,s.gp[1]+s.gp[0]+8u,2),2);}
                }
                s.flags.minus(s.gp[0],s.gp[7],4);
                // Strict improvement retains the first iterator entry on ties.
                if(std::bit_cast<std::int32_t>(s.gp[0])>std::bit_cast<std::int32_t>(s.gp[7])) {
                    store(s,0x249adcu,s.gp[4]+0x10u,4,s.gp[2]);s.gp[7]=s.gp[0];
                }
            }
            s.gp[6]=s.gp[4]+0x14u;call(s,dispatch,0x1a8e50u,0x249ae6u,0x249aebu);s.flags.logic(s.reg(0,1,0),1);
        } while(s.reg(0,1,0));
        cursor(s,0x249aefu);s.gp[3]=s.pop();
    }
    s.flags.logic(s.gp[5],4);bool valid=std::bit_cast<std::int32_t>(s.gp[5])>=0;
    if(valid) {s.flags.minus(s.gp[5],9,4);valid=std::bit_cast<std::int32_t>(s.gp[5])<9;}
    if(valid) {s.gp[1]=read(s,0x249af9u,0x52d710u);s.gp[2]=read(s,0x249affu,s.gp[4]+0xcu);store(s,0x249b03u,s.gp[1]+s.gp[5]*4u+0x14cu,4,s.gp[2]);}
    cursor(s,0x249b0au);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x14u,4);s.pop();
    ++source_objective_native_tu15_stats.owners[37];return true;
}
template<class State,class Dispatch>bool reconcile_registry(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x50u,4);s.gp[0]=s.flags.logic(0xffffffffu,4);
    for(unsigned n=0;n<6;++n)store(s,0x24b6e6u+n*4u,s.gp[4]+0x2cu+n*4u,4,s.gp[0]);
    push(s,0x24b6feu,s.gp[3]);s.gp[3]=read(s,0x24b6ffu,0x4f63b8u);
    store(s,0x24b705u,s.gp[4]+0x48u,4,s.gp[0]);store(s,0x24b709u,s.gp[4]+0x4cu,4,s.gp[0]);
    push(s,0x24b70du,s.gp[5]);s.gp[5]=0;s.flags.logic(0,4);store(s,0x24b710u,s.gp[4]+0x54u,4,s.gp[0]);
    const auto count=read(s,0x24b714u,s.gp[3]+0xc18u);s.flags.minus(count,s.gp[5],4);push(s,0x24b71au,s.gp[6]);push(s,0x24b71bu,s.gp[7]);
    store(s,0x24b71cu,s.gp[4]+0x13u,1,1);store(s,0x24b721u,s.gp[4]+0x14u,4,s.gp[5]);store(s,0x24b725u,s.gp[4]+0x18u,4,s.gp[5]);
    if(std::bit_cast<std::int32_t>(count)>0) {
        s.gp[3]=s.flags.plus(s.gp[3],0xc1cu,4);
        for(;;) {
            s.gp[5]=read(s,0x24b731u,s.gp[3]);s.gp[2]=read(s,0x24b733u,0x4ecbd0u);s.gp[0]=s.gp[5]&65535u;s.flags.logic(s.gp[0],4);
            s.gp[1]=s.gp[0]*3u;s.gp[0]=read(s,0x24b743u,s.gp[2]+0x44u);s.gp[7]=read(s,0x24b746u,s.gp[0]+s.gp[1]*4u+8u);
            s.gp[6]=extend(read(s,0x24b74au,s.gp[7]+0x17eu,2),2);call(s,dispatch,0x24cd70u,0x24b751u,0x24b756u);s.flags.logic(s.reg(0,1,0),1);
            bool keep=s.reg(0,1,0)!=0;
            if(keep) {
                s.gp[0]=extend(read(s,0x24b75au,s.gp[7]+0x17eu,2),2);s.gp[1]=read(s,0x24b761u,s.gp[4]+s.gp[0]*4u+0x3cu);
                s.flags.minus(s.gp[1],0xffffffffu,4);s.gp[0]=s.gp[4]+s.gp[0]*4u+0x3cu;keep=s.gp[1]==0xffffffffu;
            }
            if(keep)store(s,0x24b76eu,s.gp[0],4,s.gp[5]);
            else {
                s.gp[0]=read(s,0x24b772u,s.gp[4]+0x14u);store(s,0x24b776u,s.gp[4]+s.gp[0]*4u+0x1cu,4,s.gp[5]);
                s.gp[0]=s.flags.increment(s.gp[0],4,false);store(s,0x24b77bu,s.gp[4]+0x13u,1,0);store(s,0x24b780u,s.gp[4]+0x14u,4,s.gp[0]);
            }
            s.gp[0]=read(s,0x24b784u,s.gp[4]+0x18u);s.gp[1]=read(s,0x24b788u,0x4f63b8u);s.gp[2]=read(s,0x24b78eu,s.gp[1]+0xc18u);
            s.gp[0]=s.flags.increment(s.gp[0],4,false);s.gp[3]=s.flags.plus(s.gp[3],0x10u,4);s.flags.minus(s.gp[0],s.gp[2],4);
            store(s,0x24b79au,s.gp[4]+0x18u,4,s.gp[0]);if(std::bit_cast<std::int32_t>(s.gp[0])>=std::bit_cast<std::int32_t>(s.gp[2]))break;
        }
        s.gp[5]=read(s,0x24b7a0u,s.gp[4]+0x14u);
    }
    s.gp[6]=0;s.flags.logic(0,4);s.flags.logic(s.gp[5],4);
    while(std::bit_cast<std::int32_t>(s.gp[6])<std::bit_cast<std::int32_t>(s.gp[5])) {
        s.gp[7]=read(s,0x24b7b0u,s.gp[4]+s.gp[6]*4u+0x1cu);s.gp[0]=s.gp[7];call(s,dispatch,0x160b70u,0x24b7b6u,0x24b7bbu);
        push(s,0x24b7bbu,s.gp[7]);call(s,dispatch,0xba570u,0x24b7bcu,0x24b7c1u);s.gp[6]=s.flags.increment(s.gp[6],4,false);s.flags.minus(s.gp[6],s.gp[5],4);
    }
    s.gp[6]=0;s.flags.logic(0,4);s.gp[7]=s.flags.logic(0xffffffffu,4);
    for(unsigned team=0;team<9;++team) {
        call(s,dispatch,0x24cd70u,0x24b7d0u,0x24b7d5u);s.flags.logic(s.reg(0,1,0),1);bool missing=s.reg(0,1,0)!=0;
        if(missing) {const auto objective=read(s,0x24b7d9u,s.gp[4]+s.gp[6]*4u+0x3cu);s.flags.minus(objective,s.gp[7],4);missing=objective==s.gp[7];}
        if(missing) {push(s,0x24b7dfu,s.gp[6]);call(s,dispatch,0x24b5c0u,0x24b7e0u,0x24b7e5u);store(s,0x24b7e5u,s.gp[4]+0x13u,1,0);}
        s.gp[6]=s.flags.increment(s.gp[6],4,false);s.flags.minus(s.gp[6],9,4);
    }
    s.set(0,read(s,0x24b7f0u,s.gp[4]+0x13u,1),1,0);cursor(s,0x24b7f4u);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[3]=s.pop();
    s.gp[4]=s.flags.plus(s.gp[4],0x50u,4);s.pop();++source_objective_native_tu15_stats.owners[38];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
