#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool carrier_trait(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24b300u,s.gp[3]);push(s,0x24b301u,s.gp[6]);push(s,0x24b302u,s.gp[7]);
    s.gp[7]=read(s,0x24b303u,s.gp[4]+0x10u);push(s,0x24b307u,1);
    s.gp[0]=s.gp[7];s.gp[6]=s.gp[1];s.gp[3]&=0xffffff00u;s.flags.logic(0,1);
    call(s,dispatch,0x1a8fd0u,0x24b30fu,0x24b314u);
    const auto carried=s.gp[0]!=0xffffffffu;s.flags.minus(s.gp[0],0xffffffffu,4);
    s.gp[0]=read(s,0x24b317u,s.gp[4]+0x14u);
    if(!carried) {
        // The shared default owner handles a player who has no objective.
        push(s,0x24b370u,s.gp[0]);push(s,0x24b371u,s.gp[7]);s.gp[1]=s.gp[6];
        call(s,dispatch,0x25bcc0u,0x24b374u,0x24b379u);
        cursor(s,0x24b379u);s.gp[7]=s.pop();cursor(s,0x24b37au);s.gp[6]=s.pop();
        cursor(s,0x24b37bu);s.gp[3]=s.pop();cursor(s,0x24b37cu);s.pop();s.gp[4]+=8u;
    } else {
        const auto trait=std::bit_cast<std::int32_t>(s.gp[0]);
        s.flags.minus(s.gp[0],2,4);
        if(trait==2) {
            s.gp[0]=read(s,0x24b322u,0x4f3218u);s.gp[1]=read(s,0x24b327u,s.gp[0]+0x238u);
            cursor(s,0x24b32du);s.gp[7]=s.pop();s.flags.logic(s.gp[1],4);
            cursor(s,0x24b330u);s.gp[6]=s.pop();
            s.gp[0]=(s.gp[0]&0xffffff00u)|std::uint32_t(source_objective_carrier_trait(trait,0,std::bit_cast<std::int32_t>(s.gp[1])));
            cursor(s,0x24b334u);s.gp[3]=s.pop();cursor(s,0x24b335u);s.pop();s.gp[4]+=8u;
        } else {
            s.flags.minus(s.gp[0],3,4);
            if(trait!=3)s.flags.minus(s.gp[0],1,4);
            if(trait==1 || trait==3) {
                const auto option_pc=trait==1?0x24b359u:0x24b33du;
                const auto flags_pc=trait==1?0x24b35fu:0x24b343u;
                const auto pointer=read(s,option_pc,0x4f3218u);s.gp[trait==1?2:1]=pointer;
                const auto flags=read(s,flags_pc,pointer+0x22cu);
                s.gp[0]=(flags>>(trait==1?7u:6u));
                cursor(s,trait==1?0x24b365u:0x24b349u);s.gp[7]=s.pop();
                s.gp[0]=(s.gp[0]&0xffffff00u)|std::uint32_t(source_objective_carrier_trait(trait,flags,0));
                s.flags.logic(s.gp[0]&255u,1);
                cursor(s,trait==1?0x24b369u:0x24b34du);s.gp[6]=s.pop();
                cursor(s,trait==1?0x24b36cu:0x24b350u);s.gp[3]=s.pop();
                cursor(s,trait==1?0x24b36du:0x24b351u);s.pop();s.gp[4]+=8u;
            } else {
                cursor(s,0x24b37fu);s.gp[7]=s.pop();cursor(s,0x24b380u);s.gp[6]=s.pop();
                s.gp[0]&=0xffffff00u;cursor(s,0x24b383u);s.gp[3]=s.pop();
                cursor(s,0x24b384u);s.pop();s.gp[4]+=8u;
            }
        }
    }
    ++source_objective_native_tu15_stats.owners[8];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
