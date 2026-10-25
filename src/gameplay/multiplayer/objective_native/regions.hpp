#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool initialize_region(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24d530u,s.gp[5]);s.gp[5]=read(s,0x24d531u,s.gp[4]+8u);s.gp[1]=s.gp[6]+8u;s.gp[0]=s.gp[5];
    store(s,0x24d53au,s.gp[6],1,1);store(s,0x24d53du,s.gp[6]+1u,1,0);store(s,0x24d541u,s.gp[6]+4u,4,1);
    call(s,dispatch,0xfce50u,0x24d548u,0x24d54du);s.gp[0]=read(s,0x24d54du,0x4f3218u);
    const auto engine=read(s,0x24d552u,s.gp[0]+0x180u);s.flags.minus(engine,9,4);
    const auto radius=read(s,engine==9?0x24d55bu:0x24d56au,engine==9?0x46a178u:0x456910u);s.xmm[0]=_mm_set_ss(f(radius));
    store(s,engine==9?0x24d563u:0x24d572u,s.gp[6]+0x14u,4,radius);
    if(engine!=9)s.xmm[0]=_mm_set_ss(f(read(s,0x24d577u,0x46a178u)));
    store(s,0x24d57fu,s.gp[6]+0x18u,4,u(_mm_cvtss_f32(s.xmm[0])));s.xmm[0]=_mm_setzero_ps();store(s,0x24d587u,s.gp[6]+0x1cu,4,0);
    s.xmm[0]=_mm_set_ss(f(read(s,0x24d58cu,0x46a0d0u)));store(s,0x24d594u,s.gp[6]+0x20u,4,s.gp[5]);
    s.gp[2]=s.gp[7];s.gp[0]=read(s,0x24d599u,s.gp[2]);s.gp[1]=s.gp[6]+0x24u;store(s,0x24d59eu,s.gp[1],4,s.gp[0]);
    s.gp[0]=read(s,0x24d5a0u,s.gp[2]+4u);store(s,0x24d5a3u,s.gp[1]+4u,4,s.gp[0]);s.gp[2]=read(s,0x24d5a6u,s.gp[2]+8u);store(s,0x24d5a9u,s.gp[1]+8u,4,s.gp[2]);
    s.gp[1]=s.gp[7];s.gp[2]=read(s,0x24d5aeu,s.gp[1]);s.gp[0]=s.gp[6]+0x30u;store(s,0x24d5b3u,s.gp[0],4,s.gp[2]);
    s.gp[2]=read(s,0x24d5b5u,s.gp[1]+4u);store(s,0x24d5b8u,s.gp[0]+4u,4,s.gp[2]);s.gp[1]=read(s,0x24d5bbu,s.gp[1]+8u);store(s,0x24d5beu,s.gp[0]+8u,4,s.gp[1]);
    s.gp[1]=s.gp[7];s.gp[2]=s.gp[5];s.gp[0]=s.gp[6];
    store(s,0x24d5c7u,s.gp[6]+0x3cu,4,u(_mm_cvtss_f32(s.xmm[0])));store(s,0x24d5ccu,s.gp[6]+0x40u,4,u(_mm_cvtss_f32(s.xmm[0])));store(s,0x24d5d1u,s.gp[6]+0x44u,4,0);
    call(s,dispatch,0x24d390u,0x24d5d8u,0x24d5ddu);s.set(0,1,1,0);cursor(s,0x24d5dfu);s.gp[5]=s.pop();s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[35];return true;
}
template<class State,class Dispatch>bool draw_goal_marker(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[2]=read(s,0x24d920u,0x4ecc1cu);push(s,0x24d926u,s.gp[0]);s.gp[0]=read(s,0x24d927u,s.gp[2]+0x16cu);
    push(s,0x24d92du,s.gp[1]);s.gp[1]=read(s,0x24d92eu,0x4f0414u);push(s,0x24d934u,0x24d900u);s.gp[0]&=65535u;s.flags.logic(s.gp[0],4);
    push(s,0x24d93eu,0x24d760u);s.gp[0]=s.flags.shift(s.gp[0],4,4);s.gp[2]=read(s,0x24d946u,s.gp[1]+s.gp[0]+8u);
    s.gp[0]=read(s,0x24d94au,s.gp[2]+0xcu);s.gp[1]=read(s,0x24d94du,s.gp[0]+0x534u);s.gp[0]=read(s,0x24d953u,s.gp[1]+0xc4u);
    push(s,0x24d959u,0xd7100u);push(s,0x24d95eu,0x44200000u);push(s,0x24d963u,0xffffffffu);push(s,0x24d965u,0);
    call(s,dispatch,0x41480u,0x24d967u,0x24d96cu);cursor(s,0x24d96cu);s.pop();++source_objective_native_tu15_stats.owners[36];return true;
}
template<class State,class Dispatch>bool initialize_region_style(State& s,Dispatch&&) {
    if(!admitted(s))return false;
    s.xmm[0]=_mm_setzero_ps();s.xmm[1]=_mm_set_ss(f(read(s,0x24d1c3u,0x46a178u)));push(s,0x24d1cbu,s.gp[3]);s.gp[3]=read(s,0x24d1ccu,0x4f54f0u);push(s,0x24d1d2u,s.gp[6]);
    store(s,0x24d1d3u,s.gp[1],1,1);store(s,0x24d1d6u,s.gp[1]+1u,1,0);store(s,0x24d1dau,s.gp[1]+4u,4,1);
    s.gp[6]=read(s,0x24d1e1u,s.gp[2]);s.gp[0]=s.gp[1]+8u;store(s,0x24d1e6u,s.gp[0],4,s.gp[6]);
    s.gp[6]=read(s,0x24d1e8u,s.gp[2]+4u);store(s,0x24d1ebu,s.gp[0]+4u,4,s.gp[6]);s.gp[2]=read(s,0x24d1eeu,s.gp[2]+8u);store(s,0x24d1f1u,s.gp[0]+8u,4,s.gp[2]);
    store(s,0x24d1f4u,s.gp[1]+0x14u,4,0);store(s,0x24d1f9u,s.gp[1]+0x18u,4,u(_mm_cvtss_f32(s.xmm[1])));store(s,0x24d1feu,s.gp[1]+0x1cu,4,0);store(s,0x24d203u,s.gp[1]+0x20u,4,0xffffffffu);
    // Four independent palette queries are intentional. The caller may alias
    // its destination with source storage, so each query observes prior writes.
    struct PaletteSites {std::uint32_t index,player,rows,mask,scale,add,species,first,third,alternate,x,y,z,write_x,write_y,write_z,offset;};
    constexpr std::array<PaletteSites,4> sites{{
        {0x24d20a,0x24d221,0x24d22a,0x24d22d,0x24d233,0x24d239,0x24d23b,0x24d241,0x24d246,0x24d24b,0x24d250,0x24d257,0x24d25d,0x24d255,0x24d25a,0x24d260,0x24},
        {0x24d263,0x24d273,0x24d27c,0x24d27f,0x24d285,0x24d28b,0x24d28d,0x24d293,0x24d298,0x24d29d,0x24d2a2,0x24d2b1,0x24d2b7,0x24d2af,0x24d2b4,0x24d2ba,0x30},
        {0x24d2ce,0x24d2de,0x24d2e7,0x24d2ea,0x24d2f0,0x24d2f6,0x24d2f8,0x24d2fe,0x24d303,0x24d308,0x24d30d,0x24d314,0x24d31a,0x24d312,0x24d317,0x24d31d,0x4c},
        {0x24d320,0x24d330,0x24d339,0x24d33c,0x24d342,0x24d348,0x24d34a,0x24d350,0x24d355,0x24d35a,0x24d35f,0x24d366,0x24d36c,0x24d364,0x24d369,0x24d371,0x58}}};
    for(unsigned pass=0;pass<sites.size();++pass) {
        const auto& p=sites[pass];s.gp[2]=read(s,p.index,0x4c66f8u);s.flags.minus(s.gp[2],0xffffffffu,4);
        if(pass==0) {push(s,0x24d213u,s.gp[7]);s.gp[7]=read(s,0x24d214u,0x4f54f4u);}
        s.gp[6]=0x4751e4u;
        if(s.gp[2]!=0xffffffffu) {
            s.gp[2]=read(s,p.player,s.gp[3]+s.gp[2]*4u+0xcu);s.flags.minus(s.gp[2],0xffffffffu,4);
            if(s.gp[2]!=0xffffffffu) {
                const unsigned rows=pass==3?3u:0u;s.gp[rows]=read(s,p.rows,s.gp[7]+0x44u);s.gp[2]&=65535u;s.flags.logic(s.gp[2],4);
                s.gp[2]=s.flags.product(s.gp[2],0x21cu);s.gp[2]=s.flags.plus(s.gp[2],s.gp[rows],4);s.set(2,read(s,p.species,s.gp[2]+0x88u,1),1,0);s.flags.minus(s.reg(2,1,0),1,1);
                bool alternate=s.reg(2,1,0)==1;if(!alternate) {s.flags.minus(s.reg(2,1,0),3,1);alternate=s.reg(2,1,0)==3;}
                if(alternate)s.gp[6]=0x4751f4u;
            }
        }
        s.gp[2]=read(s,p.x,s.gp[6]);if(pass==1)s.xmm[0]=_mm_set_ss(f(read(s,0x24d2a4u,0x46a0d0u)));
        s.gp[0]=s.gp[1]+p.offset;store(s,p.write_x,s.gp[0],4,s.gp[2]);s.gp[2]=read(s,p.y,s.gp[6]+4u);store(s,p.write_y,s.gp[0]+4u,4,s.gp[2]);s.gp[2]=read(s,p.z,s.gp[6]+8u);
        if(pass==3) {cursor(s,0x24d36fu);s.gp[7]=s.pop();s.gp[6]=s.pop();}
        store(s,p.write_z,s.gp[0]+8u,4,s.gp[2]);
        if(pass==1) {store(s,0x24d2bdu,s.gp[1]+0x3cu,4,u(_mm_cvtss_f32(s.xmm[0])));store(s,0x24d2c2u,s.gp[1]+0x40u,4,u(_mm_cvtss_f32(s.xmm[0])));store(s,0x24d2c7u,s.gp[1]+0x44u,4,1);}
    }
    store(s,0x24d374u,s.gp[1]+0x48u,4,0);store(s,0x24d37bu,s.gp[1]+0x64u,4,u(_mm_cvtss_f32(s.xmm[0])));store(s,0x24d380u,s.gp[1]+0x68u,4,0xffffffffu);
    s.set(0,1,1,0);cursor(s,0x24d389u);s.gp[3]=s.pop();s.pop();++source_objective_native_tu15_stats.owners[45];return true;
}
template<class State,class Dispatch>bool append_region(State& s,Dispatch&&) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0xcu,4);push(s,0x24d393u,s.gp[6]);s.gp[6]=read(s,0x24d394u,0x4ecbd0u);s.gp[6]=read(s,0x24d39au,s.gp[6]+0x44u);
    s.gp[2]&=65535u;s.flags.logic(s.gp[2],4);s.gp[2]*=3u;s.gp[6]=read(s,0x24d3a6u,s.gp[6]+s.gp[2]*4u+8u);s.gp[2]=read(s,0x24d3aau,s.gp[0]+0x44u);
    s.flags.minus(s.gp[2],2,4);store(s,0x24d3b0u,s.gp[4]+0xcu,4,s.gp[6]);bool append=std::bit_cast<std::int32_t>(s.gp[2])<2;
    if(append) {const auto team=read(s,0x24d3bau,s.gp[6]+0x17eu,2);s.flags.minus(team,65535u,2);append=team!=65535u;}
    if(append) {
        push(s,0x24d3c8u,s.gp[3]);push(s,0x24d3c9u,s.gp[5]);push(s,0x24d3cau,s.gp[7]);s.gp[7]=read(s,0x24d3cbu,0x4f3218u);
        s.gp[6]=read(s,0x24d3d1u,s.gp[7]+0x230u);s.flags.logic(s.gp[6],4);store(s,0x24d3d9u,s.gp[4]+0x14u,4,1);
        if(s.gp[6])store(s,0x24d3e3u,s.gp[4]+0x14u,4,s.gp[6]);
        s.set(6,read(s,0x24d3e7u,s.gp[7]+0x246u,2),2,0);s.flags.logic(s.reg(6,2,0),2);
        if(s.reg(6,2,0)) {s.gp[6]=extend(s.reg(6,2,0),2);store(s,0x24d400u,s.gp[4]+0x10u,4,s.gp[6]);}
        else store(s,0x24d3f3u,s.gp[4]+0x10u,4,5);
        s.gp[2]=s.gp[0]+s.gp[2]*36u+0x4cu;s.gp[3]=s.gp[1];s.gp[5]=read(s,0x24d40du,s.gp[3]);store(s,0x24d40fu,s.gp[2],4,s.gp[5]);
        s.gp[5]=read(s,0x24d411u,s.gp[3]+4u);store(s,0x24d414u,s.gp[2]+4u,4,s.gp[5]);s.gp[3]=read(s,0x24d417u,s.gp[3]+8u);store(s,0x24d41au,s.gp[2]+8u,4,s.gp[3]);
        s.gp[3]=read(s,0x24d41du,s.gp[1]);s.gp[2]=read(s,0x24d41fu,s.gp[0]+0x44u);s.gp[2]=s.gp[0]+s.gp[2]*36u+0x58u;store(s,0x24d429u,s.gp[2],4,s.gp[3]);
        s.gp[3]=read(s,0x24d42bu,s.gp[1]+4u);store(s,0x24d42eu,s.gp[2]+4u,4,s.gp[3]);s.gp[1]=read(s,0x24d431u,s.gp[1]+8u);store(s,0x24d434u,s.gp[2]+8u,4,s.gp[1]);
        s.gp[3]=read(s,0x24d437u,s.gp[7]+0x180u);s.gp[1]=read(s,0x24d43du,s.gp[0]+0x44u);s.gp[2]=0;s.flags.logic(0,4);s.gp[6]=s.flags.logic(0xffffffffu,4);s.gp[5]=9;
        s.flags.minus(s.gp[3],s.gp[5],4);s.set(2,s.flags.zf,1,0);s.gp[1]=s.flags.plus(s.gp[1],2,4);s.gp[1]*=9u;s.gp[2]=s.flags.increment(s.gp[2],4,false);
        store(s,0x24d456u,s.gp[0]+s.gp[1]*4u,4,s.gp[2]);s.gp[1]=read(s,0x24d459u,s.gp[0]+0x44u);s.gp[2]=s.gp[1]*9u;s.gp[1]=read(s,0x24d45fu,s.gp[4]+0x18u);
        store(s,0x24d463u,s.gp[0]+s.gp[2]*4u+0x68u,4,0xffffffffu);s.set(1,read(s,0x24d46bu,s.gp[1]+0x17eu,2),2,0);s.flags.logic(s.reg(1,2,0),2);
        s.gp[2]=read(s,0x24d475u,0x52d710u);bool valid=signed_half(s.gp[1])>=0;
        if(valid) {s.flags.minus(s.reg(1,2,0),s.reg(5,2,0),2);valid=signed_half(s.gp[1])<signed_half(s.gp[5]);}
        if(valid) {s.gp[6]=extend(s.reg(1,2,0),2);s.gp[6]=extend(read(s,0x24d485u,s.gp[2]+s.gp[6]*2u+0x114u,2),2);}
        const auto engine=read(s,0x24d48du,s.gp[7]+0x180u);s.flags.minus(engine,s.gp[5],4);bool planted=engine==s.gp[5];
        if(planted) {s.gp[1]=extend(s.reg(1,2,0),2);const auto flags=read(s,0x24d498u,s.gp[2]+s.gp[1]+0x170u,1);s.flags.logic(flags&2u,1);planted=flags&2u;}
        s.flags.minus(s.gp[6],0xffffffffu,4);const bool countdown=s.gp[6]!=0xffffffffu;
        if(planted) {
            s.gp[1]=read(s,countdown?0x24d4cau:0x24d4a7u,s.gp[0]+0x44u);
            if(countdown) {
                s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(read(s,0x24d4cdu,s.gp[4]+0x10u)));s.gp[2]=s.gp[1]*9u;s.xmm[0]=_mm_cvtsi32_ss(s.xmm[0],si(s.gp[6]));
                cursor(s,0x24d4dau);s.gp[7]=s.pop();s.xmm[0]=_mm_div_ss(s.xmm[0],s.xmm[1]);store(s,0x24d4dfu,s.gp[0]+s.gp[2]*4u+0x64u,4,u(_mm_cvtss_f32(s.xmm[0])));
                s.gp[1]=read(s,0x24d4e5u,s.gp[0]+0x44u);s.gp[5]=s.pop();s.gp[1]=s.flags.increment(s.gp[1],4,false);s.gp[3]=s.pop();
                store(s,0x24d4ebu,s.gp[0],1,1);store(s,0x24d4eeu,s.gp[0]+0x44u,4,s.gp[1]);
            } else {
                s.xmm[0]=_mm_setzero_ps();s.gp[2]=s.gp[1]*9u;store(s,0x24d4b0u,s.gp[0]+s.gp[2]*4u+0x64u,4,0);
                s.gp[1]=read(s,0x24d4b6u,s.gp[0]+0x44u);cursor(s,0x24d4b9u);s.gp[7]=s.pop();s.gp[5]=s.pop();s.gp[1]=s.flags.increment(s.gp[1],4,false);s.gp[3]=s.pop();
                store(s,0x24d4bdu,s.gp[0],1,1);store(s,0x24d4c0u,s.gp[0]+0x44u,4,s.gp[1]);
            }
        } else {
            if(countdown) {s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(read(s,0x24d502u,s.gp[4]+0x14u)));s.xmm[0]=_mm_cvtsi32_ss(s.xmm[0],si(s.gp[6]));s.xmm[0]=_mm_div_ss(s.xmm[0],s.xmm[1]);}
            else s.xmm[0]=_mm_setzero_ps();
            s.gp[1]=read(s,0x24d510u,s.gp[0]+0x44u)*9u;store(s,0x24d516u,s.gp[0]+s.gp[1]*4u+0x64u,4,u(_mm_cvtss_f32(s.xmm[0])));
            s.gp[1]=read(s,0x24d51cu,s.gp[0]+0x44u);cursor(s,0x24d51fu);s.gp[7]=s.pop();s.gp[1]=s.flags.increment(s.gp[1],4,false);s.gp[5]=s.pop();
            store(s,0x24d522u,s.gp[0],1,1);store(s,0x24d525u,s.gp[0]+0x44u,4,s.gp[1]);s.gp[3]=s.pop();
        }
    }
    s.set(0,1,1,0);cursor(s,0x24d52bu);s.gp[6]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0xcu,4);s.pop();++source_objective_native_tu15_stats.owners[44];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
