#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool carrier_speed(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24a130u,s.gp[1]);s.xmm[0]=_mm_set_ss(f(read(s,0x24a131u,0x46a0d0u)));
    s.gp[0]=read(s,0x24a139u,s.gp[4]+8u);push(s,0x24a13du,1);store(s,0x24a13fu,s.gp[4]+4u,4,u(_mm_cvtss_f32(s.xmm[0])));
    call(s,dispatch,0x1a8fd0u,0x24a145u,0x24a14au);s.flags.minus(s.gp[0],0xffffffffu,4);
    if(s.gp[0]!=0xffffffffu) {
        s.gp[0]=read(s,0x24a14fu,0x4f3218u);s.gp[0]=read(s,0x24a154u,s.gp[0]+0x234u);
        s.gp[0]=s.flags.increment(s.gp[0],4,true);
        if(s.gp[0]!=0) {
            s.gp[0]=s.flags.increment(s.gp[0],4,true);s.xmm[0]=_mm_set_ss(f(read(s,0x24a15eu,0x46a13cu)));
            if(s.gp[0]==0)s.xmm[0]=_mm_set_ss(f(read(s,0x24a168u,0x46a198u)));
        }
        store(s,0x24a170u,s.gp[4],4,u(_mm_cvtss_f32(s.xmm[0])));
    }
    s.fp.push(ext(f(read(s,0x24a175u,s.gp[4]))));s.fp.status&=~0x200u;
    cursor(s,0x24a178u);s.gp[1]=s.pop();cursor(s,0x24a179u);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[23];return true;
}
template<class State,class Dispatch>bool carrier_event(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24a290u,s.gp[4]+4u);push(s,0x24a294u,s.gp[6]);s.gp[6]=0xffffffffu;
    s.flags.minus(s.gp[0],s.gp[6],4);bool available=s.gp[0]!=s.gp[6];
    if(available) {s.gp[0]=read(s,0x24a29cu,s.gp[4]+0xcu);s.flags.minus(s.gp[0],s.gp[6],4);available=s.gp[0]!=s.gp[6];}
    if(available) {
        s.gp[1]=(s.gp[1]&0xffffff00u)|read(s,0x24a2a4u,s.gp[4]+0x10u,1);s.flags.logic(s.gp[1]&255u,1);available=(s.gp[1]&255u)==0;
    }
    if(available) {
        push(s,0x24a2acu,1);call(s,dispatch,0x1a8fd0u,0x24a2aeu,0x24a2b3u);
        s.flags.minus(s.gp[0],s.gp[6],4);available=s.gp[0]!=s.gp[6];
    }
    if(available) {
        s.gp[1]=read(s,0x24a2b7u,0x4f3218u);s.gp[2]=read(s,0x24a2bdu,s.gp[1]+0x180u);s.flags.minus(s.gp[2],9,4);
        s.gp[0]=s.gp[2]==9?1u:0u;cursor(s,0x24a2cbu);s.gp[6]=s.pop();
        s.flags.plus(s.gp[0],0x27u,4);s.gp[0]=source_objective_carrier_event(std::bit_cast<std::int32_t>(s.gp[2]));
        cursor(s,0x24a2cfu);s.pop();s.gp[4]+=12u;
    } else {
        s.gp[0]=s.gp[6];cursor(s,0x24a2d4u);s.gp[6]=s.pop();cursor(s,0x24a2d5u);s.pop();s.gp[4]+=12u;
    }
    ++source_objective_native_tu15_stats.owners[24];return true;
}
template<class State,class Dispatch>bool carrier_statistic(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24a1e0u,s.gp[6]);s.gp[6]=read(s,0x24a1e1u,s.gp[4]+8u);s.flags.minus(s.gp[6],0xffffffffu,4);
    bool record=s.gp[6]!=0xffffffffu;
    if(record) {s.gp[0]=read(s,0x24a1eeu,s.gp[4]+0xcu);s.flags.minus(s.gp[0],0xffffffffu,4);record=s.gp[0]!=0xffffffffu;}
    if(record) {
        s.gp[1]=(s.gp[1]&0xffffff00u)|read(s,0x24a1fbu,s.gp[4]+0x10u,1);s.flags.logic(s.gp[1]&255u,1);record=(s.gp[1]&255u)==0;
    }
    if(record) {
        push(s,0x24a203u,1);call(s,dispatch,0x1a8fd0u,0x24a205u,0x24a20au);s.flags.minus(s.gp[0],0xffffffffu,4);record=s.gp[0]!=0xffffffffu;
    }
    if(record) {
        s.gp[1]=read(s,0x24a20fu,0x4f54f4u);s.gp[2]=read(s,0x24a215u,s.gp[1]+0x44u);s.gp[6]&=65535u;
        s.gp[0]=s.flags.plus(s.gp[6]*0x21cu,s.gp[2],4);s.gp[2]=read(s,0x24a228u,0x4f3218u);
        const auto engine=read(s,0x24a22eu,s.gp[2]+0x180u);s.flags.minus(engine,9,4);
        s.gp[2]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(s,0x24a235u,s.gp[0]+0xc0u,1)))));
        push(s,0x24a23cu,s.gp[3]);push(s,0x24a23du,1);s.gp[0]=s.gp[6];s.gp[1]=engine==9?0x14u:0x10u;
        call(s,dispatch,0x19b1d0u,engine==9?0x24a248u:0x24a268u,engine==9?0x24a24du:0x24a26du);
        s.gp[0]=engine==9?0x16u:0x13u;s.gp[1]=s.gp[6];s.gp[3]=1;
        call(s,dispatch,0x19baa0u,engine==9?0x24a259u:0x24a279u,engine==9?0x24a25eu:0x24a27eu);
        cursor(s,engine==9?0x24a25eu:0x24a27eu);s.gp[3]=s.pop();
    }
    cursor(s,0x24a27fu);s.gp[6]=s.pop();cursor(s,0x24a280u);s.pop();s.gp[4]+=16u;
    ++source_objective_native_tu15_stats.owners[25];return true;
}
template<class State,class Dispatch>bool carrier_indicator(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24b3c0u,s.gp[4]+4u);s.gp[2]=read(s,0x24b3c4u,0x4f54f4u);s.gp[1]=(s.gp[0]&65535u)*0x21cu;
    push(s,0x24b3d8u,s.gp[6]);s.gp[6]=read(s,0x24b3d9u,s.gp[2]+0x44u);s.gp[2]=read(s,0x24b3dcu,s.gp[4]+0xcu);
    s.gp[1]=s.flags.plus(s.gp[1],s.gp[6],4);store(s,0x24b3e2u,s.gp[2],1,1);
    s.gp[2]=(s.gp[2]&0xffffff00u)|read(s,0x24b3e5u,s.gp[1]+0xc0u,1);s.gp[6]=0xffffffffu;s.flags.minus(s.gp[2]&255u,255u,1);
    bool available=(s.gp[2]&255u)!=255u;
    if(available) {
        push(s,0x24b3f3u,1);call(s,dispatch,0x1a8fd0u,0x24b3f5u,0x24b3fau);s.flags.minus(s.gp[0],0xffffffffu,4);available=s.gp[0]!=0xffffffffu;
    }
    if(available) {
        s.gp[1]=read(s,0x24b3ffu,0x4f3218u);s.gp[2]=read(s,0x24b405u,s.gp[1]+0x180u);s.flags.minus(s.gp[2],9,4);
        cursor(s,0x24b413u);s.gp[6]=s.pop();s.gp[0]=source_objective_carrier_indicator(std::bit_cast<std::int32_t>(s.gp[2]));
        cursor(s,0x24b41bu);s.pop();s.gp[4]+=8u;
    } else {
        s.gp[0]=s.gp[6];cursor(s,0x24b420u);s.gp[6]=s.pop();cursor(s,0x24b421u);s.pop();s.gp[4]+=8u;
    }
    ++source_objective_native_tu15_stats.owners[26];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
