#pragma once
#include "halo2/native_tu15_geometry_visibility_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_bsp_surface_2d_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "rendering/native_tu15_cluster_frustum_abi.hpp"
#include "rendering/native_tu15_bsp_queue_find.hpp"
#endif

namespace halo2::native_geometry_visibility_detail {
enum class Child {frustum,auxiliary};
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_GEOMETRY_VISIBILITY");return p&&*p;}();return value;}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>3)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        const auto*c=s.instruction_cursor;
        if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
           s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
           !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
           c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
           (s.observe&&!c->observation_only_stores)||(c->first_trace&&*c->first_trace)||
           (c->second_trace&&*c->second_trace))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;
    return true;
#else
    (void)s;return false;
#endif
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace N=native_bsp_surface_2d_detail;
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
struct Comparison {bool below{};std::uint32_t flags{};};
inline Comparison compare(N::Scalar left,N::Scalar right){
    unsigned char below,equal,unordered;
    asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(left),"x"(right):"cc","memory");
    return{below!=0,2u|std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u)};
}
template<class State,class Call,class Before>class View:public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_raw_x87_detail::Scope<State>>> {
    using Scope=native_raw_x87_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    Call&call_;Scope&scope_;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4){at(pc);return word(address,width);}
    void byte_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        const std::array bytes{std::byte(value)};io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
