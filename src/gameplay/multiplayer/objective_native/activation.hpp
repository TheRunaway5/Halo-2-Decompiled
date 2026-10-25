#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool activate(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x249590u,0x4f3218u);const auto mode=read(s,0x249595u,s.gp[0]+0xcu,1);
    s.flags.minus(mode,4,1);
    if(mode!=4) {
        const auto kind=read(s,0x24959bu,s.gp[0]+0x240u);s.flags.minus(kind,1,4);
        if(kind==1) {
            // Rotation chooses the actual first active side from the roster.
            s.gp[1]=0xffffffffu;s.flags.logic(s.gp[1],4);call(s,dispatch,0x164930u,0x2495a7u,0x2495acu);
            s.gp[1]=read(s,0x2495acu,0x52d710u);store(s,0x2495b2u,s.gp[1]+0x1f4u,4,s.gp[0]);
            s.gp[0]=read(s,0x2495b8u,0x4f63b8u);s.gp[2]=read(s,0x2495bdu,s.gp[0]+0xc14u);
            s.gp[1]=read(s,0x2495c3u,0x56dfa0u+s.gp[2]*4u);s.flags.logic(s.gp[1],4);
            if(s.gp[1]) {
                s.gp[0]=read(s,0x2495ceu,s.gp[0]+0x24u);s.flags.minus(s.gp[0],0xffffffffu,4);
                if(s.gp[0]!=0xffffffffu) {
                    push(s,0x2495d6u,0x20u);push(s,0x2495d8u,s.gp[0]);call(s,dispatch,0xb7e50u,0x2495d9u,0x2495deu);
                }
            }
        }
        push(s,0x2495deu,s.gp[6]);s.gp[6]=0;s.flags.logic(0,4);
        for(unsigned team=0;team<9;++team) {
            call(s,dispatch,0x24cd70u,0x2495e1u,0x2495e6u);s.flags.logic(s.gp[0]&255u,1);
            if(s.gp[0]&255u) {push(s,0x2495eau,s.gp[6]);call(s,dispatch,0x24b5c0u,0x2495ebu,0x2495f0u);}
            ++s.gp[6];s.flags.minus(s.gp[6],9,4);
        }
        cursor(s,0x2495f6u);s.gp[6]=s.pop();
    }
    cursor(s,0x2495f7u);s.pop();++source_objective_native_tu15_stats.owners[11];return true;
}
template<class State,class Dispatch>bool update_object_color(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24a2e0u,s.gp[4]+4u);call(s,dispatch,0x24cd10u,0x24a2e4u,0x24a2e9u);
    cursor(s,0x24a2e9u);s.pop();s.gp[4]+=4u;++source_objective_native_tu15_stats.owners[12];return true;
}
template<class State,class Dispatch>bool validate_objectives(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    cursor(s,0x24c670u);dispatch(s,0x24dae0u,0x24c670u,true);
    ++source_objective_native_tu15_stats.owners[13];return true;
}
template<class State,class Dispatch>bool set_selected_team(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24c5d0u,0x4f3218u);auto mode=read(s,0x24c5d5u,s.gp[0]+0xcu,1);s.flags.minus(mode,4,1);
    if(mode!=4)call(s,dispatch,0x160f10u,0x24c5dbu,0x24c5e0u);
    s.gp[1]=read(s,0x24c5e0u,s.gp[4]+4u);s.gp[0]=read(s,0x24c5e4u,0x4f63b8u);
    push(s,0x24c5e9u,s.gp[6]);s.gp[6]=read(s,0x24c5eau,0x52d710u);store(s,0x24c5f0u,s.gp[6]+0x1f4u,4,s.gp[1]);
    s.gp[2]=read(s,0x24c5f6u,s.gp[0]+0xc14u);s.gp[1]=read(s,0x24c5fcu,0x56dfa0u+s.gp[2]*4u);s.flags.logic(s.gp[1],4);
    if(s.gp[1]) {
        s.gp[0]=read(s,0x24c607u,s.gp[0]+0x24u);s.flags.minus(s.gp[0],0xffffffffu,4);
        if(s.gp[0]!=0xffffffffu) {
            push(s,0x24c60fu,0x20u);push(s,0x24c611u,s.gp[0]);call(s,dispatch,0xb7e50u,0x24c612u,0x24c617u);
        }
    }
    s.gp[0]=read(s,0x24c617u,0x4f3218u);mode=read(s,0x24c61cu,s.gp[0]+0xcu,1);s.flags.minus(mode,4,1);
    if(mode!=4) {
        call(s,dispatch,0x15a1a0u,0x24c622u,0x24c627u);push(s,0x24c627u,0xffffffffu);
        call(s,dispatch,0x24c540u,0x24c629u,0x24c62eu);s.gp[6]=0;s.flags.logic(0,4);
        for(unsigned team=0;team<9;++team) {
            call(s,dispatch,0x24cd70u,0x24c630u,0x24c635u);s.flags.logic(s.gp[0]&255u,1);
            if(s.gp[0]&255u) {push(s,0x24c639u,s.gp[6]);call(s,dispatch,0x24b5c0u,0x24c63au,0x24c63fu);}
            ++s.gp[6];s.flags.minus(s.gp[6],9,4);
        }
        s.gp[1]=read(s,0x24c645u,0x52d710u);store(s,0x24c64bu,s.gp[1]+0x200u,4,0);
        cursor(s,0x24c655u);s.gp[6]=s.pop();cursor(s,0x24c656u);s.pop();s.gp[4]+=4u;
    } else {
        store(s,0x24c659u,s.gp[6]+0x200u,4,0);cursor(s,0x24c663u);s.gp[6]=s.pop();
        cursor(s,0x24c664u);s.pop();s.gp[4]+=4u;
    }
    ++source_objective_native_tu15_stats.owners[17];return true;
}
template<class State,class Dispatch>bool rotate_team(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    const auto request=read(s,0x24b2b0u,s.gp[4]+4u);s.flags.minus(request,1,4);
    if(request!=1) {cursor(s,0x24b2e0u);s.pop();s.gp[4]+=4u;}
    else {
        s.gp[0]=read(s,0x24b2b7u,0x52d710u);push(s,0x24b2bcu,s.gp[6]);s.gp[6]=read(s,0x24b2bdu,s.gp[0]+0x1f4u);
        s.gp[1]=s.gp[6];call(s,dispatch,0x164930u,0x24b2c5u,0x24b2cau);s.flags.minus(s.gp[6],0xffffffffu,4);
        if(s.gp[6]!=0xffffffffu)s.flags.minus(s.gp[0],0xffffffffu,4);
        s.gp[0]=std::uint32_t(source_objective_rotated_team(std::bit_cast<std::int32_t>(s.gp[6]),std::bit_cast<std::int32_t>(s.gp[0])));
        cursor(s,0x24b2d6u);s.gp[6]=s.pop();store(s,0x24b2d7u,s.gp[4]+4u,4,s.gp[0]);
        cursor(s,0x24b2dbu);dispatch(s,0x24c5d0u,0x24b2dbu,true);
    }
    ++source_objective_native_tu15_stats.owners[18];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
