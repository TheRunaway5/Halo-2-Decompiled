#pragma once
#include "rendering/models/native_tu15_prt_projection.hpp"

namespace halo2::model_light_source_detail {
using namespace prt_projection_detail;
// Only the final caller flag operation is transported; source traversal and
// channel selection use native predicates, not per-instruction flag updates.
struct Tail {
    bool addition{};std::uint32_t left{},right{};unsigned width=4;
};
struct Channel {
    std::uint32_t eax,edx;Tail tail;
    bool loaded_simd{};__m128 z{},y{};
};
// Complete1D8C80/1D8D80: select one authored lighting channel or copy the real
// default channel. Source loads retain the asymmetric cached/interleaved order.
inline Channel channel(SourcePhysicsContactMemory& memory,std::uint32_t object,
    std::uint32_t index,std::uint32_t output,bool second,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto rows=read(memory,object+0x70u);
    const auto descriptor=read(memory,rows+index*96u+0x40u);
    const auto flag=read(memory,descriptor+0x40u,1);
    const auto base=second?0x1d8d80u:0x1d8c80u;
    const auto put=[&](unsigned offset,std::uint32_t word,unsigned site){store(memory,output+offset,word,4,base+site,observe);};
    Channel out{};
    if(!flag) {
        const auto data=read(memory,descriptor+0x3cu),field=second?0x50u:0x40u;
        const auto z=read(memory,data+field+8u),y=read(memory,data+field+4u);
        out.eax=read(memory,data+field);out.edx=rows&0xffffff00u;
        out.tail={true,data,field,4};out.loaded_simd=true;out.z=scalar(z);out.y=scalar(y);
        put(0,out.eax,0x26u);put(4,y,0x28u);put(8,z,0x2du);
    } else {
        out.edx=read(memory,0x474d04u);
        out.eax=read(memory,out.edx);put(0,out.eax,0x3bu);
        out.eax=read(memory,out.edx+4u);put(4,out.eax,0x40u);
        out.edx=read(memory,out.edx+8u);put(8,out.edx,0x46u);
        out.tail={false,flag,0,1};
    }
    return out;
}
struct Source {
    std::uint32_t eax,ecx,edx,bx,stack,bp,si,di;
    __m128 first,second;Tail tail;
};
// CompleteBC200 object lighting source selection. Hierarchy walking, scenario
// ownership, defaults, and both channel transactions are native. Live stack
// words stay memory inputs because outputs can overwrite later arguments/pops.
inline void select(SourcePhysicsContactMemory& memory,Source& out,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto get=[&](std::uint32_t at,unsigned width=4){return read(memory,at,width);};
    const auto put=[&](std::uint32_t at,std::uint32_t value,std::uint32_t pc){store(memory,at,value,4,pc,observe);};
    const auto push=[&](std::uint32_t word,std::uint32_t pc){out.stack-=4u;put(out.stack,word,pc);};
    const auto pop=[&]{const auto word=get(out.stack);out.stack+=4u;return word;};
    const auto finish=[&]{out.di=pop();out.si=pop();out.bx=pop();(void)pop();out.stack+=4u;};
    const auto low=[&](std::uint32_t old,std::uint32_t byte){return (old&0xffffff00u)|(byte&255u);};
    const auto signed_half=[](std::uint32_t word){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word))));};
    const auto copy_channel=[&](bool second,std::uint32_t site,std::uint32_t next){
        out.eax=out.di;out.edx=out.si;push(next,site);
        const auto result=channel(memory,out.edx,out.eax,out.ecx,second,observe);
        out.eax=result.eax;out.edx=result.edx;out.tail=result.tail;
        if(result.loaded_simd){out.first=result.z;out.second=result.y;}
        (void)pop();
    };
    push(out.bx,0xbc200u);push(out.si,0xbc201u);out.edx=0xffffffffu;
    push(out.di,0xbc208u);out.di=get(0x4ecbd0u);
    if(out.eax!=0xffffffffu) {
        out.si=get(out.di+0x44u);
        do {
            out.edx=out.eax;out.eax&=65535u;out.eax*=3u;
            out.eax=get(out.si+out.eax*4u+8u);out.eax=get(out.eax+0x14u);
        } while(out.eax!=0xffffffffu);
    }
    out.eax=get(out.di+0x44u);out.edx=(out.edx&65535u)*3u;
    out.edx=get(out.eax+out.edx*4u+8u);out.eax=get(out.edx+0xb4u);
    bool selected=false;
    if(out.eax!=0xffffffffu) {
        out.eax&=65535u;out.si=out.eax*5u;
        out.eax=get(0x52be5cu);out.bx=get(out.eax+0x44u);out.si<<=5u;
        out.eax=low(out.eax,get(out.si+out.bx+0x18u,1));out.si+=out.bx;
        bool valid=std::bit_cast<std::int8_t>(std::uint8_t(out.eax))>=0;
        if(valid) {
            out.bx=get(out.si+0x74u);out.di=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(out.eax))));
            valid=std::int32_t(out.di)<std::int32_t(out.bx);
            if(valid)out.eax=(out.eax&0xffff0000u)|(out.di&65535u);
        }
        if(!valid)out.eax=0xffffffffu;
        out.di=signed_half(out.eax);selected=out.di!=0xffffffffu;
    }
    if(selected) {
        out.bx=low(out.bx,get(out.edx+0xc1u,1));out.bx=low(out.bx,out.bx&1u);
        push(out.bp,0xbc28cu);out.bp=get(0x474d04u);
        if(out.ecx) {
            if(out.bx&255u) {
                out.edx=out.bp;out.eax=get(out.edx);put(out.ecx,out.eax,0xbc29du);
                out.eax=get(out.edx+4u);put(out.ecx+4u,out.eax,0xbc2a2u);
                out.edx=get(out.edx+8u);put(out.ecx+8u,out.edx,0xbc2a8u);
            } else copy_channel(false,0xbc2b1u,0xbc2b6u);
        }
        out.ecx=get(out.stack+0x14u);
        if(out.ecx) {
            if(out.bx&255u) {
                out.tail={false,out.bx&255u,0,1};
                out.eax=get(out.bp);put(out.ecx,out.eax,0xbc2c5u);
                out.edx=get(out.bp+4u);put(out.ecx+4u,out.edx,0xbc2cau);
                out.eax=get(out.bp+8u);put(out.ecx+8u,out.eax,0xbc2d0u);
            } else copy_channel(true,0xbc2deu,0xbc2e3u);
        } else out.tail={false,0,0,4};
        out.bp=pop();finish();return;
    }
    if(out.ecx) {
        out.eax=out.edx+0x88u;out.si=get(out.eax);put(out.ecx,out.si,0xbc2f6u);
        out.si=get(out.eax+4u);put(out.ecx+4u,out.si,0xbc2fbu);
        out.eax=get(out.eax+8u);put(out.ecx+8u,out.eax,0xbc301u);
    }
    out.eax=get(out.stack+0x10u);out.tail={false,out.eax,0,4};
    if(out.eax) {
        const auto object=out.edx;out.edx+=0x94u;out.tail={true,object,0x94u,4};
        out.ecx=get(out.edx);put(out.eax,out.ecx,0xbc314u);
        out.ecx=get(out.edx+4u);out.di=pop();put(out.eax+4u,out.ecx,0xbc31au);
        out.edx=get(out.edx+8u);out.si=pop();put(out.eax+8u,out.edx,0xbc321u);
        out.bx=pop();(void)pop();out.stack+=4u;return;
    }
    finish();
}
template<class State>void publish_tail(State& state,const Tail& tail) {
    if(tail.addition)state.flags.plus(tail.left,tail.right,tail.width);
    else state.flags.logic(tail.left,tail.width);
}
struct ColorChannel {
    std::uint32_t value,next_component,first_component,saved_bx,count,index;
    bool accepted,negative;
};
// CompleteBCD80 secondary RGB-row selection. The authored signed16 byte count
// retains the original unsigned reciprocal quotient, including negative raw
// header words. Successful copies interleave reads/stores for overlapping views.
inline ColorChannel color_channel(SourcePhysicsContactMemory& memory,std::uint32_t handle,
    std::uint32_t output,std::uint32_t index,std::uint32_t stack,std::uint32_t saved_bx,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto signed_half=[](std::uint32_t word){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word))));};
    const auto pool=read(memory,0x4ecbd0u);const auto rows=read(memory,pool+0x44u);
    const auto object=read(memory,rows+(handle&65535u)*12u+8u);
    const auto displacement=signed_half(read(memory,object+0x126u,2));
    const auto bytes=signed_half(read(memory,object+0x124u,2));
    const auto source=object+displacement;
    const auto high=std::uint32_t((std::uint64_t(0xaaaaaaabu)*bytes)>>32u);
    const auto count=(high>>3u)>>1u;
    store(memory,stack-4u,saved_bx,4,0xbcdb4u,observe);
    const bool negative=std::bit_cast<std::int32_t>(index)<0;
    const bool accepted=!negative&&std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(count);
    ColorChannel result{count&0xffffff00u,source,0,0,count,index,accepted,negative};
    if(accepted) {
        const auto row=source+(count+index)*12u;
        result.first_component=read(memory,row);store(memory,output,result.first_component,4,0xbcdcbu,observe);
        result.value=read(memory,row+4u);store(memory,output+4u,result.value,4,0xbcdd0u,observe);
        result.next_component=read(memory,row+8u);store(memory,output+8u,result.next_component,4,0xbcdd6u,observe);
        result.value=(result.value&0xffffff00u)|1u;
    }
    result.saved_bx=read(memory,stack-4u);return result;
}
template<class State>void bridge(State& state,std::uint32_t entry) {
    if(entry==0xbcd80u) {
        const auto result=color_channel(state.memory,state.gp[0],state.gp[6],state.gp[7],state.gp[4],state.gp[3],state.observe);
        state.gp[0]=result.value;state.gp[1]=result.next_component;state.gp[2]=result.first_component;state.gp[3]=result.saved_bx;
        if(result.accepted)state.flags.plus(result.count,result.index,4);
        else if(result.negative)state.flags.logic(result.index,4);
        else state.flags.minus(result.index,result.count,4);
        (void)read(state.memory,state.gp[4]);state.gp[4]+=4u;return;
    }
    if(entry!=0xbc200u) {
        const auto result=channel(state.memory,state.gp[2],state.gp[0],state.gp[1],entry==0x1d8d80u,state.observe);
        state.gp[0]=result.eax;state.gp[2]=result.edx;publish_tail(state,result.tail);
        if(result.loaded_simd){state.xmm[0]=result.z;state.xmm[1]=result.y;}
        (void)read(state.memory,state.gp[4]);state.gp[4]+=4u;return;
    }
    Source out{state.gp[0],state.gp[1],state.gp[2],state.gp[3],state.gp[4],state.gp[5],state.gp[6],state.gp[7],
        state.xmm[0],state.xmm[1],{}};
    select(state.memory,out,state.observe);
    state.gp={out.eax,out.ecx,out.edx,out.bx,out.stack,out.bp,out.si,out.di};
    state.xmm[0]=out.first;state.xmm[1]=out.second;publish_tail(state,out.tail);
}
}
