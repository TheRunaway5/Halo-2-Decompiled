#pragma once
#include "halo2/native_tu15_projectile_function_query_kernel.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_projectile_function_query_detail {
// Original 100F61 uses XMM0 as the destination. The commutative intrinsic
// can exchange two distinct NaN payloads during host code generation.
inline __m128 ordered_multiply(__m128 destination,__m128 source){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulss %1,%0":"+x"(destination):"x"(source));return destination;
#else
    return _mm_mul_ss(destination,source);
#endif
}

// The retained source exposes more than the physical frame: PUSH publishes
// ESP before the provider, the five selector literals come from the bank's
// symbol lease, and even a branch marker can change live flags. Keep those
// boundaries around the shared typed selection and output policy.
template<bool Literal,class State>class SourceFrame {
    State&s_;
    void step(std::uint32_t pc){
        if constexpr(Literal)s_.instruction_literal(pc);
        else if(s_.services.instruction)s_.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address){return physics_world_runtime_detail::r(s_.memory,address,4);}
    __m128 scalar(std::uint32_t address){return _mm_set_ss(std::bit_cast<float>(read(address)));}
    void compare_key(std::uint32_t pc,std::uint32_t key){
        step(pc);s_.flags.minus(s_.reg(0,4,0),source_projectile_function_symbol_tu15(s_.memory,pc,key),4);
    }
    bool nonzero(std::uint32_t load,std::uint32_t compare,std::uint32_t lahf,std::uint32_t test,
                 std::uint32_t branch,std::uint32_t displacement){
        step(load);s_.xmm[0]=scalar(s_.gp[2]+displacement);
        step(compare);s_.flags.scalar_compare(s_.xmm[0],s_.xmm[1],true);
        step(lahf);s_.set(0,s_.flags.bits()|2u,1,8);
        step(test);s_.flags.logic(s_.reg(0,1,8)&0x44u,1);
        step(branch);return s_.flags.pf;
    }
    std::uint32_t finish(std::uint32_t restore,std::uint32_t ret){
        step(restore);s_.set(3,s_.pop(),4,0);
        step(ret);s_.pop();s_.gp[4]+=16u;return s_.gp[0];
    }
public:
    explicit SourceFrame(State&s):s_(s){}
    Kind begin(){
        auto&g=s_.gp;
        step(0x100e70u);g[0]=read(g[4]+4u);
        step(0x100e74u);g[1]=read(0x4ecbd0u);
        step(0x100e7au);g[2]=read(g[1]+0x44u);
        step(0x100e7du);s_.xmm[1]=_mm_xor_ps(s_.xmm[1],s_.xmm[1]);
        step(0x100e80u);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        step(0x100e85u);g[0]+=g[0]*2u;
        step(0x100e88u);g[1]=read(g[2]+g[0]*4u+8u);
        step(0x100e8cu);g[0]=read(g[1]);
        step(0x100e8eu);g[2]=read(0x4f0414u);
        step(0x100e94u);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        step(0x100e99u);g[0]=s_.flags.shift(g[0],4u,4);
        step(0x100e9cu);g[2]=read(g[2]+g[0]+8u);
        step(0x100ea0u);g[0]=read(g[4]+8u);
        compare_key(0x100ea4u,0xf000563u);
        step(0x100ea9u);s_.push(0x100ea9u,s_.reg(3,4,0));
        step(0x100eaau);s_.set(3,1u,1,0);
        step(0x100eacu);
        if(!s_.flags.zf&&s_.flags.sf==s_.flags.of){
            compare_key(0x100f04u,0x11000566u);step(0x100f09u);
            if(s_.flags.zf)return Kind::second_flag;
            compare_key(0x100f0bu,0x12000565u);step(0x100f10u);
            return s_.flags.zf?Kind::scaled_value:Kind::unsupported;
        }
        step(0x100eaeu);if(s_.flags.zf)return Kind::ratio;
        compare_key(0x100eb0u,0x6000564u);step(0x100eb5u);
        if(s_.flags.zf)return Kind::first_flag;
        compare_key(0x100eb7u,0xe000562u);step(0x100ebcu);
        return s_.flags.zf?Kind::direct_value:Kind::unsupported;
    }
    std::uint32_t unsupported(){
        step(0x100f12u);s_.set(0,s_.flags.logic(0u,1),1,0);return finish(0x100f14u,0x100f15u);
    }
    void direct_value(){step(0x100ebeu);s_.xmm[0]=scalar(s_.gp[1]+0x158u);step(0x100ec6u);}
    void flag_value(bool second){
        auto&g=s_.gp;
        if(second){
            step(0x100f80u);g[1]=read(g[1]+0x12cu);
            step(0x100f86u);g[1]=s_.flags.right(g[1],3u,4,false);
            step(0x100f89u);s_.flags.logic(s_.reg(1,1,0)&1u,1);
        }else{
            step(0x100ecbu);g[0]=read(g[1]+0x12cu);
            step(0x100ed1u);g[0]=s_.flags.right(g[0],1u,4,false);
            step(0x100ed3u);s_.flags.logic(s_.reg(0,1,0)&1u,1);
            step(0x100ed5u);
        }
        step(0x100f8cu);s_.xmm[0]=scalar(0x46a0d0u);
        step(0x100f94u);if(s_.flags.zf)zero();
    }
    bool ratio_enabled(){return nonzero(0x100edau,0x100ee2u,0x100ee5u,0x100ee6u,0x100ee9u,0xe0u);}
    void ratio(){
        step(0x100eefu);s_.xmm[0]=scalar(s_.gp[1]+0x168u);
        step(0x100ef7u);s_.xmm[0]=_mm_div_ss(s_.xmm[0],scalar(s_.gp[2]+0xe0u));
        step(0x100effu);
    }
    bool selected_one(){
        auto&g=s_.gp;
        step(0x100f18u);g[0]=read(g[1]+0x12cu);
        step(0x100f1eu);g[0]=s_.flags.right(g[0],8u,4,false);
        step(0x100f21u);s_.flags.logic(s_.reg(0,1,0)&1u,1);
        step(0x100f23u);return !s_.flags.zf;
    }
    bool first_parameter(){return nonzero(0x100f2fu,0x100f37u,0x100f3au,0x100f3bu,0x100f3eu,0x17cu);}
    bool second_parameter(){return nonzero(0x100f40u,0x100f48u,0x100f4bu,0x100f4cu,0x100f4fu,0x180u);}
    void one(){step(0x100f25u);s_.xmm[0]=scalar(0x46a0d0u);step(0x100f2du);}
    void zero(){step(0x100f96u);s_.xmm[0]=s_.xmm[1];}
    void scaled_value(){
        step(0x100f51u);s_.xmm[0]=scalar(s_.gp[1]+0x168u);
        step(0x100f59u);s_.xmm[0]=_mm_sub_ss(s_.xmm[0],scalar(s_.gp[2]+0x18cu));
        step(0x100f61u);s_.xmm[0]=ordered_multiply(s_.xmm[0],scalar(s_.gp[2]+0x194u));
    }
    bool below_zero(){
        step(0x100f69u);s_.flags.scalar_compare(s_.xmm[1],s_.xmm[0],false);
        step(0x100f6cu);return !s_.flags.cf&&!s_.flags.zf;
    }
    void upper_limit(){step(0x100f6eu);s_.xmm[2]=scalar(0x46a0d0u);}
    bool above_upper_limit(){
        step(0x100f76u);s_.flags.scalar_compare(s_.xmm[0],s_.xmm[2],false);
        step(0x100f79u);return !s_.flags.cf&&!s_.flags.zf;
    }
    void take_upper_limit(){step(0x100f7bu);s_.xmm[0]=s_.xmm[2];step(0x100f7eu);}
    std::uint32_t publish(){
        auto&g=s_.gp;
        step(0x100f99u);s_.flags.scalar_compare(s_.xmm[0],s_.xmm[1],false);
        step(0x100f9cu);g[2]=read(g[4]+16u);
        step(0x100fa0u);physics_world_runtime_detail::w(s_.memory,s_.observe,0x100fa0u,g[2],4,
            std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0])));
        step(0x100fa4u);
        if(s_.flags.cf||s_.flags.zf){
            step(0x100fb7u);g[1]=read(g[4]+20u);
            step(0x100fbbu);g[0]=s_.flags.logic(0u,4);
            step(0x100fbdu);physics_world_runtime_detail::w(s_.memory,s_.observe,0x100fbdu,g[1],1,s_.reg(0,1,0));
            step(0x100fbfu);s_.set(0,s_.reg(3,1,0),1,0);return finish(0x100fc1u,0x100fc2u);
        }
        step(0x100fa6u);g[1]=read(g[4]+20u);
        step(0x100faau);g[0]=1u;
        step(0x100fafu);physics_world_runtime_detail::w(s_.memory,s_.observe,0x100fafu,g[1],1,s_.reg(0,1,0));
        step(0x100fb1u);s_.set(0,s_.reg(3,1,0),1,0);return finish(0x100fb3u,0x100fb4u);
    }
};
template<bool Literal,class State>std::uint32_t run_source_frame(State&s){
    SourceFrame<Literal,State> view{s};return query(view);
}
} // namespace halo2::native_projectile_function_query_detail
