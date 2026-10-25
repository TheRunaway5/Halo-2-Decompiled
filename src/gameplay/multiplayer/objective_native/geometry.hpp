#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool planted_goal_team(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x10u,4);s.gp[2]=read(s,0x24b803u,0x4ecbd0u);s.gp[2]=read(s,0x24b809u,s.gp[2]+0x44u);
    s.gp[1]=(s.gp[0]&65535u)*3u;s.flags.logic(s.gp[0]&65535u,4);s.gp[1]=read(s,0x24b817u,s.gp[2]+8u+s.gp[1]*4u);
    s.gp[1]=std::uint32_t(signed_half(read(s,0x24b81bu,s.gp[1]+0x17eu,2)));push(s,0x24b822u,s.gp[5]);s.gp[5]=0xffffffffu;
    s.flags.minus(s.gp[1],0xffffffffu,4);store(s,0x24b829u,s.gp[4]+4u,4,s.gp[5]);
    const auto finish=[&](std::uint32_t saved,std::uint32_t ret) {
        s.gp[0]=s.gp[5];cursor(s,saved);s.gp[5]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x10u,4);
        cursor(s,ret);s.pop();++source_objective_native_tu15_stats.owners[27];return true;
    };
    if(s.gp[1]==0xffffffffu)return finish(0x24b940u,0x24b944u);
    push(s,0x24b833u,s.gp[7]);s.gp[7]=read(s,0x24b834u,0x52d710u);
    const auto flags=read(s,0x24b83au,s.gp[7]+0x170u+s.gp[1],1);s.flags.logic(flags&2u,1);
    if(!(flags&2u)) {cursor(s,0x24b936u);s.gp[7]=s.pop();return finish(0x24b939u,0x24b93du);}
    push(s,0x24b848u,s.gp[3]);push(s,0x24b849u,s.gp[6]);s.gp[6]=s.gp[4]+0x14u;
    call(s,dispatch,0xbbe00u,0x24b84eu,0x24b853u);
    s.xmm[3]=_mm_set_ss(f(read(s,0x24b853u,s.gp[4]+0x1cu)));s.xmm[4]=_mm_set_ss(f(read(s,0x24b859u,s.gp[4]+0x18u)));
    s.xmm[5]=_mm_set_ss(f(read(s,0x24b85fu,s.gp[4]+0x14u)));s.gp[3]=read(s,0x24b865u,0x4ecc20u);
    s.xmm[6]=_mm_set_ss(f(read(s,0x24b86bu,0x46a4c0u)));s.gp[6]=0;s.flags.logic(0,4);
    // Scan all active playing teams, primary before secondary, retaining the
    // last matching team rather than returning on the first hit.
    for(unsigned team=0;team<8;++team) {
        s.gp[1]=s.gp[6];call(s,dispatch,0x164890u,0x24b877u,0x24b87cu);s.flags.logic(s.gp[0]&255u,1);
        if(s.gp[0]&255u) {
            push(s,0x24b884u,s.gp[6]);call(s,dispatch,0x15b3b0u,0x24b885u,0x24b88au);
            s.gp[1]=std::uint32_t(signed_half(s.gp[0]));s.gp[2]=0;s.flags.logic(0,4);bool found{};
            for(unsigned goal=0;goal<2;++goal) {
                s.flags.minus(s.gp[1],0xffffffffu,4);
                if(s.gp[1]!=0xffffffffu) {
                    s.flags.logic(s.gp[2],4);
                    s.gp[0]=std::uint32_t(signed_half(read(s,goal?0x24b899u:0x24b8a0u,
                        s.gp[7]+(goal?0x26u:0x14u)+s.gp[1]*2u,2)));s.flags.minus(s.gp[0],0xffffffffu,4);
                    if(s.gp[0]!=0xffffffffu) {
                        s.gp[5]=read(s,0x24b8adu,s.gp[3]+0x11cu);s.gp[0]=s.flags.shift(s.gp[0],5,4);s.gp[0]=s.flags.plus(s.gp[0],s.gp[5],4);
                        s.gp[5]=read(s,0x24b8b8u,s.gp[0]);store(s,0x24b8bau,s.gp[4]+0x14u,4,s.gp[5]);
                        s.gp[5]=read(s,0x24b8beu,s.gp[0]+4u);s.gp[0]=read(s,0x24b8c1u,s.gp[0]+8u);
                        const auto x=f(read(s,0x24b8c4u,s.gp[4]+0x14u));store(s,0x24b8cau,s.gp[4]+0x1cu,4,s.gp[0]);
                        const auto z=f(read(s,0x24b8ceu,s.gp[4]+0x1cu));store(s,0x24b8e6u,s.gp[4]+0x18u,4,s.gp[5]);
                        const auto y=f(read(s,0x24b8eau,s.gp[4]+0x18u));
                        const auto distance=source_objective_distance_zxy({x,y,z},
                            {_mm_cvtss_f32(s.xmm[5]),_mm_cvtss_f32(s.xmm[4]),_mm_cvtss_f32(s.xmm[3])});
                        s.xmm[0]=_mm_set_ss(distance.y_squared);s.xmm[1]=_mm_set_ss(distance.y);
                        s.xmm[2]=_mm_set_ss(distance.x_squared);s.xmm[7]=_mm_set_ss(distance.squared);
                        s.flags.scalar_compare(s.xmm[6],s.xmm[7],false);found=!s.flags.cf && !s.flags.zf;
                        if(found) {s.gp[5]=s.gp[6];store(s,0x24b91eu,s.gp[4]+0x10u,4,s.gp[5]);break;}
                    }
                }
                ++s.gp[2];s.flags.minus(s.gp[2],2,4);
            }
            if(!found)s.gp[5]=read(s,0x24b916u,s.gp[4]+0x10u);
        }
        ++s.gp[6];s.flags.minus(s.gp[6],8,4);
    }
    cursor(s,0x24b92cu);s.gp[6]=s.pop();cursor(s,0x24b92du);s.gp[3]=s.pop();cursor(s,0x24b92eu);s.gp[7]=s.pop();
    return finish(0x24b931u,0x24b935u);
}
} // namespace halo2::source_objective_native_tu15_detail
