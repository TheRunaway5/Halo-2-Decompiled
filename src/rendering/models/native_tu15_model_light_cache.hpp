#pragma once
#include "rendering/models/native_tu15_model_lighting_leaves.hpp"
#include "rendering/models/native_tu15_model_color_publish.hpp"

namespace halo2::model_light_cache_detail {
using namespace prt_projection_detail;
enum class FlagKind { raw,logic,compare,increment,decrement,add };
struct Flags { FlagKind kind;std::uint32_t left{},right{};bool carry{}; };
struct Caller {
    std::uint32_t eax,ecx,edx,bx,stack,bp,si,di,other;
    __m128 simd[8];std::uint16_t status;Flags flags;
};
inline bool above(__m128 left,__m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char result;asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(left),"x"(right):"cc");return result;
#else
    return _mm_comigt_ss(left,right);
#endif
}
inline std::uint32_t signed_half(std::uint32_t value) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
}
// Complete3DDC0 cache policy, native defaults and authored record copies.
// Pool allocation/iteration are genuine owner boundaries; their existing
// instruction-shaped implementations remain separately counted residuals.
template<class Call>void update(SourcePhysicsContactMemory& memory,Caller& out,
    const SourcePhysicsContactRecordObserver& observe,Call&& owner) {
    const auto get=[&](std::uint32_t at,unsigned width=4){return read(memory,at,width);};
    const auto put=[&](std::uint32_t at,std::uint32_t value,std::uint32_t pc,unsigned width=4){store(memory,at,value,width,pc,observe);};
    const auto push=[&](std::uint32_t value,std::uint32_t pc){out.stack-=4u;put(out.stack,value,pc);};
    const auto pop=[&]{const auto value=get(out.stack);out.stack+=4u;return value;};
    const auto call=[&](std::uint32_t target,std::uint32_t site,std::uint32_t next){push(next,site);owner(target,site,out);};
    const auto finish=[&](bool failed){
        out.eax=failed?out.si:get(out.stack+0x10u);
        out.di=pop();out.si=pop();out.bp=pop();out.bx=pop();
        out.flags={FlagKind::add,out.stack,12};out.stack+=12u;(void)pop();out.stack+=4u;
    };
    out.stack-=12u;push(out.bx,0x3ddc3u);out.bx=get(out.stack+0x14u);push(out.bp,0x3ddc8u);
    out.eax=out.bx&65535u;out.bp=out.eax*3u;out.eax=get(0x4ecbd0u);out.ecx=get(out.eax+0x44u);out.bp*=4u;
    out.eax=get(out.ecx+out.bp+8u);put(out.stack+0x10u,out.eax,0x3dde2u);out.eax=get(out.eax+0xccu);
    push(out.si,0x3ddecu);out.ecx&=0xffffff00u;out.flags={FlagKind::compare,out.eax,0xffffffffu};
    push(out.di,0x3ddf2u);out.di=get(0x515f14u);
    bool found=false;
    if(out.eax!=0xffffffffu) {
        out.si=get(out.di+0x44u);out.edx=(out.eax&65535u)<<8u;
        const auto existing_owner=get(out.si+out.edx+4u);
        out.flags={FlagKind::compare,existing_owner,out.bx};
        if(existing_owner==out.bx){put(out.stack+0x10u,out.eax,0x3de0fu);found=true;}
    }
    if(!found) {
        out.edx=out.di;call(0x16e6a0u,0x3de1au,0x3de1fu);out.si=out.eax;
        put(out.stack+0x10u,out.si,0x3de24u);
        if(out.si==0xffffffffu) {
            out.simd[1]=scalar(get(0x44aac8u));out.edx=0;out.ecx=out.di;
            out.flags={FlagKind::logic,0};call(0x16ea60u,0x3de3au,0x3de3fu);out.ecx=0xffffffffu;
            if(out.eax==out.si){finish(true);return;}
            const auto make_handle=[&](bool first){
                const auto stride=get(out.di+0x24u);
                if(first)out.edx=get(out.di+0x44u);
                out.ecx=signed_half(get((first?out.edx:out.si)+stride*out.eax,2));
                out.ecx=(out.ecx<<16u)|out.eax;
            };
            make_handle(true);
            if(out.ecx==0xffffffffu){finish(true);return;}
            out.si=get(out.di+0x44u);out.simd[2]=_mm_setzero_ps();out.simd[3]=scalar(get(0x46a1dcu));
            do {
                out.bx=get(0x4c6854u);out.eax=out.ecx&65535u;out.edx=out.eax<<8u;
                out.bx-=get(out.si+out.edx+12u);
                out.simd[0]=_mm_cvtsi32_ss(out.simd[0],std::bit_cast<std::int32_t>(out.bx));
                if(above(out.simd[2],out.simd[0]))out.simd[0]=out.simd[3];
                if(above(out.simd[0],out.simd[1])) {
                    out.simd[1]=out.simd[0];put(out.stack+0x10u,out.ecx,0x3de9au);
                }
                const auto previous_index=out.eax;++out.eax;out.edx=out.eax;out.ecx=out.di;
                out.flags={FlagKind::increment,previous_index,0,true};
                call(0x16ea60u,0x3deacu,0x3deb1u);out.ecx=0xffffffffu;
                if(out.eax!=0xffffffffu)make_handle(false);
            } while(out.ecx!=0xffffffffu);
            out.bx=get(out.stack+0x20u);out.si=get(out.stack+0x10u);
            if(out.si==0xffffffffu){finish(true);return;}
        }
        out.ecx=(out.ecx&0xffffff00u)|1u;
    }
    out.si=get(out.stack+0x10u);out.edx=get(out.di+0x44u);out.si=((out.si&65535u)<<8u)+out.edx;
    if(out.ecx&255u) {
        out.di=0xffffffffu;out.eax=out.si+0x44u;
        put(out.si+4u,out.bx,0x3df09u);put(out.si+8u,out.di,0x3df0cu);put(out.si+12u,out.di,0x3df0fu);
        put(out.si+2u,0,0x3df12u,1);put(out.si+3u,0,0x3df16u,1);
        out.flags={FlagKind::logic,0xffffffffu};call(0x2c390u,0x3df1au,0x3df1fu);
        out.edx=get(0x4ecbd0u);put(out.eax,out.bx,0x3df25u);put(out.si+0x48u,0xc08e0u,0x3df27u);
        put(out.si+0xbcu,out.di,0x3df2eu);out.eax=get(out.edx+0x44u);out.eax=get(out.eax+out.bp+8u);
        out.bx=signed_half(get(out.eax+0x11au,2));out.ecx=signed_half(get(out.eax+0x118u,2));out.bx+=out.eax;
        const auto product=std::int64_t(0x66666667)*std::bit_cast<std::int32_t>(out.ecx);
        out.edx=std::uint32_t(std::uint64_t(product)>>32u);
        out.edx=std::uint32_t(std::bit_cast<std::int32_t>(out.edx)>>2u);
        out.eax=out.edx+(out.edx>>31u);out.edx=out.si+0xc0u;put(out.stack+0x14u,4,0x3df62u);
        do {
            out.ecx=out.eax;out.bp=out.ecx;out.ecx>>=2u;out.di=out.edx;out.si=out.bx;
            const auto word_delta=(out.other&1024u)?0xfffffffcu:4u;
            while(out.ecx) {
                const auto value=get(out.si);put(out.di,value,0x3df7bu);out.si+=word_delta;out.di+=word_delta;--out.ecx;
            }
            out.ecx=out.bp&3u;const auto byte_delta=(out.other&1024u)?0xffffffffu:1u;
            while(out.ecx) {
                const auto value=get(out.si,1);put(out.di,value,0x3df82u,1);out.si+=byte_delta;out.di+=byte_delta;--out.ecx;
            }
            out.ecx=get(out.stack+0x14u);const auto carry=std::uint64_t(out.edx)+16u>0xffffffffu;out.edx+=16u;
            const auto previous_count=out.ecx;--out.ecx;put(out.stack+0x14u,out.ecx,0x3df8cu);
            out.flags={FlagKind::decrement,previous_count,0,carry};
        } while(out.ecx);
        out.eax=get(out.stack+0x20u);out.ecx=get(out.stack+0x18u);out.edx=get(out.stack+0x10u);
        push(1,0x3df9eu);push(out.eax,0x3dfa0u);put(out.ecx+0xccu,out.edx,0x3dfa1u);
        call(0x3dd00u,0x3dfa7u,0x3dfacu);
    }
    finish(false);
}
template<class State>void install(State& state,const Caller& out) {
    state.gp={out.eax,out.ecx,out.edx,out.bx,out.stack,out.bp,out.si,out.di};state.other_flags=out.other;
    for(unsigned i=0;i<8;++i)state.xmm[i]=out.simd[i];state.fp.status=out.status;
    switch(out.flags.kind) {
    case FlagKind::raw:state.flags.load(out.flags.left);break;
    case FlagKind::logic:state.flags.logic(out.flags.left,4);break;
    case FlagKind::compare:state.flags.minus(out.flags.left,out.flags.right,4);break;
    case FlagKind::add:state.flags.plus(out.flags.left,out.flags.right,4);break;
    case FlagKind::increment:case FlagKind::decrement:
        state.flags.cf=out.flags.carry;state.flags.increment(out.flags.left,4,out.flags.kind==FlagKind::decrement);break;
    }
}
template<class State>Caller capture(const State& state) {
    Caller result{state.gp[0],state.gp[1],state.gp[2],state.gp[3],state.gp[4],state.gp[5],state.gp[6],state.gp[7],
        state.other_flags,{},state.fp.status,{FlagKind::raw,state.flags.bits()}};
    for(unsigned i=0;i<8;++i)result.simd[i]=state.xmm[i];return result;
}
template<class State>void bridge(State& state,const std::function<void(std::uint32_t)>& trace={}) {
    auto out=capture(state);
    const auto call=[&](std::uint32_t target,std::uint32_t site,Caller& context){
        install(state,context);if(trace)trace(target);
        if(target==0x2c390u)model_lighting_leaves_detail::bridge(state,target);
        else if(target==0x3dd00u)model_color_publish_detail::bridge(state);
        else state.required(site,target,false);
        context=capture(state);
    };
    update(state.memory,out,state.observe,call);install(state,out);
}
}
