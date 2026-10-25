#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool calculate_goal_extents(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x44u,4);push(s,0x24cef3u,s.gp[3]);push(s,0x24cef4u,s.gp[5]);push(s,0x24cef5u,s.gp[6]);s.gp[3]=0;s.flags.logic(0,4);
    for(unsigned which=0;which<2;++which) {
        s.flags.logic(s.gp[3],4);s.gp[5]=read(s,0x24cf02u,0x52d710u);
        if(which) {s.gp[0]=read(s,0x24cf11u,0x52d710u);s.set(0,read(s,0x24cf16u,s.gp[0]+s.gp[7]*2u+0x26u,2),2,0);}
        else s.set(0,read(s,0x24cf0au,s.gp[5]+s.gp[7]*2u+0x14u,2),2,0);
        s.gp[6]=extend(s.reg(0,2,0),2);s.flags.minus(s.gp[6],0xffffffffu,4);
        if(s.gp[6]!=0xffffffffu) {
            s.gp[1]=read(s,0x24cf27u,0x4f3218u);s.gp[2]=read(s,0x24cf2du,s.gp[1]+0x180u);s.xmm[4]=_mm_setzero_ps();s.gp[0]=0;s.flags.logic(0,4);s.flags.minus(s.gp[2],9,4);s.set(0,s.flags.zf,1,0);
            s.gp[2]=s.gp[4]+0x30u;push(s,0x24cf42u,s.gp[2]);push(s,0x24cf43u,8);s.gp[1]=s.gp[3]+1u;push(s,0x24cf48u,s.gp[1]);push(s,0x24cf49u,s.gp[7]);s.gp[0]=1u+s.gp[0]*2u;push(s,0x24cf4eu,s.gp[0]);push(s,0x24cf4fu,0);push(s,0x24cf51u,0);
            call(s,dispatch,0x1a8850u,0x24cf53u,0x24cf58u);s.gp[2]=read(s,0x24cf58u,0x4ecc20u);s.gp[1]=read(s,0x24cf5eu,s.gp[2]+0x11cu);s.xmm[0]=_mm_set_ss(f(read(s,0x24cf64u,0x46a0ccu)));s.gp[6]=s.flags.shift(s.gp[6],5,4);
            s.gp[2]=read(s,0x24cf6fu,s.gp[1]+s.gp[6]+4u);s.gp[6]=s.flags.plus(s.gp[6],s.gp[1],4);s.flags.logic(s.gp[3],4);s.gp[1]=read(s,0x24cf77u,s.gp[6]);store(s,0x24cf79u,s.gp[4]+0x24u,4,s.gp[1]);s.gp[1]=read(s,0x24cf7du,s.gp[6]+8u);store(s,0x24cf80u,s.gp[4]+0x2cu,4,s.gp[1]);
            s.gp[1]=s.gp[7]*3u;store(s,0x24cf87u,s.gp[4]+0x14u,4,s.gp[0]);store(s,0x24cf8bu,s.gp[4]+0x28u,4,s.gp[2]);s.gp[6]=s.gp[5]+s.gp[1]*4u;
            s.gp[2]=s.gp[7]*3u+(which?0x2au:0xfu);store(s,which?0x24cfb2u:0x24cf99u,s.gp[5]+s.gp[2]*4u,4,u(_mm_cvtss_f32(s.xmm[0])));s.xmm[0]=which?_mm_setzero_ps():s.xmm[4];
            store(s,which?0x24cfbbu:0x24cfa2u,s.gp[6]+(which?0xacu:0x40u),4,0);store(s,which?0x24cfc3u:0x24cfa7u,s.gp[6]+(which?0xb0u:0x44u),4,0);
            s.gp[1]=0;s.flags.logic(0,4);s.flags.logic(s.gp[0],4);store(s,0x24cfcfu,s.gp[4]+0x10u,4,s.gp[1]);
            if(std::bit_cast<std::int32_t>(s.gp[0])>0) {
                s.xmm[2]=_mm_set_ss(f(read(s,0x24cfd9u,s.gp[4]+0x2cu)));
                for(;;) {
                    s.gp[2]=read(s,0x24cfe0u,s.gp[4]+s.gp[1]*4u+0x30u);s.gp[0]=read(s,0x24cfe4u,0x4ecc20u);s.gp[1]=read(s,0x24cfe9u,s.gp[0]+0x11cu);s.gp[2]=s.flags.shift(s.gp[2],5,4);s.gp[1]=s.flags.plus(s.gp[1],s.gp[2],4);s.set(2,read(s,0x24cff4u,s.gp[1]+0x16u,2),2,0);const auto flags=s.reg(2,1,0);s.flags.logic(flags&1u,1);bool eligible{};
                    if(flags&1u) {
                        s.gp[0]=read(s,0x24cffdu,0x4f3218u);s.gp[5]=read(s,0x24d002u,s.gp[0]+0x240u);s.flags.minus(s.gp[5],1,4);bool generic=s.gp[5]!=1;
                        if(generic) {s.flags.minus(s.gp[5],2,4);generic=s.gp[5]!=2;}
                        if(generic) {s.gp[5]=read(s,0x24d049u,0x52d710u);eligible=true;}
                        else s.gp[5]=read(s,0x24d012u,0x52d710u);
                    }
                    if(!eligible) {
                        s.flags.logic(flags&2u,1);
                        if(flags&2u) {s.gp[0]=read(s,0x24d01du,0x4f3218u);const auto kind=read(s,0x24d022u,s.gp[0]+0x240u);s.flags.minus(kind,1,4);eligible=kind==1;}
                        if(!eligible) {s.flags.logic(flags&4u,1);if(flags&4u) {s.gp[2]=read(s,0x24d034u,0x4f3218u);const auto kind=read(s,0x24d03au,s.gp[2]+0x240u);s.flags.minus(kind,2,4);eligible=kind==2;}}
                    }
                    if(eligible) {
                        s.flags.logic(s.gp[3],4);
                        // The Xbox uses extended intermediates for horizontal
                        // radius, followed by one single-precision publication.
                        const auto center_x=ext(f(read(s,0x24d051u,s.gp[4]+0x24u)));s.fp.status&=~0x200u;
                        s.gp[0]=read(s,0x24d055u,s.gp[1]);s.gp[2]=read(s,0x24d057u,s.gp[1]+4u);store(s,0x24d05au,s.gp[4]+0x18u,4,s.gp[0]);const auto dx=sub(center_x,ext(f(read(s,0x24d05eu,s.gp[4]+0x18u))));
                        const auto center_y=ext(f(read(s,0x24d062u,s.gp[4]+0x28u)));store(s,0x24d066u,s.gp[4]+0x1cu,4,s.gp[2]);const auto dy=sub(center_y,ext(f(read(s,0x24d06au,s.gp[4]+0x1cu))));s.gp[0]=read(s,0x24d06eu,s.gp[1]+8u);store(s,0x24d071u,s.gp[4]+0x20u,4,s.gp[0]);
                        const auto radius=root(add(mul(dx,dx),mul(dy,dy)));store(s,0x24d085u,s.gp[4]+0xcu,4,single_spill(radius,s.fp.status));s.xmm[0]=_mm_set_ss(f(read(s,0x24d089u,s.gp[4]+0xcu)));
                        const auto radius_offset=which?0x2au:0xfu;const auto below=which?0xacu:0x40u;const auto above=which?0xb0u:0x44u;
                        if(which)s.gp[2]=radius_offset+s.gp[7]*3u;else s.gp[1]=radius_offset+s.gp[7]*3u;
                        const auto index=which?s.gp[2]:s.gp[1];s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(read(s,which?0x24d0e3u:0x24d095u,s.gp[5]+index*4u))),false);s.gp[1]=s.gp[5]+index*4u;
                        if(s.flags.cf||s.flags.zf)s.xmm[0]=_mm_set_ss(f(read(s,which?0x24d0eeu:0x24d0a0u,s.gp[1])));
                        s.xmm[1]=_mm_set_ss(f(read(s,which?0x24d0f2u:0x24d0a4u,s.gp[4]+0x20u)));s.xmm[3]=_mm_set_ss(f(read(s,which?0x24d0f8u:0x24d0aau,s.gp[6]+below)));store(s,which?0x24d100u:0x24d0afu,s.gp[1],4,u(_mm_cvtss_f32(s.xmm[0])));
                        s.xmm[0]=_mm_sub_ss(s.xmm[2],s.xmm[1]);s.flags.scalar_compare(s.xmm[3],s.xmm[0],false);if(!s.flags.cf&&!s.flags.zf)s.xmm[0]=s.xmm[3];store(s,which?0x24d113u:0x24d0c2u,s.gp[6]+below,4,u(_mm_cvtss_f32(s.xmm[0])));
                        s.xmm[0]=_mm_set_ss(f(read(s,which?0x24d11bu:0x24d0c7u,s.gp[6]+above)));s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[2]);s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);if(!s.flags.cf&&!s.flags.zf)s.xmm[1]=s.xmm[0];store(s,which?0x24d12fu:0x24d0d8u,s.gp[6]+above,4,u(_mm_cvtss_f32(s.xmm[1])));
                    }
                    s.gp[1]=read(s,0x24d137u,s.gp[4]+0x10u);s.gp[0]=read(s,0x24d13bu,s.gp[4]+0x14u);s.gp[1]=s.flags.increment(s.gp[1],4,false);s.flags.minus(s.gp[1],s.gp[0],4);store(s,0x24d142u,s.gp[4]+0x10u,4,s.gp[1]);if(std::bit_cast<std::int32_t>(s.gp[1])>=std::bit_cast<std::int32_t>(s.gp[0]))break;
                }
            }
            s.flags.logic(s.gp[3],4);s.xmm[0]=_mm_set_ss(f(read(s,which?0x24d176u:0x24d150u,s.gp[6]+(which?0xacu:0x40u))));s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(read(s,which?0x24d17eu:0x24d155u,0x46a178u))));store(s,which?0x24d186u:0x24d15du,s.gp[6]+(which?0xacu:0x40u),4,u(_mm_cvtss_f32(s.xmm[0])));
            s.xmm[0]=_mm_set_ss(f(read(s,which?0x24d18eu:0x24d162u,s.gp[6]+(which?0xb0u:0x44u))));s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(read(s,which?0x24d196u:0x24d167u,0x46a194u))));store(s,which?0x24d19eu:0x24d16fu,s.gp[6]+(which?0xb0u:0x44u),4,u(_mm_cvtss_f32(s.xmm[0])));
        }
        s.gp[3]=s.flags.increment(s.gp[3],4,false);s.flags.minus(s.gp[3],2,4);
    }
    cursor(s,0x24d1b0u);s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[3]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x44u,4);s.pop();++source_objective_native_tu15_stats.owners[50];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
