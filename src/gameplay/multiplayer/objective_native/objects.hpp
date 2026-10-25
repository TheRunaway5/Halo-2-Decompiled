#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool object_color(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[1]=read(s,0x24cd10u,0x4ecbd0u);s.gp[2]=read(s,0x24cd16u,s.gp[1]+0x44u);
    s.gp[4]=s.flags.minus(s.gp[4],0x18u,4);push(s,0x24cd1cu,s.gp[6]);s.gp[6]=s.gp[0];
    s.gp[0]=(s.gp[0]&65535u)*3u;s.flags.logic(s.gp[6]&65535u,4);
    s.gp[0]=read(s,0x24cd27u,s.gp[2]+8u+s.gp[0]*4u);
    s.gp[0]=(s.gp[0]&0xffff0000u)|read(s,0x24cd2bu,s.gp[0]+0x17eu,2);s.flags.minus(s.gp[0]&65535u,65535u,2);
    if(std::uint16_t(s.gp[0])!=65535u) {
        s.gp[0]=std::uint32_t(signed_half(s.gp[0]));push(s,0x24cd3bu,s.gp[7]);push(s,0x24cd3cu,s.gp[0]);
        s.gp[0]=s.gp[4]+0x18u;call(s,dispatch,0x83050u,0x24cd41u,0x24cd46u);
        s.gp[1]=read(s,0x24cd46u,s.gp[0]);s.gp[2]=read(s,0x24cd48u,s.gp[0]+4u);s.gp[0]=read(s,0x24cd4bu,s.gp[0]+8u);
        push(s,0x24cd4eu,s.gp[6]);s.gp[7]=s.gp[4]+0xcu;s.gp[6]=0;s.flags.logic(0,4);
        store(s,0x24cd55u,s.gp[4]+0xcu,4,s.gp[1]);store(s,0x24cd59u,s.gp[4]+0x10u,4,s.gp[2]);
        store(s,0x24cd5du,s.gp[4]+0x14u,4,s.gp[0]);call(s,dispatch,0xbccf0u,0x24cd61u,0x24cd66u);
        cursor(s,0x24cd66u);s.gp[7]=s.pop();
    }
    cursor(s,0x24cd67u);s.gp[6]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0x18u,4);
    cursor(s,0x24cd6bu);s.pop();++source_objective_native_tu15_stats.owners[14];return true;
}
template<class State,class Dispatch>bool spawn_objective(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[0]=read(s,0x24b500u,0x4f3218u);s.gp[1]=read(s,0x24b505u,s.gp[0]+0x180u);
    const auto assault=s.gp[1]==9;s.gp[4]=s.flags.minus(s.gp[4],0xc4u,4);s.gp[0]=0xffffffffu;
    s.flags.minus(s.gp[1],9,4);s.gp[1]=read(s,0x24b517u,0x4f63b8u);s.gp[2]=read(s,0x24b51du,s.gp[1]+0xc14u);
    s.gp[1]=read(s,0x24b523u,0x56dfa0u+s.gp[2]*4u);s.flags.logic(s.gp[1],4);
    if(s.gp[1]) {
        // Select the authored bomb/flag tag, then let the real factory own its
        // request, datum generation and object publication.
        s.gp[0]=read(s,assault?0x24b530u:0x24b55au,0x4ecc1cu);
        s.gp[1]=read(s,assault?0x24b535u:0x24b55fu,s.gp[0]+0x16cu);
        s.gp[2]=read(s,assault?0x24b53bu:0x24b565u,0x4f0414u);
        s.gp[1]&=65535u;s.gp[1]=s.flags.shift(s.gp[1],4,4);
        s.gp[0]=read(s,assault?0x24b54au:0x24b574u,s.gp[2]+8u+s.gp[1]);
        s.gp[0]=read(s,assault?0x24b54eu:0x24b578u,s.gp[0]+0xcu);
        s.gp[0]=read(s,assault?0x24b551u:0x24b57bu,s.gp[0]+(assault?0x3cu:4u));
    }
    push(s,0x24b57eu,s.gp[6]);push(s,0x24b57fu,0);push(s,0x24b581u,0xffffffffu);push(s,0x24b583u,s.gp[0]);
    s.gp[6]=s.gp[4]+0x10u;call(s,dispatch,0xb9960u,0x24b588u,0x24b58du);
    s.gp[1]=read(s,0x24b58du,s.gp[7]);s.gp[2]=read(s,0x24b58fu,s.gp[7]+4u);s.gp[0]=read(s,0x24b592u,s.gp[7]+8u);
    store(s,0x24b595u,s.gp[4]+0x20u,4,s.gp[1]);s.gp[1]=s.gp[6];push(s,0x24b59bu,s.gp[1]);
    store(s,0x24b59cu,s.gp[4]+0x28u,4,s.gp[2]);store(s,0x24b5a0u,s.gp[4]+0x2cu,4,s.gp[0]);
    call(s,dispatch,0xb9b70u,0x24b5a4u,0x24b5a9u);s.gp[6]=s.gp[0];s.flags.minus(s.gp[6],0xffffffffu,4);
    if(s.gp[6]!=0xffffffffu) {push(s,0x24b5b0u,s.gp[6]);call(s,dispatch,0xb1a70u,0x24b5b1u,0x24b5b6u);}
    s.gp[0]=s.gp[6];cursor(s,0x24b5b8u);s.gp[6]=s.pop();s.gp[4]=s.flags.plus(s.gp[4],0xc4u,4);
    cursor(s,0x24b5bfu);s.pop();++source_objective_native_tu15_stats.owners[15];return true;
}
template<class State,class Dispatch>bool reset_team(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    s.gp[4]=s.flags.minus(s.gp[4],0xcu,4);push(s,0x24b5c3u,s.gp[5]);s.gp[5]=read(s,0x24b5c4u,s.gp[4]+0x14u);
    push(s,0x24b5c8u,s.gp[6]);s.gp[6]=read(s,0x24b5c9u,0x4f63b8u);push(s,0x24b5cfu,s.gp[7]);
    s.gp[7]=read(s,0x24b5d0u,0x52d710u);store(s,0x24b5d6u,s.gp[7]+0x1a0u+s.gp[5]*2u,2,65535);
    const auto dirty=[&](unsigned stage) {
        const std::array<std::uint32_t,3> selector_pc{0x24b5e0u,0x24b618u,0x24b644u};
        const std::array<std::uint32_t,3> object_pc{0x24b5e6u,0x24b61eu,0x24b64au};
        const std::array<std::uint32_t,3> entity_pc{0x24b5f1u,0x24b629u,0x24b655u};
        const std::array<std::uint32_t,3> mask_pc{0x24b5f9u,0x24b631u,0x24b65du};
        const std::array<std::uint32_t,3> argument_pc{0x24b5feu,0x24b636u,0x24b662u};
        const std::array<std::uint32_t,3> call_pc{0x24b5ffu,0x24b637u,0x24b663u};
        const std::array<std::uint32_t,3> rejoin{0x24b604u,0x24b63cu,0x24b668u};
        const auto selector_register=stage==0?0u:stage==1?1u:2u;
        s.gp[selector_register]=read(s,selector_pc[stage],s.gp[6]+0xc14u);
        const auto pointer_register=stage==0?1u:0u;
        s.gp[pointer_register]=read(s,object_pc[stage],0x56dfa0u+s.gp[selector_register]*4u);
        s.flags.logic(s.gp[pointer_register],4);
        if(s.gp[pointer_register]) {
            const auto entity_register=stage==2?6u:0u;
            s.gp[entity_register]=read(s,entity_pc[stage],s.gp[6]+0x24u);s.flags.minus(s.gp[entity_register],0xffffffffu,4);
            if(s.gp[entity_register]!=0xffffffffu) {
                push(s,mask_pc[stage],stage==0?0x100u:stage==1?0x80u:0x200u);push(s,argument_pc[stage],s.gp[entity_register]);
                call(s,dispatch,0xb7e50u,call_pc[stage],rejoin[stage]);
            }
        }
    };
    dirty(0);store(s,0x24b604u,s.gp[7]+0x114u+s.gp[5]*2u,2,65535);
    store(s,0x24b60eu,s.gp[7]+0x126u+s.gp[5]*2u,2,0);dirty(1);
    store(s,0x24b63cu,s.gp[7]+0x170u+s.gp[5],1,0);dirty(2);
    const auto team=std::bit_cast<std::int32_t>(s.gp[5]);s.flags.minus(s.gp[5],8,4);
    bool marker_available=true;
    if(team==8)s.gp[0]=read(s,0x24b66du,s.gp[7]+0x38u);
    else {
        s.flags.logic(s.gp[5],4);if(team>=0)s.flags.minus(s.gp[5],8,4);
        marker_available=team>=0 && team<8;
        if(marker_available) {
            push(s,0x24b67bu,s.gp[5]);call(s,dispatch,0x15b3b0u,0x24b67cu,0x24b681u);
            s.gp[0]=std::uint32_t(signed_half(s.gp[0]));s.flags.minus(s.gp[0],0xffffffffu,4);
            marker_available=s.gp[0]!=0xffffffffu;
            if(marker_available)s.gp[0]=std::uint32_t(signed_half(read(s,0x24b689u,s.gp[7]+2u+s.gp[0]*2u,2)));
        }
    }
    if(marker_available) {
        s.flags.minus(s.gp[0],0xffffffffu,4);marker_available=s.gp[0]!=0xffffffffu;
        if(marker_available) {
            s.gp[1]=read(s,0x24b693u,0x4ecc20u);s.gp[2]=read(s,0x24b699u,s.gp[1]+0x11cu);
            s.gp[0]=s.flags.shift(s.gp[0],5,4);s.gp[1]=read(s,0x24b6a2u,s.gp[2]+s.gp[0]+4u);
            s.gp[0]=s.flags.plus(s.gp[0],s.gp[2],4);s.gp[2]=read(s,0x24b6a8u,s.gp[0]);
            store(s,0x24b6aau,s.gp[4]+0xcu,4,s.gp[2]);s.gp[2]=read(s,0x24b6aeu,s.gp[0]+8u);s.gp[7]=s.gp[4]+0xcu;
            store(s,0x24b6b5u,s.gp[4]+0x10u,4,s.gp[1]);store(s,0x24b6b9u,s.gp[4]+0x14u,4,s.gp[2]);
            call(s,dispatch,0x24b500u,0x24b6bdu,0x24b6c2u);s.flags.minus(s.gp[0],0xffffffffu,4);
            if(s.gp[0]!=0xffffffffu) {
                push(s,0x24b6c7u,s.gp[5]);push(s,0x24b6c8u,s.gp[0]);call(s,dispatch,0x160a90u,0x24b6c9u,0x24b6ceu);
            }
        }
    }
    cursor(s,0x24b6ceu);s.gp[7]=s.pop();cursor(s,0x24b6cfu);s.gp[6]=s.pop();cursor(s,0x24b6d0u);s.gp[5]=s.pop();
    s.gp[4]=s.flags.plus(s.gp[4],0xcu,4);cursor(s,0x24b6d4u);s.pop();s.gp[4]+=4u;
    ++source_objective_native_tu15_stats.owners[16];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