public:
    using Scalar=N::Scalar;
    View(State&s,Call&call,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}},call_(call),scope_(scope){}
    static Scalar add(Scalar a,Scalar b){return N::add(a,b);}
    static Scalar subtract(Scalar a,Scalar b){return N::subtract(a,b);}
    static Scalar multiply(Scalar a,Scalar b){return N::multiply(a,b);}
    void begin(){
        auto&g=s_.gp;g[2]=read(0x135970u,g[4]+0x20u);
        push(0x135974u,g[3]);push(0x135975u,g[5]);g[5]=read(0x135976u,g[4]+0x20u);
        push(0x13597au,g[6]);g[6]=g[0];g[0]=read(0x13597du,g[4]+0x28u);s_.set(3,1u,1,0);
        byte_store(0x135983u,g[0],g[3]);byte_store(0x135985u,g[2],g[3]);byte_store(0x135987u,g[5],0u);
    }
    unsigned mode(){
        s_.gp[0]=read(0x13598bu,s_.gp[6]+0x2a7cu);s_.flags.logic(s_.gp[0],4);
        if(s_.gp[0])s_.flags.minus(s_.gp[0],1u,4);
        return s_.gp[0];
    }
    bool skip(){const auto value=read(0x135999u,s_.gp[4]+0x20u,1);s_.set(0,value,1,0);s_.flags.logic(value,1);return value!=0;}
    void accept_skip(){s_.set(3,1u,1,0);}
    bool frustum(){
        auto&g=s_.gp;g[0]=std::uint32_t(std::int32_t(std::int16_t(read(0x1359a1u,g[4]+0x1cu,2))));
        s_.xmm[7]=N::scalar(read(0x1359a6u,g[4]+0x10u));push(0x1359acu,g[7]);g[7]=read(0x1359adu,g[6]);
        push(0x1359afu,g[5]);push(0x1359b0u,g[1]);push(0x1359b1u,0x1359b6u);
        scope_.child([&]{call_(s_,Child::frustum,before_);});
        s_.set(3,g[0]&255u,1,0);s_.flags.logic(g[3]&255u,1);g[7]=pop(0x1359bau);return (g[3]&255u)!=0;
    }
    bool contained(){const auto value=read(0x1359bdu,s_.gp[5],1);s_.flags.minus(value,0u,1);return value!=0;}
    void promote_small(){
        auto&g=s_.gp;g[0]=read(0x1359c3u,g[6]+0x2a78u);s_.flags.logic(g[0],4);if(g[0])return;
        s_.xmm[0]=N::scalar(read(0x1359cdu,0x46a104u));const auto radius=N::scalar(read(0x1359d5u,g[4]+0x10u));
        const auto comparison=compare(s_.xmm[0],radius);s_.flags.load(comparison.flags);
        if(!comparison.below)byte_store(0x1359dcu,g[5],1u);
    }
    bool visible(){s_.flags.logic(s_.gp[3]&255u,1);return (s_.gp[3]&255u)!=0;}
    Scalar scalar(Read field,const Progress<Scalar>&d){
        auto&g=s_.gp;std::uint32_t pc{},address{};unsigned target{};
        switch(field){
        case Read::center_z:pc=0x135a22u;address=g[1]+8u;target=3;break;
        case Read::volume_z:s_.xmm[3]=d.z;pc=0x135a27u;address=g[6]+0x2a70u;target=8;break;
        case Read::center_x:s_.xmm[3]=d.z;pc=0x135a2fu;address=g[1];target=1;break;
        case Read::volume_x:s_.xmm[1]=d.x;pc=0x135a33u;address=g[6]+0x2a68u;target=8;break;
        case Read::center_y:s_.xmm[1]=d.x;pc=0x135a3bu;address=g[1]+4u;target=2;break;
        case Read::volume_y:s_.xmm[2]=d.y;pc=0x135a40u;address=g[6]+0x2a6cu;target=8;break;
        case Read::radius:s_.xmm[2]=d.y;pc=0x135a48u;address=g[4]+0x10u;target=0;break;
        case Read::volume_radius:s_.xmm[0]=d.radius;pc=0x135a4eu;address=g[6]+0x2a74u;target=8;break;
        }
        const auto value=N::scalar(read(pc,address));if(target<8)s_.xmm[target]=value;return value;
    }
    bool overlap_result(const Progress<Scalar>&d){
        s_.xmm[0]=d.combined_radius;s_.xmm[1]=d.radius_squared;s_.xmm[2]=d.y;s_.xmm[3]=d.square_x;s_.xmm[4]=d.distance_squared;
        const auto comparison=compare(d.radius_squared,d.distance_squared);s_.flags.load(comparison.flags);return !comparison.below;
    }
    void sphere_containment(){
        auto&g=s_.gp;scope_.require_capacity(5);
        auto value=read(0x135a7fu,g[6]+0x2a68u);asm volatile("flds %0"::"m"(value):"memory","st");s_.set(3,g[0]&255u,1,0);
        value=read(0x135a87u,g[1]);asm volatile("fsubs %0"::"m"(value):"memory","st");
        value=read(0x135a89u,g[6]+0x2a6cu);asm volatile("flds %0"::"m"(value):"memory","st");
        value=read(0x135a8fu,g[1]+4u);asm volatile("fsubs %0"::"m"(value):"memory","st");
        value=read(0x135a92u,g[6]+0x2a70u);asm volatile("flds %0"::"m"(value):"memory","st");
        value=read(0x135a98u,g[1]+8u);asm volatile("fsubs %0"::"m"(value):"memory","st");
        // Three live extended differences -> their ordered Euclidean norm.
        // Cleanup preserves original live caller values and x87 C1 behavior.
        asm volatile("fld %%st(0);fmul %%st(1),%%st;fld %%st(2);fmul %%st(3),%%st;faddp %%st,%%st(1);fld %%st(3);fmul %%st(4),%%st;faddp %%st,%%st(1);fsqrt;fstp %%st(3);fstp %%st(0);fstp %%st(0)":::"memory","st","st(1)","st(2)","st(3)","st(4)");
        value=read(0x135ab3u,g[4]+0x10u);asm volatile("fadds %0"::"m"(value):"memory","st");
        value=read(0x135ab7u,g[6]+0x2a74u);std::uint32_t ah;unsigned char overflow;
        asm volatile("flds %2;fcomip %%st(1),%%st;fstp %%st(0);lahf;seto %1":"=a"(ah),"=qm"(overflow):"m"(value):"cc","memory","st","st(1)");
        const auto flags=((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)|2u;s_.flags.load(flags);
        if(flags&1u){g[0]=s_.flags.logic(0u,4);byte_store(0x135ad5u,g[5],g[0]);}
        else byte_store(0x135ac3u,g[5],g[0]);
    }
    void auxiliary(){
        auto&g=s_.gp;g[2]=read(0x1359e4u,g[4]+0x14u);s_.flags.logic(g[2],4);if(!g[2])return;
        const auto flags=read(0x1359ecu,g[6]+0x2a60u,1);s_.flags.logic(flags&1u,1);if(!(flags&1u))return;
        g[6]=read(0x1359f5u,g[4]+0x18u);push(0x1359f9u,0x1359feu);scope_.child([&]{call_(s_,Child::auxiliary,before_);});
        g[1]=read(0x1359feu,g[4]+0x2cu);g[2]=read(0x135a02u,g[4]+0x28u);
        s_.flags.minus(g[0]&65535u,65535u,2);s_.set(0,!s_.flags.zf,1,0);
        byte_store(0x135a0du,g[1],g[0]);byte_store(0x135a0fu,g[2],g[0]);
    }
    std::uint32_t finish(){auto&g=s_.gp;g[6]=pop(0x135a11u);g[5]=pop(0x135a12u);s_.set(0,g[3]&255u,1,0);g[3]=pop(0x135a15u);pop(0x135a16u);g[4]+=32u;return g[0];}
    std::uint32_t outside(){auto&g=s_.gp;g[6]=pop(0x135acbu);g[5]=pop(0x135accu);s_.set(0,s_.flags.logic(0u,1),1,0);g[3]=pop(0x135acfu);pop(0x135ad0u);g[4]+=32u;return g[0];}
};
template<class State,class Call,class Before=Noop>bool run_with(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    native_raw_x87_detail::Scope scope{s};View view{s,call,before,scope};admit(view);return true;
}
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call fallback,Before before={}){
    return native_geometry_visibility_detail::run_with(s,[&](auto&child,Child which,auto&observe){
        if(which==Child::frustum){if(native_cluster_frustum_abi_detail::bridge(child,observe))return;}
        else if(native_bsp_queue_find_detail::bridge(child,observe))return;
        fallback(child,which,observe);
    },std::move(before));
}
#else
template<class State,class...Args>bool run_with(State&,Args&&...){return false;}
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_geometry_visibility_detail
