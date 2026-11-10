#pragma once
#include "halo2/native_tu15_segment_sphere_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#include <cstdlib>
#include <thread>
namespace halo2::native_segment_sphere_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
template<class State,class Before,class Scope>class Frame:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}}{}
    Scalar scalar(Read field,const Progress&d){
        std::uint32_t pc,address;int reg=-1;
        switch(field){
        case Read::center_y:pc=0x120fa0u;address=s_.gp[1]+4u;reg=4;break;
        case Read::first_y:pc=0x120fa5u;address=s_.gp[0]+4u;break;
        case Read::center_z:s_.xmm[4]=d.dy;pc=0x120faau;address=s_.gp[1]+8u;reg=5;break;
        case Read::first_z:pc=0x120fafu;address=s_.gp[0]+8u;break;
        case Read::center_x:s_.xmm[5]=d.dz;pc=0x120fb4u;address=s_.gp[1];reg=3;break;
        case Read::first_x:pc=0x120fb8u;address=s_.gp[0];break;
        case Read::radius:s_.xmm[0]=d.term;s_.xmm[2]=d.distance;s_.xmm[3]=d.dx;s_.xmm[6]=d.zero;pc=0x120fdcu;address=s_.gp[4]+4u;reg=0;break;
        case Read::scratch_z:pc=0x121009u;address=s_.gp[4]+8u;reg=1;break;
        case Read::scratch_y:pc=0x12101au;address=s_.gp[4]+4u;reg=5;break;
        case Read::scratch_x:pc=0x12102eu;address=s_.gp[4];reg=4;break;
        default:throw std::runtime_error("Invalid segment sphere scalar acquisition");
        }
        at(pc);const auto value=N::scalar(word(address));if(reg>=0)s_.xmm[unsigned(reg)]=value;return value;
    }
    void reserve(const Progress&d){s_.xmm[1]=d.radius_squared;s_.gp[4]=s_.flags.minus(s_.gp[4],12u,4);}
    std::uint32_t integer(Read field,const Progress&){
        unsigned reg;std::uint32_t pc,offset;
        switch(field){
        case Read::direction_x:reg=0;pc=0x120ffdu;offset=0;break;
        case Read::direction_y:reg=1;pc=0x120fffu;offset=4;break;
        case Read::direction_z:reg=2;pc=0x121002u;offset=8;break;
        default:throw std::runtime_error("Invalid segment sphere integer acquisition");
        }
        at(pc);const auto value=word(s_.gp[2]+offset);s_.gp[reg]=value;return value;
    }
    void store(Write field,const Progress&d,std::uint32_t value){
        std::uint32_t pc,offset;
        switch(field){
        case Write::scratch_z:pc=0x121005u;offset=8;break;
        case Write::scratch_y:s_.xmm[0]=d.dot;pc=0x121016u;offset=4;break;
        case Write::scratch_x:s_.xmm[0]=d.dot;s_.xmm[7]=d.product;pc=0x12102bu;offset=0;break;
        }
        Base::store(pc,s_.gp[4]+offset,value,[]{});
    }
    Comparison test(Test test,const Progress&d,Scalar first,Scalar second){
        auto&x=s_.xmm;std::uint32_t pc;
        switch(test){
        case Test::inside:
            pc=0x120ff0u;x[0]=d.radius;x[1]=d.radius_squared;x[2]=d.distance;x[3]=d.dx;x[4]=d.dy;x[5]=d.dz;x[6]=d.zero;break;
        case Test::approaching:
            pc=0x12103eu;x[0]=d.dot;x[1]=d.direction_z;x[2]=d.distance;x[3]=d.dx;x[4]=d.direction_x;x[5]=d.direction_y;x[6]=d.zero;x[7]=d.product;break;
        case Test::discriminant:
            pc=0x121077u;x[0]=d.dot;x[1]=d.length;x[2]=d.distance;x[3]=d.discriminant;x[4]=d.scaled_distance;break;
        case Test::endpoint:pc=0x121087u;x[2]=d.endpoint;break;
        case Test::endpoint_discriminant:pc=0x121097u;x[0]=d.endpoint_squared;break;
        }
        at(pc);const auto result=compare(first,second);s_.flags.load(result.flags);return result;
    }
    void finish(Result result,const Progress&){
        const bool accepted=result==Result::byte_true||result==Result::word_true;
        const unsigned width=result==Result::byte_false||result==Result::byte_true?1u:4u;
        s_.set(0,accepted?1u:s_.flags.logic(0u,width),width,0);
        s_.gp[4]=s_.flags.plus(s_.gp[4],12u,4);
        const auto pc=width==1u?(accepted?0x120ffau:0x121048u):(accepted?0x1210a4u:0x1210acu);
        pop(pc);s_.gp[4]+=4u;
    }
};
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SEGMENT_SPHERE");return p&&*p;}();return enabled;}
template<class State>bool admitted(const State&s){
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction)return !s.instruction_cursor&&!s.literal_service_sites&&!s.observe;
    const auto*c=s.instruction_cursor;
    return s.literal_service_sites&&c&&c==&s.services.instruction_cursor&&c->position&&c->callback_type&&
        s.services.instruction.target_type()==*c->callback_type&&c->executor==std::this_thread::get_id()&&
        c->literal_service_sites&&c->service_sites.size()==3&&c->service_sites[0]==0x38846eu&&
        c->service_sites[1]==0x21f590u&&c->service_sites[2]==0x33b595u&&
        !(c->first_trace&&*c->first_trace)&&!(c->second_trace&&*c->second_trace)&&
        (!s.observe||c->observation_only_stores);
}
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    const auto execute=[&](auto&scope){Frame frame{s,before,scope};Progress d;intersects(frame,d);};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};execute(scope);return;}
    native_raw_x87_detail::Scope scope{s};execute(scope);
}
template<class State>bool bridge(State&s){
    if(!admitted(s)||(!native_raw_x87_detail::supported(s)&&!native_sse_mmx_detail::supported(s)))return false;
    run_private(s);return true;
}
} // namespace halo2::native_segment_sphere_detail
#else
namespace halo2::native_segment_sphere_detail {template<class State>bool bridge(State&){return false;}}
#endif
