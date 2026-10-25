#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool objective_touched(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[1]=read(s,0x24a910u,0x4f54f4u);s.gp[4]=s.flags.minus(s.gp[4],0x24u,4);push(s,0x24a919u,s.gp[3]);s.gp[3]=read(s,0x24a91au,s.gp[4]+0x2cu);push(s,0x24a91eu,s.gp[6]);s.gp[6]=read(s,0x24a91fu,s.gp[1]+0x44u);s.gp[1]=read(s,0x24a922u,s.gp[4]+0x34u);push(s,0x24a926u,s.gp[7]);s.gp[7]=s.gp[3]&65535u;s.flags.logic(s.gp[7],4);s.gp[0]=s.flags.product(s.gp[7],0x21cu);
    s.gp[1]&=65535u;s.flags.logic(s.gp[1],4);s.gp[2]=s.gp[1]*3u;s.gp[1]=read(s,0x24a940u,0x4ecbd0u);s.gp[1]=read(s,0x24a946u,s.gp[1]+0x44u);s.gp[2]=read(s,0x24a949u,s.gp[1]+s.gp[2]*4u+8u);s.gp[0]=s.flags.plus(s.gp[0],s.gp[6],4);s.gp[6]=extend(read(s,0x24a94fu,s.gp[2]+0x17eu,2),2);
    s.flags.minus(s.gp[6],0xffffffffu,4);bool eligible=s.gp[6]!=0xffffffffu,extra_saved{};
    const auto process=[&] {
        if(!eligible)return;s.flags.minus(s.gp[6],8,4);if(s.gp[6]==8)return;
        push(s,0x24a968u,s.gp[5]);extra_saved=true;s.gp[5]=read(s,0x24a969u,0x4f3218u);s.gp[1]=read(s,0x24a96fu,s.gp[5]+0x180u);s.flags.minus(s.gp[1],9,4);
        if(s.gp[1]!=9) {
            s.gp[2]=extend(read(s,0x24a97au,s.gp[0]+0xc0u,1),1);s.flags.minus(s.gp[6],s.gp[2],4);
            if(s.gp[6]!=s.gp[2]) {s.flags.minus(s.gp[1],9,4);return;}
        } else {s.gp[2]=extend(read(s,0x24a98eu,s.gp[0]+0xc0u,1),1);s.flags.minus(s.gp[6],s.gp[2],4);if(s.gp[6]==s.gp[2])return;}
        s.set(0,4,1,0);const auto options=read(s,0x24a99fu,s.gp[5]+0x22cu,1);s.flags.logic(options&s.reg(0,1,0),1);
        if(!(options&4u)) {
            s.gp[2]=read(s,0x24aa68u,0x52d710u);const auto flags=read(s,0x24aa6eu,s.gp[2]+s.gp[6]+0x170u,1);s.flags.logic(flags&1u,1);
            if(flags&1u) {s.set(0,s.flags.logic(0,1),1,0);s.gp[1]=s.gp[3];call(s,dispatch,0x24c390u,0x24aa7cu,0x24aa81u);}return;
        }
        const auto mode=read(s,0x24a9abu,s.gp[5]+0xcu,1);s.flags.minus(mode,s.reg(0,1,0),1);if(mode==s.reg(0,1,0))return;
        s.set(0,read(s,0x24a9b4u,s.gp[5]+0x1128u,1),1,0);s.flags.logic(s.reg(0,1,0),1);if(s.reg(0,1,0))return;
        s.gp[0]=read(s,0x24a9c2u,0x52d710u);const auto flags=read(s,0x24a9c7u,s.gp[0]+s.gp[6]+0x170u,1);s.flags.logic(flags&1u,1);if(!(flags&1u))return;
        s.flags.minus(s.gp[1],9,4);const auto assault=s.gp[1]==9;s.gp[0]=s.gp[7];push(s,0x24a9dau,1);s.gp[1]=assault?22u:17u;
        call(s,dispatch,0x19b1d0u,assault?0x24a9e3u:0x24a9efu,assault?0x24a9e8u:0x24a9f4u);
        if(!assault) {s.gp[3]=1;s.gp[0]=20;s.gp[1]=s.gp[7];call(s,dispatch,0x19baa0u,0x24aa00u,0x24aa05u);s.gp[3]=read(s,0x24aa05u,s.gp[4]+0x38u);}
        s.gp[0]=read(s,0x24aa09u,s.gp[5]+0x180u);s.gp[1]=0;s.flags.logic(0,4);s.flags.minus(s.gp[0],9,4);s.gp[1]=source_objective_event_audience(std::bit_cast<std::int32_t>(s.gp[0]));s.gp[0]=s.gp[4]+0x10u;s.gp[2]=3;s.flags.plus(s.gp[1]-3u,3,4);
        call(s,dispatch,0x1a87c0u,0x24aa27u,0x24aa2cu);s.gp[1]=s.gp[0];s.gp[0]=s.gp[3];call(s,dispatch,0x1a87f0u,0x24aa30u,0x24aa35u);
        s.flags.minus(s.reg(6,2,0),8,2);s.gp[0]=s.reg(6,2,0)==8?s.flags.logic(0xffffffffu,4):extend(s.reg(6,2,0),2);s.gp[1]=extend(s.reg(0,2,0),2);s.gp[0]=s.gp[4]+0x10u;store(s,0x24aa4au,s.gp[4]+0x28u,4,s.gp[1]);call(s,dispatch,0x1a87a0u,0x24aa4eu,0x24aa53u);
        s.gp[0]=read(s,0x24aa53u,s.gp[4]+0x3cu);push(s,0x24aa57u,0);call(s,dispatch,0x24b950u,0x24aa59u,0x24aa5eu);
    };
    process();if(extra_saved) {cursor(s,0x24aa81u);s.gp[5]=s.pop();}cursor(s,0x24aa82u);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[3]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x24u,4);s.pop();s.gp[4]+=8u;++source_objective_native_tu15_stats.owners[52];return true;
}
template<class State,class Dispatch>bool draw_assault_goals(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24aa90u,0x4f3218u);s.gp[1]=read(s,0x24aa95u,s.gp[0]+0x180u);s.gp[4]=s.flags.minus(s.gp[4],0xcu,4);s.flags.minus(s.gp[1],9,4);unsigned saved{};
    const auto process=[&] {
        if(s.gp[1]!=9)return;s.gp[1]=read(s,0x24aaa7u,0x4f54f4u);s.gp[2]=read(s,0x24aaadu,s.gp[1]+0x44u);s.gp[1]=read(s,0x24aab0u,s.gp[0]+0x240u);push(s,0x24aab6u,s.gp[3]);push(s,0x24aab7u,s.gp[5]);saved=2;
        s.gp[5]=read(s,0x24aab8u,s.gp[4]+0x18u)&65535u;s.flags.logic(s.gp[5],4);s.gp[5]=s.flags.product(s.gp[5],0x21cu);s.gp[5]=s.flags.plus(s.gp[5],s.gp[2],4);s.flags.minus(s.gp[1],2,4);
        if(s.gp[1]!=2) {
            s.set(0,read(s,0x24aacfU,s.gp[5]+0xc0u,1),1,0);s.flags.minus(s.reg(0,1,0),255,1);if(s.reg(0,1,0)==255)return;
            s.set(3,extend(s.reg(0,1,0),1),2,0);call(s,dispatch,0x160e50u,0x24aae1u,0x24aae6u);s.flags.logic(s.gp[0],4);if(!s.gp[0])return;
            const auto objective=read(s,0x24aaeeu,s.gp[0]);s.flags.minus(objective,0xffffffffu,4);if(objective==0xffffffffu)return;
        }
        push(s,0x24aaf7u,s.gp[6]);push(s,0x24aaf8u,s.gp[7]);saved=4;s.gp[7]=0;s.flags.logic(0,4);s.gp[3]=s.flags.logic(0xffffffffu,4);s.gp[6]=0x14u;
        for(unsigned team=0;team<9;++team) {
            s.gp[2]=read(s,0x24ab10u,0x4f63b8u);s.set(0,read(s,0x24ab16u,s.gp[2]+s.gp[6]-4u,2),2,0);s.flags.minus(s.reg(0,2,0),65535u,2);bool eligible=s.reg(0,2,0)!=65535u;
            if(eligible) {s.set(1,extend(read(s,0x24ab25u,s.gp[5]+0xc0u,1),1),2,0);s.flags.minus(s.reg(0,2,0),s.reg(1,2,0),2);eligible=s.reg(0,2,0)!=s.reg(1,2,0);}
            if(eligible) {s.gp[1]=extend(s.reg(0,2,0),2);call(s,dispatch,0x164890u,0x24ab39u,0x24ab3eu);s.flags.logic(s.reg(0,1,0),1);eligible=s.reg(0,1,0)!=0;}
            if(eligible) {
                for(unsigned which=0;which<2;++which) {
                    if(which) {s.gp[0]=read(s,0x24ab8au,0x52d710u);s.set(0,read(s,0x24ab8fu,s.gp[0]+s.gp[6]+0x12u,2),2,0);}
                    else {s.gp[2]=read(s,0x24ab46u,0x52d710u);s.set(0,read(s,0x24ab4cu,s.gp[2]+s.gp[6],2),2,0);}
                    s.flags.minus(s.reg(0,2,0),65535u,2);if(s.reg(0,2,0)==65535u)continue;
                    s.gp[1]=read(s,which?0x24ab9au:0x24ab56u,0x4ecc20u);s.gp[2]=read(s,which?0x24aba0u:0x24ab5cu,s.gp[1]+0x11cu);s.gp[0]=extend(s.reg(0,2,0),2);s.gp[0]=s.flags.shift(s.gp[0],5,4);
                    s.gp[1]=read(s,which?0x24abacu:0x24ab68u,s.gp[2]+s.gp[0]+4u);s.gp[0]=s.flags.plus(s.gp[0],s.gp[2],4);s.gp[2]=read(s,which?0x24abb2u:0x24ab6eu,s.gp[0]);store(s,which?0x24abb4u:0x24ab70u,s.gp[4]+0x10u,4,s.gp[2]);s.gp[2]=read(s,which?0x24abb8u:0x24ab74u,s.gp[0]+8u);store(s,which?0x24abbbu:0x24ab77u,s.gp[4]+0x14u,4,s.gp[1]);
                    s.gp[0]=s.gp[4]+0x10u;s.gp[1]=which?s.gp[3]:s.gp[7];store(s,which?0x24abc5u:0x24ab81u,s.gp[4]+0x18u,4,s.gp[2]);call(s,dispatch,0x24d920u,which?0x24abc9u:0x24ab85u,which?0x24abceu:0x24ab8au);
                }
            }
            s.gp[7]=s.flags.increment(s.gp[7],4,false);s.gp[6]=s.flags.plus(s.gp[6],2,4);s.gp[3]=s.flags.increment(s.gp[3],4,true);s.flags.minus(s.gp[6],0x26u,4);
        }
    };
    process();if(saved==4) {cursor(s,0x24abdcu);s.gp[7]=s.pop();s.gp[6]=s.pop();}if(saved>=2) {cursor(s,0x24abdeu);s.gp[5]=s.pop();s.gp[3]=s.pop();}
    s.gp[4]=s.flags.plus(s.gp[4],0xcu,4);s.pop();s.gp[4]+=4u;++source_objective_native_tu15_stats.owners[53];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
