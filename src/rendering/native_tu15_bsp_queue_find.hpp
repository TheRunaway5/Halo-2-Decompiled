#pragma once
#include "halo2/native_tu15_id_search_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#endif

namespace halo2::native_bsp_queue_find_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_BSP_QUEUE_FIND");return p&&*p;}();return value;}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        const auto*c=s.instruction_cursor;
        if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
           s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
           !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
           c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
           (c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
    }else if(s.instruction_cursor||s.literal_service_sites)return false;
    // This leaf performs no stores and has no child. Its observer is unused.
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return native_sse_mmx_detail::supported(s);
    return native_raw_x87_detail::supported(s);
#else
    (void)s;return false;
#endif
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
template<class State,class Before,class Scope>class View:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;using Base::s_;using Base::at;using Base::word;using Base::pop;
public:
    View(State&s,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}}{}
    std::uint32_t count(){
        at(0x166c00u);s_.gp[1]=word(s_.gp[2]+4u,2);s_.gp[0]=s_.flags.logic(0,4);s_.flags.logic(s_.gp[1],4);return s_.gp[1];
    }
    void open_rows(){at(0x166c0au);s_.gp[2]=word(s_.gp[2]+12u);}
    std::uint32_t item(std::uint32_t){at(0x166c10u);return word(s_.gp[2]);}
    std::uint32_t key()const{return s_.gp[6];}
    std::uint32_t found(std::uint32_t,std::uint32_t value){s_.flags.minus(value,s_.gp[6],4);pop(0x166c20u);return s_.gp[0];}
    void advance(std::uint32_t,std::uint32_t value){
        s_.flags.minus(value,s_.gp[6],4);s_.gp[0]=s_.flags.increment(s_.gp[0],4,false);s_.gp[2]=s_.flags.plus(s_.gp[2],4u,4);s_.flags.minus(s_.gp[0],s_.gp[1],4);
    }
    std::uint32_t absent(){s_.set(0,s_.flags.logic((s_.gp[0]&0xffffu)|0xffffu,2),2,0);pop(0x166c20u);return s_.gp[0];}
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    const auto execute=[&](auto&scope){View view{s,before,scope};native_id_search_detail::find(view);};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};execute(scope);return true;}
    native_raw_x87_detail::Scope scope{s};execute(scope);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_bsp_queue_find_detail
