#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool return_objective(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[1]=read(s,0x24b950u,0x4ecbd0u);s.gp[2]=read(s,0x24b956u,s.gp[1]+0x44u);s.gp[4]=s.flags.minus(s.gp[4],0xbcu,4);push(s,0x24b95fu,s.gp[3]);push(s,0x24b960u,s.gp[5]);push(s,0x24b961u,s.gp[6]);push(s,0x24b962u,s.gp[7]);s.gp[7]=s.gp[0];s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);s.gp[0]*=3u;s.gp[5]=read(s,0x24b96du,s.gp[2]+s.gp[0]*4u+8u);s.gp[0]=read(s,0x24b971u,0x4f3218u);
    const auto engine=read(s,0x24b976u,s.gp[0]+0x180u);s.flags.minus(engine,9,4);bool explode=engine==9;
    if(explode) {s.set(1,read(s,0x24b983u,s.gp[0]+0x1128u,1),1,0);s.flags.logic(s.reg(1,1,0),1);explode=s.reg(1,1,0)==0;}
    bool reload_context=true;
    const auto explosion=[&] {
        if(!explode)return;
        s.gp[0]=read(s,0x24b991u,0x4ecc1cu);s.gp[1]=read(s,0x24b996u,s.gp[0]+0x16cu);s.gp[2]=read(s,0x24b99cu,0x4f0414u);s.gp[6]=extend(read(s,0x24b9a2u,s.gp[5]+0x17eu,2),2);s.gp[1]&=65535u;s.flags.logic(s.gp[1],4);s.gp[1]=s.flags.shift(s.gp[1],4,4);s.flags.minus(s.gp[6],0xffffffffu,4);const bool team=s.gp[6]!=0xffffffffu;
        s.gp[0]=read(s,0x24b9b5u,s.gp[2]+s.gp[1]+8u);s.gp[1]=read(s,0x24b9b9u,s.gp[0]+0xcu);s.gp[0]=read(s,0x24b9bcu,s.gp[1]+0x534u);store(s,0x24b9c2u,s.gp[4]+0x14u,4,s.gp[0]);store(s,0x24b9c6u,s.gp[4]+0x10u,4,0xffffffffu);if(!team)return;
        s.set(1,read(s,0x24b9d4u,s.gp[4]+0xd0u,1),1,0);s.flags.logic(s.reg(1,1,0),1);bool planted=s.reg(1,1,0)==0;s.gp[3]=read(s,0x24b9ddu,0x52d710u);
        if(planted) {const auto flags=read(s,0x24b9e9u,s.gp[3]+s.gp[6]+0x170u,1);s.flags.logic(flags&2u,1);planted=flags&2u;}
        if(planted) {
            s.gp[0]=s.gp[7];call(s,dispatch,0x24b800u,0x24b9f9u,0x24b9feu);s.flags.minus(s.gp[0],0xffffffffu,4);bool score=s.gp[0]!=0xffffffffu;s.gp[6]=read(s,0x24ba01u,s.gp[3]+s.gp[6]*4u+0x17cu);store(s,0x24ba08u,s.gp[4]+0x18u,4,s.gp[0]);store(s,0x24ba0cu,s.gp[4]+0x10u,4,s.gp[6]);
            if(score) {s.flags.minus(s.gp[6],0xffffffffu,4);score=s.gp[6]!=0xffffffffu;}
            if(score) {s.gp[0]=s.gp[6];s.gp[6]=read(s,0x24ba19u,0x4f54f4u);call(s,dispatch,0x16e940u,0x24ba1fu,0x24ba24u);s.flags.logic(s.gp[0],4);score=s.gp[0]!=0;}
            if(score) {
                s.gp[2]=read(s,0x24ba28u,s.gp[4]+0x18u);s.gp[0]=read(s,0x24ba2cu,s.gp[4]+0x10u);push(s,0x24ba30u,s.gp[2]);call(s,dispatch,0x24c0f0u,0x24ba31u,0x24ba36u);
                s.gp[0]=extend(read(s,0x24ba36u,s.gp[5]+0x17eu,2),2);s.gp[3]=read(s,0x24ba3du,0x52d710u);push(s,0x24ba43u,0x400u);store(s,0x24ba48u,s.gp[3]+s.gp[0]*4u+0x17cu,4,0xffffffffu);call(s,dispatch,0xb1a10u,0x24ba53u,0x24ba58u);
            }
            s.gp[0]=read(s,0x24ba58u,s.gp[4]+0x14u);s.gp[1]=read(s,0x24ba5cu,s.gp[0]+0xe0u);s.gp[2]=read(s,0x24ba62u,s.gp[0]+0xe8u);store(s,0x24ba68u,s.gp[4]+0x14u,4,s.gp[1]);store(s,0x24ba6cu,s.gp[4]+0x10u,4,s.gp[2]);store(s,0x24ba70u,s.gp[4]+0x18u,4,0);
        } else {s.gp[0]=read(s,0x24ba7au,s.gp[0]+0xf0u);store(s,0x24ba80u,s.gp[4]+0x14u,4,s.gp[0]);store(s,0x24ba84u,s.gp[4]+0x18u,4,1);}
        const auto effect=read(s,0x24ba8cu,s.gp[4]+0x14u);s.flags.minus(effect,0xffffffffu,4);
        if(effect!=0xffffffffu) {
            s.gp[3]=read(s,0x24ba97u,0x474d10u);s.gp[1]=s.gp[3];s.gp[2]=read(s,0x24ba9fu,s.gp[1]);s.gp[0]=read(s,0x24baa1u,s.gp[1]+4u);s.gp[1]=read(s,0x24baa4u,s.gp[1]+8u);store(s,0x24baa7u,s.gp[4]+0x2cu,4,s.gp[0]);s.gp[6]=s.gp[4]+0x1cu;s.gp[0]=s.gp[7];store(s,0x24bab1u,s.gp[4]+0x28u,4,s.gp[2]);store(s,0x24bab5u,s.gp[4]+0x30u,4,s.gp[1]);call(s,dispatch,0xbbe00u,0x24bab9u,0x24babeu);
            s.gp[1]=s.gp[4]+0xc0u;s.gp[0]=s.gp[7];call(s,dispatch,0xbe1c0u,0x24bac7u,0x24baccu);s.gp[2]=read(s,0x24baccu,s.gp[4]+0x24u);s.gp[0]=read(s,0x24bad0u,s.gp[4]+0x1cu);s.gp[1]=read(s,0x24bad4u,s.gp[4]+0x20u);
            constexpr std::array<std::uint32_t,5> z_pc{0x24bad8,0x24badf,0x24bae6,0x24baea,0x24baee};
            for(unsigned n=0;n<5;++n)store(s,z_pc[n],s.gp[4]+0xacu-n*0x1cu,4,s.gp[2]);
            constexpr std::array<std::uint32_t,10> xy_pc{0x24baf2,0x24baf9,0x24bb00,0x24bb07,0x24bb0e,0x24bb12,0x24bb16,0x24bb1a,0x24bb1e,0x24bb22};
            for(unsigned n=0;n<5;++n) {store(s,xy_pc[n*2u],s.gp[4]+0xa4u-n*0x1cu,4,s.gp[0]);store(s,xy_pc[n*2u+1u],s.gp[4]+0xa8u-n*0x1cu,4,s.gp[1]);}
            s.xmm[0]=_mm_setzero_ps();s.gp[2]=s.gp[3];s.gp[0]=read(s,0x24bb2bu,s.gp[2]);store(s,0x24bb2du,s.gp[4]+0x40u,4,s.gp[0]);s.gp[1]=read(s,0x24bb31u,s.gp[2]+4u);s.gp[0]=read(s,0x24bb34u,0x474d1cu);store(s,0x24bb39u,s.gp[4]+0x44u,4,s.gp[1]);s.gp[2]=read(s,0x24bb3du,s.gp[2]+8u);store(s,0x24bb40u,s.gp[4]+0x48u,4,s.gp[2]);
            s.gp[1]=read(s,0x24bb44u,s.gp[0]);store(s,0x24bb46u,s.gp[4]+0x5cu,4,s.gp[1]);s.gp[2]=read(s,0x24bb4au,s.gp[0]+4u);store(s,0x24bb4du,s.gp[4]+0x60u,4,s.gp[2]);s.gp[0]=read(s,0x24bb51u,s.gp[0]+8u);store(s,0x24bb54u,s.gp[4]+0x64u,4,s.gp[0]);
            s.gp[1]=s.gp[3];s.gp[2]=read(s,0x24bb5au,s.gp[1]);store(s,0x24bb5cu,s.gp[4]+0x78u,4,s.gp[2]);s.gp[0]=read(s,0x24bb60u,s.gp[1]+4u);s.gp[2]=read(s,0x24bb63u,s.gp[3]);store(s,0x24bb65u,s.gp[4]+0x7cu,4,s.gp[0]);s.gp[1]=read(s,0x24bb69u,s.gp[1]+8u);s.gp[0]=read(s,0x24bb6cu,s.gp[3]+4u);store(s,0x24bb6fu,s.gp[4]+0x80u,4,s.gp[1]);s.gp[1]=read(s,0x24bb76u,s.gp[3]+8u);
            s.xmm[1]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(read(s,0x24bb7cu,s.gp[4]+0x28u))));store(s,0x24bb82u,s.gp[4]+0xb0u,4,s.gp[2]);s.gp[2]=read(s,0x24bb89u,s.gp[4]+0x14u);store(s,0x24bb8du,s.gp[4]+0x94u,4,u(_mm_cvtss_f32(s.xmm[1])));
            s.xmm[1]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(read(s,0x24bb99u,s.gp[4]+0x2cu))));s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(read(s,0x24bb9fu,s.gp[4]+0x30u))));store(s,0x24bba5u,s.gp[4]+0xb4u,4,s.gp[0]);store(s,0x24bbacu,s.gp[4]+0xb8u,4,s.gp[1]);
            push(s,0x24bbb3u,s.gp[2]);s.gp[0]=1;s.gp[1]=s.gp[4]+0x38u;s.gp[2]=5;s.gp[6]=s.gp[4]+0xc4u;store(s,0x24bbc9u,s.gp[4]+0x9cu,4,u(_mm_cvtss_f32(s.xmm[1])));store(s,0x24bbd2u,s.gp[4]+0xa0u,4,u(_mm_cvtss_f32(s.xmm[0])));
            constexpr std::array<std::uint32_t,5> kinds{0x700054c,0x70000c0,0x20000ca,0x8000550,0x400054f},kind_pc{0x24bbdb,0x24bbe3,0x24bbeb,0x24bbf6,0x24bc01};
            for(unsigned n=0;n<5;++n)store(s,kind_pc[n],s.gp[4]+0x50u+n*0x1cu,4,kinds[n]);
            call(s,dispatch,0x1799f0u,0x24bc0cu,0x24bc11u);s.gp[0]=read(s,0x24bc11u,s.gp[4]+0x18u);push(s,0x24bc15u,s.gp[0]);s.gp[6]=s.gp[4]+0x20u;call(s,dispatch,0xb2270u,0x24bc1au,0x24bc1fu);s.gp[3]=read(s,0x24bc1fu,0x52d710u);
        }
        s.gp[1]=read(s,0x24bc25u,s.gp[4]+0x10u);s.gp[0]=s.flags.logic(0xffffffffu,4);s.flags.minus(s.gp[1],s.gp[0],4);
        if(s.gp[1]==s.gp[0]) {reload_context=false;return;}
        store(s,0x24bc34u,s.gp[4]+0xb0u,2,s.reg(0,2,0));s.gp[6]=s.gp[4]+0x1cu;s.gp[0]=s.gp[7];call(s,dispatch,0xbbe00u,0x24bc42u,0x24bc47u);s.gp[1]=read(s,0x24bc47u,s.gp[4]+0x10u);push(s,0x24bc4bu,s.gp[1]);s.gp[2]=s.gp[4]+0x38u;call(s,dispatch,0xd8bb0u,0x24bc50u,0x24bc55u);
        s.gp[1]=s.gp[4]+0x3cu;s.gp[0]=s.gp[7];call(s,dispatch,0xbe1c0u,0x24bc5bu,0x24bc60u);s.xmm[0]=_mm_set_ss(f(read(s,0x24bc60u,0x46a0d0u)));s.gp[0]=s.gp[4]+0x50u;s.gp[1]=s.gp[7];store(s,0x24bc6eu,s.gp[4]+0x88u,4,u(_mm_cvtss_f32(s.xmm[0])));call(s,dispatch,0xbc330u,0x24bc77u,0x24bc7cu);
        s.gp[2]=read(s,0x24bc7cu,s.gp[4]+0x24u);s.gp[0]=read(s,0x24bc80u,s.gp[4]+0x1cu);s.gp[1]=read(s,0x24bc84u,s.gp[4]+0x20u);store(s,0x24bc88u,s.gp[4]+0x6cu,4,s.gp[2]);store(s,0x24bc8cu,s.gp[4]+0x60u,4,s.gp[2]);s.gp[2]=read(s,0x24bc90u,0x474d10u);
        store(s,0x24bc96u,s.gp[4]+0x64u,4,s.gp[0]);store(s,0x24bc9au,s.gp[4]+0x58u,4,s.gp[0]);store(s,0x24bc9eu,s.gp[4]+0x68u,4,s.gp[1]);store(s,0x24bca2u,s.gp[4]+0x5cu,4,s.gp[1]);s.gp[0]=read(s,0x24bca6u,s.gp[2]);store(s,0x24bca8u,s.gp[4]+0x70u,4,s.gp[0]);s.gp[1]=read(s,0x24bcacu,s.gp[2]+4u);store(s,0x24bcafu,s.gp[4]+0x74u,4,s.gp[1]);s.gp[2]=read(s,0x24bcb3u,s.gp[2]+8u);
        push(s,0x24bcb6u,0xffffffffu);s.gp[0]=s.gp[4]+0x38u;push(s,0x24bcbcu,s.gp[0]);store(s,0x24bcbdu,s.gp[4]+0x80u,4,s.gp[2]);store(s,0x24bcc4u,s.gp[4]+0xc0u,1,25);call(s,dispatch,0xd91d0u,0x24bcccu,0x24bcd1u);
    };
    explosion();if(reload_context)s.gp[3]=read(s,0x24bcd1u,0x52d710u);
    // Common reset follows effects and damage. It clears timers and state in
    // source order, then moves the same object to its authored home marker.
    s.set(0,read(s,0x24bcd7u,s.gp[5]+0x17eu,2),2,0);s.flags.minus(s.reg(0,2,0),65535u,2);
    const auto reset=[&] {
        if(s.reg(0,2,0)==65535u)return;s.gp[0]=extend(s.reg(0,2,0),2);call(s,dispatch,0x24b4c0u,0x24bcebu,0x24bcf0u);
        s.gp[1]=extend(read(s,0x24bcf0u,s.gp[5]+0x17eu,2),2);store(s,0x24bcf7u,s.gp[3]+s.gp[1]*2u+0x114u,2,65535u);s.gp[2]=extend(read(s,0x24bd01u,s.gp[5]+0x17eu,2),2);s.gp[6]=s.gp[0];s.gp[0]=read(s,0x24bd0au,0x51df54u);s.set(1,read(s,0x24bd0fu,s.gp[0]+2u,2),2,0);s.gp[0]=read(s,0x24bd13u,0x4f63b8u);store(s,0x24bd18u,s.gp[3]+s.gp[2]*2u+0x126u,2,s.reg(1,2,0));
        s.gp[2]=read(s,0x24bd20u,s.gp[0]+0xc14u);s.gp[1]=read(s,0x24bd26u,0x56dfa0u+s.gp[2]*4u);s.flags.logic(s.gp[1],4);
        if(s.gp[1]) {s.gp[1]=read(s,0x24bd31u,s.gp[0]+0x24u);s.flags.minus(s.gp[1],0xffffffffu,4);if(s.gp[1]!=0xffffffffu) {push(s,0x24bd39u,0x80u);push(s,0x24bd3eu,s.gp[1]);call(s,dispatch,0xb7e50u,0x24bd3fu,0x24bd44u);s.gp[0]=read(s,0x24bd44u,0x4f63b8u);}}
        s.gp[1]=read(s,0x24bd49u,0x4f3218u);const auto kind=read(s,0x24bd4fu,s.gp[1]+0x180u);s.flags.minus(kind,9,4);
        if(kind==9) {
            s.gp[2]=extend(read(s,0x24bd58u,s.gp[5]+0x17eu,2),2);store(s,0x24bd5fu,s.gp[3]+s.gp[2]*2u+0x1a0u,2,65535u);s.gp[1]=read(s,0x24bd69u,s.gp[0]+0xc14u);s.gp[2]=read(s,0x24bd6fu,0x56dfa0u+s.gp[1]*4u);s.flags.logic(s.gp[2],4);
            if(s.gp[2]) {s.gp[1]=read(s,0x24bd7au,s.gp[0]+0x24u);s.flags.minus(s.gp[1],0xffffffffu,4);if(s.gp[1]!=0xffffffffu) {push(s,0x24bd82u,0x100u);push(s,0x24bd87u,s.gp[1]);call(s,dispatch,0xb7e50u,0x24bd88u,0x24bd8du);s.gp[0]=read(s,0x24bd8du,0x4f63b8u);}}
        }
        s.flags.minus(s.gp[6],0xffffffffu,4);if(s.gp[6]==0xffffffffu)return;
        s.gp[2]=extend(read(s,0x24bd9bu,s.gp[5]+0x17eu,2),2);s.gp[1]=s.gp[3]+s.gp[2]+0x170u;store(s,0x24bda9u,s.gp[1],1,s.flags.logic(read(s,0x24bda9u,s.gp[1],1)&0xfeu,1));
        s.gp[1]=extend(read(s,0x24bdacu,s.gp[5]+0x17eu,2),2);s.set(2,read(s,0x24bdb3u,s.gp[3]+s.gp[1]+0x170u,1),1,0);s.gp[1]=s.gp[3]+s.gp[1]+0x170u;s.set(2,s.flags.logic(s.reg(2,1,0)&0xfdu,1),1,0);store(s,0x24bdc4u,s.gp[1],1,s.reg(2,1,0));
        s.gp[2]=extend(read(s,0x24bdc6u,s.gp[5]+0x17eu,2),2);s.gp[1]=s.gp[3]+s.gp[2]+0x170u;store(s,0x24bdd4u,s.gp[1],1,s.flags.logic(read(s,0x24bdd4u,s.gp[1],1)&0xf7u,1));s.gp[1]=read(s,0x24bdd7u,s.gp[0]+0xc14u);s.gp[2]=read(s,0x24bdddu,0x56dfa0u+s.gp[1]*4u);s.flags.logic(s.gp[2],4);
        if(s.gp[2]) {s.gp[0]=read(s,0x24bde8u,s.gp[0]+0x24u);s.flags.minus(s.gp[0],0xffffffffu,4);if(s.gp[0]!=0xffffffffu) {push(s,0x24bdf0u,0x200u);push(s,0x24bdf5u,s.gp[0]);call(s,dispatch,0xb7e50u,0x24bdf6u,0x24bdfbu);}}
        store(s,0x24bdfbu,s.gp[3]+0x204u,1,s.flags.logic(read(s,0x24bdfbu,s.gp[3]+0x204u,1)|1u,1));s.gp[2]=read(s,0x24be02u,0x4ecc20u);s.gp[3]=read(s,0x24be08u,s.gp[2]+0x11cu);s.gp[6]=s.flags.shift(s.gp[6],5,4);s.gp[0]=read(s,0x24be11u,s.gp[3]+s.gp[6]);s.gp[1]=read(s,0x24be14u,s.gp[3]+s.gp[6]+4u);s.gp[2]=read(s,0x24be18u,s.gp[3]+s.gp[6]+8u);s.gp[6]=s.flags.plus(s.gp[6],s.gp[3],4);store(s,0x24be1eu,s.gp[4]+0x1cu,4,s.gp[0]);s.gp[0]=s.gp[4]+0x1cu;push(s,0x24be26u,s.gp[0]);s.gp[6]=s.gp[7];store(s,0x24be29u,s.gp[4]+0x24u,4,s.gp[1]);store(s,0x24be2du,s.gp[4]+0x28u,4,s.gp[2]);
        call(s,dispatch,0x160da0u,0x24be31u,0x24be36u);s.gp[0]=read(s,0x24be36u,0x52d710u);store(s,0x24be3bu,s.gp[0]+0x204u,1,s.flags.logic(read(s,0x24be3bu,s.gp[0]+0x204u,1)&0xfeu,1));
    };
    reset();cursor(s,0x24be42u);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[3]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0xbcu,4);s.pop();s.gp[4]+=4u;++source_objective_native_tu15_stats.owners[54];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
