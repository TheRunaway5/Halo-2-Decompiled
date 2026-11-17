#pragma once
#include "halo2/native_tu15_codec_rotation_interpolation_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include <bit>

namespace halo2::native_codec_rotation_interpolation_detail {
struct AlignedAccessFault{
    std::uint32_t instruction,address,execution_flags,saved_fault_flags;
};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io{
    Scope*scope;
    template<class Callback>void operator()(Callback&&callback)const{scope->io(std::forward<Callback>(callback));}
};
template<class State>std::uint32_t product(State&s,std::uint32_t left,std::uint32_t right){
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
    std::uint32_t overflow{};
    asm volatile("addb $127,%%al;sahf;imull %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"=c"(overflow),"+r"(left):"r"(right):"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return left;
}
template<class State>std::uint32_t shift4(State&s,std::uint32_t value){
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
    std::uint32_t overflow{};
    asm volatile("addb $127,%%al;sahf;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"=c"(overflow),"+r"(value)::"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
}
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;using Base::before_;
    static __m128 scalar(std::uint32_t value){return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});}
    static std::uint32_t bits(__m128 value){return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];}
    void aligned(std::uint32_t pc,std::uint32_t address){
        if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw AlignedAccessFault{pc,address,flags,flags|0x10000u};}
    }
    __m128 vector(std::uint32_t pc,std::uint32_t address){
        at(pc);aligned(pc,address);std::array<std::byte,16>bytes{};
        io([&]{s_.memory.read(address,bytes);});return std::bit_cast<__m128>(bytes);
    }
    void returned_boundary(std::uint32_t pc,bool memory){
        io([&]{s_.instruction_literal(pc);});
        if(memory)io([&]{before_(s_,pc);});
    }
public:
    struct Input{__m128 first,second,factor;};
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    Input input(){
        auto&g=s_.gp;g[4]=s_.flags.minus(g[4],16u,4);
        at(0x296243u);g[0]=word(0x510f60u);
        at(0x296248u);g[1]=word(g[0]+20u);
        at(0x29624bu);const auto index=word(0x510f94u);g[1]=product(s_,g[1],index);
        at(0x296252u);g[2]=word(0x510fa0u);
        at(0x296258u);s_.xmm[0]=scalar(word(0x510f4cu));g[0]=g[0]+g[1]+32u;
        at(0x296264u);g[1]=word(0x510f48u);
        Base::store(0x29626au,g[4],g[2],[]{});
        at(0x29626du);g[2]=word(0x510f44u);
        g[1]=shift4(s_,g[1]);g[1]=s_.flags.plus(g[1],g[0],4);
        g[2]=shift4(s_,g[2]);g[2]=s_.flags.plus(g[2],g[0],4);
        Base::store(0x29627du,g[4]+4u,bits(s_.xmm[0]),[]{});
        Base::store(0x296283u,g[4]+8u,g[1],[]{});
        Base::store(0x296287u,g[4]+12u,g[2],[]{});
        at(0x29628bu);g[0]=word(g[4]+12u);
        at(0x29628fu);g[1]=word(g[4]+8u);
        at(0x296293u);s_.xmm[3]=scalar(word(g[4]+4u));
        s_.xmm[1]=vector(0x296299u,g[0]);s_.xmm[2]=vector(0x29629cu,g[1]);
        return {s_.xmm[1],s_.xmm[2],s_.xmm[3]};
    }
    __m128 subtract(__m128 left,__m128 right)const{
        asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    __m128 multiply(__m128 left,__m128 right)const{
        asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    __m128 add(__m128 left,__m128 right)const{
        asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    __m128 opposite(__m128 value)const{return _mm_shuffle_ps(value,value,78);}
    __m128 alternate(__m128 value)const{return _mm_shuffle_ps(value,value,17);}
    __m128 interpolate(const Input&input){
        s_.xmm[3]=_mm_shuffle_ps(input.factor,input.factor,0);
        return add(multiply(subtract(input.second,input.first),s_.xmm[3]),input.first);
    }
    __m128 inverse(__m128 norm,__m128 quaternion,__m128 opposite,__m128 alternate){
        s_.xmm[0]=norm;s_.xmm[2]=quaternion;s_.xmm[7]=opposite;s_.xmm[6]=alternate;
        at(0x2962c6u);
        if(!s_.services.reciprocal_sqrt)throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
        auto lanes=std::bit_cast<std::array<std::uint32_t,4>>(norm);
        for(auto&lane:lanes)io([&]{lane=s_.services.reciprocal_sqrt(lane);});
        s_.xmm[0]=std::bit_cast<__m128>(lanes);
        returned_boundary(0x2962c9u,true);s_.gp[2]=word(s_.gp[4]);
        return s_.xmm[0];
    }
    __m128 scale([[maybe_unused]] __m128 quaternion,[[maybe_unused]] __m128 inverse){
        returned_boundary(0x2962ccu,false);return multiply(s_.xmm[2],s_.xmm[0]);
    }
    void output(__m128 value){
        s_.xmm[2]=value;returned_boundary(0x2962cfu,true);
        const auto output_address=s_.gp[2];aligned(0x2962cfu,output_address);
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&s_.xmm[2],16);
        for(unsigned offset:{0u,8u}){
            const auto address=output_address+offset;SourcePhysicsContactRecordStore event{0x2962cfu,address,{},{}};
            if(s_.observe){event.before.resize(8);io([&]{s_.memory.read(address,event.before);});}
            const auto part=std::span(bytes).subspan(offset,8);io([&]{s_.memory.write(address,part);});
            if(s_.observe){event.written.assign(part.begin(),part.end());io([&]{s_.observe(event);});}
        }
        returned_boundary(0x2962d2u,false);s_.gp[4]=s_.flags.plus(s_.gp[4],16u,4);
        returned_boundary(0x2962d5u,true);(void)word(s_.gp[4]);s_.gp[4]+=4u;
    }
};
// Private arithmetic/frame candidate; no production admission or default.
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    native_integer_fp_detail::Scope scope{s};Frame view{s,before,scope};decode(view);
}
} // namespace halo2::native_codec_rotation_interpolation_detail
#endif
