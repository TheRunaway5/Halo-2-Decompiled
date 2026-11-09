#pragma once
#include "halo2/native_tu15_camera_instance_bitmap_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#include "physics/runtime/native_tu15_bsp_surface_polygon.hpp"
#include <cstdlib>
#include <thread>
namespace halo2::native_camera_instance_bitmap_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_CAMERA_INSTANCE_BITMAP");return p&&*p;}();return enabled;}
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
template<class State>std::uint32_t ordered_shift(State&s,std::uint32_t value,unsigned count){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    s.flags.load(native_bsp_surface_polygon_detail::shifted_flags(value,count,s.flags.bits()));return value<<count;
#else
    return s.flags.shift(value,count,4);
#endif
}
template<class State,class Before,class Scope>class Frame:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}}{}
    std::uint32_t instance()const{return s_.gp[0];}
    std::uint32_t cluster(const Progress&d){
        s_.flags.minus(d.instance,0xffffffffu,4);at(d.per_instance?0x186e4au:0x186e35u);
        const auto bits=word(0x474c24u,2);const auto value=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(bits))));
        s_.gp[d.per_instance?2:0]=value;return bits;
    }
    std::uint32_t base(const Progress&d){at(d.per_instance?0x186e51u:0x186e3cu);return s_.gp[1]=word(0x4f9b50u);}
    void finish(const Progress&d){
        if(d.per_instance){s_.gp[2]=ordered_shift(s_,d.cluster,10u);s_.gp[2]=s_.flags.plus(s_.gp[2],d.base,4);}
        else s_.gp[0]=ordered_shift(s_,d.cluster,5u);
        s_.gp[0]=d.address;pop(d.per_instance?0x186e63u:0x186e49u);
    }
};
template<class State>bool policy_admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction)return !s.instruction_cursor&&!s.literal_service_sites;
    if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
    const auto*p=s.instruction_cursor;
    return p==&s.services.instruction_cursor&&p->callback_type&&s.services.instruction.target_type()==*p->callback_type&&
        p->executor==std::this_thread::get_id()&&p->literal_service_sites&&p->service_sites.size()==3&&
        p->service_sites[0]==0x38846eu&&p->service_sites[1]==0x21f590u&&p->service_sites[2]==0x33b595u&&
        !(p->first_trace&&*p->first_trace)&&!(p->second_trace&&*p->second_trace);
#endif
}
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    auto execute=[&](auto&scope){Frame frame{s,before,scope};Progress d;locate(frame,d);frame.finish(d);};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};execute(scope);return;}
    native_raw_x87_detail::Scope scope{s};execute(scope);
}
template<class State>bool bridge(State&s){
    if(!policy_admitted(s)||(!native_raw_x87_detail::supported(s)&&!native_sse_mmx_detail::supported(s)))return false;
    run_private(s);return true;
}
} // namespace halo2::native_camera_instance_bitmap_detail
