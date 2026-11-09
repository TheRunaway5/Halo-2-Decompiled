#pragma once
#include "rendering/models/native_tu15_model_light_source.hpp"
#include "rendering/models/native_tu15_model_color.hpp"

namespace halo2::model_color_publish_detail {
using namespace prt_projection_detail;
enum class FlagOperation { logic,compare,add };
struct Flags { FlagOperation operation;std::uint32_t left,right;unsigned width=4; };
struct Caller {
    std::uint32_t eax,ecx,edx,bx,stack,bp,si,di;
    __m128 first;std::uint16_t condition;Flags flags;
};
using Boundary=std::function<void(std::uint32_t,const Caller&)>;
// Complete3DD00 lighting RGB publication. Both children are typed native
// kernels. Frame words and every per-channel argument remain live memory,
// including the original call/argument stores and aliased saved-register pops.
inline void publish(SourcePhysicsContactMemory& memory,Caller& out,
    const SourcePhysicsContactRecordObserver& observe,const Boundary& boundary={}) {
    const auto get=[&](std::uint32_t at,unsigned width=4){return read(memory,at,width);};
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned width,std::uint32_t pc){store(memory,at,value,width,pc,observe);};
    const auto push=[&](std::uint32_t value,std::uint32_t pc){out.stack-=4u;put(out.stack,value,4,pc);};
    const auto pop=[&]{const auto value=get(out.stack);out.stack+=4u;return value;};
    const auto observe_child=[&](std::uint32_t entry){if(boundary)boundary(entry,out);};
    out.eax=get(out.stack+4u);out.ecx=get(0x4ecbd0u);out.edx=get(out.ecx+0x44u);
    out.eax=(out.eax&65535u)*3u;out.ecx=get(out.edx+out.eax*4u+8u);out.eax=get(out.ecx+0xccu);
    out.stack-=12u;
    bool enabled=out.eax!=0xffffffffu;
    if(enabled) {
        out.edx=(out.edx&0xffffff00u)|get(out.stack+0x14u,1);
        if(!(out.edx&255u)) {
            out.ecx=get(out.ecx);out.edx=get(0x4f0414u);
            out.ecx=(out.ecx&65535u)<<4u;out.ecx=get(out.edx+out.ecx+8u);
            enabled=(get(out.ecx+0x1cu,1)&1u)!=0;
        }
    }
    if(enabled) {
        out.edx=get(0x515f14u);out.ecx=get(out.edx+0x44u);
        push(out.bx,0x3dd57u);out.eax&=65535u;push(out.bp,0x3dd5du);
        out.eax=(out.eax<<8u)+out.ecx;push(out.si,0x3dd63u);out.bx=out.eax;push(out.di,0x3dd66u);
        put(out.bx+0xb8u,0,1,0x3dd67u);out.di=0;out.bp=out.bx+0xa8u;
        out.flags={FlagOperation::logic,0,0,4};
        do {
            out.eax=get(out.stack+0x20u);out.si=out.stack+0x10u;
            push(0x3dd8du,0x3dd88u);observe_child(0xbcd80u);
            const auto channel=model_light_source_detail::color_channel(memory,out.eax,out.si,out.di,out.stack,out.bx,observe);
            out.eax=channel.value;out.ecx=channel.next_component;out.edx=channel.first_component;out.bx=channel.saved_bx;
            (void)pop();out.flags={FlagOperation::logic,out.eax&255u,0,1};
            if(out.eax&255u) {
                out.eax=out.si;push(out.eax,0x3dd93u);push(0x3dd99u,0x3dd94u);observe_child(0x1349e0u);
                const auto packed=model_color_detail::pack(memory,out.stack,observe);
                out.eax=packed.value;out.edx=packed.second;out.first=scalar(packed.third);
                out.condition=packed.status&0x200u;(void)pop();
                put(out.bp,out.eax,4,0x3dd99u);
                out.eax=(out.eax&0xffffff00u)|get(out.bx+0xb8u,1);out.stack+=4u;
                out.eax=(out.eax&0xffffff00u)|((out.eax+1u)&255u);
                put(out.bx+0xb8u,out.eax&255u,1,0x3dda7u);
            }
            ++out.di;out.bp+=4u;out.flags={FlagOperation::compare,out.di,4,4};
        } while(std::bit_cast<std::int32_t>(out.di)<4);
        out.di=pop();out.si=pop();out.bp=pop();out.bx=pop();
    }
    out.flags={FlagOperation::add,out.stack,12,4};out.stack+=12u;(void)pop();out.stack+=8u;
}
template<class State>void install(State& state,const Caller& out) {
    state.gp={out.eax,out.ecx,out.edx,out.bx,out.stack,out.bp,out.si,out.di};state.xmm[0]=out.first;
    state.fp.status=(state.fp.status&~0x200u)|out.condition;
    switch(out.flags.operation) {
    case FlagOperation::logic:state.flags.logic(out.flags.left,out.flags.width);break;
    case FlagOperation::compare:state.flags.minus(out.flags.left,out.flags.right,out.flags.width);break;
    case FlagOperation::add:state.flags.plus(out.flags.left,out.flags.right,out.flags.width);break;
    }
}
template<class State>void bridge(State& state,const std::function<void(std::uint32_t)>& trace={}) {
    Caller out{state.gp[0],state.gp[1],state.gp[2],state.gp[3],state.gp[4],state.gp[5],state.gp[6],state.gp[7],
        state.xmm[0],std::uint16_t(state.fp.status&0x200u),{FlagOperation::logic,0,0,4}};
    Boundary boundary;
    if(trace)boundary=[&](std::uint32_t entry,const Caller& call){install(state,call);trace(entry);};
    publish(state.memory,out,state.observe,boundary);install(state,out);
}
}
