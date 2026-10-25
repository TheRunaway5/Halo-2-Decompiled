#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>void neutral_indicator(State& s,Dispatch&& dispatch) {
    const auto kind=read(s,0x24ad61u,s.gp[7]+0x240u);s.flags.minus(kind,2,4);if(kind!=2)return;
    s.gp[3]=8;call(s,dispatch,0x160e50u,0x24ad73u,0x24ad78u);s.flags.logic(s.gp[0],4);if(!s.gp[0])return;
    s.gp[1]=read(s,0x24ad80u,0x52d710u);s.set(2,read(s,0x24ad86u,s.gp[1]+0x178u,1),1,0);s.gp[0]=read(s,0x24ad8cu,s.gp[0]+0xcu);s.set(1,s.flags.logic((~s.reg(2,1,0))&1u,1),1,0);s.flags.minus(s.gp[0],0xffffffffu,4);s.set(3,!s.flags.zf,1,0);s.flags.logic(s.reg(1,1,0),1);store(s,0x24ad9eu,s.gp[4]+0x13u,1,s.reg(3,1,0));
    bool dropped=false;
    if(!s.reg(1,1,0)) {s.flags.logic(s.reg(3,1,0),1);store(s,0x24ada6u,s.gp[4]+0x11u,1,1);dropped=!s.reg(3,1,0);}
    if(!dropped)store(s,0x24adadu,s.gp[4]+0x11u,1,0);
    s.gp[6]=read(s,0x24adb2u,0x474c74u);s.gp[3]=read(s,0x24adb8u,s.gp[6]);store(s,0x24adbau,s.gp[4]+0x1cu,4,s.gp[3]);s.gp[3]=read(s,0x24adbeu,s.gp[6]+4u);s.gp[6]=read(s,0x24adc1u,s.gp[6]+8u);store(s,0x24adc4u,s.gp[4]+0x20u,4,s.gp[3]);s.set(3,read(s,0x24adc8u,s.gp[4]+0x13u,1),1,0);s.flags.logic(s.reg(3,1,0),1);store(s,0x24adceu,s.gp[4]+0x14u,1,0);store(s,0x24add3u,s.gp[4]+0x12u,1,0);store(s,0x24add8u,s.gp[4]+0x24u,4,s.gp[6]);
    bool same=false;
    if(s.reg(3,1,0)) {
        s.gp[6]=read(s,0x24addeu,0x4f54f4u);s.gp[6]=read(s,0x24ade4u,s.gp[6]+0x44u);s.set(3,read(s,0x24ade7u,s.gp[5]+0xc0u,1),1,0);s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);s.gp[0]=s.flags.product(s.gp[0],0x21cu);s.set(0,read(s,0x24adf8u,s.gp[6]+s.gp[0]+0xc0u,1),1,0);s.flags.minus(s.reg(0,1,0),s.reg(3,1,0),1);s.set(3,1,1,0);same=s.flags.zf;
    }
    if(!same)s.set(3,read(s,0x24ae05u,s.gp[4]+0x14u,1),1,0);
    s.gp[0]=read(s,0x24ae09u,s.gp[7]+0x180u);s.flags.minus(s.gp[0],9,4);const bool assault=s.gp[0]==9;
    if(assault) {s.flags.logic(s.reg(2,1,0)&8u,1);if(s.reg(2,1,0)&8u)store(s,0x24ae19u,s.gp[4]+0x12u,1,1);}
    s.set(2,read(s,0x24ae1eu,s.gp[4]+0x13u,1),1,0);s.flags.logic(s.reg(2,1,0),1);
    if(s.reg(2,1,0)) {s.flags.logic(s.reg(3,1,0),1);if(s.reg(3,1,0))return;}
    s.flags.minus(s.gp[0],9,4);s.flags.logic(s.reg(1,1,0),1);bool home_or_dropped=s.reg(1,1,0)!=0;
    if(!home_or_dropped) {s.set(0,read(s,assault?0x24ae37u:0x24ae6du,s.gp[4]+0x11u,1),1,0);s.flags.logic(s.reg(0,1,0),1);home_or_dropped=s.reg(0,1,0)!=0;}
    bool visible{};
    if(home_or_dropped) {const auto mode=read(s,0x24ae8bu,s.gp[7]+0x23cu);s.flags.minus(mode,3,4);s.set(0,!s.flags.zf,1,0);s.flags.logic(s.reg(0,1,0),1);visible=s.reg(0,1,0)!=0;}
    else {
        s.flags.logic(s.reg(2,1,0),1);if(!s.reg(2,1,0))return;s.flags.logic(s.reg(3,1,0),1);if(s.reg(3,1,0))return;
        s.gp[0]=read(s,assault?0x24ae4fu:0x24ae7du,s.gp[7]+0x23cu);
        if(assault) {
            s.gp[0]=s.flags.minus(s.gp[0],0,4);visible=s.gp[0]==0;
            if(!visible) {s.gp[0]=s.flags.minus(s.gp[0],2,4);if(s.gp[0]==0) {s.set(0,read(s,0x24ae63u,s.gp[4]+0x12u,1),1,0);s.flags.logic(s.reg(0,1,0),1);visible=s.reg(0,1,0)!=0;}}
        } else {s.gp[0]=s.flags.increment(s.gp[0],4,true);visible=s.gp[0]==0;if(!visible) {s.gp[0]=s.flags.increment(s.gp[0],4,true);visible=s.gp[0]==0;}}
    }
    if(!visible)return;
    s.gp[3]=8;call(s,dispatch,0x160e50u,0x24ae9eu,0x24aea3u);s.flags.logic(s.gp[0],4);if(!s.gp[0])return;
    s.gp[1]=read(s,0x24aea7u,s.gp[0]+0xcu);s.flags.minus(s.gp[1],0xffffffffu,4);
    if(s.gp[1]!=0xffffffffu) {
        s.gp[2]=read(s,0x24aeafu,0x4f54f4u);s.gp[2]=read(s,0x24aeb5u,s.gp[2]+0x44u);s.gp[1]&=65535u;s.flags.logic(s.gp[1],4);s.gp[1]=s.flags.product(s.gp[1],0x21cu);s.set(1,read(s,0x24aec4u,s.gp[2]+s.gp[1]+0xc0u,1),1,0);const auto team=read(s,0x24aecbu,s.gp[5]+0xc0u,1);s.flags.minus(s.reg(1,1,0),team,1);if(s.reg(1,1,0)==team)return;
    }
    s.gp[2]=read(s,0x24aed3u,s.gp[0]);push(s,0x24aed5u,s.gp[2]);s.gp[7]=s.gp[4]+0x20u;s.gp[6]=s.gp[4]+0x54u;call(s,dispatch,0x24d530u,0x24aedeu,0x24aee3u);s.flags.logic(s.reg(0,1,0),1);
    if(s.reg(0,1,0)) {s.gp[0]=s.gp[6];push(s,0x24aee9u,s.gp[0]);call(s,dispatch,0x25c4cfu,0x24aeeau,0x24aeefu);}
}
template<class State,class Dispatch>void team_indicator(State& s,Dispatch&& dispatch) {
    s.gp[0]=read(s,0x24aef1u,0x4f63b8u);s.gp[1]=read(s,0x24aef6u,s.gp[0]+0xc14u);s.gp[2]=read(s,0x24aefcu,0x56dfa0u+s.gp[1]*4u);s.flags.logic(s.gp[2],4);if(!s.gp[2])return;
    s.gp[2]=read(s,0x24af0bu,0x4f3218u);s.set(1,read(s,0x24af11u,s.gp[2]+0x184u,1),1,0);s.set(1,s.flags.logic(s.reg(1,1,0)&1u,1),1,0);store(s,0x24af1au,s.gp[4]+0x14u,1,s.reg(1,1,0));if(!s.reg(1,1,0))return;
    s.flags.logic(s.gp[5],4);if(std::bit_cast<std::int32_t>(s.gp[5])<0)return;s.flags.minus(s.gp[5],8,4);if(std::bit_cast<std::int32_t>(s.gp[5])>=8)return;
    s.gp[2]=read(s,0x24af35u,s.gp[0]+0xcu,2);s.gp[0]=1;s.gp[1]=s.gp[5];s.gp[0]=s.flags.shift(s.gp[0],s.reg(1,1,0),4);s.flags.logic(s.gp[0]&s.gp[2],4);s.set(0,!s.flags.zf,1,0);s.flags.logic(s.reg(0,1,0),1);if(!s.reg(0,1,0))return;
    s.gp[3]=s.gp[5];call(s,dispatch,0x160e50u,0x24af51u,0x24af56u);s.gp[1]=read(s,0x24af56u,s.gp[4]+0x18u);store(s,0x24af5au,s.gp[4]+0x2cu,4,s.gp[0]);s.gp[0]=extend(read(s,0x24af5eu,s.gp[1]+0xc0u,1),1);s.flags.minus(s.gp[5],s.gp[0],4);bool draw_home{};
    if(s.gp[5]==s.gp[0]) {s.set(1,read(s,0x24af69u,s.gp[4]+0x15u,1),1,0);s.flags.logic(s.reg(1,1,0),1);draw_home=s.reg(1,1,0)!=0;if(!draw_home)s.flags.minus(s.gp[5],s.gp[0],4);}
    else {s.set(0,read(s,0x24af75u,s.gp[4]+0x17u,1),1,0);s.flags.logic(s.reg(0,1,0),1);draw_home=s.reg(0,1,0)!=0;}
    s.gp[7]=read(s,draw_home?0x24af9du:0x24af7du,0x52d710u);
    if(!draw_home) {const auto selected=read(s,0x24af83u,s.gp[7]+0x1f4u);s.flags.minus(s.gp[5],selected,4);if(s.gp[5]==selected) {s.set(0,read(s,0x24af8fu,s.gp[4]+0x16u,1),1,0);s.flags.logic(s.reg(0,1,0),1);draw_home=s.reg(0,1,0)!=0;}}
    if(draw_home) {
        push(s,0x24afa3u,s.gp[5]);call(s,dispatch,0x15b3b0u,0x24afa4u,0x24afa9u);s.gp[6]=extend(s.reg(0,2,0),2);s.flags.minus(s.gp[6],0xffffffffu,4);
        if(s.gp[6]!=0xffffffffu) {
            s.gp[3]=0;s.flags.logic(0,4);
            for(unsigned which=0;which<2;++which) {
                s.flags.logic(s.gp[3],4);s.set(0,read(s,which?0x24afbeu:0x24afb7u,s.gp[7]+s.gp[6]*2u+(which?0x26u:0x14u),2),2,0);s.gp[0]=extend(s.reg(0,2,0),2);s.flags.minus(s.gp[0],0xffffffffu,4);
                if(s.gp[0]!=0xffffffffu) {
                    s.gp[2]=read(s,0x24afcbu,0x4ecc20u);s.gp[1]=read(s,0x24afd1u,s.gp[2]+0x11cu);s.gp[0]=s.flags.shift(s.gp[0],5,4);s.gp[2]=read(s,0x24afdau,s.gp[1]+s.gp[0]+4u);s.gp[0]=s.flags.plus(s.gp[0],s.gp[1],4);s.gp[1]=read(s,0x24afe0u,s.gp[0]);s.gp[0]=read(s,0x24afe2u,s.gp[0]+8u);store(s,0x24afe5u,s.gp[4]+0x1cu,4,s.gp[1]);store(s,0x24afe9u,s.gp[4]+0x20u,4,s.gp[2]);s.gp[2]=s.gp[4]+0x1cu;s.gp[1]=s.gp[4]+0x50u;store(s,0x24aff5u,s.gp[4]+0x24u,4,s.gp[0]);call(s,dispatch,0x24d1c0u,0x24aff9u,0x24affeu);s.flags.logic(s.reg(0,1,0),1);
                    if(s.reg(0,1,0)) {push(s,0x24b002u,s.gp[1]);call(s,dispatch,0x25c4cfu,0x24b003u,0x24b008u);s.gp[7]=read(s,0x24b008u,0x52d710u);}
                }
                s.gp[3]=s.flags.increment(s.gp[3],4,false);s.flags.minus(s.gp[3],2,4);
            }
        }
    }
    s.gp[0]=read(s,0x24b014u,s.gp[4]+0x2cu);s.flags.logic(s.gp[0],4);if(!s.gp[0])return;
    s.set(3,read(s,0x24b020u,s.gp[7]+s.gp[5]+0x170u,1),1,0);s.gp[2]=s.gp[0];s.gp[6]=read(s,0x24b029u,s.gp[2]+0xcu);s.set(3,s.flags.logic((~s.reg(3,1,0))&1u,1),1,0);s.flags.minus(s.gp[6],0xffffffffu,4);s.set(0,!s.flags.zf,1,0);s.flags.logic(s.reg(3,1,0),1);store(s,0x24b039u,s.gp[4]+0x12u,1,s.reg(0,1,0));
    bool dropped=false;if(!s.reg(3,1,0)) {s.flags.logic(s.reg(0,1,0),1);store(s,0x24b041u,s.gp[4]+0x11u,1,1);dropped=!s.reg(0,1,0);}if(!dropped)store(s,0x24b048u,s.gp[4]+0x11u,1,0);
    push(s,0x24b04du,s.gp[5]);s.gp[0]=s.gp[4]+0x48u;store(s,0x24b052u,s.gp[4]+0x18u,1,0);store(s,0x24b057u,s.gp[4]+0x17u,1,0);call(s,dispatch,0x83050u,0x24b05cu,0x24b061u);s.gp[1]=read(s,0x24b061u,s.gp[0]);s.gp[2]=read(s,0x24b063u,s.gp[0]+4u);s.gp[0]=read(s,0x24b066u,s.gp[0]+8u);store(s,0x24b069u,s.gp[4]+0x38u,4,s.gp[0]);s.set(0,read(s,0x24b06du,s.gp[4]+0x12u,1),1,0);s.flags.logic(s.reg(0,1,0),1);store(s,0x24b073u,s.gp[4]+0x30u,4,s.gp[1]);store(s,0x24b077u,s.gp[4]+0x34u,4,s.gp[2]);
    bool same=false;
    if(s.reg(0,1,0)) {s.gp[1]=read(s,0x24b07du,0x4f54f4u);s.gp[2]=read(s,0x24b083u,s.gp[1]+0x44u);s.gp[1]=read(s,0x24b086u,s.gp[4]+0x18u);s.gp[6]&=65535u;s.flags.logic(s.gp[6],4);s.gp[6]=s.flags.product(s.gp[6],0x21cu);s.set(0,read(s,0x24b096u,s.gp[2]+s.gp[6]+0xc0u,1),1,0);const auto team=read(s,0x24b09du,s.gp[1]+0xc0u,1);s.flags.minus(s.reg(0,1,0),team,1);s.set(2,1,1,0);same=s.reg(0,1,0)==team;}
    if(!same)s.set(2,read(s,0x24b0a7u,s.gp[4]+0x14u,1),1,0);
    s.gp[0]=read(s,0x24b0abu,0x4f3218u);s.gp[1]=read(s,0x24b0b0u,s.gp[0]+0x180u);s.flags.minus(s.gp[1],9,4);const auto assault=s.gp[1]==9;bool armed=false;
    if(assault) {const auto flags=read(s,0x24b0bbu,s.gp[7]+s.gp[5]+0x170u,1);s.flags.logic(flags&8u,1);s.set(0,1,1,0);armed=flags&8u;}if(!armed)s.set(0,read(s,0x24b0c7u,s.gp[4]+0x13u,1),1,0);
    const auto carried=read(s,0x24b0cbu,s.gp[4]+0x12u,1);s.flags.minus(carried,0,1);if(carried) {s.flags.logic(s.reg(2,1,0),1);if(s.reg(2,1,0))return;}
    s.flags.minus(s.gp[1],9,4);bool visible{};
    if(assault) {
        s.gp[1]=read(s,0x24b0dfu,s.gp[4]+0x18u);s.gp[2]=extend(read(s,0x24b0e3u,s.gp[1]+0xc0u,1),1);s.flags.minus(s.gp[5],s.gp[2],4);visible=s.gp[5]==s.gp[2];
        if(!visible) {
            s.gp[1]=read(s,0x24b0eeu,0x4f3218u);s.gp[1]=read(s,0x24b0f4u,s.gp[1]+0x23cu);s.gp[1]=s.flags.minus(s.gp[1],0,4);visible=s.gp[1]==0;
            if(!visible) {s.gp[1]=s.flags.increment(s.gp[1],4,true);if(!s.gp[1])s.set(0,read(s,0x24b107u,s.gp[4]+0x11u,1),1,0);else {s.gp[1]=s.flags.increment(s.gp[1],4,true);if(s.gp[1])return;}s.flags.logic(s.reg(0,1,0),1);visible=s.reg(0,1,0)!=0;}
        }
    } else {
        s.gp[2]=read(s,0x24b10du,s.gp[4]+0x18u);s.gp[0]=extend(read(s,0x24b111u,s.gp[2]+0xc0u,1),1);s.flags.minus(s.gp[5],s.gp[0],4);visible=s.gp[5]!=s.gp[0];
        if(!visible) {
            s.gp[1]=read(s,0x24b11cu,0x4f3218u);s.gp[0]=read(s,0x24b122u,s.gp[1]+0x23cu);s.gp[0]=s.flags.minus(s.gp[0],0,4);
            if(!s.gp[0])s.set(0,read(s,0x24b13au,s.gp[4]+0x11u,1),1,0);
            else {s.gp[0]=s.flags.increment(s.gp[0],4,true);if(!s.gp[0])visible=true;else {s.gp[0]=s.flags.increment(s.gp[0],4,true);if(s.gp[0])return;s.flags.logic(s.reg(3,1,0),1);s.set(0,s.flags.zf,1,0);}}
            if(!visible) {s.flags.logic(s.reg(0,1,0),1);visible=s.reg(0,1,0)!=0;}
        }
    }
    if(!visible)return;s.gp[2]=read(s,0x24b142u,s.gp[4]+0x2cu);s.gp[0]=read(s,0x24b146u,s.gp[2]);push(s,0x24b148u,s.gp[0]);s.gp[7]=s.gp[4]+0x34u;s.gp[6]=s.gp[4]+0xe4u;call(s,dispatch,0x24d530u,0x24b154u,0x24b159u);s.flags.logic(s.reg(0,1,0),1);if(s.reg(0,1,0)) {s.gp[1]=s.gp[6];push(s,0x24b15fu,s.gp[1]);call(s,dispatch,0x25c4cfu,0x24b160u,0x24b165u);}
}
template<class State,class Dispatch>void allied_indicators(State& s,Dispatch&& dispatch) {
    const auto local=read(s,0x24b16fu,s.gp[4]+0x28u);s.flags.minus(local,0xffffffffu,4);if(local==0xffffffffu)return;
    s.gp[0]=read(s,0x24b17au,0x4f54f4u);s.gp[7]=read(s,0x24b17fu,s.gp[0]+0x44u);s.gp[2]=read(s,0x24b182u,s.gp[4]+0x40u);s.gp[1]=read(s,0x24b186u,s.gp[7]+s.gp[2]+0x2cu);s.gp[7]=s.flags.plus(s.gp[7],s.gp[2],4);s.gp[3]=s.flags.logic(0xffffffffu,4);s.flags.minus(s.gp[1],s.gp[3],4);if(s.gp[1]==s.gp[3])return;
    s.gp[6]=s.gp[4]+0x30u;store(s,0x24b19bu,s.gp[4]+0x34u,4,s.gp[0]);store(s,0x24b19fu,s.gp[4]+0x3cu,4,s.gp[3]);store(s,0x24b1a3u,s.gp[4]+0x38u,4,s.gp[3]);call(s,dispatch,0x1a8e50u,0x24b1a7u,0x24b1acu);s.flags.logic(s.reg(0,1,0),1);
    while(s.reg(0,1,0)) {
        s.gp[6]=read(s,0x24b1c0u,s.gp[4]+0x38u);const auto self=read(s,0x24b1c4u,s.gp[4]+0x28u);s.flags.minus(s.gp[6],self,4);
        const auto append=[&] {
            if(s.gp[6]==self)return;
            s.gp[1]=read(s,0x24b1ceu,0x4f63b8u);s.gp[1]=read(s,0x24b1d4u,s.gp[1]+0xc14u);s.gp[1]=read(s,0x24b1dau,0x56dfa0u+s.gp[1]*4u);s.flags.logic(s.gp[1],4);const auto registered=s.gp[1]!=0;s.gp[2]=read(s,0x24b1e3u,s.gp[4]+0x30u);s.set(0,extend(read(s,0x24b1e7u,s.gp[7]+0xc0u,1),1),2,0);s.set(2,extend(read(s,0x24b1efu,s.gp[2]+0xc0u,1),1),2,0);
            if(registered) {
                s.gp[5]=read(s,0x24b1f9u,s.gp[1]);push(s,0x24b1fbu,s.gp[0]);push(s,0x24b1fcu,s.gp[2]);const auto relationship=read(s,0x24b1fdu,s.gp[5]+0x6cu);call(s,dispatch,relationship,0x24b1fdu,0x24b200u);s.flags.logic(s.reg(0,1,0),1);if(s.reg(0,1,0))return;
            }
            push(s,0x24b208u,s.gp[6]);s.gp[0]=s.gp[4]+0x54u;call(s,dispatch,0x164fd0u,0x24b20du,0x24b212u);s.flags.logic(s.reg(0,1,0),1);if(!s.reg(0,1,0))return;
            push(s,0x24b216u,1);s.gp[0]=s.gp[6];call(s,dispatch,0x1a8fd0u,0x24b21au,0x24b21fu);s.gp[6]=s.gp[0];s.flags.minus(s.gp[6],s.gp[3],4);
            bool objective=s.gp[6]!=s.gp[3];
            if(objective) {
                s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);s.gp[2]=s.gp[0]*3u;s.gp[0]=read(s,0x24b22du,0x4ecbd0u);s.gp[1]=read(s,0x24b232u,s.gp[0]+0x44u);s.gp[2]=read(s,0x24b235u,s.gp[1]+s.gp[2]*4u+8u);s.gp[0]=extend(read(s,0x24b239u,s.gp[2]+0x17eu,2),2);s.flags.minus(s.gp[0],s.gp[3],4);objective=s.gp[0]!=s.gp[3];
            }
            if(objective) {
                s.flags.minus(s.gp[0],8,4);
                if(s.gp[0]==8) {s.gp[0]=read(s,0x24b249u,0x474c74u);s.gp[1]=read(s,0x24b24eu,s.gp[0]);s.gp[2]=read(s,0x24b250u,s.gp[0]+4u);}
                else {push(s,0x24b255u,s.gp[0]);s.gp[0]=s.gp[4]+0x48u;call(s,dispatch,0x83050u,0x24b25au,0x24b25fu);s.gp[1]=read(s,0x24b25fu,s.gp[0]);s.gp[2]=read(s,0x24b261u,s.gp[0]+4u);}
                s.gp[0]=read(s,0x24b264u,s.gp[0]+8u);store(s,0x24b267u,s.gp[4]+0x1cu,4,s.gp[1]);store(s,0x24b26bu,s.gp[4]+0x20u,4,s.gp[2]);store(s,0x24b26fu,s.gp[4]+0x24u,4,s.gp[0]);s.gp[1]=s.gp[4]+0x1cu;s.gp[2]=s.gp[6];s.gp[0]=s.gp[4]+0x50u;call(s,dispatch,0x24d390u,0x24b27du,0x24b282u);
            }
            s.gp[1]=s.gp[4]+0x50u;push(s,0x24b286u,s.gp[1]);call(s,dispatch,0x25c4cfu,0x24b287u,0x24b28cu);
        };
        append();s.gp[6]=s.gp[4]+0x30u;call(s,dispatch,0x1a8e50u,0x24b290u,0x24b295u);s.flags.logic(s.reg(0,1,0),1);
    }
}
template<class State,class Dispatch>bool objective_indicators(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x160u,4);s.gp[0]=read(s,0x24acb6u,s.gp[4]+0x164u);push(s,0x24acbdu,s.gp[3]);push(s,0x24acbeu,s.gp[5]);push(s,0x24acbfu,s.gp[6]);s.gp[6]=s.flags.logic(0xffffffffu,4);s.flags.minus(s.gp[0],0xffffffffu,4);push(s,0x24acc6u,s.gp[7]);store(s,0x24acc7u,s.gp[4]+0x28u,4,s.gp[6]);
    if(s.gp[0]!=0xffffffffu) {s.gp[1]=read(s,0x24accdu,0x4f54f0u);s.gp[6]=read(s,0x24acd3u,s.gp[1]+s.gp[0]*4u+0xcu);store(s,0x24acd7u,s.gp[4]+0x28u,4,s.gp[6]);}
    s.gp[2]=read(s,0x24acdbu,0x4f54f4u);s.gp[5]=read(s,0x24ace1u,s.gp[2]+0x44u);s.gp[7]=read(s,0x24ace4u,0x4f3218u);s.gp[0]=s.gp[6]&65535u;s.flags.logic(s.gp[0],4);s.gp[0]=s.flags.product(s.gp[0],0x21cu);s.gp[5]=s.flags.plus(s.gp[5],s.gp[0],4);store(s,0x24acf9u,s.gp[4]+0x40u,4,s.gp[0]);
    const auto engine=read(s,0x24acfdu,s.gp[7]+0x180u);s.flags.minus(engine,9,4);store(s,0x24ad04u,s.gp[4]+0x18u,4,s.gp[5]);store(s,0x24ad08u,s.gp[4]+0x15u,1,0);store(s,0x24ad0du,s.gp[4]+0x17u,1,0);store(s,0x24ad12u,s.gp[4]+0x16u,1,0);push(s,0x24ad17u,1);s.gp[0]=s.gp[6];const bool assault=engine==9;
    call(s,dispatch,0x1a8fd0u,assault?0x24ad1du:0x24ad48u,assault?0x24ad22u:0x24ad4du);s.flags.minus(s.gp[0],0xffffffffu,4);bool carrier=s.gp[0]!=0xffffffffu;
    if(!carrier) {push(s,assault?0x24ad27u:0x24ad52u,s.gp[6]);call(s,dispatch,0x24abf0u,assault?0x24ad28u:0x24ad53u,assault?0x24ad2du:0x24ad58u);s.flags.logic(s.reg(0,1,0),1);carrier=s.reg(0,1,0)!=0;}
    if(carrier) {
        if(assault) {const auto kind=read(s,0x24ad31u,s.gp[7]+0x240u);s.flags.minus(kind,1,4);store(s,kind==1?0x24ad3au:0x24ad41u,s.gp[4]+(kind==1?0x16u:0x17u),1,1);}
        else store(s,0x24ad5cu,s.gp[4]+0x15u,1,1);
    }
    neutral_indicator(s,dispatch);s.gp[5]=0;s.flags.logic(0,4);
    for(unsigned team=0;team<8;++team) {team_indicator(s,dispatch);s.gp[5]=s.flags.increment(s.gp[5],4,false);s.flags.minus(s.gp[5],8,4);}
    allied_indicators(s,dispatch);cursor(s,0x24b29du);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[3]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x160u,4);s.pop();s.gp[4]+=4u;++source_objective_native_tu15_stats.owners[56];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
