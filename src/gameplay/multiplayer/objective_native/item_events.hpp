#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool objective_picked_up(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24a2f0u,0x4f3218u);s.set(1,read(s,0x24a2f5u,s.gp[0]+0xcu,1),1,0);
    s.gp[4]=s.flags.minus(s.gp[4],0x24u,4);s.flags.minus(s.reg(1,1,0),4,1);unsigned saved{};
    const auto process=[&] {
        if(s.reg(1,1,0)==4)return;
        s.gp[1]=read(s,0x24a304u,s.gp[4]+0x28u);s.gp[0]=read(s,0x24a308u,0x4ecbd0u);s.gp[2]=read(s,0x24a30du,s.gp[0]+0x44u);
        s.gp[1]&=65535u;s.flags.logic(s.gp[1],4);s.gp[1]=s.flags.shift(s.gp[1]*3u,2,4);
        s.gp[2]=read(s,0x24a31cu,s.gp[2]+s.gp[1]+8u);s.gp[2]=read(s,0x24a320u,s.gp[2]);s.gp[2]&=65535u;s.flags.logic(s.gp[2],4);
        push(s,0x24a328u,s.gp[6]);saved=1;s.gp[6]=read(s,0x24a329u,0x4f0414u);s.gp[2]=s.flags.shift(s.gp[2],4,4);
        s.gp[2]=read(s,0x24a332u,s.gp[6]+s.gp[2]+8u);const auto classification=read(s,0x24a336u,s.gp[2]+0x290u,2);s.flags.minus(classification,1,2);if(classification!=1)return;
        s.gp[2]=read(s,0x24a344u,s.gp[0]+0x44u);s.gp[0]=read(s,0x24a347u,s.gp[4]+0x30u);s.gp[1]=read(s,0x24a34bu,s.gp[2]+s.gp[1]+8u);
        s.gp[6]=extend(read(s,0x24a34fu,s.gp[1]+0x17eu,2),2);s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);s.flags.minus(s.gp[6],0xffffffffu,4);const auto valid=s.gp[6]!=0xffffffffu;s.gp[0]*=3u;
        push(s,0x24a361u,s.gp[3]);saved=2;s.gp[3]=read(s,0x24a362u,s.gp[2]+s.gp[0]*4u+8u);store(s,0x24a366u,s.gp[4]+0x30u,4,s.gp[3]);if(!valid)return;
        push(s,0x24a370u,s.gp[5]);s.gp[5]=read(s,0x24a371u,0x52d710u);push(s,0x24a377u,s.gp[7]);saved=4;
        s.gp[7]=read(s,0x24a378u,s.gp[3]+0x13cu);s.flags.minus(s.gp[7],0xffffffffu,4);
        if(s.gp[7]!=0xffffffffu) {
            s.gp[2]=read(s,0x24a383u,0x4f54f4u);s.gp[1]=read(s,0x24a389u,s.gp[2]+0x44u);s.gp[7]&=65535u;s.flags.logic(s.gp[7],4);
            s.gp[0]=s.flags.product(s.gp[7],0x21cu);s.gp[0]=s.flags.plus(s.gp[0],s.gp[1],4);
            const auto away=read(s,0x24a39cu,s.gp[5]+s.gp[6]+0x170u,1);s.flags.logic(away&1u,1);
            if(!(away&1u)) {
                s.gp[1]=read(s,0x24a3a6u,0x4f3218u);const auto engine=read(s,0x24a3acu,s.gp[1]+0x180u);s.flags.minus(engine,9,4);
                s.gp[2]=extend(read(s,0x24a3b3u,s.gp[0]+0xc0u,1),1);push(s,0x24a3bau,1);s.gp[0]=s.gp[7];s.gp[1]=engine==9?21u:15u;
                call(s,dispatch,0x19b1d0u,engine==9?0x24a3e6u:0x24a3c5u,engine==9?0x24a3ebu:0x24a3cau);
                if(engine!=9) {
                    s.gp[3]=1;s.gp[0]=18;s.gp[1]=s.gp[7];call(s,dispatch,0x19baa0u,0x24a3d6u,0x24a3dbu);s.gp[3]=read(s,0x24a3dbu,s.gp[4]+0x38u);
                }
            }
        }
        s.set(0,read(s,0x24a3ebu,s.gp[5]+s.gp[6]+0x170u,1),1,0);s.set(0,s.flags.logic(s.reg(0,1,0)|1u,1),1,0);
        s.set(2,s.reg(0,1,0),1,0);s.set(2,s.flags.logic(s.reg(2,1,0)&0xfdu,1),1,0);
        store(s,0x24a3f9u,s.gp[5]+s.gp[6]+0x170u,1,s.reg(0,1,0));push(s,0x24a400u,0x200u);store(s,0x24a405u,s.gp[5]+s.gp[6]+0x170u,1,s.reg(2,1,0));
        call(s,dispatch,0xb1a10u,0x24a40cu,0x24a411u);s.flags.logic(s.gp[6],4);bool team=std::bit_cast<std::int32_t>(s.gp[6])>=0;
        if(team) {s.flags.minus(s.gp[6],9,4);team=std::bit_cast<std::int32_t>(s.gp[6])<9;}
        if(team) {
            push(s,0x24a41au,0x80u);store(s,0x24a41fu,s.gp[5]+s.gp[6]*2u+0x114u,2,65535u);store(s,0x24a429u,s.gp[5]+s.gp[6]*2u+0x126u,2,0);
            call(s,dispatch,0xb1a10u,0x24a433u,0x24a438u);
        }
        s.gp[7]=read(s,0x24a438u,s.gp[3]+0x13cu);s.flags.minus(s.gp[7],0xffffffffu,4);if(s.gp[7]==0xffffffffu)return;
        s.flags.logic(s.gp[6],4);if(std::bit_cast<std::int32_t>(s.gp[6])<0)return;s.flags.minus(s.gp[6],9,4);if(std::bit_cast<std::int32_t>(s.gp[6])>=9)return;
        s.gp[2]=read(s,0x24a44cu,0x4f3218u);s.gp[0]=read(s,0x24a452u,s.gp[2]+0x180u);s.gp[1]=0;s.flags.logic(0,4);s.flags.minus(s.gp[0],9,4);
        s.gp[1]=source_objective_event_audience(std::bit_cast<std::int32_t>(s.gp[0]));s.gp[0]=s.gp[4]+0x10u;s.gp[2]=1;s.flags.plus(s.gp[1]-3u,3,4);
        call(s,dispatch,0x1a87c0u,0x24a470u,0x24a475u);s.gp[1]=s.gp[0];s.gp[0]=s.gp[7];call(s,dispatch,0x1a87f0u,0x24a479u,0x24a47eu);
        s.flags.minus(s.reg(6,2,0),8,2);s.gp[0]=s.reg(6,2,0)==8?s.flags.logic(0xffffffffu,4):extend(s.reg(6,2,0),2);
        s.gp[0]=extend(s.reg(0,2,0),2);store(s,0x24a48fu,s.gp[4]+0x28u,4,s.gp[0]);s.gp[0]=s.gp[4]+0x10u;call(s,dispatch,0x1a87a0u,0x24a497u,0x24a49cu);
    };
    process();
    if(saved==4) {cursor(s,0x24a49cu);s.gp[7]=s.pop();s.gp[5]=s.pop();}
    if(saved>=2) {cursor(s,0x24a49eu);s.gp[3]=s.pop();}
    if(saved>=1) {cursor(s,0x24a49fu);s.gp[6]=s.pop();}
    s.gp[4]=s.flags.plus(s.gp[4],0x24u,4);s.pop();s.gp[4]+=8u;++source_objective_native_tu15_stats.owners[39];return true;
}
template<class State,class Dispatch>bool objective_dropped(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[2]=read(s,0x24a4b0u,0x4f3218u);s.set(0,read(s,0x24a4b6u,s.gp[2]+0xcu,1),1,0);
    s.gp[4]=s.flags.minus(s.gp[4],0x24u,4);s.flags.minus(s.reg(0,1,0),4,1);unsigned saved{};
    const auto process=[&] {
        if(s.reg(0,1,0)==4)return;
        s.gp[0]=read(s,0x24a4c4u,0x4ecbd0u);s.gp[1]=read(s,0x24a4c9u,s.gp[0]+0x44u);s.gp[0]=read(s,0x24a4ccu,s.gp[4]+0x28u)&65535u;s.flags.logic(s.gp[0],4);
        push(s,0x24a4d5u,s.gp[6]);s.gp[0]*=3u;s.gp[0]=read(s,0x24a4d9u,s.gp[1]+s.gp[0]*4u+8u);s.gp[6]=read(s,0x24a4ddu,s.gp[0])&65535u;s.flags.logic(s.gp[6],4);
        push(s,0x24a4e5u,s.gp[7]);saved=2;s.gp[7]=read(s,0x24a4e6u,0x4f0414u);s.gp[6]=s.flags.shift(s.gp[6],4,4);s.gp[6]=read(s,0x24a4efu,s.gp[7]+s.gp[6]+8u);
        const auto classification=read(s,0x24a4f3u,s.gp[6]+0x290u,2);s.flags.minus(classification,1,2);if(classification!=1)return;
        s.gp[6]=extend(read(s,0x24a501u,s.gp[0]+0x17eu,2),2);s.flags.logic(s.gp[6],4);if(std::bit_cast<std::int32_t>(s.gp[6])<0)return;
        s.flags.minus(s.gp[6],9,4);if(std::bit_cast<std::int32_t>(s.gp[6])>=9)return;
        s.gp[0]=read(s,0x24a519u,s.gp[4]+0x34u);s.gp[7]=read(s,0x24a51du,0x52d710u);s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);s.gp[0]*=3u;
        push(s,0x24a52bu,s.gp[5]);saved=3;s.gp[5]=read(s,0x24a52cu,s.gp[1]+s.gp[0]*4u+8u);
        const auto away=read(s,0x24a530u,s.gp[7]+s.gp[6]+0x170u,1);s.flags.logic(away&1u,1);
        if(away&1u) {
            s.set(1,read(s,0x24a53au,s.gp[2]+0x230u,2),2,0);s.gp[2]=read(s,0x24a541u,0x51df54u);store(s,0x24a547u,s.gp[7]+s.gp[6]*2u+0x114u,2,s.reg(1,2,0));
            s.set(0,read(s,0x24a54fu,s.gp[2]+2u,2),2,0);store(s,0x24a553u,s.gp[7]+s.gp[6]*2u+0x126u,2,s.reg(0,2,0));
        } else {store(s,0x24a55du,s.gp[7]+s.gp[6]*2u+0x114u,2,65535u);store(s,0x24a567u,s.gp[7]+s.gp[6]*2u+0x126u,2,0);}
        push(s,0x24a571u,0x80u);call(s,dispatch,0xb1a10u,0x24a576u,0x24a57bu);s.gp[0]=read(s,0x24a57bu,0x4f3218u);
        const auto engine=read(s,0x24a580u,s.gp[0]+0x180u);s.flags.minus(engine,9,4);
        if(engine==9) {
            const auto state=read(s,0x24a589u,s.gp[7]+s.gp[6]+0x170u,1);s.flags.logic(state&1u,1);bool retain=state&1u;
            if(retain) {const auto options=read(s,0x24a593u,s.gp[0]+0x22cu,1);s.flags.logic(options&0x20u,1);retain=options&0x20u;}
            if(!retain)store(s,0x24a59cu,s.gp[7]+s.gp[6]*2u+0x1a0u,2,65535u);
            push(s,0x24a5a6u,0x100u);call(s,dispatch,0xb1a10u,0x24a5abu,0x24a5b0u);s.gp[0]=read(s,0x24a5b0u,0x4f3218u);
        }
        s.gp[5]=read(s,0x24a5b5u,s.gp[5]+0x13cu);s.flags.minus(s.gp[5],0xffffffffu,4);if(s.gp[5]==0xffffffffu)return;
        const auto suppress=read(s,0x24a5c0u,s.gp[7]+0x204u,1);s.flags.logic(suppress&1u,1);if(suppress&1u)return;
        s.gp[2]=read(s,0x24a5c9u,s.gp[0]+0x180u);s.gp[1]=0;s.flags.logic(0,4);s.flags.minus(s.gp[2],9,4);
        s.gp[1]=source_objective_event_audience(std::bit_cast<std::int32_t>(s.gp[2]));s.gp[0]=s.gp[4]+0xcu;s.gp[2]=2;s.flags.plus(s.gp[1]-3u,3,4);
        call(s,dispatch,0x1a87c0u,0x24a5e7u,0x24a5ecu);s.gp[1]=s.gp[0];s.gp[0]=s.gp[5];call(s,dispatch,0x1a87f0u,0x24a5f0u,0x24a5f5u);
        s.flags.minus(s.reg(6,2,0),8,2);s.gp[0]=s.reg(6,2,0)==8?s.flags.logic(0xffffffffu,4):extend(s.reg(6,2,0),2);
        s.gp[1]=extend(s.reg(0,2,0),2);s.gp[0]=s.gp[4]+0xcu;store(s,0x24a60au,s.gp[4]+0x24u,4,s.gp[1]);call(s,dispatch,0x1a87a0u,0x24a60eu,0x24a613u);
    };
    process();if(saved==3) {cursor(s,0x24a613u);s.gp[5]=s.pop();}if(saved>=2) {cursor(s,0x24a614u);s.gp[7]=s.pop();s.gp[6]=s.pop();}
    s.gp[4]=s.flags.plus(s.gp[4],0x24u,4);s.pop();s.gp[4]+=8u;++source_objective_native_tu15_stats.owners[40];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
