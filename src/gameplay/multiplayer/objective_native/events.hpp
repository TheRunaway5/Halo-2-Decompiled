#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool announce_selected_team(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24c540u,0x4f3218u);s.set(1,read(s,0x24c545u,s.gp[0]+0xcu,1),1,0);
    s.gp[4]=s.flags.minus(s.gp[4],0x24u,4);s.flags.minus(s.reg(1,1,0),4,1);
    bool announce=s.reg(1,1,0)!=4;
    if(announce) {const auto kind=read(s,0x24c550u,s.gp[0]+0x240u);s.flags.minus(kind,1,4);announce=kind==1;}
    if(announce) {
        s.gp[2]=read(s,0x24c559u,s.gp[0]+0x180u);s.gp[1]=0;s.flags.logic(0,4);s.flags.minus(s.gp[2],9,4);
        const auto engine=std::bit_cast<std::int32_t>(s.gp[2]);s.gp[2]=read(s,0x24c564u,0x52d710u);s.set(1,!s.flags.zf,1,0);
        s.gp[0]=s.flags.logic(0xffffffffu,4);
        store(s,0x24c570u,s.gp[4]+0xcu,4,s.gp[0]);store(s,0x24c574u,s.gp[4]+0x14u,4,s.gp[0]);
        store(s,0x24c578u,s.gp[4]+0x18u,4,s.gp[0]);store(s,0x24c57cu,s.gp[4]+0x20u,2,s.gp[0]);
        s.gp[0]=read(s,0x24c581u,s.gp[2]+0x1f4u);s.gp[1]=source_objective_event_audience(engine);
        s.flags.plus(s.gp[1]-3u,3,4);store(s,0x24c58eu,s.gp[4],4,s.gp[1]);
        s.gp[1]=read(s,0x24c591u,s.gp[4]+0x28u);push(s,0x24c595u,s.gp[7]);s.gp[2]=s.gp[4]+4u;
        store(s,0x24c59au,s.gp[4]+8u,4,6);store(s,0x24c5a2u,s.gp[4]+0x20u,4,0);
        store(s,0x24c5aau,s.gp[4]+0x14u,4,s.gp[0]);store(s,0x24c5aeu,s.gp[4]+0xcu,4,s.gp[1]);
        call(s,dispatch,0xb1e80u,0x24c5b2u,0x24c5b7u);s.gp[7]=s.gp[4]+4u;call(s,dispatch,0x1a8740u,0x24c5bbu,0x24c5c0u);
        cursor(s,0x24c5c0u);s.gp[7]=s.pop();
    }
    s.gp[4]=s.flags.plus(s.gp[4],0x24u,4);cursor(s,0x24c5c4u);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[32];return true;
}
template<class State,class Dispatch>bool announce_capture(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x24u,4);push(s,0x24c0f3u,s.gp[5]);push(s,0x24c0f4u,s.gp[6]);
    s.gp[6]=s.gp[0];push(s,0x24c0f7u,s.gp[7]);push(s,0x24c0f8u,s.gp[6]);s.gp[0]=1;
    call(s,dispatch,0x15e1e0u,0x24c0feu,0x24c103u);s.flags.logic(s.reg(0,1,0),1);
    s.gp[7]=read(s,0x24c105u,0x4f3218u);s.gp[5]=read(s,0x24c10bu,0x4f54f4u);
    if(!s.flags.zf) {
        s.gp[2]=read(s,0x24c113u,s.gp[7]+0x180u);s.gp[1]=0;s.flags.logic(0,4);
        s.gp[0]=s.gp[6]&65535u;s.flags.logic(s.gp[0],4);s.flags.minus(s.gp[2],9,4);s.set(1,s.flags.zf,1,0);
        s.gp[2]=s.flags.product(s.gp[0],0x21cu);push(s,0x24c130u,s.gp[3]);s.gp[3]=read(s,0x24c131u,s.gp[5]+0x44u);
        s.gp[2]=extend(read(s,0x24c134u,s.gp[3]+s.gp[2]+0xc0u,1),1);push(s,0x24c13cu,1);s.gp[1]=14u+s.gp[1]*4u;
        call(s,dispatch,0x19b1d0u,0x24c145u,0x24c14au);cursor(s,0x24c14au);s.gp[3]=s.pop();
    }
    s.gp[2]=read(s,0x24c14bu,s.gp[7]+0x180u);const auto engine=std::bit_cast<std::int32_t>(s.gp[2]);
    s.gp[1]=read(s,0x24c151u,s.gp[5]+0x44u);s.gp[0]=0;s.flags.logic(0,4);s.flags.minus(s.gp[2],9,4);s.set(0,!s.flags.zf,1,0);
    store(s,0x24c15cu,s.gp[4]+0x18u,4,s.gp[6]);s.gp[6]&=65535u;s.flags.logic(s.gp[6],4);s.gp[6]=s.flags.product(s.gp[6],0x21cu);
    s.gp[0]=source_objective_event_audience(engine);s.flags.plus(s.gp[0]-3u,3,4);store(s,0x24c173u,s.gp[4]+0xcu,4,s.gp[0]);
    s.gp[0]=s.flags.logic(0xffffffffu,4);
    store(s,0x24c17au,s.gp[4]+0x1cu,4,s.gp[0]);store(s,0x24c17eu,s.gp[4]+0x24u,4,s.gp[0]);
    store(s,0x24c182u,s.gp[4]+0x14u,4,s.gp[0]);store(s,0x24c186u,s.gp[4]+0x20u,4,s.gp[0]);store(s,0x24c18au,s.gp[4]+0x2cu,2,s.gp[0]);
    s.gp[0]=read(s,0x24c18fu,s.gp[4]+0x34u);store(s,0x24c193u,s.gp[4]+0x10u,4,5);store(s,0x24c19bu,s.gp[4]+0x28u,4,0);
    s.gp[2]=extend(read(s,0x24c1a3u,s.gp[1]+s.gp[6]+0xc0u,1),1);store(s,0x24c1abu,s.gp[4]+0x24u,4,s.gp[0]);
    const auto mode=read(s,0x24c1afu,s.gp[7]+0xcu,1);s.flags.minus(mode,4,1);store(s,0x24c1b3u,s.gp[4]+0x1cu,4,s.gp[2]);
    if(mode!=4) {
        s.gp[2]=s.gp[4]+0xcu;call(s,dispatch,0xb1e80u,0x24c1bdu,0x24c1c2u);s.gp[7]=s.gp[4]+0xcu;
        call(s,dispatch,0x1a8740u,0x24c1c6u,0x24c1cbu);
    }
    cursor(s,0x24c1cbu);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x24u,4);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[33];return true;
}
template<class State,class Dispatch>bool announce_capture_blocked(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x2cu,4);push(s,0x24c393u,s.gp[6]);push(s,0x24c394u,s.gp[7]);s.gp[7]=read(s,0x24c395u,0x4f3218u);
    s.set(2,read(s,0x24c39bu,s.gp[7]+0xcu,1),1,0);s.gp[6]=s.gp[1]&65535u;s.flags.logic(s.gp[6],4);s.flags.minus(s.reg(2,1,0),4,1);
    bool announce=s.reg(2,1,0)!=4;
    if(announce) {s.flags.logic(s.gp[6],4);announce=std::bit_cast<std::int32_t>(s.gp[6])>=0;}
    if(announce) {s.flags.minus(s.gp[6],16,4);announce=std::bit_cast<std::int32_t>(s.gp[6])<16;}
    if(announce) {
        s.gp[2]=read(s,0x24c3c0u,0x51df54u);s.gp[2]=read(s,0x24c3c6u,s.gp[2]+8u);push(s,0x24c3c9u,s.gp[3]);s.gp[3]=read(s,0x24c3cau,0x52d710u);
        const auto last=read(s,0x24c3d0u,s.gp[3]+s.gp[6]*4u+0x1b4u);s.flags.minus(s.gp[2],last,4);
        if(std::bit_cast<std::int32_t>(s.gp[2])>std::bit_cast<std::int32_t>(last)) {
            s.gp[3]=read(s,0x24c3ddu,s.gp[7]+0x180u);s.set(0,s.flags.minus(0,s.reg(0,1,0),1),1,0);store(s,0x24c3e5u,s.gp[4]+0x20u,4,s.gp[1]);
            s.gp[1]=read(s,0x24c3e9u,0x4f54f4u);store(s,0x24c3efu,s.gp[4]+0x30u,4,0);
            s.gp[0]=s.flags.borrow(s.gp[0],s.gp[0],4);s.gp[0]=s.flags.logic(s.gp[0]&6u,4);s.gp[0]=s.flags.plus(s.gp[0],7,4);s.gp[2]=0;s.flags.logic(0,4);
            store(s,0x24c401u,s.gp[4]+0x18u,4,s.gp[0]);s.flags.minus(s.gp[3],9,4);s.set(2,!s.flags.zf,1,0);s.gp[0]=s.flags.logic(0xffffffffu,4);
            store(s,0x24c40eu,s.gp[4]+0x24u,4,s.gp[0]);store(s,0x24c412u,s.gp[4]+0x1cu,4,s.gp[0]);store(s,0x24c416u,s.gp[4]+0x28u,4,s.gp[0]);
            store(s,0x24c41au,s.gp[4]+0x2cu,4,s.gp[0]);store(s,0x24c41eu,s.gp[4]+0x34u,2,s.gp[0]);s.gp[0]=s.flags.product(s.gp[6],0x21cu);
            s.gp[2]=source_objective_event_audience(std::bit_cast<std::int32_t>(s.gp[3]));s.flags.plus(s.gp[2]-3u,3,4);store(s,0x24c432u,s.gp[4]+0x14u,4,s.gp[2]);
            s.gp[2]=read(s,0x24c436u,s.gp[1]+0x44u);s.gp[0]=extend(read(s,0x24c439u,s.gp[2]+s.gp[0]+0xc0u,1),1);
            store(s,0x24c441u,s.gp[4]+0x24u,4,s.gp[0]);s.gp[0]=s.gp[4]+0x14u;call(s,dispatch,0x1a87a0u,0x24c449u,0x24c44eu);
            s.gp[0]=read(s,0x24c44eu,0x51df54u);s.gp[2]=extend(read(s,0x24c453u,s.gp[0]+2u,2),2);s.gp[1]=read(s,0x24c457u,s.gp[0]+8u);
            s.xmm[0]=_mm_cvtsi32_ss(s.xmm[0],si(s.gp[2]));s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(read(s,0x24c45eu,0x46a120u))));
            store(s,0x24c466u,s.gp[4]+0xcu,4,u(_mm_cvtss_f32(s.xmm[0])));s.fp.push(ext(f(read(s,0x24c46cu,s.gp[4]+0xcu))));s.fp.status&=~0x200u;
            store(s,0x24c470u,s.gp[4]+0x10u,4,integer_spill(s.fp.st(0),s.fp.status));s.fp.pop();s.gp[0]=read(s,0x24c474u,s.gp[4]+0x10u);
            s.gp[0]=s.flags.plus(s.gp[0],s.gp[1],4);s.gp[1]=read(s,0x24c47au,0x52d710u);store(s,0x24c480u,s.gp[1]+s.gp[6]*4u+0x1b4u,4,s.gp[0]);
        }
        cursor(s,0x24c487u);s.gp[3]=s.pop();
    }
    cursor(s,0x24c488u);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x2cu,4);s.pop();++source_objective_native_tu15_stats.owners[43];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
