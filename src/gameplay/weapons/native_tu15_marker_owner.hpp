#pragma once
#include "halo2/native_tu15_marker_owner_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "gameplay/weapons/native_tu15_object_parent_mode.hpp"

namespace halo2::native_marker_owner_detail {
enum class Observation{actual,canonical};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before,class Call>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_model_marker_lookup_detail::Io<native_raw_x87_detail::Scope<State>>>{
    using Scope=native_raw_x87_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_model_marker_lookup_detail::Io<Scope>>;
    using Base::s_;using Base::word;
    Scope&scope_;Call&call_;Observation observation_;
    std::uint32_t pc(std::uint32_t actual)const{return observation_==Observation::actual?actual:actual-0x1140u;}
    void push(std::uint32_t actual,std::uint32_t value){Base::push(pc(actual),value);}
    std::uint32_t pop(std::uint32_t actual){return Base::pop(pc(actual));}
    std::uint32_t read(std::uint32_t actual,std::uint32_t address,unsigned width=4){Base::at(pc(actual));return word(address,width);}
public:
    Frame(State&s,Before&before,Scope&scope,Call&call,Observation observation):Base{s,before,{&scope}},scope_(scope),call_(call),observation_(observation){}
    void open_record(){
        auto&g=s_.gp;
        g[1]=read(0xbacd0u,s_.image==SourcePhysicsContactImage::TitleUpdate15?0x4ecbd0u:0x4e78d0u);g[2]=read(0xbacd6u,g[1]+0x44u);
        push(0xbacd9u,g[6]);push(0xbacdau,g[7]);
        g[7]=read(0xbacdbu,g[4]+12u);g[0]=s_.flags.logic(g[7]&65535u,4);g[0]*=3u;
        g[6]=read(0xbace9u,g[2]+g[0]*4u+8u);
    }
    std::uint32_t parent(){
        s_.gp[0]=read(0xbacedu,s_.gp[6]+0x14u);s_.flags.minus(s_.gp[0],0xffffffffu,4);return s_.gp[0];
    }
    bool inherits_marker(){
        auto&g=s_.gp;const auto datum=g[1]=read(0xbacf5u,g[6]);
        g[2]=read(0xbacf7u,s_.image==SourcePhysicsContactImage::TitleUpdate15?0x4f0414u:0x4eb114u);
        g[1]=(datum&65535u)<<4u;s_.flags.load(native_model_marker_lookup_detail::offset_flags(datum));
        g[1]=read(0xbad06u,g[1]+g[2]+8u);
        s_.set(2,read(0xbad0au,g[1]+2u,1),1,0);s_.set(2,s_.reg(2,1,0)>>4u,1,0);
        s_.flags.logic(s_.reg(2,1,0)&1u,1);return!s_.flags.zf;
    }
    void promote(std::uint32_t){
        auto&g=s_.gp;g[7]=pop(0xbad15u);g[6]=pop(0xbad16u);
        Base::store(pc(0xbad17u),g[4]+4u,g[0],[]{});
    }
    bool unit_candidate(){
        const auto flags=read(0xbad20u,s_.gp[6]+4u,1);s_.flags.logic(flags&1u,1);return!s_.flags.zf;
    }
    bool supported_kind(){
        s_.set(1,read(0xbad26u,s_.gp[6]+0xaau,1),1,0);
        s_.gp[0]=1u<<(s_.reg(1,1,0)&31u);s_.flags.logic(s_.gp[0]&0x1cu,1);return!s_.flags.zf;
    }
    bool pickup_parent(){
        s_.gp[1]=s_.gp[7];push(0xbad39u,s_.image==SourcePhysicsContactImage::TitleUpdate15?0xbad3eu:0xb9bfeu);
        scope_.child([&]{call_(s_,s_.image==SourcePhysicsContactImage::TitleUpdate15?0xfc510u:0xfb5f0u,pc(0xbad39u));});
        s_.flags.logic(s_.reg(0,1,0),1);return!s_.flags.zf;
    }
    std::uint32_t finish(bool inherited){
        auto&g=s_.gp;
        if(inherited){g[0]=read(0xbad42u,g[6]+0x154u);g[7]=pop(0xbad48u);g[6]=pop(0xbad49u);pop(0xbad4au);}
        else{g[0]=g[7];g[7]=pop(0xbad4fu);g[6]=pop(0xbad50u);pop(0xbad51u);}
        g[4]+=4u;return g[0];
    }
};
// Private TU1.5 dense-masked candidate. Admission/defaults and opaque-MMX child
// transport remain unqualified; callers provide the genuine complete child.
template<class State,class Call,class Before=Noop>
std::uint32_t run_private(State&s,Call call,Before before={},Observation observation=Observation::actual){
    native_raw_x87_detail::Scope scope{s};Frame view{s,before,scope,call,observation};return lookup(view);
}
} // namespace halo2::native_marker_owner_detail
#endif
