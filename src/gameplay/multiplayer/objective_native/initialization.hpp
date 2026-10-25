#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool initialize(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x249460u,s.gp[1]);push(s,0x249461u,s.gp[3]);
    push(s,0x249462u,s.gp[5]);push(s,0x249463u,s.gp[6]);
    s.gp[6]=read(s,0x249464u,0x4f63b8u)+0xfcu;
    s.gp[0]=0;s.flags.logic(0,4);push(s,0x249472u,s.gp[7]);
    s.gp[1]=0x82u;s.gp[7]=s.gp[6];
    // Clear the complete shared 0x208-byte bank before publishing its pointer.
    // Retain the caller's direction flag and each original DWORD publication.
    for(;s.gp[1];--s.gp[1]) {
        store(s,0x24947au,s.gp[7],4,0);
        s.gp[7]+=(s.other_flags&1024u)?0xfffffffcu:4u;
    }
    s.gp[5]=s.gp[0]=0xffffffffu;s.flags.logic(s.gp[5],4);
    for(unsigned n=0;n<4;++n)store(s,0x249481u+n*3u,s.gp[6]+0x26u+n*4u,4,s.gp[0]);
    store(s,0x24948du,0x52d710u,4,s.gp[6]);store(s,0x249493u,s.gp[6]+0x36u,2,s.gp[0]);
    s.gp[7]=0;s.flags.logic(0,4);s.gp[3]=s.gp[6]+0x14u;
    // Each authored team has a spawn, primary goal, optional secondary goal,
    // and two capture regions. Queries retain their own marker/float owner.
    for(unsigned team=0;team<9;++team) {
        push(s,0x2494a0u,0);s.gp[0]=(s.gp[0]&0xffffff00u)|1u;s.gp[2]=s.gp[7];
        store(s,0x2494a6u,s.gp[4]+0x14u,4,s.gp[5]);
        call(s,dispatch,0x24ce00u,0x2494aau,0x2494afu);
        s.gp[1]=s.gp[4]+0x10u;store(s,0x2494b3u,s.gp[3]-0x12u,2,s.gp[0]);
        push(s,0x2494b7u,s.gp[1]);s.gp[0]&=0xffffff00u;s.flags.logic(0,1);s.gp[2]=s.gp[7];
        call(s,dispatch,0x24ce00u,0x2494bcu,0x2494c1u);
        s.gp[2]=(s.gp[2]&0xffff0000u)|read(s,0x2494c1u,s.gp[4]+0x10u,2);
        store(s,0x2494c6u,s.gp[3],2,s.gp[0]);store(s,0x2494c9u,s.gp[3]+0x12u,2,s.gp[2]);
        call(s,dispatch,0x24cef0u,0x2494cdu,0x2494d2u);
        ++s.gp[7];s.gp[3]+=2u;s.flags.minus(s.gp[7],9,4);
    }
    push(s,0x2494dbu,0);s.gp[0]=(s.gp[0]&0xffffff00u)|1u;s.gp[2]=s.gp[5];
    call(s,dispatch,0x24ce00u,0x2494e1u,0x2494e6u);
    store(s,0x2494e6u,s.gp[6]+0x38u,4,s.gp[0]);store(s,0x2494e9u,s.gp[6],1,1);
    s.gp[0]=s.gp[5];
    for(unsigned n=0;n<4;++n)store(s,0x2494eeu+n*6u,s.gp[6]+0x114u+n*4u,4,s.gp[0]);
    store(s,0x249506u,s.gp[6]+0x124u,2,s.gp[0]);
    for(unsigned n=0;n<9;++n)store(s,0x24950du+n*6u,s.gp[6]+0x14cu+n*4u,4,s.gp[5]);
    cursor(s,0x249543u);s.gp[7]=s.pop();
    for(unsigned n=0;n<9;++n)store(s,0x249544u+n*6u,s.gp[6]+0x17cu+n*4u,4,s.gp[5]);
    cursor(s,0x24957au);s.gp[6]=s.pop();cursor(s,0x24957bu);s.gp[5]=s.pop();
    s.gp[0]=(s.gp[0]&0xffffff00u)|1u;
    cursor(s,0x24957eu);s.gp[3]=s.pop();cursor(s,0x24957fu);s.gp[1]=s.pop();
    cursor(s,0x249580u);s.pop();++source_objective_native_tu15_stats.owners[7];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
