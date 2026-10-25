#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool update_overtime_latch(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24c490u,0x52d710u);push(s,0x24c495u,s.gp[5]);s.gp[5]=read(s,0x24c496u,0x4f3218u);
    store(s,0x24c49cu,s.gp[0]+0x1fcu,1,0);const auto options=read(s,0x24c4a3u,s.gp[5]+0x22cu,1);s.flags.logic(options&2u,1);
    if(options&2u) {
        push(s,0x24c4b0u,s.gp[7]);push(s,0x24c4b1u,s.gp[3]);s.gp[7]=0;s.flags.logic(0,4);push(s,0x24c4b4u,s.gp[6]);bool latched{};
        for(unsigned team=0;team<9;++team) {
            s.gp[3]=s.gp[7];call(s,dispatch,0x160e50u,0x24c4b7u,0x24c4bcu);s.gp[6]=s.gp[0];s.flags.logic(s.gp[6],4);
            if(s.gp[6]) {
                s.gp[3]=read(s,0x24c4c2u,s.gp[6]+0xcu);s.gp[1]=read(s,0x24c4c5u,0x52d710u);s.flags.minus(s.gp[3],0xffffffffu,4);const auto planted=s.gp[3]!=0xffffffffu;
                s.set(3,read(s,0x24c4ceu,s.gp[1]+s.gp[7]+0x170u,1),1,0);const auto state=s.reg(3,1,0);s.set(0,planted,1,0);
                s.set(1,(state>>2u)&1u,1,0);s.set(2,(~state)&1u,1,0);s.flags.logic(s.reg(2,1,0),1);
                const auto engine=read(s,0x24c4e7u,s.gp[5]+0x180u);s.flags.minus(engine,9,4);
                bool armed=false;if(engine==9) {s.flags.logic(state&2u,1);armed=state&2u;}
                s.set(3,armed,1,0);if(!armed)s.flags.logic(0,1);
                s.flags.logic(s.reg(0,1,0),1);latched=s.reg(0,1,0)!=0;
                if(!latched) {s.flags.logic(s.reg(1,1,0),1);latched=s.reg(1,1,0)!=0;}
                if(!latched) {s.flags.logic(s.reg(3,1,0),1);latched=s.reg(3,1,0)!=0;}
                if(!latched) {
                    s.flags.logic(s.reg(2,1,0),1);
                    if(s.reg(2,1,0)) {s.gp[0]=read(s,0x24c50bu,s.gp[6]);push(s,0x24c50du,0);call(s,dispatch,0x24d5f0u,0x24c50fu,0x24c514u);s.flags.logic(s.reg(0,1,0),1);latched=s.reg(0,1,0)!=0;}
                }
                if(latched)break;
            }
            s.gp[7]=s.flags.increment(s.gp[7],4,false);s.flags.minus(s.gp[7],9,4);
        }
        if(latched) {
            s.gp[2]=read(s,0x24c523u,0x52d710u);cursor(s,0x24c529u);s.gp[6]=s.pop();s.gp[3]=s.pop();store(s,0x24c52bu,s.gp[2]+0x1fcu,1,1);s.gp[7]=s.pop();
        } else {cursor(s,0x24c51eu);s.gp[6]=s.pop();s.gp[3]=s.pop();s.gp[7]=s.pop();}
    }
    cursor(s,0x24c533u);s.gp[5]=s.pop();s.pop();++source_objective_native_tu15_stats.owners[41];return true;
}
template<class State,class Dispatch>bool round_tick(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[1]=read(s,0x24a070u,0x4f63b8u);s.gp[0]=read(s,0x24a076u,s.gp[1]+0xc14u);s.gp[2]=read(s,0x24a07cu,0x56dfa0u+s.gp[0]*4u);s.flags.logic(s.gp[2],4);
    bool active=s.gp[2]!=0;
    if(active) {const auto phase=read(s,0x24a08bu,s.gp[1]+0x6cu,2);s.flags.minus(phase,1,2);active=phase==1;}
    if(active) {
        s.gp[2]=read(s,0x24a096u,0x4f3218u);s.gp[0]=extend(read(s,0x24a09cu,s.gp[2]+0xcu,1),1);s.flags.minus(s.gp[0],4,4);active=s.gp[0]==4;
        if(!active) {const auto host=read(s,0x24a0a5u,s.gp[1]+0xc04u);s.flags.minus(host,1,4);active=host==1;}
        if(active) {
            s.flags.minus(s.gp[0],4,4);if(s.gp[0]!=4)call(s,dispatch,0x24dae0u,0x24a0b3u,0x24a0b8u);
            push(s,0x24a0b8u,s.gp[3]);s.gp[3]=0;s.flags.logic(0,4);
            for(unsigned team=0;team<9;++team) {
                call(s,dispatch,0x160e50u,0x24a0c0u,0x24a0c5u);s.flags.logic(s.gp[0],4);
                if(s.gp[0]) {s.gp[0]=read(s,0x24a0c9u,s.gp[0]);push(s,0x24a0cbu,s.gp[0]);call(s,dispatch,0x249b20u,0x24a0ccu,0x24a0d1u);}
                s.gp[3]=s.flags.increment(s.gp[3],4,false);s.flags.minus(s.gp[3],9,4);
            }
            push(s,0x24a0d7u,s.gp[7]);s.gp[7]=read(s,0x24a0d8u,0x4f3218u);s.set(0,read(s,0x24a0deu,s.gp[7]+0xcu,1),1,0);s.set(3,4,1,0);
            s.flags.minus(s.reg(0,1,0),s.reg(3,1,0),1);
            if(s.reg(0,1,0)!=s.reg(3,1,0)) {
                call(s,dispatch,0x24c490u,0x24a0e7u,0x24a0ecu);const auto mode=read(s,0x24a0ecu,s.gp[7]+0xcu,1);s.flags.minus(mode,s.reg(3,1,0),1);bool rotate=mode!=s.reg(3,1,0);
                if(rotate) {const auto kind=read(s,0x24a0f1u,s.gp[7]+0x240u);s.flags.minus(kind,1,4);rotate=kind==1;}
                if(rotate) {
                    s.gp[1]=read(s,0x24a0fau,0x52d710u);s.gp[1]=read(s,0x24a100u,s.gp[1]+0x1f4u);s.flags.logic(s.gp[1],4);rotate=std::bit_cast<std::int32_t>(s.gp[1])>=0;
                    if(rotate) {s.flags.minus(s.gp[1],8,4);rotate=std::bit_cast<std::int32_t>(s.gp[1])<8;}
                    if(rotate) {
                        call(s,dispatch,0x164890u,0x24a10fu,0x24a114u);s.flags.logic(s.reg(0,1,0),1);
                        if(!s.reg(0,1,0)) {push(s,0x24a118u,0);s.gp[3]=s.flags.logic(0xffffffffu,4);call(s,dispatch,0x15ddc0u,0x24a11du,0x24a122u);}
                    }
                }
            }
            cursor(s,0x24a122u);s.gp[7]=s.pop();s.gp[3]=s.pop();
        }
    }
    cursor(s,0x24a124u);s.pop();++source_objective_native_tu15_stats.owners[42];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
