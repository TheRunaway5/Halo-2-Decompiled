#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_sphere_ray_cast_kernel.hpp"

namespace halo2::native_sphere_ray_cast_detail {
namespace N=native_mopp_ray_detail;
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&scope_;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address){at(pc);return word(address);}
    void floating(std::uint32_t pc,std::uint32_t address){const auto value=read(pc,address);scope_.require_capacity(1);N::push(value);}
    void duplicate(){scope_.require_capacity(1);asm volatile("fld %%st(0)":::"memory","st");}
    void divide(std::uint32_t value){asm volatile("fdivs %0"::"m"(value):"memory","st");}
    void spill(std::uint32_t pc,std::uint32_t address,bool pop_value){
        at(pc);const auto pending=scope_.stage_single(pop_value);
        Base::store(pc,address,pending.bits,[&]{scope_.commit(pending);},true);
    }
    void comparison(std::uint16_t status,unsigned mask){
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|status;s_.flags.logic((status>>8u)&mask,1);
    }
    N::Comparison compare(std::uint32_t pc,std::uint32_t address,unsigned mask,bool pop_value=true){
        const auto value=read(pc,address);const auto result=N::compare(value,pop_value);comparison(result.status,mask);return result;
    }
    static __m128 shuffle_y(__m128 value){return _mm_shuffle_ps(value,value,0x55);}
    static __m128 shuffle_z(__m128 value){return _mm_shuffle_ps(value,value,0xaa);}
    static __m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
    static std::uint32_t low(__m128 value){return N::words(value)[0];}
    void dot(unsigned partial,unsigned total,std::uint32_t pc,std::uint32_t address){
        auto&x=s_.xmm;x[partial]=shuffle_y(x[0]);x[total]=x[0];
        x[partial]=add_scalar(x[partial],x[0]);x[total]=shuffle_z(x[total]);x[total]=add_scalar(x[total],x[partial]);
        Base::store(pc,address,low(x[total]),[]{});
    }
    void boolean(std::uint32_t pc,bool value){
        auto&g=s_.gp;g[0]=read(value?0x2e8fb2u:0x2e8f12u,g[5]+8u);
        at(pc);const auto address=g[0];SourcePhysicsContactRecordStore event{pc,address,{},{std::byte(value)}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        io([&]{s_.memory.write(address,event.written);});if(s_.observe)io([&]{s_.observe(event);});
    }
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}},scope_(scope){}
    void begin(){auto&g=s_.gp;push(0x2e8dd0u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x10u,4);g[2]=read(0x2e8dd9u,g[5]+12u);}
    void initial_geometry(){
        auto&g=s_.gp;auto&x=s_.xmm;floating(0x2e8ddcu,g[1]+12u);x[2]=Base::vector(0x2e8ddfu,g[2]);
        duplicate();x[3]=Base::vector(0x2e8de4u,g[2]+16u);N::multiply_second();x[1]=N::subtract(x[3],x[2]);
        spill(0x2e8df0u,g[4]+8u,true);x[0]=N::multiply(x[1],x[2]);x[4]=x[0];N::discard();
        // The original publishes EAX between the shuffle copies and ADDSS.
        x[4]=shuffle_y(x[4]);x[5]=x[0];g[0]=g[4]+4u;
        x[4]=add_scalar(x[4],x[0]);x[5]=shuffle_z(x[5]);x[5]=add_scalar(x[5],x[4]);
        Base::store(0x2e8e16u,g[0],low(x[5]),[]{});
        floating(0x2e8e1au,g[4]+4u);floating(0x2e8e1eu,g[4]+4u);
    }
    bool approaching(){return (compare(0x2e8e22u,0x46a0e8u,1).status&0x100u)!=0u;}
    void direction_length(){
        auto&g=s_.gp;auto&x=s_.xmm;floating(0x2e8e33u,g[4]+4u);x[0]=x[1];N::multiply(read(0x2e8e3au,g[4]+4u));
        x[0]=N::multiply(x[0],x[1]);x[1]=shuffle_y(x[0]);x[4]=x[0];g[0]=g[4];
        x[1]=add_scalar(x[1],x[0]);x[4]=shuffle_z(x[4]);x[4]=add_scalar(x[4],x[1]);
        Base::store(0x2e8e5au,g[0],low(x[4]),[]{});
        floating(0x2e8e5eu,g[4]);N::multiply(read(0x2e8e61u,g[4]+8u));N::multiply(read(0x2e8e65u,0x41c510u));
    }
    bool needs_recentering(){
        std::uint16_t status;asm volatile("fcompp;fnstsw %0":"=a"(status)::"memory","st","st(1)");comparison(status,5);
        const auto pair=(status>>8u)&5u;return pair==1u||pair==4u;
    }
    bool recenter_possible(){N::discard();floating(0x2e8e76u,s_.gp[4]);return compare(0x2e8e79u,s_.gp[4]+8u,5).not_less();}
    void recenter(){
        auto&g=s_.gp;auto&x=s_.xmm;floating(0x2e8e88u,g[4]+4u);asm volatile("fchs":::"memory","st");
        spill(0x2e8e8eu,g[4]+4u,false);divide(read(0x2e8e92u,g[4]));spill(0x2e8e95u,g[4]+12u,true);
        floating(0x2e8e99u,0x46a0e8u);const auto t=read(0x2e8e9fu,g[4]+12u);x[0]=N::broadcast(t);x[1]=N::multiply(x[0],x[3]);
        x[3]=Base::vector(0x2e8eafu,0x41c500u);x[3]=N::subtract(x[3],x[0]);x[0]=N::multiply(x[3],x[2]);x[0]=N::add(x[0],x[1]);
    }
    void original_origin(){Base::store(0x2e8ec4u,s_.gp[4]+4u,0u,[]{});s_.xmm[0]=s_.xmm[2];}
    void discriminant(){
        auto&g=s_.gp;auto&x=s_.xmm;x[0]=N::multiply(x[0],x[0]);duplicate();x[1]=x[0];N::multiply_second();
        x[1]=shuffle_y(x[1]);x[2]=x[0];g[0]=g[4]+12u;x[1]=add_scalar(x[1],x[0]);x[2]=shuffle_z(x[2]);x[2]=add_scalar(x[2],x[1]);
        Base::store(0x2e8ef0u,g[0],low(x[2]),[]{});floating(0x2e8ef4u,g[4]+12u);
        N::subtract(read(0x2e8ef8u,g[4]+8u));N::multiply(read(0x2e8efcu,g[4]));
        asm volatile(".intel_syntax noprefix;fsubp st(1),st(0);.att_syntax prefix":::"memory","st","st(1)");
    }
    bool intersects(){const auto result=compare(0x2e8f01u,0x46a0e8u,0x41,false);const auto pair=(result.status>>8u)&0x41u;return pair==0u||pair==0x41u;}
    bool root_before_endpoint(){
        asm volatile("fxch %%st(1);fchs;fxch %%st(1);fsqrt;.intel_syntax noprefix;fsubp st(1),st(0);.att_syntax prefix":::"memory","st","st(1)");
        N::add(read(0x2e8f28u,s_.gp[4]+4u));spill(0x2e8f2cu,s_.gp[4]+8u,false);
        return (compare(0x2e8f30u,s_.gp[4],1).status&0x100u)!=0u;
    }
    bool root_nonnegative(){floating(0x2e8f3au,s_.gp[4]+8u);return compare(0x2e8f3eu,0x46a0e8u,5).not_less();}
    void publish_hit(){
        auto&g=s_.gp;auto&x=s_.xmm;floating(0x2e8f4bu,g[4]+8u);g[0]=read(0x2e8f4fu,g[5]+16u);divide(read(0x2e8f52u,g[4]));
        duplicate();spill(0x2e8f57u,g[0]+0x14u,true);x[1]=Base::vector(0x2e8f5au,g[2]+16u);x[3]=Base::vector(0x2e8f5eu,0x41c500u);
        spill(0x2e8f65u,g[4]+12u,true);x[0]=N::packed({read(0x2e8f69u,g[4]+12u),0,0,0});floating(0x2e8f6fu,0x46a0d0u);
        x[0]=N::broadcast(low(x[0]));x[2]=N::multiply(x[0],x[1]);x[1]=Base::vector(0x2e8f7fu,g[2]);
        x[3]=N::subtract(x[3],x[0]);x[0]=N::multiply(x[3],x[1]);x[0]=N::add(x[0],x[2]);Base::vector_store(0x2e8f8eu,g[0],x[0]);
        divide(read(0x2e8f91u,g[1]+12u));Base::store(0x2e8f94u,g[0]+16u,0xffffffffu,[]{});spill(0x2e8f9bu,g[4]+12u,true);
        x[1]=N::packed({read(0x2e8f9fu,g[4]+12u),0,0,0});x[2]=N::broadcast(low(x[1]));x[2]=N::multiply(x[2],x[0]);Base::vector_store(0x2e8fafu,g[0],x[2]);
    }
    void finish(bool hit,unsigned discard){while(discard--)N::discard();boolean(hit?0x2e8fb5u:0x2e8f15u,hit);auto&g=s_.gp;g[4]=g[5];g[5]=pop(hit?0x2e8fbau:0x2e8f1au);pop(hit?0x2e8fbbu:0x2e8f1bu);g[4]+=12u;}
};
template<class State,class Before>bool run_private(State&s,Before&before){native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope};return cast(frame);}
} // namespace halo2::native_sphere_ray_cast_detail
