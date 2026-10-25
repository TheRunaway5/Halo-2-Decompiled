#pragma once

namespace halo2::source_objective_native_tu15_detail {
// The per-player objective tick has two paths: arm an unarmed bomb while
// inside the enemy goal, or complete a flag capture / armed-bomb delivery.
// Object, round, announcement and score mutations stay with their real owners.
template<class State,class Dispatch>bool player_tick(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x2496b0u,0x4f3218u);s.set(1,read(s,0x2496b5u,s.gp[0]+0xcu,1),1,0);
    s.gp[4]=s.flags.minus(s.gp[4],0x3cu,4);s.flags.minus(s.reg(1,1,0),4,1);
    unsigned saved{};
    const auto process=[&] {
        if(s.reg(1,1,0)==4)return;
        call(s,dispatch,0x15dd10u,0x2496c4u,0x2496c9u);s.flags.logic(s.reg(0,1,0),1);if(!s.reg(0,1,0))return;
        s.gp[1]=read(s,0x2496d1u,0x4f54f4u);s.gp[2]=read(s,0x2496d7u,s.gp[1]+0x44u);
        push(s,0x2496dau,s.gp[3]);s.gp[3]=read(s,0x2496dbu,s.gp[4]+0x44u)&65535u;
        s.flags.logic(s.gp[3],4);s.gp[3]=s.flags.product(s.gp[3],0x21cu);
        s.gp[0]=read(s,0x2496ebu,s.gp[3]+s.gp[2]+0x2cu);s.gp[3]=s.flags.plus(s.gp[3],s.gp[2],4);
        push(s,0x2496f1u,s.gp[6]);s.gp[6]=s.flags.logic(0xffffffffu,4);saved=2;
        s.flags.minus(s.gp[0],s.gp[6],4);if(s.gp[0]==s.gp[6])return;
        s.gp[1]=s.gp[0]&65535u;s.flags.logic(s.gp[1],4);s.gp[2]=s.gp[1]*3u;
        s.gp[1]=read(s,0x249708u,0x4ecbd0u);s.gp[1]=read(s,0x24970eu,s.gp[1]+0x44u);
        push(s,0x249711u,s.gp[5]);push(s,0x249712u,s.gp[7]);saved=4;
        s.gp[7]=read(s,0x249713u,s.gp[1]+s.gp[2]*4u+8u);
        s.set(1,extend(read(s,0x249717u,s.gp[7]+0x212u,1),1),2,0);
        call(s,dispatch,0xce230u,0x24971fu,0x249724u);
        const auto detached=read(s,0x249724u,s.gp[7]+0x1fcu,2);s.flags.minus(detached,s.reg(6,2,0),2);
        s.gp[5]=s.gp[0];store(s,0x24972du,s.gp[4]+0x14u,4,s.gp[5]);if(detached!=s.reg(6,2,0))return;
        s.flags.minus(s.gp[5],s.gp[6],4);if(s.gp[5]==s.gp[6])return;
        s.flags.minus(s.reg(5,2,0),s.reg(6,2,0),2);if(s.reg(5,2,0)==s.reg(6,2,0))return;
        call(s,dispatch,0x106350u,0x249748u,0x24974du);s.flags.minus(s.reg(0,2,0),1,2);if(s.reg(0,2,0)!=1)return;
        s.gp[2]=read(s,0x249757u,0x4f3218u);s.set(0,read(s,0x24975du,s.gp[2]+0x1128u,1),1,0);
        s.flags.logic(s.reg(0,1,0),1);if(s.reg(0,1,0))return;
        s.gp[1]=read(s,0x24976bu,0x4ecbd0u);s.gp[2]=read(s,0x249771u,s.gp[1]+0x44u);
        s.gp[0]=s.gp[5]&65535u;s.flags.logic(s.gp[0],4);s.gp[0]*=3u;
        s.gp[0]=read(s,0x24977eu,s.gp[2]+s.gp[0]*4u+8u);s.gp[7]=extend(read(s,0x249782u,s.gp[0]+0x17eu,2),2);
        s.flags.minus(s.gp[7],s.gp[6],4);if(s.gp[7]==s.gp[6])return;
        s.gp[1]=read(s,0x249791u,0x4f63b8u);s.gp[2]=read(s,0x249797u,s.gp[1]+0xc14u);
        s.gp[1]=read(s,0x24979du,0x56dfa0u+s.gp[2]*4u);s.gp[5]=read(s,0x2497a4u,s.gp[4]+0x50u);
        s.gp[0]=read(s,0x2497a8u,s.gp[1]);push(s,0x2497aau,1);push(s,0x2497acu,s.gp[5]);
        const auto pickup=read(s,0x2497adu,s.gp[0]+0x8cu);call(s,dispatch,pickup,0x2497adu,0x2497b3u);
        s.flags.logic(s.reg(0,1,0),1);
        if(!s.reg(0,1,0)) {
            call(s,dispatch,0x13b360u,0x2497b7u,0x2497bcu);s.flags.logic(s.reg(0,1,0),1);
            if(!s.reg(0,1,0)) {s.gp[0]=s.gp[5];call(s,dispatch,0x1628f0u,0x2497c2u,0x2497c7u);}
        }
        s.gp[1]=read(s,0x2497c7u,0x4f3218u);const auto engine=read(s,0x2497cdu,s.gp[1]+0x180u);
        s.flags.minus(engine,9,4);s.gp[5]=read(s,0x2497d4u,0x52d710u);
        bool unarmed=engine==9;
        if(unarmed) {const auto flags=read(s,0x2497e0u,s.gp[5]+s.gp[7]+0x170u,1);s.flags.logic(flags&8u,1);unarmed=!(flags&8u);}
        if(unarmed) {
            s.set(0,read(s,0x2497eeu,s.gp[3]+0xc0u,1),1,0);s.flags.minus(s.reg(0,1,0),255,1);if(s.reg(0,1,0)==255)return;
            s.gp[2]=read(s,0x2497fcu,s.gp[3]+0x2cu);s.gp[0]=extend(s.reg(0,1,0),1);push(s,0x249802u,s.gp[2]);
            call(s,dispatch,0x24d970u,0x249803u,0x249808u);s.flags.logic(s.reg(0,1,0),1);
            if(s.reg(0,1,0)) {
                const auto progress=read(s,0x249810u,s.gp[5]+s.gp[7]*2u+0x1a0u,2);s.flags.minus(progress,s.reg(6,2,0),2);s.gp[3]=10;
                if(progress==s.reg(6,2,0)) {
                    s.gp[0]=read(s,0x24981fu,s.gp[4]+0x50u);s.gp[1]=s.gp[4]+0x28u;
                    store(s,0x249827u,s.gp[4]+0x28u,4,s.gp[3]);store(s,0x24982bu,s.gp[4]+0x2cu,4,14);
                    for(unsigned n=0;n<5;++n)store(s,0x249833u+n*4u,s.gp[4]+0x30u+n*4u,4,s.gp[6]);
                    store(s,0x249847u,s.gp[4]+0x44u,4,0);store(s,0x24984fu,s.gp[4]+0x48u,2,s.gp[6]);
                    call(s,dispatch,0x1a87f0u,0x249854u,0x249859u);s.gp[0]=s.gp[1];call(s,dispatch,0x1a87a0u,0x24985bu,0x249860u);
                    s.gp[5]=read(s,0x249860u,0x52d710u);
                }
                arm(s);
                if(s.flags.zf) {
                    s.set(2,read(s,0x2498a0u,s.gp[5]+s.gp[7]+0x170u,1),1,0);s.gp[0]=read(s,0x2498a7u,s.gp[4]+0x50u);
                    s.set(2,s.flags.logic(s.reg(2,1,0)|8u,1),1,0);store(s,0x2498aeu,s.gp[5]+s.gp[7]+0x170u,1,s.reg(2,1,0));
                    s.set(1,s.reg(2,1,0),1,0);s.set(1,s.flags.logic(s.reg(1,1,0)|1u,1),1,0);
                    store(s,0x2498bau,s.gp[5]+s.gp[7]+0x170u,1,s.reg(1,1,0));s.gp[1]=s.gp[4]+0x28u;
                    store(s,0x2498c5u,s.gp[4]+0x28u,4,s.gp[3]);store(s,0x2498c9u,s.gp[4]+0x2cu,4,15);
                    for(unsigned n=0;n<5;++n)store(s,0x2498d1u+n*4u,s.gp[4]+0x30u+n*4u,4,s.gp[6]);
                    store(s,0x2498e5u,s.gp[4]+0x44u,4,0);store(s,0x2498edu,s.gp[4]+0x48u,2,s.gp[6]);
                    call(s,dispatch,0x1a87f0u,0x2498f2u,0x2498f7u);s.gp[0]=s.gp[1];call(s,dispatch,0x1a87a0u,0x2498f9u,0x2498feu);
                    push(s,0x2498feu,0x200u);call(s,dispatch,0xb1a10u,0x249903u,0x249908u);
                    push(s,0x249908u,0x100u);call(s,dispatch,0xb1a10u,0x24990du,0x249912u);return;
                }
            } else decay(s);
            push(s,0x249934u,0x100u);call(s,dispatch,0xb1a10u,0x249939u,0x24993eu);return;
        }
        s.gp[0]=read(s,0x249948u,s.gp[3]+0x2cu);s.gp[6]=s.gp[4]+0x1cu;call(s,dispatch,0xbbe00u,0x24994fu,0x249954u);
        s.gp[2]=extend(read(s,0x249954u,s.gp[3]+0xc0u,1),1);push(s,0x24995bu,s.gp[2]);s.gp[0]=s.gp[6];push(s,0x24995eu,s.gp[0]);
        call(s,dispatch,0x24c1e0u,0x24995fu,0x249964u);s.flags.minus(s.gp[0],0xffffffffu,4);
        store(s,0x249967u,s.gp[4]+0x18u,4,s.gp[0]);if(s.gp[0]==0xffffffffu)return;
        s.gp[6]=read(s,0x249971u,0x4f3218u);const auto options=read(s,0x249977u,s.gp[6]+0x22cu,1);s.flags.logic(options&8u,1);
        bool require_home=options&8u;
        if(require_home) {
            s.gp[0]=read(s,0x249980u,s.gp[6]+0x240u);s.flags.minus(s.gp[0],1,4);require_home=s.gp[0]!=1;
            if(require_home) {s.flags.minus(s.gp[0],2,4);require_home=s.gp[0]!=2;}
            if(require_home) {const auto kind=read(s,0x249990u,s.gp[6]+0x180u);s.flags.minus(kind,9,4);require_home=kind!=9;}
        }
        bool home=true;
        if(require_home) {
            s.set(3,read(s,0x249999u,s.gp[3]+0xc0u,1),1,0);store(s,0x24999fu,s.gp[4]+0x13u,1,s.reg(3,1,0));
            s.set(3,extend(s.reg(3,1,0),1),2,0);call(s,dispatch,0x160e50u,0x2499a7u,0x2499acu);
            s.flags.logic(s.gp[0],4);home=s.gp[0]!=0;
            if(home) {
                s.gp[1]=extend(read(s,0x2499b0u,s.gp[4]+0x13u,1),1);s.set(0,read(s,0x2499b5u,s.gp[5]+s.gp[1]+0x170u,1),1,0);
                s.set(0,s.flags.logic((~s.reg(0,1,0))&1u,1),1,0);home=s.reg(0,1,0)!=0;
            }
        }
        if(!home) {
            s.gp[1]=read(s,0x249a2bu,s.gp[4]+0x50u);s.set(0,1,1,0);call(s,dispatch,0x24c390u,0x249a31u,0x249a36u);return;
        }
        call(s,dispatch,0x13b340u,0x2499c2u,0x2499c7u);s.flags.logic(s.reg(0,1,0),1);if(!s.reg(0,1,0))return;
        const auto kind=read(s,0x2499cbu,s.gp[6]+0x180u);s.flags.minus(kind,9,4);
        if(kind==9) {
            s.gp[2]=read(s,0x2499d4u,s.gp[4]+0x50u);s.gp[0]=read(s,0x2499d8u,s.gp[4]+0x14u);
            push(s,0x2499dcu,s.gp[2]);push(s,0x2499ddu,s.gp[0]);s.gp[0]=read(s,0x2499deu,s.gp[4]+0x20u);
            call(s,dispatch,0x24be90u,0x2499e2u,0x2499e7u);return;
        }
        s.flags.minus(s.reg(7,2,0),8,2);s.gp[6]=s.reg(7,2,0)==8?s.flags.logic(0xffffffffu,4):extend(s.reg(7,2,0),2);
        s.gp[3]=read(s,0x2499ffu,s.gp[4]+0x50u);push(s,0x249a03u,s.gp[7]);push(s,0x249a04u,1);
        call(s,dispatch,0x19bc10u,0x249a06u,0x249a0bu);s.gp[1]=extend(s.reg(6,2,0),2);push(s,0x249a0eu,s.gp[1]);s.gp[0]=s.gp[3];
        call(s,dispatch,0x24c0f0u,0x249a11u,0x249a16u);s.gp[0]=read(s,0x249a16u,s.gp[4]+0x14u);push(s,0x249a1au,0);
        call(s,dispatch,0x24b950u,0x249a1cu,0x249a21u);
    };
    process();
    if(saved==4) {cursor(s,0x249a36u);s.gp[7]=s.pop();cursor(s,0x249a37u);s.gp[5]=s.pop();}
    if(saved>=2) {cursor(s,0x249a38u);s.gp[6]=s.pop();cursor(s,0x249a39u);s.gp[3]=s.pop();}
    s.gp[4]=s.flags.plus(s.gp[4],0x3cu,4);cursor(s,0x249a3du);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[31];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
