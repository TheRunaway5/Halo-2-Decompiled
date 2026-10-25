#pragma once

namespace halo2::source_objective_native_tu15_detail {
inline bool overtime(WorldState& s) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24a620u,s.gp[4]+4u);s.gp[2]=read(s,0x24a624u,0x51df54u);
    s.xmm[0]=_mm_set_ss(f(read(s,0x24a62au,0x46a100u)));
    const auto seconds=source_objective_overtime_seconds(std::bit_cast<std::int32_t>(s.gp[0]),
        f(read(s,0x24a636u,s.gp[2]+4u)));
    s.xmm[1]=_mm_move_ss(s.xmm[1],_mm_set_ss(seconds));s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
    if(!s.flags.cf) {
        s.gp[1]=(s.gp[1]&0xffffff00u)|read(s,0x24a640u,s.gp[4]+8u,1);
        const auto advance=(s.gp[1]&255u)!=0;s.flags.logic(s.gp[1]&255u,1);s.gp[1]=read(s,0x24a646u,0x52d710u);
        if(advance) {
            push(s,0x24a64eu,s.gp[3]);s.gp[3]=(s.gp[3]&0xffffff00u)|read(s,0x24a64fu,s.gp[4]+0x10u,1);
            bool eligible=(s.gp[3]&255u)!=0;s.flags.logic(s.gp[3]&255u,1);
            if(eligible) {
                push(s,0x24a657u,s.gp[6]);s.gp[6]=read(s,0x24a658u,0x4f3218u);
                const auto flags=read(s,0x24a65eu,s.gp[6]+0x22cu,1);s.flags.logic(flags&2u,1);
                cursor(s,0x24a665u);s.gp[6]=s.pop();eligible=(flags&2u)!=0;
            }
            if(!eligible)store(s,0x24a6a0u,s.gp[1]+0x200u,4,std::uint32_t(source_objective_overtime_tick(0,false,false,0)));
            else {
                s.gp[3]=(s.gp[3]&0xffffff00u)|read(s,0x24a668u,s.gp[1]+0x1fcu,1);s.flags.logic(s.gp[3]&255u,1);
                if(s.gp[3]&255u) {
                    s.gp[2]=std::uint32_t(signed_half(read(s,0x24a672u,s.gp[2]+2u,2)));
                    s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],std::bit_cast<std::int32_t>(s.gp[2]));s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
                    store(s,0x24a67eu,s.gp[4]+0xcu,4,u(_mm_cvtss_f32(s.xmm[1])));
                    s.fp.push(ext(f(read(s,0x24a684u,s.gp[4]+0xcu))));s.fp.status&=~0x200u;
                    const auto window=integer_spill(s.fp.st(0),s.fp.status);store(s,0x24a688u,s.gp[4]+0x10u,4,window);s.fp.pop();
                    s.gp[2]=read(s,0x24a68cu,s.gp[4]+0x10u);
                    store(s,0x24a690u,s.gp[1]+0x200u,4,std::uint32_t(source_objective_overtime_tick(0,true,true,std::bit_cast<std::int32_t>(s.gp[2]))));
                } else {
                    const auto previous=read(s,0x24a698u,s.gp[1]+0x200u);
                    const auto next=source_objective_overtime_tick(std::bit_cast<std::int32_t>(previous),true,false,0);
                    s.flags.increment(previous,4,true);store(s,0x24a698u,s.gp[1]+0x200u,4,std::uint32_t(next));
                }
            }
            cursor(s,0x24a6aau);s.gp[3]=s.pop();
        }
        s.gp[1]=read(s,0x24a6abu,s.gp[1]+0x200u);s.flags.minus(s.gp[0],s.gp[1],4);
        s.gp[0]=std::uint32_t(std::max(std::bit_cast<std::int32_t>(s.gp[0]),std::bit_cast<std::int32_t>(s.gp[1])));
    }
    cursor(s,0x24a6b7u);s.pop();s.gp[4]+=12u;++source_objective_native_tu15_stats.owners[22];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
