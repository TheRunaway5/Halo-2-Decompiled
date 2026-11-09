#pragma once
#include "halo2/native_tu15_camera_instance_admission_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#include <cstdlib>
#include <thread>

namespace halo2::native_camera_instance_admission_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_CAMERA_INSTANCE_ADMISSION");return p&&*p;}();return enabled;}
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
template<class State,class Before,class Scope>class Frame:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}}{}
    std::uint32_t instance()const{return s_.gp[6];}
    std::uint32_t definition()const{return s_.gp[2];}
    std::uint32_t read(Read kind,const Progress&d){
        std::uint32_t pc{},address{};unsigned width=4;int destination=-1;
        switch(kind){
        case Read::count:pc=0x16b810u;address=s_.gp[2]+0x98u;destination=1;break;
        case Read::rows:pc=0x16b81cu;address=s_.gp[2]+0x9cu;destination=2;break;
        case Read::last_flags:pc=0x16b822u;address=s_.gp[2]+s_.gp[1]*8u-4u;width=1;break;
        case Read::mask:pc=0x16b829u;address=s_.gp[4]+4u;break;
        case Read::scenario:pc=0x16b833u;address=0x4ecc18u;destination=1;break;
        case Read::cluster_count:pc=0x16b839u;address=s_.gp[1]+0xc4u;destination=2;break;
        case Read::cluster_table:pc=0x16b843u;address=s_.gp[1]+0xc8u;destination=1;break;
        case Read::table_count:pc=0x16b851u;address=s_.gp[1]+0x40u;break;
        case Read::mapping_rows:pc=0x16b856u;address=s_.gp[1]+0x44u;destination=1;break;
        case Read::mapping_entry:pc=0x16b859u;address=s_.gp[1]+s_.gp[6]*4u;width=2;break;
        }
        at(pc);const auto value=word(address,width);if(destination>=0)s_.gp[unsigned(destination)]=value;
        switch(kind){
        case Read::count:s_.flags.logic(value,4);s_.set(0,1u,1,0);break;
        case Read::last_flags:s_.flags.logic(value&0x10u,1);break;
        case Read::mask:s_.flags.logic(value&0x40000u,4);break;
        case Read::cluster_count:case Read::cluster_table:s_.flags.logic(value,4);break;
        case Read::table_count:s_.flags.minus(d.instance,value,4);break;
        case Read::mapping_entry:s_.flags.minus(value,0xffffffffu,2);break;
        default:break;
        }
        return value;
    }
    void index(const Progress&d){s_.flags.logic(d.instance,4);}
    void finish(bool accepted){if(!accepted)s_.set(0,s_.flags.logic(0u,1),1,0);pop(0x16b862u);s_.gp[4]+=4u;}
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
    auto execute=[&](auto&scope){Frame frame{s,before,scope};Progress d;frame.finish(admit(frame,d));};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};execute(scope);return;}
    native_raw_x87_detail::Scope scope{s};execute(scope);
}
template<class State>bool bridge(State&s){
    if(!policy_admitted(s)||(!native_raw_x87_detail::supported(s)&&!native_sse_mmx_detail::supported(s)))return false;
    run_private(s);return true;
}
} // namespace halo2::native_camera_instance_admission_detail
