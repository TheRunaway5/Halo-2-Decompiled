#pragma once
#include "halo2/native_tu15_affine_inverse_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#include <thread>
#include <cstdlib>
namespace halo2::native_affine_inverse_detail {
enum class Observation {actual,canonical,world};
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
template<class State,class Before,class Scope>class Frame:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;Observation observation_;
    std::uint32_t pc(std::uint32_t actual)const{
        bool canonical=observation_==Observation::canonical;
        if(observation_==Observation::world){
            if constexpr(requires{s_.selected_observation_pcs;})canonical=!s_.selected_observation_pcs||s_.image!=SourcePhysicsContactImage::TitleUpdate15;
            else canonical=true;
        }
        return canonical?actual-0x1390u:actual;
    }
    void at(std::uint32_t actual){Base::at(pc(actual));}
    void scalar(unsigned reg,std::uint32_t value){s_.xmm[reg]=N::scalar(value);}
    std::uint32_t pop(std::uint32_t actual){at(actual);const auto value=Base::word(s_.gp[4]);s_.gp[4]+=4u;return value;}
public:
    Frame(State&s,Before&before,Scope&scope,Observation observation):Base{s,before,Io<Scope>{&scope}},observation_(observation){}
    void begin(const Progress&){scalar(3,0);const auto address=s_.gp[4]-4u;Base::store(pc(0x144097u),address,s_.gp[7],[&]{s_.gp[4]=address;});}
    bool zero_scale(const Progress&d){
        at(0x144098u);const auto comparison=compare(d.initial_scale,0);s_.flags.load(comparison.flags);
        s_.gp[7]=s_.gp[0];s_.set(0,comparison.flags,1,8);s_.flags.logic(s_.reg(0,1,8)&0x44u,1);return comparison.same();
    }
    bool unit_scale(const Progress&d){
        at(0x1440b3u);const auto comparison=compare(d.retained_scale,d.one);s_.flags.load(comparison.flags);
        s_.set(0,comparison.flags,1,8);s_.flags.logic(s_.reg(0,1,8)&0x44u,1);return comparison.same();
    }
    void clear_begin(const Progress&){s_.gp[1]=13u;s_.gp[0]=s_.flags.logic(0u,4);}
    void clear_word(const Progress&,unsigned){
        Base::store(pc(0x1441b7u),s_.gp[7],0u,[&]{s_.gp[7]+=(s_.other_flags&0x400u)?0xfffffffcu:4u;--s_.gp[1];});
    }
    std::uint32_t word(Read field,const Progress&d){
        std::uint32_t actual{},address{};int vector=-1,integer=-1;
        switch(field){
        case Read::scale:actual=0x144090u;address=s_.gp[1];vector=0;break;
        case Read::retained_scale:actual=0x1440a7u;address=s_.gp[1];vector=4;break;
        case Read::one:actual=0x1440abu;address=s_.image==SourcePhysicsContactImage::TitleUpdate15?0x46a0d0u:0x465030u;vector=2;break;
        case Read::position_x:scalar(0,0);actual=0x1440bdu;address=s_.gp[1]+40u;break;
        case Read::position_y:scalar(0,d.px);scalar(1,0);actual=0x1440c5u;address=s_.gp[1]+44u;break;
        case Read::position_z:scalar(1,d.py);actual=0x1440cau;address=s_.gp[1]+48u;break;
        case Read::forward_x:
            scalar(0,d.px);scalar(1,d.py);scalar(3,d.pz);scalar(2,d.unit_scale?d.one:d.pz);scalar(4,d.unit_scale?d.retained_scale:d.py);
            actual=0x1440fau;address=s_.gp[1]+4u;integer=0;break;
        case Read::left_y:actual=0x144100u;address=s_.gp[1]+20u;integer=2;break;
        case Read::output_forward_x:actual=0x144103u;address=s_.gp[7]+4u;vector=5;break;
        case Read::up_z:actual=0x14410bu;address=s_.gp[1]+36u;integer=0;break;
        case Read::left_x:actual=0x144111u;address=s_.gp[1]+16u;vector=2;break;
        case Read::forward_y:actual=0x144116u;address=s_.gp[1]+8u;integer=2;break;
        case Read::up_x:actual=0x144121u;address=s_.gp[1]+28u;vector=2;break;
        case Read::forward_z:actual=0x144126u;address=s_.gp[1]+12u;integer=0;break;
        case Read::output_up_x:actual=0x144130u;address=s_.gp[7]+28u;vector=4;break;
        case Read::output_left_x:scalar(4,d.tx);actual=0x14413du;address=s_.gp[7]+16u;vector=5;break;
        case Read::up_y:actual=0x144147u;address=s_.gp[1]+32u;vector=2;break;
        case Read::left_z:actual=0x14414cu;address=s_.gp[1]+24u;integer=1;break;
        case Read::output_forward_y:scalar(4,d.tx);scalar(5,d.left_product);actual=0x144157u;address=s_.gp[7]+8u;vector=5;break;
        case Read::output_left_z:actual=0x144168u;address=s_.gp[7]+32u;vector=4;break;
        case Read::output_left_y:scalar(4,d.ty);scalar(5,d.py);actual=0x144178u;address=s_.gp[7]+20u;break;
        case Read::output_forward_z:actual=0x144186u;address=s_.gp[7]+12u;vector=4;break;
        case Read::output_up_z:scalar(4,d.tz);actual=0x14418fu;address=s_.gp[7]+36u;vector=0;break;
        }
        at(actual);const auto value=Base::word(address);if(vector>=0)scalar(unsigned(vector),value);if(integer>=0)s_.gp[unsigned(integer)]=value;return value;
    }
    void store(Write field,const Progress&d,std::uint32_t value){
        std::uint32_t actual{},offset{};
        switch(field){
        case Write::scale:
            scalar(0,d.px);scalar(1,d.py);scalar(3,d.pz);
            if(d.unit_scale){actual=0x1440f6u;scalar(2,d.one);scalar(4,d.retained_scale);}
            else{actual=0x1440e2u;scalar(2,d.reciprocal);scalar(4,d.reciprocal);}break;
        case Write::forward_x:actual=0x1440fdu;offset=4;break;
        case Write::left_y:actual=0x144108u;offset=20;break;
        case Write::up_z:actual=0x14410eu;offset=36;break;
        case Write::forward_y:actual=0x144119u;offset=8;break;
        case Write::left_x:actual=0x14411eu;offset=16;break;
        case Write::up_x:scalar(5,d.forward_product);actual=0x14412du;offset=28;break;
        case Write::forward_z:actual=0x144142u;offset=12;break;
        case Write::position_x:actual=0x14415cu;offset=40;break;
        case Write::left_z:scalar(5,d.ty);actual=0x144165u;offset=32;break;
        case Write::position_y:scalar(4,d.ty);scalar(5,d.final_y_term);actual=0x144181u;offset=44;break;
        case Write::up_y:actual=0x144194u;offset=24;break;
        case Write::position_z:scalar(0,d.up_product);scalar(2,d.uy);scalar(4,d.tz);actual=0x1441a9u;offset=48;break;
        }
        Base::store(pc(actual),s_.gp[7]+offset,value,[]{});
    }
    void finish(const Progress&,bool cleared){s_.gp[7]=pop(cleared?0x1441b9u:0x1441aeu);pop(cleared?0x1441bau:0x1441afu);}
};
template<class State,class Before=Noop>void run_private(State&s,Before before={},Observation observation=Observation::actual){
    const auto execute=[&](auto&scope){Frame frame{s,before,scope,observation};Progress d;d.source=s.gp[1];d.destination=s.gp[0];invert(frame,d);};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};execute(scope);return;}
    native_raw_x87_detail::Scope scope{s};execute(scope);
}
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_AFFINE_INVERSE");return p&&*p;}();return enabled;}
template<class State>bool admitted(const State&s,Observation observation){
    if(reference()||(s.image!=SourcePhysicsContactImage::TitleUpdate15&&s.image!=SourcePhysicsContactImage::MapPack11))return false;
    // Actual-named generated copies own the TU1.5 operand image only. The
    // canonical/world owners retain the separately qualified disc mapping.
    if(observation==Observation::actual&&s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if(!s.services.instruction)return !s.instruction_cursor&&!s.literal_service_sites&&!s.observe;
    const auto*c=s.instruction_cursor;
    return s.literal_service_sites&&c&&c==&s.services.instruction_cursor&&c->position&&c->callback_type&&
        s.services.instruction.target_type()==*c->callback_type&&c->executor==std::this_thread::get_id()&&
        c->literal_service_sites&&c->service_sites.size()==3&&c->service_sites[0]==0x38846eu&&
        c->service_sites[1]==0x21f590u&&c->service_sites[2]==0x33b595u&&
        !(c->first_trace&&*c->first_trace)&&!(c->second_trace&&*c->second_trace)&&
        (!s.observe||c->observation_only_stores);
}
template<class State>bool bridge(State&s,Observation observation=Observation::actual){
    if(!admitted(s,observation)||(!native_raw_x87_detail::supported(s)&&!native_sse_mmx_detail::supported(s)))return false;
    run_private(s,Noop{},observation);return true;
}
} // namespace halo2::native_affine_inverse_detail
#else
namespace halo2::native_affine_inverse_detail {
enum class Observation {actual,canonical,world};
template<class State>bool bridge(State&,Observation=Observation::actual){return false;}
}
#endif
