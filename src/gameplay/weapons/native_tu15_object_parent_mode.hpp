#pragma once
#include "halo2/native_tu15_object_parent_mode_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "gameplay/weapons/native_tu15_model_marker_lookup.hpp"

namespace halo2::native_object_parent_mode_detail {
enum class Observation {actual,canonical};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before,class Scope>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_model_marker_lookup_detail::Io<Scope>>{
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_model_marker_lookup_detail::Io<Scope>>;
    using Base::s_;using Base::word;Observation observation_;
    std::uint32_t pc(std::uint32_t actual)const{return observation_==Observation::actual?actual:actual-0xf20u;}
    std::uint32_t read(std::uint32_t actual,std::uint32_t address,unsigned width=4){
        Base::at(pc(actual));return word(address,width);
    }
public:
    Frame(State&s,Before&before,Scope&scope,Observation observation):
        Base{s,before,native_model_marker_lookup_detail::Io<Scope>{&scope}},observation_(observation){}
    std::uint32_t objects(){
        return s_.gp[2]=read(0xfc510u,s_.image==SourcePhysicsContactImage::TitleUpdate15?0x4ecbd0u:0x4e78d0u);
    }
    std::uint32_t rows(std::uint32_t objects){return s_.gp[2]=read(0xfc516u,objects+0x44u);}
    std::uint32_t record(std::uint32_t rows,std::uint32_t index){
        s_.gp[1]=s_.flags.logic(index,4);s_.gp[1]*=3u;
        return s_.gp[1]=read(0xfc522u,rows+s_.gp[1]*4u+8u);
    }
    std::uint8_t flags(std::uint32_t record){
        const auto value=read(0xfc526u,record+0x12cu,1);
        s_.set(2,value,1,0);return std::uint8_t(value);
    }
    bool finish(std::uint8_t flags,bool result){
        s_.set(0,0u,1,0);s_.flags.logic(flags&1u,1);
        if(flags&1u){s_.set(1,flags>>1u,1,0);s_.flags.logic((flags>>1u)&1u,1);}
        if(result)s_.set(0,1u,1,0);
        Base::pop(pc(0xfc53eu));return result;
    }
};
template<class State,class Before,class Scope>
bool run_scoped(State&s,Before&before,Scope&scope,Observation observation){
    const auto datum=s.gp[1];Frame view{s,before,scope,observation};return lookup(view,datum);
}
template<class State,class Before=Noop>
bool run_private(State&s,Before before={},Observation observation=Observation::actual){
    if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};return run_scoped(s,before,scope,observation);}
    native_raw_x87_detail::Scope scope{s};return run_scoped(s,before,scope,observation);
}
} // namespace halo2::native_object_parent_mode_detail
#endif
