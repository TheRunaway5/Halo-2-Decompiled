#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool goal_at_position(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x18u,4);s.gp[0]=read(s,0x24c1e3u,0x4f63b8u);s.gp[1]=read(s,0x24c1e8u,s.gp[0]+0xc14u);s.gp[2]=read(s,0x24c1eeu,0x56dfa0u+s.gp[1]*4u);
    s.xmm[3]=_mm_set_ss(f(read(s,0x24c1f5u,0x46a0d0u)));push(s,0x24c1fdu,s.gp[3]);push(s,0x24c1feu,s.gp[5]);s.gp[5]=read(s,0x24c1ffu,0x52d710u);push(s,0x24c205u,s.gp[6]);push(s,0x24c206u,s.gp[7]);
    store(s,0x24c207u,s.gp[4]+0x14u,4,0xffffffffu);s.gp[6]=0;s.flags.logic(0,4);store(s,0x24c211u,s.gp[4]+0x18u,4,s.gp[2]);
    for(unsigned team=0;team<8;++team) {
        s.gp[0]=read(s,0x24c220u,s.gp[4]+0x18u);s.flags.logic(s.gp[0],4);bool active=s.gp[0]!=0;
        if(active) {
            s.gp[3]=read(s,0x24c22cu,0x4f3218u);s.set(0,read(s,0x24c232u,s.gp[3]+0x184u,1),1,0);s.set(0,s.flags.logic(s.reg(0,1,0)&1u,1),1,0);store(s,0x24c23au,s.gp[4]+0x13u,1,s.reg(0,1,0));active=s.reg(0,1,0)!=0;
        }
        if(active) {s.flags.logic(s.gp[6],4);active=std::bit_cast<std::int32_t>(s.gp[6])>=0;}
        if(active) {s.flags.minus(s.gp[6],8,4);active=std::bit_cast<std::int32_t>(s.gp[6])<8;}
        if(active) {
            s.gp[1]=read(s,0x24c255u,0x4f63b8u);s.gp[2]=read(s,0x24c25bu,s.gp[1]+0xcu,2);s.gp[0]=1;s.gp[1]=s.gp[6];s.gp[0]=s.flags.shift(s.gp[0],s.reg(1,1,0),4);
            s.flags.logic(s.gp[0]&s.gp[2],4);s.set(0,!s.flags.zf,1,0);s.flags.logic(s.reg(0,1,0),1);active=s.reg(0,1,0)!=0;
        }
        if(active) {
            push(s,0x24c275u,s.gp[6]);call(s,dispatch,0x15b3b0u,0x24c276u,0x24c27bu);s.gp[2]=extend(s.reg(0,2,0),2);s.flags.minus(s.gp[2],0xffffffffu,4);active=s.gp[2]!=0xffffffffu;
        }
        if(active) {
            s.gp[3]=read(s,0x24c287u,s.gp[3]+0x240u);s.gp[7]=0;s.flags.logic(0,4);
            for(unsigned which=0;which<2;++which) {
                s.flags.logic(s.gp[7],4);s.set(0,read(s,which?0x24c29bu:0x24c294u,s.gp[5]+s.gp[2]*2u+(which?0x26u:0x14u),2),2,0);
                s.flags.minus(s.gp[3],1,4);s.gp[1]=extend(s.reg(0,2,0),2);bool selected_assault=false,eligible=true;
                if(s.gp[3]==1) {s.gp[0]=read(s,0x24c2a8u,0x4f3218u);const auto engine=read(s,0x24c2adu,s.gp[0]+0x180u);s.flags.minus(engine,9,4);selected_assault=engine==9;}
                if(selected_assault) {
                    const auto player_team=read(s,0x24c2b6u,s.gp[4]+0x30u);s.flags.minus(s.gp[6],player_team,4);eligible=s.gp[6]!=player_team;
                    if(eligible) {const auto selected=read(s,0x24c2c0u,s.gp[5]+0x1f4u);s.flags.minus(s.gp[6],selected,4);eligible=s.gp[6]==selected;}
                } else {
                    s.gp[0]=read(s,0x24c2cdu,0x4f3218u);const auto engine=read(s,0x24c2d2u,s.gp[0]+0x180u);s.flags.minus(engine,9,4);s.gp[0]=read(s,0x24c2d9u,s.gp[4]+0x30u);
                    s.flags.minus(s.gp[6],s.gp[0],4);s.set(0,engine==9?!s.flags.zf:s.flags.zf,1,0);s.flags.logic(s.reg(0,1,0),1);eligible=s.reg(0,1,0)!=0;
                }
                if(eligible) {s.flags.minus(s.gp[1],0xffffffffu,4);eligible=s.gp[1]!=0xffffffffu;}
                if(eligible) {
                    s.gp[5]=read(s,0x24c2f4u,0x4ecc20u);s.gp[0]=s.flags.shift(s.gp[1],5,4);s.gp[0]=s.flags.plus(s.gp[0],read(s,0x24c2ffu,s.gp[5]+0x11cu),4);
                    s.gp[5]=read(s,0x24c305u,s.gp[0]);store(s,0x24c307u,s.gp[4]+0x1cu,4,s.gp[5]);s.gp[5]=read(s,0x24c30bu,s.gp[0]+4u);s.gp[0]=read(s,0x24c30eu,s.gp[0]+8u);
                    s.xmm[0]=_mm_set_ss(f(read(s,0x24c311u,s.gp[4]+0x1cu)));store(s,0x24c317u,s.gp[4]+0x24u,4,s.gp[0]);s.gp[0]=read(s,0x24c31bu,s.gp[4]+0x2cu);
                    s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(read(s,0x24c31fu,s.gp[0]))));s.xmm[2]=_mm_set_ss(f(read(s,0x24c323u,s.gp[4]+0x24u)));s.xmm[2]=_mm_sub_ss(s.xmm[2],_mm_set_ss(f(read(s,0x24c329u,s.gp[0]+8u))));
                    s.xmm[4]=_mm_mul_ss(s.xmm[2],s.xmm[2]);store(s,0x24c335u,s.gp[4]+0x20u,4,s.gp[5]);s.xmm[1]=_mm_set_ss(f(read(s,0x24c339u,s.gp[4]+0x20u)));s.xmm[1]=_mm_sub_ss(s.xmm[1],_mm_set_ss(f(read(s,0x24c33fu,s.gp[0]+4u))));
                    s.gp[5]=read(s,0x24c344u,0x52d710u);s.xmm[2]=_mm_mul_ss(s.xmm[0],s.xmm[0]);s.xmm[0]=_mm_mul_ss(s.xmm[1],s.xmm[1]);s.xmm[4]=_mm_add_ss(s.xmm[4],s.xmm[2]);s.xmm[4]=_mm_add_ss(s.xmm[4],s.xmm[0]);s.flags.scalar_compare(s.xmm[3],s.xmm[4],false);
                    if(!s.flags.cf&&!s.flags.zf) {store(s,0x24c371u,s.gp[4]+0x14u,4,s.gp[1]);break;}
                }
                s.gp[7]=s.flags.increment(s.gp[7],4,false);s.flags.minus(s.gp[7],2,4);
            }
        }
        s.gp[6]=s.flags.increment(s.gp[6],4,false);s.flags.minus(s.gp[6],8,4);
    }
    s.gp[0]=read(s,0x24c37fu,s.gp[4]+0x14u);cursor(s,0x24c383u);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[3]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x18u,4);s.pop();s.gp[4]+=8u;
    ++source_objective_native_tu15_stats.owners[46];return true;
}
template<class State,class Dispatch>bool player_near_objective(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[1]=read(s,0x24d5f0u,0x4ecc1cu);s.gp[2]=read(s,0x24d5f6u,s.gp[1]+0x16cu);s.gp[1]=read(s,0x24d5fcu,0x4f0414u);s.gp[2]&=65535u;s.flags.logic(s.gp[2],4);s.gp[2]=s.flags.shift(s.gp[2],4,4);
    s.gp[2]=read(s,0x24d60bu,s.gp[1]+s.gp[2]+8u);s.gp[1]=read(s,0x24d60fu,s.gp[2]+0xcu);s.gp[1]=read(s,0x24d612u,s.gp[1]+0x534u);s.xmm[5]=_mm_set_ss(f(read(s,0x24d618u,s.gp[1]+0xd8u)));
    s.gp[1]=s.gp[0]&65535u;s.flags.logic(s.gp[1],4);s.gp[4]=s.flags.minus(s.gp[4],0x28u,4);s.gp[2]=s.gp[1]*3u;s.gp[1]=read(s,0x24d62eu,0x4ecbd0u);s.gp[1]=read(s,0x24d634u,s.gp[1]+0x44u);s.gp[2]=read(s,0x24d637u,s.gp[1]+s.gp[2]*4u+8u);
    push(s,0x24d63bu,s.gp[5]);push(s,0x24d63cu,s.gp[6]);push(s,0x24d63du,s.gp[7]);s.gp[7]=extend(read(s,0x24d63eu,s.gp[2]+0x17eu,2),2);s.flags.minus(s.reg(7,2,0),8,2);s.xmm[5]=_mm_mul_ss(s.xmm[5],s.xmm[5]);
    s.gp[1]=s.reg(7,2,0)==8?s.flags.logic(0xffffffffu,4):extend(s.reg(7,2,0),2);s.gp[6]=s.gp[4]+0xcu;s.gp[5]=extend(s.reg(1,2,0),2);call(s,dispatch,0xbbe00u,0x24d65eu,0x24d663u);s.flags.minus(s.gp[7],0xffffffffu,4);bool found{};
    if(s.gp[7]!=0xffffffffu) {
        s.gp[0]=read(s,0x24d66cu,0x4f54f4u);s.gp[6]=s.gp[4]+0x24u;store(s,0x24d675u,s.gp[4]+0x28u,4,s.gp[0]);store(s,0x24d679u,s.gp[4]+0x30u,4,0xffffffffu);store(s,0x24d681u,s.gp[4]+0x2cu,4,0xffffffffu);
        call(s,dispatch,0x1a8f10u,0x24d689u,0x24d68eu);s.flags.logic(s.reg(0,1,0),1);
        if(s.reg(0,1,0)) {
            s.gp[7]=read(s,0x24d696u,0x4f3218u);s.xmm[6]=_mm_set_ss(f(read(s,0x24d69cu,s.gp[4]+0x14u)));s.xmm[7]=_mm_set_ss(f(read(s,0x24d6a2u,s.gp[4]+0x10u)));
            do {
                s.gp[0]=read(s,0x24d6b0u,s.gp[4]+0x24u);const auto engine=read(s,0x24d6b4u,s.gp[7]+0x180u);s.flags.minus(engine,9,4);s.gp[1]=extend(read(s,0x24d6bbu,s.gp[0]+0xc0u,1),1);s.gp[0]=read(s,0x24d6c2u,s.gp[0]+0x2cu);
                // Original24D6C5..24D6D8: CTF checks the opposing team;
                // Assault checks the bomb's owning team. Neutral accepts both.
                s.flags.minus(s.gp[1],s.gp[5],4);bool consider=engine==9?s.gp[1]==s.gp[5]:s.gp[1]!=s.gp[5];
                if(!consider) {const auto kind=read(s,0x24d6d1u,s.gp[7]+0x240u);s.flags.minus(kind,2,4);consider=kind==2;}
                if(consider) {
                    s.gp[6]=s.gp[4]+0x18u;call(s,dispatch,0xbbe00u,0x24d6deu,0x24d6e3u);s.xmm[0]=_mm_set_ss(f(read(s,0x24d6e3u,s.gp[4]+0xcu)));s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(read(s,0x24d6e9u,s.gp[4]+0x18u))));
                    s.xmm[2]=_mm_sub_ss(s.xmm[6],_mm_set_ss(f(read(s,0x24d6f2u,s.gp[4]+0x20u))));s.xmm[3]=_mm_mul_ss(s.xmm[2],s.xmm[2]);s.xmm[1]=_mm_sub_ss(s.xmm[7],_mm_set_ss(f(read(s,0x24d702u,s.gp[4]+0x1cu))));
                    s.xmm[2]=_mm_mul_ss(s.xmm[1],s.xmm[1]);s.xmm[1]=_mm_mul_ss(s.xmm[0],s.xmm[0]);s.xmm[3]=_mm_add_ss(s.xmm[3],s.xmm[2]);s.xmm[3]=_mm_add_ss(s.xmm[3],s.xmm[1]);s.flags.scalar_compare(s.xmm[5],s.xmm[3],false);found=!s.flags.cf&&!s.flags.zf;if(found)break;
                }
                s.gp[6]=s.gp[4]+0x24u;call(s,dispatch,0x1a8f10u,0x24d727u,0x24d72cu);s.flags.logic(s.reg(0,1,0),1);
            } while(s.reg(0,1,0));
        }
    }
    bool output{};
    if(found) {s.gp[1]=read(s,0x24d73bu,s.gp[4]+0x38u);s.flags.logic(s.gp[1],4);s.set(0,1,1,0);output=s.gp[1]!=0;}
    else s.set(0,s.flags.logic(0,1),1,0);
    if(output) {s.gp[2]=read(s,0x24d745u,s.gp[4]+0x2cu);cursor(s,0x24d749u);s.gp[7]=s.pop();s.gp[6]=s.pop();store(s,0x24d74bu,s.gp[1],4,s.gp[2]);s.gp[5]=s.pop();}
    else {cursor(s,0x24d732u);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();}
    s.gp[4]=s.flags.plus(s.gp[4],0x28u,4);s.pop();s.gp[4]+=4u;++source_objective_native_tu15_stats.owners[47];return true;
}
template<class State,class Dispatch>bool inside_arming_goal(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x14u,4);push(s,0x24d973u,s.gp[3]);push(s,0x24d974u,s.gp[5]);push(s,0x24d975u,s.gp[6]);push(s,0x24d976u,s.gp[7]);push(s,0x24d977u,s.gp[0]);
    call(s,dispatch,0x15b3b0u,0x24d978u,0x24d97du);s.gp[6]=s.gp[4]+0x18u;store(s,0x24d981u,s.gp[4]+0x13u,1,0);s.gp[7]=s.gp[0];s.gp[0]=read(s,0x24d988u,s.gp[4]+0x28u);
    call(s,dispatch,0xbbe00u,0x24d98cu,0x24d991u);s.gp[2]=read(s,0x24d991u,0x4f63b8u);s.gp[0]=read(s,0x24d997u,0x52d710u);
    s.xmm[5]=_mm_set_ss(f(read(s,0x24d99cu,s.gp[4]+0x20u)));s.xmm[3]=_mm_set_ss(f(read(s,0x24d9a2u,s.gp[4]+0x1cu)));s.xmm[4]=_mm_set_ss(f(read(s,0x24d9a8u,s.gp[4]+0x18u)));
    s.gp[1]=extend(s.reg(7,2,0),2);s.gp[6]=0;s.flags.logic(0,4);s.gp[2]=s.flags.plus(s.gp[2],0x10u,4);store(s,0x24d9b6u,s.gp[4]+0x14u,4,s.gp[1]);s.gp[5]=s.gp[0]+0xa8u;s.gp[7]=s.gp[0]+0x26u;store(s,0x24d9c3u,s.gp[4]+0x28u,4,s.gp[2]);
    for(unsigned team=0;team<9;++team) {
        const auto player_team=read(s,0x24d9d0u,s.gp[4]+0x14u);s.flags.minus(s.gp[6],player_team,4);const auto own=s.gp[6]==player_team;
        s.gp[0]=read(s,0x24d9d4u,s.gp[4]+0x28u);s.set(0,read(s,0x24d9d8u,s.gp[0],2),2,0);bool eligible=!own;
        if(eligible) {s.flags.minus(s.reg(0,2,0),65535u,2);eligible=s.reg(0,2,0)!=65535u;}
        if(eligible) {s.gp[1]=extend(s.reg(0,2,0),2);call(s,dispatch,0x164890u,0x24d9eeu,0x24d9f3u);s.flags.logic(s.reg(0,1,0),1);eligible=s.reg(0,1,0)!=0;}
        if(eligible) {
            s.gp[2]=0;s.flags.logic(0,4);
            for(unsigned which=0;which<2;++which) {
                s.flags.logic(s.gp[2],4);s.set(0,read(s,which?0x24da0au:0x24da04u,which?s.gp[7]:s.gp[7]-0x12u,2),2,0);
                s.gp[0]=extend(s.reg(0,2,0),2);s.flags.minus(s.gp[0],0xffffffffu,4);
                bool inside=false;
                if(s.gp[0]!=0xffffffffu) {
                    s.flags.logic(s.gp[2],4);s.gp[1]=which?s.gp[5]:s.gp[5]-0x6cu;s.gp[3]=read(s,0x24da22u,0x4ecc20u);s.xmm[2]=_mm_set_ss(f(read(s,0x24da28u,s.gp[1])));
                    s.gp[0]=s.flags.shift(s.gp[0],5,4);s.gp[0]=s.flags.plus(s.gp[0],read(s,0x24da2fu,s.gp[3]+0x11cu),4);s.xmm[0]=s.xmm[4];s.gp[3]=read(s,0x24da38u,s.gp[0]);store(s,0x24da3au,s.gp[4]+0x18u,4,s.gp[3]);
                    s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(read(s,0x24da3eu,s.gp[4]+0x18u))));s.gp[3]=read(s,0x24da44u,s.gp[0]+4u);s.gp[0]=read(s,0x24da47u,s.gp[0]+8u);s.xmm[6]=_mm_mul_ss(s.xmm[0],s.xmm[0]);store(s,0x24da51u,s.gp[4]+0x1cu,4,s.gp[3]);
                    s.xmm[1]=_mm_sub_ss(s.xmm[3],_mm_set_ss(f(read(s,0x24da58u,s.gp[4]+0x1cu))));s.xmm[0]=_mm_mul_ss(s.xmm[1],s.xmm[1]);s.xmm[6]=_mm_add_ss(s.xmm[6],s.xmm[0]);s.xmm[0]=_mm_mul_ss(s.xmm[2],s.xmm[2]);s.flags.scalar_compare(s.xmm[0],s.xmm[6],false);store(s,0x24da73u,s.gp[4]+0x20u,4,s.gp[0]);
                    inside=!s.flags.cf&&!s.flags.zf;
                    if(inside) {
                        s.xmm[0]=_mm_set_ss(f(read(s,0x24da79u,s.gp[4]+0x20u)));s.xmm[2]=_mm_set_ss(f(read(s,0x24da7fu,s.gp[1]+8u)));s.xmm[1]=_mm_sub_ss(s.xmm[5],s.xmm[0]);s.flags.scalar_compare(s.xmm[2],s.xmm[1],false);inside=!s.flags.cf&&!s.flags.zf;
                        if(inside) {s.xmm[1]=_mm_set_ss(f(read(s,0x24da90u,s.gp[1]+4u)));s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[5]);s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);inside=!s.flags.cf&&!s.flags.zf;}
                    }
                }
                if(inside) {store(s,0x24daaau,s.gp[4]+0x13u,1,1);break;}
                s.gp[2]=s.flags.increment(s.gp[2],4,false);s.flags.minus(s.gp[2],2,4);
            }
        }
        s.gp[3]=read(s,0x24daafu,s.gp[4]+0x28u);s.gp[6]=s.flags.increment(s.gp[6],4,false);s.gp[3]=s.flags.plus(s.gp[3],2,4);s.gp[7]=s.flags.plus(s.gp[7],2,4);s.gp[5]=s.flags.plus(s.gp[5],0xcu,4);s.flags.minus(s.gp[6],9,4);store(s,0x24dac0u,s.gp[4]+0x28u,4,s.gp[3]);
    }
    s.set(0,read(s,0x24dacau,s.gp[4]+0x13u,1),1,0);cursor(s,0x24daceu);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[3]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x14u,4);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[48];return true;
}
template<class State,class Dispatch>bool reconcile_home_positions(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0x1cu,4);push(s,0x24dae3u,s.gp[3]);push(s,0x24dae4u,s.gp[5]);push(s,0x24dae5u,s.gp[6]);push(s,0x24dae6u,s.gp[7]);call(s,dispatch,0x24b6e0u,0x24dae7u,0x24daecu);
    s.xmm[5]=_mm_set_ss(f(read(s,0x24daecu,0x46a9c0u)));store(s,0x24daf4u,s.gp[4]+0x13u,1,s.reg(0,1,0));s.gp[3]=0;s.flags.logic(0,4);
    for(unsigned team=0;team<9;++team) {
        call(s,dispatch,0x160e50u,0x24db00u,0x24db05u);s.gp[5]=s.gp[0];s.flags.logic(s.gp[5],4);bool check=s.gp[5]!=0;
        if(check) {const auto object=read(s,0x24db0fu,s.gp[5]);s.flags.minus(object,0xffffffffu,4);check=object!=0xffffffffu;}
        if(check) {s.gp[0]=read(s,0x24db19u,0x52d710u);const auto flags=read(s,0x24db1eu,s.gp[0]+s.gp[3]+0x170u,1);s.flags.logic(flags&1u,1);check=!(flags&1u);}
        if(check) {
            const auto carried=read(s,0x24db2cu,s.gp[5]+8u);s.flags.minus(carried,0xffffffffu,4);bool away=carried!=0xffffffffu;
            if(!away) {
                s.gp[0]=s.gp[3];call(s,dispatch,0x24b4c0u,0x24db38u,0x24db3du);s.gp[7]=s.gp[0];s.flags.minus(s.gp[7],0xffffffffu,4);check=s.gp[7]!=0xffffffffu;
                if(check) {
                    s.gp[0]=read(s,0x24db48u,s.gp[5]);s.gp[6]=s.gp[4]+0x20u;call(s,dispatch,0xbbe00u,0x24db4fu,0x24db54u);s.gp[0]=read(s,0x24db54u,0x4ecc20u);s.gp[1]=read(s,0x24db59u,s.gp[0]+0x11cu);s.gp[7]=s.flags.shift(s.gp[7],5,4);
                    s.gp[0]=read(s,0x24db62u,s.gp[1]+s.gp[7]+8u);s.gp[2]=read(s,0x24db66u,s.gp[1]+s.gp[7]+4u);s.gp[7]=s.flags.plus(s.gp[7],s.gp[1],4);s.gp[1]=read(s,0x24db6cu,s.gp[7]);store(s,0x24db6eu,s.gp[4]+0x1cu,4,s.gp[0]);
                    s.xmm[2]=_mm_set_ss(f(read(s,0x24db72u,s.gp[4]+0x1cu)));s.xmm[2]=_mm_sub_ss(s.xmm[2],_mm_set_ss(f(read(s,0x24db78u,s.gp[4]+0x28u))));s.xmm[3]=_mm_mul_ss(s.xmm[2],s.xmm[2]);store(s,0x24db85u,s.gp[4]+0x18u,4,s.gp[2]);
                    s.xmm[1]=_mm_set_ss(f(read(s,0x24db89u,s.gp[4]+0x18u)));s.xmm[1]=_mm_sub_ss(s.xmm[1],_mm_set_ss(f(read(s,0x24db8fu,s.gp[4]+0x24u))));s.xmm[2]=s.xmm[1];store(s,0x24db98u,s.gp[4]+0x14u,4,s.gp[1]);
                    s.xmm[0]=_mm_set_ss(f(read(s,0x24db9cu,s.gp[4]+0x14u)));s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(read(s,0x24dba2u,s.gp[4]+0x20u))));s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[1]);s.xmm[1]=_mm_mul_ss(s.xmm[0],s.xmm[0]);s.xmm[3]=_mm_add_ss(s.xmm[3],s.xmm[2]);s.xmm[3]=_mm_add_ss(s.xmm[3],s.xmm[1]);s.flags.scalar_compare(s.xmm[3],s.xmm[5],false);away=!s.flags.cf&&!s.flags.zf;
                    if(away)s.gp[0]=read(s,0x24dbc0u,0x52d710u);
                }
            }
            if(away) {
                const auto at=s.gp[0]+s.gp[3]+0x170u;store(s,0x24dbc5u,at,1,s.flags.logic(read(s,0x24dbc5u,at,1)|1u,1));
                s.gp[0]=read(s,0x24dbcdu,0x4f63b8u);s.gp[1]=read(s,0x24dbd2u,s.gp[0]+0xc14u);s.gp[2]=read(s,0x24dbd8u,0x56dfa0u+s.gp[1]*4u);s.flags.logic(s.gp[2],4);
                if(s.gp[2]) {s.gp[0]=read(s,0x24dbe3u,s.gp[0]+0x24u);s.flags.minus(s.gp[0],0xffffffffu,4);if(s.gp[0]!=0xffffffffu) {push(s,0x24dbebu,0x200u);push(s,0x24dbf0u,s.gp[0]);call(s,dispatch,0xb7e50u,0x24dbf1u,0x24dbf6u);}}
                store(s,0x24dbf6u,s.gp[4]+0x13u,1,0);
            }
        }
        s.gp[3]=s.flags.increment(s.gp[3],4,false);s.flags.minus(s.gp[3],9,4);
    }
    s.set(0,read(s,0x24dc05u,s.gp[4]+0x13u,1),1,0);cursor(s,0x24dc09u);s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[3]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x1cu,4);s.pop();++source_objective_native_tu15_stats.owners[49];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
