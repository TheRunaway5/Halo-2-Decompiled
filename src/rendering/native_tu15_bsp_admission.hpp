#pragma once
#include "halo2/native_tu15_bsp_admission_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_id_search_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "rendering/native_tu15_cluster_frustum_abi.hpp"
#include "rendering/native_tu15_geometry_visibility.hpp"
#include "rendering/native_tu15_fog_membership.hpp"
#include "rendering/native_tu15_visibility_association.hpp"
#endif

namespace halo2::native_bsp_admission_detail {
// Retained ABI view for the shared typed cluster/instance controller.
enum class Child {cluster_fog,instance_fog,merge_fog,geometry,association,geometry_frustum,geometry_auxiliary};
struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_BSP_ADMISSION");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>3)return false;
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
template<class State>bool admitted(const State&s){return !reference()&&policy_admitted(s);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace N=native_bsp_surface_2d_detail;
struct BitLocation {std::uint32_t count{},word{},mask{},flags{};};
inline BitLocation cluster_bit(std::uint32_t index){
    auto count=index,word=index;std::uint32_t mask,ah;unsigned char overflow;
    asm volatile("andl $31,%%ecx;sarl $5,%%edx;movl $1,%%ebx;shll %%cl,%%ebx;lahf;seto %4"
        :"+c"(count),"+d"(word),"=&b"(mask),"=a"(ah),"=qm"(overflow)::"cc","memory");
    return{count,word,mask,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)|2u};
}
inline BitLocation association_bit(std::uint32_t index){
    auto count=index,word=index;std::uint32_t mask,ah;unsigned char overflow;
    asm volatile("sarl $5,%%edx;andl $31,%%ecx;movl $1,%%ebx;shll %%cl,%%ebx;lahf;seto %4"
        :"+c"(count),"+d"(word),"=&b"(mask),"=a"(ah),"=qm"(overflow)::"cc","memory");
    return{count,word,mask,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)|2u};
}
inline std::pair<std::uint32_t,std::uint32_t> doubled(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;shll $1,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc","memory");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)|2u};
}
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
template<class State,class Call,class Before>class View:public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_raw_x87_detail::Scope<State>>> {
    using Scope=native_raw_x87_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    Scope&scope_;Call&call_;bool opened_{},first_instance_{};
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4){at(pc);return word(address,width);}
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    template<class Operation>void modify(std::uint32_t pc,std::uint32_t address,unsigned width,Operation operation){
        at(pc);const auto prior=word(address,width);auto flags=s_.flags;const auto value=operation(flags,prior);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});s_.flags=flags;
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    static std::uint32_t signed_half(std::uint32_t value){return std::uint32_t(std::int32_t(std::int16_t(value)));}
    static bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
    static std::uint32_t bits(N::Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
    static std::uint32_t compare_radius(){
        std::uint32_t ah;unsigned char overflow;
        asm volatile("fcomip %%st(1),%%st;fstp %%st(0);lahf;seto %1":"=a"(ah),"=qm"(overflow)::"cc","memory","st","st(1)");
        return ((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)|2u;
    }
    struct ClusterLookup {
        View&v;
        std::uint32_t count(){auto&g=v.s_.gp;g[2]=v.read(0x13705bu,g[7]+4u,2);g[0]=v.s_.flags.logic(0u,4);v.s_.flags.logic(g[2],4);return g[2];}
        void open_rows(){auto&g=v.s_.gp;g[7]=v.read(0x137069u,g[7]+12u);}
        std::uint32_t item(std::uint32_t){return v.read(0x137070u,v.s_.gp[7]);}
        std::uint32_t key()const{return v.s_.gp[1];}
        bool found(std::uint32_t,std::uint32_t value){v.s_.flags.minus(value,key(),4);v.s_.flags.minus(v.s_.gp[0]&65535u,65535u,2);return !v.s_.flags.zf;}
        void advance(std::uint32_t,std::uint32_t value){auto&g=v.s_.gp;v.s_.flags.minus(value,key(),4);++g[0];g[7]+=4u;v.s_.flags.minus(g[0],g[2],4);}
        bool absent()const{return false;}
    };
public:
    View(State&s,Call&call,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}},scope_(scope),call_(call){}
    template<class Operation>void retained_modify(std::uint32_t pc,std::uint32_t address,unsigned width,Operation operation){modify(pc,address,width,operation);}
    bool begin(){
        auto&g=s_.gp;g[4]=s_.flags.minus(g[4],0x3cu,4);push(0x136ea3u,g[5]);g[5]=read(0x136ea4u,g[4]+0x44u);
        const auto flags=read(0x136ea8u,g[5]+0x2a60u,1);s_.flags.logic(flags&0x20u,1);if(flags&0x20u)return false;
        g[1]=read(0x136eb5u,g[5]);g[0]=s_.flags.logic(0u,4);s_.flags.minus(read(0x136ebau,g[1]+0xa6cu,2),0u,2);
        const bool positive=!s_.flags.zf&&s_.flags.sf==s_.flags.of;write(0x136ec1u,g[4]+8u,0u);if(!positive)return false;
        push(0x136ecbu,g[3]);push(0x136eccu,g[6]);push(0x136ecdu,g[7]);opened_=true;return true;
    }
    void cluster(){
        auto&g=s_.gp;g[0]=signed_half(g[0]);g[7]=read(0x136ed3u,0x4ecc18u);g[2]=read(0x136ed9u,g[7]+0xa0u);
        write(0x136edfu,g[4]+0x3cu,g[0]);g[0]=native_cluster_frustum_abi_detail::multiply<26>(s_,g[0]);g[0]+=g[1]+0xa6eu;
        g[1]=signed_half(read(0x136eedu,g[0],2));g[6]=native_cluster_frustum_abi_detail::multiply<176>(s_,g[1]);g[6]=s_.flags.plus(g[6],g[2],4);
    }
    void mark_cluster(){
        auto&g=s_.gp;const auto location=cluster_bit(g[1]);g[1]=location.count;g[2]=location.word;g[3]=location.mask;s_.flags.load(location.flags);
        g[1]=read(0x136f09u,g[5]+0x21cu+g[2]*4u);g[2]=g[5]+0x21cu+g[2]*4u;write(0x136f17u,g[4]+0x18u,g[0]);
        g[1]=s_.flags.logic(g[1]|g[3],4);write(0x136f1du,g[2],g[1]);g[1]=signed_half(read(0x136f1fu,g[0],2));
        s_.set(2,read(0x136f22u,g[4]+0x14u,1),1,0);write(0x136f26u,g[5]+0x1cu+g[1],g[2],1);
    }
    bool cluster_has_geometry(){
        s_.flags.minus(read(0x136f2au,s_.gp[6]+0x24u,2),0u,2);const bool present=!s_.flags.cf&&!s_.flags.zf;
        write(0x136f2fu,s_.gp[4]+0x20u,s_.gp[6]);return present;
    }
    void cluster_sides(){
        auto&g=s_.gp;auto&x=s_.xmm;scope_.require_capacity(2);
        g[1]=signed_half(read(0x136f39u,g[0],2));g[3]=read(0x136f3cu,g[7]+0xa0u);
        x[3]=N::scalar(read(0x136f42u,0x46a0ccu));g[0]=native_cluster_frustum_abi_detail::multiply<176>(s_,g[1]);
        x[2]=N::scalar(read(0x136f52u,g[0]+g[3]+0x64u));x[2]=N::add(x[2],N::scalar(read(0x136f58u,g[0]+g[3]+0x68u)));
        x[5]=N::scalar(read(0x136f5eu,g[0]+g[3]+0x68u));x[1]=N::scalar(read(0x136f64u,g[0]+g[3]+0x5cu));
        x[1]=N::add(x[1],N::scalar(read(0x136f6au,g[0]+g[3]+0x60u)));x[4]=N::scalar(read(0x136f70u,g[0]+g[3]+0x60u));
        x[0]=N::scalar(read(0x136f76u,g[0]+g[3]+0x54u));x[0]=N::add(x[0],N::scalar(read(0x136f7cu,g[0]+g[3]+0x58u)));
        g[0]=s_.flags.plus(g[0],g[3],4);x[2]=N::multiply(x[2],x[3]);x[1]=N::multiply(x[1],x[3]);x[5]=N::subtract(x[5],x[2]);
        write(0x136f90u,g[4]+0x48u,bits(x[5]));auto value=read(0x136f96u,g[4]+0x48u);asm volatile("flds %0"::"m"(value):"memory","st");
        value=read(0x136f9au,g[4]+0x48u);asm volatile("fmuls %0"::"m"(value):"memory","st");
        x[4]=N::subtract(x[4],x[1]);write(0x136fa2u,g[4]+0x44u,bits(x[4]));value=read(0x136fa8u,g[4]+0x44u);asm volatile("flds %0"::"m"(value):"memory","st");
        value=read(0x136facu,g[4]+0x44u);asm volatile("fmuls %0"::"m"(value):"memory","st");
        x[0]=N::multiply(x[0],x[3]);x[3]=N::scalar(read(0x136fb4u,g[0]+0x58u));x[3]=N::subtract(x[3],x[0]);
        asm volatile("faddp %%st,%%st(1)":::"memory","st","st(1)");write(0x136fbfu,g[4]+0x40u,bits(x[3]));
        value=read(0x136fc5u,g[4]+0x40u);asm volatile("flds %0"::"m"(value):"memory","st");
        g[0]=read(0x136fc9u,g[5]+0x2a60u);s_.flags.logic((g[0]>>8u)&2u,1);const bool clipping=!s_.flags.zf;
        value=read(0x136fd2u,g[4]+0x40u);asm volatile("fmuls %0"::"m"(value):"memory","st");
        write(0x136fd6u,g[4]+0x1cu,g[1]);asm volatile("faddp %%st,%%st(1);fsqrt":::"memory","st","st(1)");
        if(clipping){
            x[3]=N::scalar(read(0x136fe0u,g[5]+0x2a90u));x[0]=N::multiply(x[0],N::scalar(read(0x136fe8u,g[5]+0x2a88u)));
            x[3]=N::multiply(x[3],x[2]);x[2]=N::scalar(read(0x136ff4u,g[5]+0x2a8cu));x[2]=N::multiply(x[2],x[1]);
            x[3]=N::add(x[3],x[2]);x[3]=N::add(x[3],x[0]);x[3]=N::subtract(x[3],N::scalar(read(0x137008u,g[5]+0x2a94u)));
            x[0]=_mm_setzero_ps();s_.set(3,s_.flags.logic(0u,1),1,0);auto result=native_geometry_visibility_detail::compare(x[0],x[3]);s_.flags.load(result.flags);
            write(0x137018u,g[4]+0x24u,bits(x[3]));write(0x13701eu,g[4]+0x50u,0u,1);
            if(!s_.flags.cf&&!s_.flags.zf)s_.set(3,1u,1,0);
            result=native_geometry_visibility_detail::compare(x[3],x[0]);s_.flags.load(result.flags);
            if(!s_.flags.cf&&!s_.flags.zf)write(0x13702cu,g[4]+0x50u,1u,1);
            value=read(0x137031u,g[4]+0x24u);asm volatile("flds %0;fabs;fxch %%st(1)"::"m"(value):"memory","st","st(1)");
            s_.flags.load(compare_radius());if(s_.flags.cf)return;
        }else asm volatile("fstp %%st(0)":::"memory","st");
        write(0x137043u,g[4]+0x50u,1u,1);s_.set(3,1u,1,0);
    }
    bool cluster_allowed(){
        auto&g=s_.gp;g[7]=read(0x13704au,g[5]+0x2ad0u);s_.flags.logic(g[7],4);if(!g[7])return true;
        s_.flags.logic(g[0]&1u,1);if(!(g[0]&1u))return true;
        g[7]=read(0x137058u,g[7]+12u);ClusterLookup lookup{*this};return native_id_search_detail::find(lookup);
    }
    void cluster_fog(){
        auto&g=s_.gp;g[7]=read(0x13708bu,0x4c67acu);push(0x137091u,0x137096u);scope_.child([&]{call_(s_,Child::cluster_fog,before_);});
        g[3]=(g[3]&255u)?0xffff8000u:0u;g[0]=((g[0]&255u)?0x400u:0u)|g[3]|0x3000u;g[2]=g[0];s_.flags.logic(g[0],4);
    }
    bool cluster_room(){
        auto&g=s_.gp;g[0]=read(0x1370b2u,g[5]+12u);g[1]=read(0x1370b5u,g[0]+4u,2);g[7]=read(0x1370b9u,g[0]);--g[7];s_.flags.minus(g[1],g[7],4);return less(g[1],g[7]);
    }
    void append_cluster(){
        auto&g=s_.gp;g[7]=read(0x1370c0u,g[0]+12u);g[3]=read(0x1370c3u,g[4]+0x1cu);write(0x1370c7u,g[7]+g[1]*4u,g[3]);
        s_.set(1,read(0x1370cau,g[4]+0x50u,1),1,0);g[7]=read(0x1370ceu,g[0]+8u);
        g[1]=((g[1]&255u)?0x4000u:0u)|g[2];s_.flags.logic(g[1],4);g[2]=read(0x1370ddu,g[0]+4u,2);write(0x1370e1u,g[7]+g[2]*2u,g[1],2);
        g[1]=read(0x1370e5u,g[0]+4u,2);g[2]=read(0x1370e9u,g[0]+16u);write(0x1370ecu,g[2]+g[1]*4u,0u);
        g[1]=read(0x1370f3u,g[0]+4u,2);g[2]=read(0x1370f7u,g[0]+20u);write(0x1370fau,g[2]+g[1]*2u,65535u,2);
        modify(0x137100u,g[0]+4u,2,[](auto&flags,auto value){return flags.increment(value,2,false);});
    }
    bool begin_instances(){
        auto&g=s_.gp;g[1]=read(0x137104u,g[6]+0x98u);g[0]=s_.flags.logic(0u,4);s_.flags.logic(g[1],4);
        const bool any=!s_.flags.zf&&s_.flags.sf==s_.flags.of;write(0x13710eu,g[4]+0x1cu,0u);first_instance_=true;return any;
    }
    bool instance_seen(){
        auto&g=s_.gp;if(!first_instance_)g[6]=read(0x13711au,g[4]+0x20u);first_instance_=false;
        g[1]=read(0x137120u,g[6]+0x9cu);s_.set(1,read(0x137126u,g[1]+g[0]*2u,2),2,0);
        g[7]=read(0x13712au,0x4ecc18u);g[0]=read(0x137130u,g[7]+0x144u);g[2]=g[1]&65535u;
        g[3]=read(0x137139u,g[7]+0x13cu);g[7]=read(0x13713fu,0x4f44ecu);
        g[6]=native_cluster_frustum_abi_detail::multiply<88>(s_,g[2]);g[6]=s_.flags.plus(g[6],g[0],4);
        g[0]=signed_half(read(0x13714cu,g[6]+0x34u,2));g[0]=native_cluster_frustum_abi_detail::multiply<200>(s_,g[0]);g[1]=signed_half(g[1]);g[0]=s_.flags.plus(g[0],g[3],4);
        s_.flags.minus(read(0x13715bu,0x4f44f0u+g[1]*4u),g[7],4);const bool seen=s_.flags.zf;
        write(0x137162u,g[4]+0x34u,g[2]);write(0x137166u,g[4]+0x38u,g[1]);return seen;
    }
    bool instance_has_geometry(){
        auto&g=s_.gp;g[1]=read(0x137170u,g[5]+0x2ad0u);s_.flags.logic(g[1],4);if(g[1])g[1]=read(0x13717au,g[1]+0x18u);
        s_.flags.minus(read(0x13717du,g[0]+0x24u,2),0u,2);return !s_.flags.cf&&!s_.flags.zf;
    }
    bool instance_visible(){
        auto&g=s_.gp;g[0]=g[4]+0x11u;push(0x13718cu,g[0]);g[0]=g[4]+0x16u;push(0x137191u,g[0]);g[0]=g[4]+0x1bu;push(0x137196u,g[0]);
        g[0]=read(0x137197u,g[4]+0x20u);push(0x13719bu,0u);push(0x13719du,g[0]);push(0x13719eu,g[2]);push(0x13719fu,g[1]);
        g[1]=read(0x1371a0u,g[6]+0x48u);g[7]=g[6]+0x3cu;push(0x1371a6u,g[1]);g[1]=g[7];g[0]=g[5];push(0x1371abu,0x1371b0u);
        scope_.child([&]{call_(s_,Child::geometry,before_);});s_.flags.logic(g[0]&255u,1);return (g[0]&255u)!=0;
    }
    bool force_instance(){const auto value=read(0x1371b4u,s_.gp[5]+0x2a60u,1);s_.set(0,value,1,0);s_.flags.logic(value,1);return s_.flags.sf;}
    void instance_sides(){
        auto&g=s_.gp;auto&x=s_.xmm;scope_.require_capacity(2);
        g[0]=read(0x1371c2u,g[5]+0x2a60u);s_.flags.logic((g[0]>>8u)&2u,1);const bool clipping=!s_.flags.zf;
        x[0]=N::scalar(read(0x1371cbu,g[6]+0x48u));write(0x1371d0u,g[4]+0x30u,0xffffffffu);write(0x1371d8u,g[4]+0x2cu,bits(x[0]));
        if(clipping){
            x[0]=N::scalar(read(0x1371e0u,g[5]+0x2a90u));x[0]=N::multiply(x[0],N::scalar(read(0x1371e8u,g[7]+8u)));
            x[1]=N::scalar(read(0x1371edu,g[5]+0x2a8cu));x[1]=N::multiply(x[1],N::scalar(read(0x1371f5u,g[7]+4u)));
            x[0]=N::add(x[0],x[1]);x[1]=N::scalar(read(0x1371feu,g[7]));x[1]=N::multiply(x[1],N::scalar(read(0x137202u,g[5]+0x2a88u)));
            x[0]=N::add(x[0],x[1]);x[0]=N::subtract(x[0],N::scalar(read(0x13720eu,g[5]+0x2a94u)));
            x[1]=_mm_setzero_ps();s_.set(3,s_.flags.logic(0u,1),1,0);auto result=native_geometry_visibility_detail::compare(x[1],x[0]);s_.flags.load(result.flags);
            write(0x13721eu,g[4]+0x28u,bits(x[0]));write(0x137224u,g[4]+0x50u,g[3],1);
            if(!s_.flags.cf&&!s_.flags.zf)write(0x13722au,g[4]+0x50u,1u,1);
            result=native_geometry_visibility_detail::compare(x[0],x[1]);s_.flags.load(result.flags);if(!s_.flags.cf&&!s_.flags.zf)s_.set(3,1u,1,0);
            auto value=read(0x137236u,g[4]+0x28u);asm volatile("flds %0;fabs"::"m"(value):"memory","st");
            value=read(0x13723cu,g[4]+0x2cu);asm volatile("flds %0"::"m"(value):"memory","st");s_.flags.load(compare_radius());if(s_.flags.cf)return;
        }
        s_.set(3,1u,1,0);write(0x137248u,g[4]+0x50u,g[3],1);
    }
    void instance_fog(){auto&g=s_.gp;g[2]=read(0x13724cu,g[4]+0x18u);g[1]=signed_half(read(0x137250u,g[2],2));g[7]=read(0x137253u,0x4c67acu);push(0x137259u,0x13725eu);scope_.child([&]{call_(s_,Child::instance_fog,before_);});}
    void instance_flags(){
        auto&g=s_.gp;s_.set(1,g[0]&255u,1,0);s_.set(1,s_.flags.minus(0u,g[1]&255u,1),1,0);
        s_.set(2,read(0x137262u,g[4]+0x11u,1),1,0);s_.set(0,read(0x137266u,g[4]+0x12u,1),1,0);
        const auto fog=s_.flags.cf?0x400u:0u;g[3]=(g[3]&255u)?0x4000u:0u;g[2]=(g[2]&255u)?0x2000u:0u;g[1]=fog|g[3]|g[2];s_.flags.logic(g[1],4);
        s_.set(2,read(0x13728au,g[4]+0x13u,1),1,0);g[0]=(g[0]&255u)?0x1000u:0u;g[1]|=g[0];s_.flags.logic(g[1],4);
        s_.set(0,read(0x137299u,g[4]+0x50u,1),1,0);g[2]=(g[2]&255u)?0x800u:0u;g[1]|=g[2];g[0]=(g[0]&255u)?0xffff8000u:0u;
        g[1]|=g[0];g[6]=g[1];s_.flags.logic(g[1],4);
    }
    bool contained(){s_.flags.logic(s_.gp[6]&0x800u,4);return (s_.gp[6]&0x800u)!=0;}
    void none_association(){s_.gp[1]=read(0x1372ceu,s_.gp[4]+0x30u);}
    void allocate_association(){auto&g=s_.gp;g[0]=read(0x1372beu,g[4]+0x14u);push(0x1372c2u,g[0]);g[2]=g[5];push(0x1372c5u,0x1372cau);scope_.child([&]{call_(s_,Child::association,before_);});g[1]=g[0];}
    bool instance_room(){auto&g=s_.gp;g[0]=read(0x1372d2u,g[5]+0x18u);g[2]=read(0x1372d5u,g[0]+4u,2);g[7]=read(0x1372d9u,g[0]);--g[7];s_.flags.minus(g[2],g[7],4);return less(g[2],g[7]);}
    void append_instance(){
        auto&g=s_.gp;g[7]=read(0x1372e4u,g[0]+12u);g[3]=read(0x1372e7u,g[4]+0x34u);write(0x1372ebu,g[7]+g[2]*4u,g[3]);
        g[2]=read(0x1372eeu,g[0]+4u,2);g[7]=read(0x1372f2u,g[0]+8u);write(0x1372f5u,g[7]+g[2]*2u,g[6],2);
        g[2]=read(0x1372f9u,g[0]+4u,2);g[6]=read(0x1372fdu,g[0]+16u);write(0x137300u,g[6]+g[2]*4u,0u);
        g[2]=read(0x137307u,g[0]+4u,2);g[6]=read(0x13730bu,g[0]+20u);write(0x13730eu,g[6]+g[2]*2u,g[1],2);
        modify(0x137312u,g[0]+4u,2,[](auto&flags,auto value){return flags.increment(value,2,false);});
        g[0]=read(0x137316u,0x4f44ecu);g[1]=read(0x13731bu,g[4]+0x38u);write(0x13731fu,0x4f44f0u+g[1]*4u,g[0]);
    }
    void merge_instance(){
        auto&g=s_.gp;g[1]=read(0x13732bu,g[5]+0x18u);g[6]=signed_half(read(0x13732eu,g[1]+4u,2));
        g[7]=s_.flags.logic(0xffffffffu,4);s_.set(3,s_.flags.logic(0u,1),1,0);g[0]=s_.flags.logic(0u,4);s_.flags.logic(g[6],4);
        // The source retains the last probed cursor even on an unsuccessful
        // lookup, and tests a successful match at the next loop boundary.
        if(less(0u,g[6])){
            do{
                s_.flags.logic(g[3]&255u,1);if(!s_.flags.zf)break;
                g[3]=read(0x137344u,g[1]+12u);g[7]=signed_half(g[0]);
                s_.flags.minus(read(0x13734au,g[3]+g[7]*4u),g[2],4);g[7]=g[0];s_.set(3,s_.flags.zf,1,0);
                g[0]=s_.flags.increment(g[0],4,false);s_.flags.minus(g[0],g[6],4);
            }while(less(g[0],g[6]));
        }
        g[2]=read(0x137357u,g[1]+8u);const auto shifted=doubled(signed_half(g[7]),s_.flags.bits());g[6]=shifted.first;s_.flags.load(shifted.second);
        s_.flags.logic(read(0x13735fu,g[6]+g[2]+1u,1)&8u,1);
        if(s_.flags.zf){
            g[0]=read(0x137366u,g[1]+20u);g[2]=signed_half(read(0x137369u,g[6]+g[0],2));g[1]=read(0x13736du,g[4]+0x3cu);
            const auto location=association_bit(g[1]);g[0]=g[5]+(0x298u+location.word+g[2]*4u)*4u;g[1]=location.count;g[2]=location.mask;s_.flags.load(location.flags);
            modify(0x13738bu,g[0],4,[&](auto&flags,auto value){return flags.logic(value|g[2],4);});
        }
        g[0]=read(0x13738du,g[4]+0x18u);g[1]=signed_half(read(0x137391u,g[0],2));g[7]=read(0x137394u,0x4c67acu);
        push(0x13739au,0x13739fu);scope_.child([&]{call_(s_,Child::merge_fog,before_);});s_.flags.logic(g[0]&255u,1);
        if(!s_.flags.zf){g[1]=read(0x1373a3u,g[5]+0x18u);g[2]=read(0x1373a6u,g[1]+8u);g[6]=s_.flags.plus(g[6],g[2],4);
            modify(0x1373abu,g[6]+1u,1,[](auto&flags,auto value){return flags.logic(value|4u,1);});}
    }
    bool next_instance(){
        auto&g=s_.gp;g[0]=read(0x1373afu,g[4]+0x1cu);g[1]=read(0x1373b3u,g[4]+0x20u);g[2]=read(0x1373b7u,g[1]+0x98u);
        ++g[0];s_.flags.minus(g[0],g[2],4);const bool next=less(g[0],g[2]);write(0x1373c0u,g[4]+0x1cu,g[0]);return next;
    }
    bool next_cluster(){
        auto&g=s_.gp;g[0]=read(0x1373cau,g[4]+0x14u);g[1]=read(0x1373ceu,g[5]);g[0]=s_.flags.increment(g[0],4,false);
        s_.flags.minus(g[0]&65535u,read(0x1373d2u,g[1]+0xa6cu,2),2);const bool next=s_.flags.sf!=s_.flags.of;write(0x1373d9u,g[4]+0x14u,g[0]);return next;
    }
    void finish(){
        auto&g=s_.gp;if(opened_){g[7]=pop(0x1373e3u);g[6]=pop(0x1373e4u);g[3]=pop(0x1373e5u);}
        g[5]=pop(0x1373e6u);g[4]=s_.flags.plus(g[4],0x3cu,4);pop(0x1373eau);g[4]+=4u;
    }
};
template<class State,class Call,class Before=Noop>bool run_with(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    native_raw_x87_detail::Scope scope{s};View view{s,call,before,scope};native_bsp_admission_detail::collect(view);return true;
}
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call fallback,Before before={}){
    return native_bsp_admission_detail::run_with(s,[&](auto&child,Child which,auto&observe){
        if(which==Child::geometry){
            if(native_geometry_visibility_detail::bridge(child,[&](auto&state,auto kind,auto&progress){fallback(state,kind==native_geometry_visibility_detail::Child::frustum?Child::geometry_frustum:Child::geometry_auxiliary,progress);},observe))return;
        }else if(which==Child::association){if(native_visibility_association_detail::bridge(child,observe))return;}
        else if(native_fog_membership_detail::bridge(child,observe))return;
        fallback(child,which,observe);
    },std::move(before));
}
// Exact generated owners install this bridge with their separately pinned
// canonical fallback and real child-dispatch boundaries.
#else
template<class State,class...Args>bool run_with(State&,Args&&...){return false;}
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
// Exact body-local fallback primitives preserve the unchanged CPU's exposed
// shift context and the commit boundary of RMW flags. Original full body pins
// remain authority: the binder reverses only these exact known spellings.
template<class State>__m128 retained_add(State&s,__m128 a,__m128 b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image==SourcePhysicsContactImage::TitleUpdate15)return N::add(a,b);
#endif
    return _mm_add_ss(a,b);
}
template<class State>__m128 retained_subtract(State&s,__m128 a,__m128 b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image==SourcePhysicsContactImage::TitleUpdate15)return N::subtract(a,b);
#endif
    return _mm_sub_ss(a,b);
}
template<class State>__m128 retained_multiply(State&s,__m128 a,__m128 b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image==SourcePhysicsContactImage::TitleUpdate15)return N::multiply(a,b);
#endif
    return _mm_mul_ss(a,b);
}
template<class State>void retained_cluster_bit(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(policy_admitted(s)){const auto location=cluster_bit(s.gp[2]*32u+s.gp[1]);s.gp[3]=location.mask;s.flags.load(location.flags);return;}
#endif
    s.gp[3]=s.flags.shift(s.gp[3],s.reg(1,1,0),4);
}
template<class State>void retained_double(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(policy_admitted(s)){const auto shifted=doubled(s.gp[6],s.flags.bits());s.gp[6]=shifted.first;s.flags.load(shifted.second);return;}
#endif
    s.gp[6]=s.flags.shift(s.gp[6],1u,4);
}
template<class State>void retained_association_bit(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(policy_admitted(s)){const auto location=association_bit(s.gp[1]);s.gp[2]=location.mask;s.flags.load(location.flags);return;}
#endif
    s.gp[2]=s.flags.shift(s.gp[2],s.reg(1,1,0),4);
}
template<class State>void retained_rmw(State&s,std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t mask,bool increment){
    const auto operation=[&](auto&flags,auto value){return increment?flags.increment(value,width,false):flags.logic(value|mask,width);};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(policy_admitted(s)){native_raw_x87_detail::Scope scope{s};Noop before;auto call=[](auto&,auto,auto&){};View view{s,call,before,scope};view.retained_modify(pc,address,width,operation);return;}
#endif
    physics_world_runtime_detail::w(s.memory,s.observe,pc,address,width,operation(s.flags,physics_world_runtime_detail::r(s.memory,address,width)));
}
} // namespace halo2::native_bsp_admission_detail
