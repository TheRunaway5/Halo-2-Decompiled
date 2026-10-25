#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>void objective_countdown(State& s,Dispatch&& dispatch) {
    s.set(2,read(s,0x249d42u,s.gp[6]+s.gp[7]*2u+0x114u,2),2,0);s.flags.minus(s.reg(2,2,0),65535u,2);if(s.reg(2,2,0)==65535u)return;
    s.gp[1]=read(s,0x249d54u,0x4f3218u);const auto kind=read(s,0x249d5au,s.gp[1]+0x180u);s.flags.minus(kind,9,4);bool count=false;
    if(kind==9) {s.set(0,read(s,0x249d63u,s.gp[4]+0x10u,1),1,0);s.flags.logic(s.reg(0,1,0),1);count=s.reg(0,1,0)!=0;}
    if(!count) {s.set(0,read(s,0x249d6bu,s.gp[4]+0x11u,1),1,0);s.flags.logic(s.reg(0,1,0),1);if(s.reg(0,1,0))return;}
    s.gp[0]=extend(read(s,0x249d77u,s.gp[6]+s.gp[7]*2u+0x126u,2),2);s.gp[0]=s.flags.increment(s.gp[0],4,true);s.gp[3]=read(s,0x249d80u,0x51df54u);
    if(std::bit_cast<std::int32_t>(s.gp[0])<0)s.gp[0]=s.flags.logic(0,4);
    else {s.gp[1]=extend(read(s,0x249d8cu,s.gp[3]+2u,2),2);s.flags.minus(s.gp[0],s.gp[1],4);if(std::bit_cast<std::int32_t>(s.gp[0])>std::bit_cast<std::int32_t>(s.gp[1]))s.gp[0]=s.gp[1];}
    s.flags.logic(s.reg(0,2,0),2);store(s,0x249d99u,s.gp[6]+s.gp[7]*2u+0x126u,2,s.reg(0,2,0));if(s.reg(0,2,0))return;
    s.gp[0]=extend(s.reg(2,2,0),2);s.gp[2]=s.gp[0]-1u;s.flags.logic(s.gp[2],4);
    if(std::bit_cast<std::int32_t>(s.gp[2])<0)s.gp[0]=s.flags.logic(0,4);
    else {s.gp[1]=read(s,0x249db5u,s.gp[4]+0x1cu);s.gp[0]=s.flags.increment(s.gp[0],4,true);s.flags.minus(s.gp[0],s.gp[1],4);if(s.gp[0]>s.gp[1])s.gp[0]=s.gp[1];}
    push(s,0x249dc0u,0x80u);store(s,0x249dc5u,s.gp[6]+s.gp[7]*2u+0x114u,2,s.reg(0,2,0));call(s,dispatch,0xb1a10u,0x249dcdu,0x249dd2u);
    const auto remaining=read(s,0x249dd2u,s.gp[6]+s.gp[7]*2u+0x114u,2);s.flags.minus(remaining,0,2);
    if(remaining) {s.set(2,read(s,0x249e7bu,s.gp[3]+2u,2),2,0);store(s,0x249e7fu,s.gp[6]+s.gp[7]*2u+0x126u,2,s.reg(2,2,0));return;}
    s.set(0,read(s,0x249de1u,s.gp[4]+0x12u,1),1,0);s.flags.logic(s.reg(0,1,0),1);bool expired=s.reg(0,1,0)==0;
    if(expired) {s.gp[0]=read(s,0x249dedu,s.gp[4]+0x20u);s.gp[1]=read(s,0x249df1u,s.gp[0]+4u);s.gp[1]=s.flags.right(s.gp[1],8,4,false);s.flags.logic(s.reg(1,1,0)&1u,1);expired=s.reg(1,1,0)&1u;}
    if(!expired) {s.set(1,read(s,0x249e6du,s.gp[3]+2u,2),2,0);store(s,0x249e71u,s.gp[6]+s.gp[7]*2u+0x126u,2,s.reg(1,2,0));return;}
    s.gp[2]=read(s,0x249dfcu,0x4f3218u);s.gp[0]=read(s,0x249e02u,s.gp[2]+0x180u);s.flags.minus(s.gp[0],9,4);bool announce=true;
    if(s.gp[0]==9) {const auto flags=read(s,0x249e0du,s.gp[6]+s.gp[7]+0x170u,1);s.flags.logic(flags&2u,1);announce=!(flags&2u);}
    if(announce) {const auto flags=read(s,0x249e17u,s.gp[6]+s.gp[7]+0x170u,1);s.flags.logic(flags&1u,1);announce=flags&1u;}
    if(announce) {
        s.gp[1]=0;s.flags.logic(0,4);s.flags.minus(s.gp[0],9,4);s.gp[1]=source_objective_event_audience(std::bit_cast<std::int32_t>(s.gp[0]));s.gp[0]=s.gp[4]+0x24u;s.gp[2]=4;s.flags.plus(s.gp[1]-3u,3,4);call(s,dispatch,0x1a87c0u,0x249e39u,0x249e3eu);
        s.flags.minus(s.reg(7,2,0),8,2);s.gp[0]=s.reg(7,2,0)==8?s.flags.logic(0xffffffffu,4):extend(s.reg(7,2,0),2);s.gp[0]=extend(s.reg(0,2,0),2);store(s,0x249e4fu,s.gp[4]+0x3cu,4,s.gp[0]);s.gp[0]=s.gp[4]+0x24u;call(s,dispatch,0x1a87a0u,0x249e57u,0x249e5cu);
    }
    push(s,0x249e5cu,0);s.gp[0]=s.gp[5];call(s,dispatch,0x24b950u,0x249e60u,0x249e65u);s.gp[6]=read(s,0x249e65u,0x52d710u);
}
template<class State,class Dispatch>void objective_contest(State& s,Dispatch&& dispatch) {
    s.gp[0]=read(s,0x249c41u,0x4f3218u);const auto engine=read(s,0x249c46u,s.gp[0]+0x180u);s.flags.minus(engine,9,4);bool near_test=true;
    if(engine==9) {s.flags.logic(s.reg(3,1,0),1);near_test=s.reg(3,1,0)==0;}
    if(near_test) {
        s.gp[5]=read(s,0x249c57u,s.gp[4]+0x4cu);s.gp[1]=s.gp[4]+0x18u;push(s,0x249c5fu,s.gp[1]);s.gp[0]=s.gp[5];call(s,dispatch,0x24d5f0u,0x249c62u,0x249c67u);s.flags.logic(s.reg(0,1,0),1);
        if(s.reg(0,1,0)) {
            const auto flags=read(s,0x249c6fu,s.gp[6]+s.gp[7]+0x170u,1);s.flags.logic(flags&4u,1);bool announce=!(flags&4u);
            if(announce) {
                s.gp[0]=read(s,0x249c79u,0x51df54u);s.gp[1]=extend(read(s,0x249c7eu,s.gp[0]+2u,2),2);s.gp[2]=extend(read(s,0x249c82u,s.gp[6]+s.gp[7]*2u+0x138u,2),2);s.gp[1]=s.flags.shift(s.gp[1],2,4);s.flags.minus(s.gp[2],s.gp[1],4);announce=std::bit_cast<std::int32_t>(s.gp[2])>std::bit_cast<std::int32_t>(s.gp[1]);
            }
            if(announce) {
                s.gp[2]=read(s,0x249c91u,0x4f3218u);s.gp[0]=read(s,0x249c97u,s.gp[2]+0x180u);s.gp[2]=0;s.flags.logic(0,4);s.flags.minus(s.gp[0],9,4);s.set(2,s.flags.zf,1,0);s.gp[1]=0;s.flags.logic(0,4);s.flags.minus(s.gp[0],9,4);s.gp[1]=source_objective_event_audience(std::bit_cast<std::int32_t>(s.gp[0]));s.gp[0]=s.gp[4]+0x24u;s.gp[2]=12u+s.gp[2]*4u;s.flags.plus(s.gp[1]-3u,3,4);call(s,dispatch,0x1a87c0u,0x249cbfu,0x249cc4u);
                s.flags.minus(s.reg(7,2,0),8,2);s.gp[0]=s.reg(7,2,0)==8?s.flags.logic(0xffffffffu,4):extend(s.reg(7,2,0),2);s.gp[0]=extend(s.reg(0,2,0),2);store(s,0x249cd5u,s.gp[4]+0x3cu,4,s.gp[0]);s.gp[0]=read(s,0x249cd9u,s.gp[4]+0x18u);s.gp[1]=s.gp[4]+0x24u;call(s,dispatch,0x1a87f0u,0x249ce1u,0x249ce6u);s.gp[0]=s.gp[1];call(s,dispatch,0x1a87a0u,0x249ce8u,0x249cedu);s.gp[6]=read(s,0x249cedu,0x52d710u);
            }
            const auto at=s.gp[6]+s.gp[7]+0x170u;store(s,0x249cf3u,at,1,s.flags.logic(read(s,0x249cf3u,at,1)|4u,1));store(s,0x249cfbu,s.gp[4]+0x11u,1,1);store(s,0x249d00u,s.gp[6]+s.gp[7]*2u+0x138u,2,0);push(s,0x249d38u,0x200u);call(s,dispatch,0xb1a10u,0x249d3du,0x249d42u);return;
        }
        s.flags.logic(s.reg(3,1,0),1);bool clear=s.reg(3,1,0)!=0;
        if(!clear) {s.set(0,read(s,0x249d10u,s.gp[4]+0x12u,1),1,0);s.flags.logic(s.reg(0,1,0),1);clear=s.reg(0,1,0)!=0;}
        if(clear)store(s,0x249d26u,s.gp[6]+s.gp[7]*2u+0x138u,2,0);
        else {const auto at=s.gp[6]+s.gp[7]*2u+0x138u;store(s,0x249d18u,at,2,s.flags.increment(read(s,0x249d18u,at,2),2,false));}
    } else {s.gp[5]=read(s,0x249d22u,s.gp[4]+0x4cu);store(s,0x249d26u,s.gp[6]+s.gp[7]*2u+0x138u,2,0);}
    const auto at=s.gp[6]+s.gp[7]+0x170u;store(s,0x249d30u,at,1,s.flags.logic(read(s,0x249d30u,at,1)&0xfbu,1));push(s,0x249d38u,0x200u);call(s,dispatch,0xb1a10u,0x249d3du,0x249d42u);
}
template<class State,class Dispatch>bool objective_tick(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x38u,4);s.gp[0]=read(s,0x249b23u,s.gp[4]+0x3cu);s.gp[1]=read(s,0x249b27u,0x4ecbd0u);s.gp[2]=read(s,0x249b2du,s.gp[1]+0x44u);s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);push(s,0x249b35u,s.gp[5]);s.gp[5]=read(s,0x249b36u,0x4f3218u);push(s,0x249b3cu,s.gp[6]);s.gp[6]=read(s,0x249b3du,s.gp[5]+0x230u);s.gp[0]*=3u;s.gp[0]=read(s,0x249b46u,s.gp[2]+s.gp[0]*4u+8u);push(s,0x249b4au,s.gp[7]);s.gp[7]=extend(read(s,0x249b4bu,s.gp[0]+0x17eu,2),2);s.flags.minus(s.gp[7],0xffffffffu,4);store(s,0x249b55u,s.gp[4]+0x1cu,4,s.gp[0]);store(s,0x249b59u,s.gp[4]+0x18u,4,s.gp[6]);
    const bool team=s.gp[7]!=0xffffffffu;
    if(team) {
        push(s,0x249b63u,s.gp[3]);s.gp[3]=s.gp[7];call(s,dispatch,0x160e50u,0x249b66u,0x249b6bu);s.flags.minus(s.gp[6],5,4);store(s,0x249b6eu,s.gp[4]+0x14u,4,s.gp[0]);if(s.gp[6]<=5)store(s,0x249b74u,s.gp[4]+0x1cu,4,5);
        const auto mode=read(s,0x249b7cu,s.gp[5]+0xcu,1);s.flags.minus(mode,4,1);bool host=mode!=4;
        if(host) {s.flags.logic(s.gp[7],4);host=std::bit_cast<std::int32_t>(s.gp[7])>=0;}
        if(host) {s.flags.minus(s.gp[7],9,4);host=std::bit_cast<std::int32_t>(s.gp[7])<9;}
        if(host) {s.flags.logic(s.gp[0],4);host=s.gp[0]!=0;}
        if(host) {
            s.gp[0]=read(s,0x249b9fu,s.gp[4]+0x14u);s.gp[1]=read(s,0x249ba3u,s.gp[0]+0xcu);s.gp[6]=read(s,0x249ba6u,0x52d710u);s.set(0,read(s,0x249bacu,s.gp[6]+s.gp[7]+0x170u,1),1,0);s.gp[5]=read(s,0x249bb3u,0x4f3218u);s.gp[5]=read(s,0x249bb9u,s.gp[5]+0x180u);s.flags.minus(s.gp[1],0xffffffffu,4);s.set(3,s.reg(0,1,0),1,0);s.set(2,!s.flags.zf,1,0);s.set(3,s.flags.logic((~s.reg(3,1,0))&1u,1),1,0);s.flags.minus(s.gp[5],9,4);
            store(s,0x249bcfu,s.gp[4]+0x12u,1,s.reg(2,1,0));store(s,0x249bd3u,s.gp[4]+0x13u,1,s.reg(3,1,0));store(s,0x249bd7u,s.gp[4]+0x11u,1,0);
            bool armed=false;if(s.gp[5]==9) {s.flags.logic(s.reg(0,1,0)&8u,1);store(s,0x249be0u,s.gp[4]+0x10u,1,1);armed=s.reg(0,1,0)&8u;}if(!armed)store(s,0x249be7u,s.gp[4]+0x10u,1,0);
            s.flags.logic(s.reg(2,1,0),1);store(s,0x249beeu,s.gp[4]+0x18u,4,0xffffffffu);bool reset_timer=s.reg(2,1,0)!=0;
            if(reset_timer) {
                s.gp[2]=read(s,0x249bf8u,s.gp[4]+0x14u);s.gp[0]=extend(read(s,0x249bfcu,s.gp[2]+4u,2),2);s.gp[2]=0;s.flags.logic(0,4);s.flags.minus(s.gp[5],9,4);s.set(2,s.flags.zf,1,0);push(s,0x249c08u,s.gp[0]);s.gp[3]=s.gp[1];s.gp[2]=s.flags.increment(s.gp[2],4,false);push(s,0x249c0cu,s.gp[2]);call(s,dispatch,0x19bca0u,0x249c0du,0x249c12u);s.gp[6]=read(s,0x249c12u,0x52d710u);s.set(3,read(s,0x249c18u,s.gp[4]+0x13u,1),1,0);
            } else {s.flags.logic(s.reg(3,1,0),1);reset_timer=s.reg(3,1,0)!=0;}
            if(reset_timer) {const auto remaining=read(s,0x249c22u,s.gp[6]+s.gp[7]*2u+0x114u,2);s.flags.minus(remaining,65535u,2);if(remaining!=65535u) {push(s,0x249c2du,0x80u);store(s,0x249c32u,s.gp[6]+s.gp[7]*2u+0x114u,2,65535u);call(s,dispatch,0xb1a10u,0x249c3cu,0x249c41u);}}
            objective_contest(s,dispatch);objective_countdown(s,dispatch);
            s.gp[2]=s.gp[5];call(s,dispatch,0x165610u,0x249e89u,0x249e8eu);s.flags.logic(s.reg(0,1,0),1);
            if(s.reg(0,1,0)) {
                s.gp[0]=read(s,0x249e92u,0x4f3218u);s.gp[2]=read(s,0x249e97u,s.gp[0]+0x180u);s.gp[1]=0;s.flags.logic(0,4);s.flags.minus(s.gp[2],9,4);s.gp[1]=source_objective_event_audience(std::bit_cast<std::int32_t>(s.gp[2]));s.gp[0]=s.gp[4]+0x24u;s.gp[2]=4;s.flags.plus(s.gp[1]-3u,3,4);call(s,dispatch,0x1a87c0u,0x249eb5u,0x249ebau);
                s.flags.minus(s.reg(7,2,0),8,2);s.gp[0]=s.reg(7,2,0)==8?s.flags.logic(0xffffffffu,4):extend(s.reg(7,2,0),2);s.gp[1]=extend(s.reg(0,2,0),2);s.gp[0]=s.gp[4]+0x24u;store(s,0x249ecfu,s.gp[4]+0x3cu,4,s.gp[1]);call(s,dispatch,0x1a87a0u,0x249ed3u,0x249ed8u);push(s,0x249ed8u,0);s.gp[0]=s.gp[5];call(s,dispatch,0x24b950u,0x249edcu,0x249ee1u);s.gp[6]=read(s,0x249ee1u,0x52d710u);
            }
            s.gp[0]=s.gp[5];call(s,dispatch,0x249a40u,0x249ee9u,0x249eeeu);s.gp[0]=read(s,0x249eeeu,s.gp[4]+0x14u);s.gp[5]=read(s,0x249ef2u,0x4f3218u);
        } else s.gp[6]=read(s,0x249efau,0x52d710u);
        const auto engine=read(s,0x249f00u,s.gp[5]+0x180u);s.flags.minus(engine,9,4);
        if(engine!=9) {
            const auto carrier=read(s,0x249f0du,s.gp[0]+0xcu);s.flags.minus(carrier,0xffffffffu,4);s.set(3,!s.flags.zf,1,0);s.flags.logic(s.reg(3,1,0),1);bool away=s.reg(3,1,0)==0;
            if(away) {const auto flags=read(s,0x249f18u,s.gp[6]+s.gp[7]+0x170u,1);s.flags.logic(flags&1u,1);store(s,0x249f20u,s.gp[4]+0x4cu,1,1);away=flags&1u;}if(!away)store(s,0x249f27u,s.gp[4]+0x4cu,1,0);
            call(s,dispatch,0x150900u,0x249f2cu,0x249f31u);s.gp[6]=s.gp[0];s.flags.minus(s.gp[6],0xffffffffu,4);
            while(s.gp[6]!=0xffffffffu) {
                s.flags.minus(s.gp[6],0xffffffffu,4);s.gp[2]=read(s,0x249f45u,0x4f54f0u);s.gp[0]=read(s,0x249f4bu,s.gp[2]+s.gp[6]*4u+0xcu);s.flags.minus(s.gp[0],0xffffffffu,4);
                if(s.gp[0]!=0xffffffffu) {
                    s.gp[1]=read(s,0x249f54u,0x4f54f4u);s.gp[2]=read(s,0x249f5au,s.gp[1]+0x44u);s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);s.gp[0]=s.flags.product(s.gp[0],0x21cu);s.gp[0]=extend(read(s,0x249f68u,s.gp[2]+s.gp[0]+0xc0u,1),1);s.flags.minus(s.gp[0],s.gp[7],4);
                    if(s.gp[0]==s.gp[7]) {
                        s.flags.logic(s.reg(3,1,0),1);if(s.reg(3,1,0)) {push(s,0x249f78u,5);push(s,0x249f7au,1);s.gp[0]=6;s.gp[2]=s.gp[6];call(s,dispatch,0x208a73u,0x249f83u,0x249f88u);}
                        s.set(0,read(s,0x249f88u,s.gp[4]+0x4cu,1),1,0);s.flags.logic(s.reg(0,1,0),1);if(s.reg(0,1,0)) {push(s,0x249f90u,6);push(s,0x249f92u,1);s.gp[0]=5;s.gp[2]=s.gp[6];call(s,dispatch,0x208a73u,0x249f9bu,0x249fa0u);}
                    }
                }
                s.gp[2]=s.flags.logic(0xffffffffu,4);s.flags.minus(s.gp[6],0xffffffffu,4);s.gp[0]=s.gp[6]==0xffffffffu?s.flags.logic(0,4):s.gp[6]+1u;s.flags.minus(s.gp[0],4,4);
                if(std::bit_cast<std::int32_t>(s.gp[0])<4) {
                    s.gp[1]=read(s,0x249fb4u,0x4f54f0u);s.gp[1]+=s.gp[0]*4u+0xcu;
                    for(;;) {const auto player=read(s,0x249fc0u,s.gp[1]);s.flags.minus(player,0xffffffffu,4);if(player!=0xffffffffu) {s.gp[2]=s.gp[0];break;}s.gp[0]=s.flags.increment(s.gp[0],4,false);s.gp[1]=s.flags.plus(s.gp[1],4,4);s.flags.minus(s.gp[0],4,4);if(std::bit_cast<std::int32_t>(s.gp[0])>=4)break;}
                }
                s.flags.minus(s.gp[2],0xffffffffu,4);s.gp[6]=s.gp[2];
            }
        } else {
            const auto flags=read(s,0x249fe7u,s.gp[6]+s.gp[7]+0x170u,1);s.flags.logic(flags&2u,1);
            if(flags&2u) {
                s.gp[0]=read(s,0x249ff1u,s.gp[4]+0x4cu);call(s,dispatch,0x24b800u,0x249ff5u,0x249ffau);s.gp[7]=s.gp[0];s.flags.minus(s.gp[7],0xffffffffu,4);
                if(s.gp[7]!=0xffffffffu) {
                    call(s,dispatch,0x150900u,0x24a001u,0x24a006u);s.gp[6]=s.gp[0];s.flags.minus(s.gp[6],0xffffffffu,4);
                    while(s.gp[6]!=0xffffffffu) {
                        s.gp[2]=read(s,0x24a00du,0x4f54f0u);s.gp[0]=read(s,0x24a013u,s.gp[2]+s.gp[6]*4u+0xcu);s.flags.minus(s.gp[0],0xffffffffu,4);
                        if(s.gp[0]!=0xffffffffu) {
                            s.gp[1]=read(s,0x24a01cu,0x4f54f4u);s.gp[2]=read(s,0x24a022u,s.gp[1]+0x44u);s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);s.gp[0]=s.flags.product(s.gp[0],0x21cu);s.gp[0]=extend(read(s,0x24a030u,s.gp[2]+s.gp[0]+0xc0u,1),1);s.flags.minus(s.gp[0],s.gp[7],4);
                            if(s.gp[0]==s.gp[7]) {push(s,0x24a03cu,0xffffffffu);push(s,0x24a03eu,1);s.gp[0]=8;s.gp[2]=s.gp[6];call(s,dispatch,0x208a73u,0x24a047u,0x24a04cu);}
                        }
                        s.gp[1]=s.gp[6];call(s,dispatch,0x150930u,0x24a04eu,0x24a053u);s.gp[6]=s.gp[0];s.flags.minus(s.gp[6],0xffffffffu,4);
                    }
                }
            }
        }
        cursor(s,0x24a05au);s.gp[3]=s.pop();
    }
    cursor(s,0x24a05bu);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x38u,4);s.pop();s.gp[4]+=4u;++source_objective_native_tu15_stats.owners[55];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
