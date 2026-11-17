#pragma once
#include "halo2/native_tu15_projectile_function_query_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_host_fp_io.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_projectile_function_query_detail {
struct HostIo{template<class Function>void operator()(Function&&function)const{
    native_host_fp_io_detail::preserve(std::forward<Function>(function));
}};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
inline __m128 subtract(__m128 first,__m128 second){asm volatile("subss %1,%0":"+x"(first):"x"(second));return first;}
inline __m128 multiply(__m128 first,__m128 second){asm volatile("mulss %1,%0":"+x"(first):"x"(second));return first;}
inline __m128 divide(__m128 first,__m128 second){asm volatile("divss %1,%0":"+x"(first):"x"(second));return first;}
template<class State>std::uint32_t shift4(State&s,std::uint32_t value){
    // Original100E99 is C1 E0 04. Preserve the host instruction's complete
    // status publication; retained software shift clears undefined AF.
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
    std::uint32_t overflow{};
    asm volatile("addb $127,%%al;sahf;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"=c"(overflow),"+r"(value)::"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
}
// Private unchanged-i386 adapter. Source symbol services, callback formats and
// predecrement PUSH publication require separate retained-provider transports.
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo>{
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address){at(pc);return word(address);}
    __m128 scalar(std::uint32_t pc,std::uint32_t address){return _mm_set_ss(std::bit_cast<float>(read(pc,address)));}
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);Base::io([&]{s_.memory.read(address,event.before);});}
        const std::array data{std::byte(value)};Base::io([&]{s_.memory.write(address,data);});
        if(s_.observe){event.written.assign(data.begin(),data.end());Base::io([&]{s_.observe(event);});}
    }
    bool nonzero(std::uint32_t pc,std::uint32_t address){
        s_.xmm[0]=scalar(pc,address);s_.flags.scalar_compare(s_.xmm[0],s_.xmm[1],true);
        s_.set(0,s_.flags.bits()|2u,1,8);s_.flags.logic(s_.reg(0,1,8)&0x44u,1);
        return s_.flags.pf;
    }
    std::uint32_t finish(std::uint32_t restore,std::uint32_t ret){
        s_.gp[3]=pop(restore);pop(ret);s_.gp[4]+=16u;return s_.gp[0];
    }
