#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_camera_instance_query_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <stdexcept>
#include <cstdlib>
#include <thread>

namespace halo2::native_camera_instance_query_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_CAMERA_INSTANCE_QUERY");return p&&*p;}();return enabled;}
template<class Scope>struct Io {Scope*scope;template<class Function>void operator()(Function&&f){scope->io(std::forward<Function>(f));}};
template<unsigned Factor,class State>std::uint32_t product(State&s,std::uint32_t value){
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);unsigned overflow{};
    asm volatile("addb $127,%%al;sahf;imull %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(overflow),"+r"(value):"i"(Factor):"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
}
// Raw caller transport for the shared geometric DATA policy. Child dispatch
// executes the namespace's genuine owners with their live returned ABI.
template<class State,class Before,class Scope,class Dispatch>class Frame:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    Scope&scope_;Dispatch&dispatch_;
    void child(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){push(pc,returned);scope_.child([&]{dispatch_(s_,target,pc);});}
    static std::uint32_t bits(__m128 value){return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];}
    static __m128 packed(std::uint32_t value){return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});}
public:
    using Scalar=__m128;using Data=Progress<Scalar>;
    Frame(State&s,Before&before,Scope&scope,Dispatch&dispatch):Base{s,before,Io<Scope>{&scope}},scope_(scope),dispatch_(dispatch){}
    std::uint32_t begin(const Data&){auto&g=s_.gp;g[4]=s_.flags.minus(g[4],0x468u,4);push(0x16c006u,g[5]);push(0x16c007u,g[6]);push(0x16c008u,g[7]);g[6]=g[0];return g[6];}
    std::uint32_t word(Read kind,const Data&d){
        auto&g=s_.gp;std::uint32_t pc{},address{};unsigned width=4,reg{};
        switch(kind){
        case Read::scenario:pc=0x16c00bu;address=0x4ecc18u;reg=0;break;
        case Read::instances:pc=0x16c010u;address=g[0]+0x144u;reg=2;break;
        case Read::definitions:pc=0x16c016u;address=g[0]+0x13cu;reg=1;break;
        case Read::mask:pc=0x16c01cu;address=g[4]+0x478u;reg=0;break;
        case Read::definition_index:g[7]=product<0x58u>(s_,g[6]);pc=0x16c028u;address=g[2]+g[7]+0x34u;reg=5;width=2;break;
        case Read::fraction:at(0x16c20au);g[1]=Base::word(g[4]+0x484u);at(0x16c211u);s_.xmm[0]=packed(Base::word(g[3]+4u));return bits(s_.xmm[0]);
        case Read::material:at(0x16c229u);s_.set(0,Base::word(g[4]+0x6eu,2),2,0);g[1]=s_.flags.logic(0xffffffffu,4);s_.flags.minus(g[0]&0xffffu,g[1]&0xffffu,2);return g[0]&0xffffu;
        case Read::material_scenario:g[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(d.material))));g[2]=g[0]+g[0]*4u;pc=0x16c242u;address=0x4ecc18u;reg=0;break;
        case Read::material_table:pc=0x16c247u;address=g[0]+0x10u;reg=0;break;
        case Read::selected_material:
            if(std::uint16_t(d.material)==0xffffu)g[0]=0x48a0b0u;else g[0]=g[0]+g[2]*4u+8u;
            at(0x16c255u);s_.set(2,Base::word(g[0],2),2,0);return g[2]&0xffffu;
        default:throw std::runtime_error("Invalid native instance-query word DATA");
        }
        at(pc);const auto value=Base::word(address,width);
        g[reg]=kind==Read::definition_index?std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value)))):value;
        return value;
    }
    bool admit(const Data&){auto&g=s_.gp;g[7]=s_.flags.plus(g[7],g[2],4);g[5]=product<0xc8u>(s_,g[5]);g[5]=s_.flags.plus(g[5],g[1],4);push(0x16c037u,g[0]);g[2]=g[5];child(0x16c03au,0x16b810u,0x16c03fu);s_.flags.logic(g[0]&255u,1);return(g[0]&255u)!=0;}
    void resume(Child,Data&d){d.index=s_.gp[6];d.instance=s_.gp[7];d.definition=s_.gp[5];}
    bool returned_sphere(const Data&){s_.flags.logic(s_.gp[0]&255u,1);return(s_.gp[0]&255u)!=0;}
    bool near_sphere(const Data&d){auto&g=s_.gp;at(0x16c047u);g[1]=Base::word(g[7]+0x48u);at(0x16c04au);g[2]=Base::word(g[4]+0x484u);push(0x16c051u,g[1]);at(0x16c052u);g[1]=Base::word(g[4]+0x484u);g[0]=g[7]+0x3cu;child(0x16c05cu,0x120fa0u,0x16c061u);return returned_sphere(d);}
    bool finish(bool found,const Data&){auto&g=s_.gp;g[7]=pop(found?0x16c272u:0x16c280u);g[6]=pop(found?0x16c273u:0x16c281u);
        if(found)s_.set(0,1u,1,0);else s_.set(0,s_.flags.logic(0u,1),1,0);
        g[5]=pop(found?0x16c276u:0x16c284u);g[4]=s_.flags.plus(g[4],0x468u,4);pop(found?0x16c27du:0x16c28bu);g[4]+=16u;return found;}
    void inverse(const Data&){auto&g=s_.gp;g[0]=g[4]+12u;g[1]=g[7];child(0x16c06fu,0x144090u,0x16c074u);}
    void phase(Phase kind,const Data&d){auto&g=s_.gp;
        switch(kind){
        case Phase::origin_pointer:at(0x16c074u);g[0]=Base::word(g[4]+0x480u);break;
        case Phase::origin_decision:s_.set(0,s_.flags.bits()|2u,1,8);s_.flags.logic(s_.reg(0,1,8)&0x44u,1);break;
        case Phase::displacement_pointer:at(0x16c0ccu);g[0]=Base::word(g[4]+0x484u);break;
        case Phase::displacement_decision:s_.xmm[7]=d.z;s_.set(0,s_.flags.bits()|2u,1,8);s_.flags.logic(s_.reg(0,1,8)&0x44u,1);break;
        case Phase::bitmap_arguments:s_.xmm[0]=d.term;g[2]=g[4]+0x4cu;break;
        case Phase::bitmap_index:s_.xmm[0]=d.term;s_.xmm[2]=d.delta_z;g[0]=g[6];break;
        }
    }
    Scalar scalar(Read kind,const Data&d){auto&x=s_.xmm;auto&g=s_.gp;std::uint32_t pc{},address{};int destination=-1;
        switch(kind){
        case Read::inverse_scale:pc=0x16c07bu;address=g[4]+12u;destination=6;break;
        case Read::origin_one:pc=0x16c081u;address=0x46a0d0u;break;
        case Read::origin_x:pc=0x16c088u;address=g[0];destination=3;break;
        case Read::origin_y:pc=0x16c08cu;address=g[0]+4u;destination=4;break;
        case Read::origin_z:pc=0x16c091u;address=g[0]+8u;destination=5;break;
        case Read::matrix_28:if(d.scale_origin){x[0]=d.oz;x[3]=d.ox;x[4]=d.oy;x[5]=d.oz;}pc=0x16c0bau;address=g[4]+0x28u;destination=0;break;
        case Read::matrix_1c:pc=0x16c0c0u;address=g[4]+0x1cu;destination=2;break;
        case Read::matrix_20:pc=0x16c0c6u;address=g[4]+0x20u;destination=7;break;
        case Read::displacement_one:x[2]=d.m1c_y;pc=0x16c0d7u;address=0x46a0d0u;break;
        case Read::matrix_10:x[1]=d.x;pc=0x16c0e9u;address=g[4]+0x10u;destination=2;break;
        case Read::matrix_34:x[1]=d.x;x[2]=d.term;pc=0x16c0f7u;address=g[4]+0x34u;break;
        case Read::matrix_2c:pc=0x16c107u;address=g[4]+0x2cu;destination=1;break;
        case Read::matrix_14:x[2]=d.y;pc=0x16c118u;address=g[4]+0x14u;destination=7;break;
        case Read::matrix_38:x[2]=d.y;x[7]=d.term;pc=0x16c126u;address=g[4]+0x38u;break;
        case Read::matrix_30:pc=0x16c132u;address=g[4]+0x30u;destination=2;break;
        case Read::matrix_24:x[7]=d.z;pc=0x16c13fu;address=g[4]+0x24u;destination=5;break;
        case Read::matrix_18:x[5]=d.term;pc=0x16c149u;address=g[4]+0x18u;destination=4;break;
        case Read::displacement_z:x[7]=d.z;pc=0x16c153u;address=g[0]+8u;destination=5;break;
        case Read::displacement_x:x[4]=d.term;pc=0x16c15cu;address=g[0];destination=3;break;
        case Read::displacement_y:x[7]=d.z;pc=0x16c164u;address=g[0]+4u;destination=4;break;
        case Read::matrix_3c:pc=0x16c169u;address=g[4]+0x3cu;break;
        case Read::delta_matrix_1c:x[3]=d.dx;x[4]=d.dy;x[5]=d.dz;pc=0x16c187u;address=g[4]+0x1cu;destination=6;break;
        case Read::delta_matrix_10:x[0]=d.delta_x;x[6]=d.term;pc=0x16c199u;address=g[4]+0x10u;destination=6;break;
        case Read::delta_matrix_20:pc=0x16c1adu;address=g[4]+0x20u;destination=0;break;
        case Read::delta_matrix_14:x[0]=d.term;x[1]=d.delta_y;pc=0x16c1bfu;address=g[4]+0x14u;destination=0;break;
        case Read::delta_matrix_24:x[0]=d.term;x[1]=d.delta_y;pc=0x16c1cdu;address=g[4]+0x24u;destination=0;break;
        case Read::delta_matrix_18:x[2]=d.delta_z;pc=0x16c1e3u;address=g[4]+0x18u;destination=0;break;
        default:throw std::runtime_error("Invalid native instance-query scalar DATA");
        }
        at(pc);const auto value=packed(Base::word(address));if(destination>=0)x[unsigned(destination)]=value;return value;
    }
    Scalar multiply(Scalar left,Scalar right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
    Scalar add(Scalar left,Scalar right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
    bool different(Scalar left,Scalar right,Comparison,const Data&){s_.flags.scalar_compare(left,right,true);return!s_.flags.zf||s_.flags.pf;}
    void store(Write kind,Scalar value,const Data&d){auto&x=s_.xmm;std::uint32_t pc{},offset{};
        switch(kind){
        case Write::origin_x:x[1]=d.x;x[7]=d.m20_y;pc=0x16c101u;offset=0x40u;break;
        case Write::origin_y:x[2]=d.y;pc=0x16c12cu;offset=0x44u;break;
        case Write::origin_z:x[7]=d.z;pc=0x16c173u;offset=0x48u;break;
        case Write::delta_x:x[0]=d.delta_x;x[6]=d.term;pc=0x16c1a7u;offset=0x4cu;break;
        case Write::delta_y:x[1]=d.delta_y;pc=0x16c1f9u;offset=0x58u;break;
        case Write::delta_z:x[2]=d.delta_z;pc=0x16c1ffu;offset=0x5cu;break;
        default:throw std::runtime_error("Invalid native instance-query scalar output");
        }
        Base::store(pc,s_.gp[4]+offset,bits(value),[]{});
    }
    void prepare_bitmap(const Data&){auto&g=s_.gp;push(0x16c1e9u,g[2]);g[0]=g[4]+0x44u;push(0x16c1eeu,g[0]);}
    std::uint32_t bitmap(const Data&){child(0x16c205u,0x186e30u,0x16c20au);return s_.gp[0];}
    bool returned_bsp(const Data&){s_.flags.logic(s_.gp[0]&255u,1);return(s_.gp[0]&255u)!=0;}
    bool bsp(const Data&d){auto&g=s_.gp;push(0x16c216u,g[0]);push(0x16c217u,8u);g[2]=g[5]+0x70u;g[0]=g[4]+0x68u;child(0x16c220u,0x1e68e0u,0x16c225u);return returned_bsp(d);}
    std::uint32_t default_material(const Data&){return 0x48a0b0u;}
    void store_word(Write kind,std::uint32_t value,const Data&){auto&g=s_.gp;std::uint32_t pc{},address{};unsigned width=4;
        switch(kind){
        case Write::result_kind:pc=0x16c234u;address=g[3];break;
        case Write::material:pc=0x16c258u;address=g[3]+0x24u;width=2;value=g[2];break;
        case Write::object:pc=0x16c25cu;address=g[3]+0x38u;value=g[1];break;
        case Write::region:pc=0x16c25fu;address=g[3]+0x40u;value=g[1];break;
        case Write::instance:g[1]=g[7];g[2]=g[4]+0x58u;g[0]=g[3];pc=0x16c26au;address=g[3]+0x3cu;value=g[6];break;
        default:throw std::runtime_error("Invalid native instance-query word output");
        }
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);Base::io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        Base::io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);Base::io([&]{s_.observe(event);});}
    }
    void publish(const Data&){child(0x16c26du,0x16b9a0u,0x16c272u);}

};
template<class State,class Before,class Dispatch>void run_private(State&s,Before before,Dispatch dispatch){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope,dispatch};Progress<__m128>d;query(frame,d);
}template<class State,class Before,class Dispatch>void run_resolved_private(State&s,Before before,Dispatch dispatch){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope,dispatch};Progress<__m128>d;
    d.index=s.gp[6];d.instance=s.gp[7];d.definition=s.gp[5];query_resolved(frame,d);
}
template<class State,class Before,class Dispatch>void run_returned_bsp_private(State&s,Before before,Dispatch dispatch){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope,dispatch};Progress<__m128>d;
    d.index=s.gp[6];d.instance=s.gp[7];d.definition=s.gp[5];
    if(!frame.returned_bsp(d))frame.finish(false,d);
    else {frame.resume(Child::bsp,d);query_hit(frame,d);}
}
template<class State,class Before,class Dispatch>void run_returned_sphere_private(State&s,Before before,Dispatch dispatch){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope,dispatch};Progress<__m128>d;
    d.index=s.gp[6];d.instance=s.gp[7];d.definition=s.gp[5];
    if(!frame.returned_sphere(d))frame.finish(false,d);
    else {frame.resume(Child::sphere,d);frame.inverse(d);}
}
template<class State>bool admitted(const State&s){
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||s.fp.count>6u||(s.other_flags&0x400u))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction)return !s.instruction_cursor&&!s.literal_service_sites&&!s.observe;
    const auto*c=s.instruction_cursor;
    if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
       s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
       !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
       c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
       (c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace)||
       (s.observe&&!c->observation_only_stores))return false;
    return true;
}
template<class State,class Dispatch,class Before=Noop>bool bridge(State&s,Dispatch dispatch,Before before={}){
    if(!admitted(s)||!native_raw_x87_detail::supported(s))return false;
    run_private(s,before,dispatch);return true;
}

}
#else
namespace halo2::native_camera_instance_query_detail {
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
}
#endif
