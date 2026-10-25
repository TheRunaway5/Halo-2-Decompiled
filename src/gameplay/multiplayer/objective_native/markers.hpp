#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool select_marker(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x24u,4);const auto spawn=(s.gp[0]&255u)!=0;s.flags.logic(s.gp[0]&255u,1);
    push(s,0x24ce05u,s.gp[3]);push(s,0x24ce06u,s.gp[5]);s.gp[5]=read(s,0x24ce07u,0x4f3218u);
    push(s,0x24ce0du,s.gp[6]);push(s,0x24ce0eu,s.gp[7]);
    const auto engine=read(s,spawn?0x24ce11u:0x24ce25u,s.gp[5]+0x180u);
    s.gp[spawn?6u:1u]=engine;s.flags.minus(engine,9,4);
    s.gp[0]=source_objective_marker_type(std::bit_cast<std::int32_t>(engine),spawn);
    if(spawn)s.flags.logic(s.gp[0],4);
    s.xmm[4]=_mm_setzero_ps();s.gp[1]=s.gp[4]+0x14u;push(s,0x24ce3eu,s.gp[1]);push(s,0x24ce3fu,8);
    push(s,0x24ce41u,0);push(s,0x24ce43u,s.gp[2]);push(s,0x24ce44u,s.gp[0]);push(s,0x24ce45u,0);push(s,0x24ce47u,0);
    call(s,dispatch,0x1a8850u,0x24ce49u,0x24ce4eu);s.gp[3]=s.gp[0];s.gp[0]=0xffffffffu;s.gp[7]=0;s.flags.logic(s.gp[3],4);
    bool natural_end{};
    if(std::bit_cast<std::int32_t>(s.gp[3])>0) {
        s.gp[1]=read(s,0x24ce59u,0x4ecc20u);s.gp[2]=read(s,0x24ce5fu,s.gp[1]+0x11cu);store(s,0x24ce65u,s.gp[4]+0x10u,4,s.gp[2]);
        for(;;) {
            s.gp[6]=read(s,0x24ce70u,s.gp[4]+0x14u+s.gp[7]*4u);s.gp[2]=read(s,0x24ce74u,s.gp[4]+0x10u);
            s.gp[1]=s.flags.plus(s.gp[6]*32u,s.gp[2],4);
            s.gp[1]=(s.gp[1]&0xffff0000u)|read(s,0x24ce7fu,s.gp[1]+0x16u,2);const auto flags=std::uint16_t(s.gp[1]);
            // Preserve the original option-read short circuit and the first
            // eight query results; later eligible markers do not displace them.
            bool eligible{};s.flags.logic(flags&1u,1);
            if(flags&1u) {
                s.gp[2]=read(s,0x24ce88u,s.gp[5]+0x240u);const auto kind=std::bit_cast<std::int32_t>(s.gp[2]);
                s.flags.minus(s.gp[2],1,4);if(kind!=1)s.flags.minus(s.gp[2],2,4);
                eligible=kind!=1 && kind!=2 && source_objective_marker_eligible(flags,kind);
            }
            if(!eligible) {
                s.flags.logic(flags&2u,1);
                if(flags&2u) {const auto kind=read(s,0x24ce9du,s.gp[5]+0x240u);s.flags.minus(kind,1,4);eligible=kind==1;}
                if(!eligible) {
                    s.flags.logic(flags&4u,1);
                    if(flags&4u) {const auto kind=read(s,0x24ceabu,s.gp[5]+0x240u);s.flags.minus(kind,2,4);eligible=kind==2;}
                }
            }
            if(eligible) {
                s.flags.minus(s.gp[0],0xffffffffu,4);
                if(s.gp[0]!=0xffffffffu) {
                    s.gp[1]=read(s,0x24cecau,s.gp[4]+0x38u);s.flags.logic(s.gp[1],4);
                    if(s.gp[1]) {s.gp[2]=read(s,0x24ced2u,s.gp[4]+0x14u+s.gp[7]*4u);store(s,0x24ced6u,s.gp[1],4,s.gp[2]);}
                    break;
                }
                s.gp[0]=s.gp[6];
            }
            ++s.gp[7];s.flags.minus(s.gp[7],s.gp[3],4);
            if(s.gp[7]>=s.gp[3]) {natural_end=true;break;}
        }
    }
    cursor(s,natural_end?0x24cec0u:0x24ced8u);s.gp[7]=s.pop();cursor(s,natural_end?0x24cec1u:0x24ced9u);s.gp[6]=s.pop();
    cursor(s,natural_end?0x24cec2u:0x24cedau);s.gp[5]=s.pop();cursor(s,natural_end?0x24cec3u:0x24cedbu);s.gp[3]=s.pop();
    s.gp[4]=s.flags.plus(s.gp[4],0x24u,4);cursor(s,natural_end?0x24cec7u:0x24cedfu);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[30];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
