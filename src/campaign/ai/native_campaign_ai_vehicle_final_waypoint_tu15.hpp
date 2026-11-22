#pragma once

#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::vehicle_movement_tu15 {

using State=physics_world_runtime_detail::WorldState;
using physics_world_runtime_detail::r;
using physics_world_runtime_detail::w;

// Vehicle path has consumed its final waypoint, with neither direction hint
// active. The original returns two status bytes but leaves the vector alone.
// Preserve the caller's frame, comparisons and the genuine radius child.
template<class Radius>
bool final_waypoint(State& state,Radius radius) {
    auto& g=state.gp;
    const auto first=g[4];
    const auto view=[&](std::uint32_t address,unsigned width) {
        return state.memory.try_read_view(address,width);
    };
    const auto word=[](std::span<const std::byte> bytes) {
        std::uint32_t value{};
        std::memcpy(&value,bytes.data(),4);
        return value;
    };
    const auto input=[&](unsigned offset) {return view(first+offset,4);};
    const auto caller=input(0),flags_out=input(12),direction_out=input(16),vector_out=input(20);
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15 || state.active_mmx || state.fp.count ||
       !caller || word(*caller)!=0x2a40dau || !flags_out || word(*flags_out)!=first+0x2fu ||
       !direction_out || word(*direction_out)!=first+0x26u || !vector_out || word(*vector_out)!=first+0x60u)
        return false;
    const auto pool=view(0x5020c8u,4);
    if(!pool)return false;
    const auto rows=view(word(*pool)+0x44u,4);
    if(!rows)return false;
    const auto actor_row=word(*rows)+(g[7]&0xffffu)*0x888u;
    const auto blocked=view(actor_row+0x456u,1),direction=view(actor_row+0x4d4u,2),path=view(actor_row+0x539u,2);
    const auto signed_byte=[](std::byte b){return std::bit_cast<std::int8_t>(b);};
    if(!blocked || (*blocked)[0]!=std::byte{} || !direction || (*direction)[0]!=std::byte{} ||
       (*direction)[1]!=std::byte{} || !path || int(signed_byte((*path)[1]))+1<int(signed_byte((*path)[0])))
        return false;

    const auto load=[&](std::uint32_t address,unsigned width=4) {return r(state.memory,address,width);};
    const auto flag=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        w(state.memory,state.observe,pc,address,1,value);
    };
    g[4]-=0x1cu;
    const auto actor_rows=load(load(0x5020c8u)+0x44u);
    state.push(0x201a2cu,g[3]);state.push(0x201a2du,g[5]);state.push(0x201a2eu,g[6]);
    const auto frame=g[4];
    g[6]=actor_rows+(g[7]&0xffffu)*0x888u;
    const auto object_rows=load(load(0x4ecbd0u)+0x44u);
    const auto definition=load(load(object_rows+(g[0]&0xffffu)*12u+8u));
    const auto tags=load(0x4f0414u);
    const int waypoint_count=std::bit_cast<std::int8_t>(std::uint8_t(load(g[6]+0x539u,1)));
    g[5]=load(tags+(definition&0xffffu)*16u+8u);
    const int current_waypoint=std::bit_cast<std::int8_t>(std::uint8_t(load(g[6]+0x53au,1)));
    state.set(1,load(g[6]+0x456u,1),1,0);
    state.set(3,0,1,0);
    g[0]=(std::uint32_t(current_waypoint+1)&0xffffff00u)|unsigned(current_waypoint+1>=waypoint_count);
    flag(0x201a85u,frame+0xeu,0);
    flag(0x201a89u,frame+0xfu,g[0]&0xffu);
    flag(0x201a8du,frame+0xdu,0);
    state.flags.minus(load(g[6]+0x4d5u,1),0,1);
    state.flags.minus(load(g[6]+0x4d4u,1),0,1);
    g[1]=g[7];
    state.push(0x201b47u,0x201b4cu);
    radius(state);
    state.flags.scalar_compare(state.xmm[0],_mm_set_ss(physics_world_runtime_detail::f(load(frame+0x30u))),false);
    if(!state.flags.cf && !state.flags.zf)flag(0x201bf1u,frame+0xeu,1);
    else state.flags.minus(load(frame+0xdu,1),g[3]&0xffu,1);
    const auto exhausted=load(frame+0xfu,1);
    g[2]=load(frame+0x34u);
    flag(0x201c00u,g[2],exhausted);
    (void)load(g[6]+0x4d5u,1);
    g[0]=load(frame+0x38u);
    g[6]=state.pop();g[5]=state.pop();
    flag(0x201c14u,g[0],0);
    state.set(0,load(g[4]+6u,1),1,0);
    g[3]=state.pop();
    g[4]=state.flags.plus(g[4],0x1cu,4);
    state.pop();g[4]+=0x14u;
    return true;
}
}