public:
    Frame(State&s,Before&before):Base{s,before,HostIo{}}{}
    Kind begin(){
        auto&g=s_.gp;
        g[0]=read(0x100e70u,g[4]+4u);g[1]=read(0x100e74u,0x4ecbd0u);
        g[2]=read(0x100e7au,g[1]+0x44u);s_.xmm[1]=_mm_xor_ps(s_.xmm[1],s_.xmm[1]);
        g[0]=s_.flags.logic(g[0]&0xffffu,4);g[0]+=g[0]*2u;
        g[1]=read(0x100e88u,g[2]+g[0]*4u+8u);g[0]=read(0x100e8cu,g[1]);
        g[2]=read(0x100e8eu,0x4f0414u);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        g[0]=shift4(s_,g[0]);g[2]=read(0x100e9cu,g[2]+g[0]+8u);
        g[0]=read(0x100ea0u,g[4]+8u);s_.flags.minus(g[0],0xf000563u,4);
        Base::push(0x100ea9u,g[3]);s_.set(3,1u,1,0);
        if(!s_.flags.zf&&s_.flags.sf==s_.flags.of){
            s_.flags.minus(g[0],0x11000566u,4);if(s_.flags.zf)return Kind::second_flag;
            s_.flags.minus(g[0],0x12000565u,4);if(s_.flags.zf)return Kind::scaled_value;
            return Kind::unsupported;
        }
        if(s_.flags.zf)return Kind::ratio;
        s_.flags.minus(g[0],0x6000564u,4);if(s_.flags.zf)return Kind::first_flag;
        s_.flags.minus(g[0],0xe000562u,4);return s_.flags.zf?Kind::direct_value:Kind::unsupported;
    }
    std::uint32_t unsupported(){s_.set(0,s_.flags.logic(0u,1),1,0);return finish(0x100f14u,0x100f15u);}
    void direct_value(){s_.xmm[0]=scalar(0x100ebeu,s_.gp[1]+0x158u);}
    void flag_value(bool second){
        auto&g=s_.gp;
        if(second){g[1]=read(0x100f80u,g[1]+0x12cu);g[1]=s_.flags.right(g[1],3u,4,false);s_.flags.logic(s_.reg(1,1,0)&1u,1);}
        else{g[0]=read(0x100ecbu,g[1]+0x12cu);g[0]=s_.flags.right(g[0],1u,4,false);s_.flags.logic(s_.reg(0,1,0)&1u,1);}
        s_.xmm[0]=scalar(0x100f8cu,0x46a0d0u);if(s_.flags.zf)zero();
    }
    bool ratio_enabled(){return nonzero(0x100edau,s_.gp[2]+0xe0u);}
    void ratio(){
        s_.xmm[0]=scalar(0x100eefu,s_.gp[1]+0x168u);
        s_.xmm[0]=divide(s_.xmm[0],scalar(0x100ef7u,s_.gp[2]+0xe0u));
    }
    bool selected_one(){
        auto&g=s_.gp;g[0]=read(0x100f18u,g[1]+0x12cu);
        g[0]=s_.flags.right(g[0],8u,4,false);s_.flags.logic(s_.reg(0,1,0)&1u,1);return !s_.flags.zf;
    }
    bool first_parameter(){return nonzero(0x100f2fu,s_.gp[2]+0x17cu);}
    bool second_parameter(){return nonzero(0x100f40u,s_.gp[2]+0x180u);}
    void one(){s_.xmm[0]=scalar(0x100f25u,0x46a0d0u);}
    void zero(){s_.xmm[0]=s_.xmm[1];}
    void scaled_value(){
        s_.xmm[0]=scalar(0x100f51u,s_.gp[1]+0x168u);
        s_.xmm[0]=subtract(s_.xmm[0],scalar(0x100f59u,s_.gp[2]+0x18cu));
        s_.xmm[0]=multiply(s_.xmm[0],scalar(0x100f61u,s_.gp[2]+0x194u));
    }
    bool below_zero(){s_.flags.scalar_compare(s_.xmm[1],s_.xmm[0],false);return !s_.flags.cf&&!s_.flags.zf;}
    void upper_limit(){s_.xmm[2]=scalar(0x100f6eu,0x46a0d0u);}
    bool above_upper_limit(){s_.flags.scalar_compare(s_.xmm[0],s_.xmm[2],false);return !s_.flags.cf&&!s_.flags.zf;}
    void take_upper_limit(){s_.xmm[0]=s_.xmm[2];}
    std::uint32_t publish(){
        auto&g=s_.gp;s_.flags.scalar_compare(s_.xmm[0],s_.xmm[1],false);
        g[2]=read(0x100f9cu,g[4]+16u);
        Base::store(0x100fa0u,g[2],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0])),[]{});
        if(s_.flags.cf||s_.flags.zf){
            g[1]=read(0x100fb7u,g[4]+20u);g[0]=s_.flags.logic(0u,4);
            store_byte(0x100fbdu,g[1],s_.reg(0,1,0));s_.set(0,s_.reg(3,1,0),1,0);
            return finish(0x100fc1u,0x100fc2u);
        }
        g[1]=read(0x100fa6u,g[4]+20u);g[0]=1u;store_byte(0x100fafu,g[1],s_.reg(0,1,0));
        s_.set(0,s_.reg(3,1,0),1,0);return finish(0x100fb3u,0x100fb4u);
    }
};
template<class State,class Before=Noop>std::uint32_t run_private(State&s,Before before={}){
    Frame view{s,before};return query(view);
}
} // namespace halo2::native_projectile_function_query_detail
#endif
