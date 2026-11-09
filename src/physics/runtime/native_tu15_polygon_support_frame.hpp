#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_polygon_support_kernel.hpp"
#include <cstdlib>
#include <thread>

namespace halo2::native_polygon_support_legacy_detail {
template<class State>bool bridge(State&);
template<class State>__m128 ordered_product(State&,__m128,__m128);
template<class State>__m128 ordered_sum(State&,__m128,__m128);
template<class State>std::uint32_t ordered_shift(State&,std::uint32_t);
template<class State>void ordered_store(State&,std::uint32_t,std::uint32_t,__m128);
}
namespace halo2::native_polygon_support_detail {
template<class State>__m128 ordered_product(State&s,__m128 left,__m128 right){
    if(s.image==SourcePhysicsContactImage::MapPack11)return native_polygon_support_legacy_detail::ordered_product(s,left,right);
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return _mm_mul_ps(left,right);
    asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;
}
template<class State>__m128 ordered_sum(State&s,__m128 left,__m128 right){
    if(s.image==SourcePhysicsContactImage::MapPack11)return native_polygon_support_legacy_detail::ordered_sum(s,left,right);
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return _mm_add_ss(left,right);
    asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;
}
template<class State>std::uint32_t ordered_shift(State&s,std::uint32_t value){
    if(s.image==SourcePhysicsContactImage::MapPack11)return native_polygon_support_legacy_detail::ordered_shift(s,value);
    return s.image==SourcePhysicsContactImage::TitleUpdate15?
        native_mopp_aabb_abi_detail::shifted_immediate<4>(s,value):s.flags.shift(value,4u,4);
}
template<class State,class Before,bool Actual=true>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    static constexpr std::uint32_t site(std::uint32_t actual){return Actual?actual:actual-0x48f0u;}
    static __m128 packed(Vector value){return native_mopp_ray_detail::packed(value);}
    static Vector words(__m128 value){return native_mopp_ray_detail::words(value);}
    static __m128 scalar(std::uint32_t bits){return packed({bits,0,0,0});}
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}}{}
    void begin(){auto&g=s_.gp;push(site(0x1f6610u),g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);push(site(0x1f6616u),g[1]);}
    std::uint32_t count(){at(site(0x1f6617u));return s_.gp[2]=word(s_.gp[1]+0x14u);}
    Vector initial_best(){at(site(0x1f661au));s_.xmm[1]=scalar(word(0x44aac8u));return words(s_.xmm[1]);}
    void initialize(){auto&g=s_.gp;push(site(0x1f6622u),g[3]);push(site(0x1f6623u),g[6]);push(site(0x1f6624u),g[7]);g[7]=s_.flags.logic(0,4);g[0]=s_.flags.logic(0,4);}
    void count_test(const Progress&){s_.flags.logic(s_.gp[2]&s_.gp[2],4);}
    Vector direction(){at(site(0x1f662du));s_.gp[6]=word(s_.gp[5]+8u);s_.xmm[2]=Base::vector(site(0x1f6630u),s_.gp[6]);return words(s_.xmm[2]);}
    void vertices(){s_.gp[6]=s_.gp[1]+0x20u;}
    Vector vertex(const Progress&){s_.xmm[0]=Base::vector(site(0x1f6636u),s_.gp[6]);return words(s_.xmm[0]);}
    Vector multiply(Vector left,Vector right){return words(native_mopp_ray_detail::multiply(packed(left),packed(right)));}
    Vector add_low(Vector left,Vector right){auto value=packed(left);const auto other=packed(right);asm volatile("addss %1,%0":"+x"(value):"x"(other):"memory");return words(value);}
    Vector shuffle_y(Vector value){const auto input=packed(value);return words(_mm_shuffle_ps(input,input,0x55));}
    Vector shuffle_z(Vector value){const auto input=packed(value);return words(_mm_shuffle_ps(input,input,0xaa));}
    void store_dot(const Progress&d){
        s_.xmm[0]=packed(d.products);s_.xmm[3]=packed(d.xy);s_.xmm[4]=packed(d.dot);s_.gp[3]=s_.gp[4]+12u;
        Base::store(site(0x1f6656u),s_.gp[3],d.dot[0],[]{});
    }
    Vector load_dot(){at(site(0x1f665au));s_.xmm[0]=scalar(word(s_.gp[4]+12u));return words(s_.xmm[0]);}
    bool greater(const Progress&d){
        const auto candidate=packed(d.candidate),best=packed(d.best);std::uint32_t flags;
        asm volatile("comiss %2,%1;lahf":"=a"(flags):"x"(candidate),"x"(best):"cc","memory");
        // COMISS clears OF/SF/AF. Capture its native relation once, without
        // touching caller storage through a PUSHF/POP temporary.
        s_.flags.load((flags>>8u)&0xd5u);return (flags&0x4100u)==0u;
    }
    void improved(const Progress&d){s_.xmm[1]=packed(d.candidate);s_.gp[7]=s_.gp[0];}
    void advance(const Progress&){auto&g=s_.gp;g[0]=s_.flags.increment(g[0],4,false);g[6]=s_.flags.plus(g[6],16u,4);s_.flags.minus(g[0],g[2],4);}
    std::uint32_t output(){at(site(0x1f6672u));return s_.gp[0]=word(s_.gp[5]+12u);}
    void select(const Progress&){auto&g=s_.gp;g[7]=s_.flags.plus(g[7],2u,4);g[7]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[7]);}
    Vector selected_vertex(const Progress&){s_.xmm[0]=Base::vector(site(0x1f667bu),s_.gp[1]+s_.gp[7]);return words(s_.xmm[0]);}
    void restore_before_store(){s_.gp[7]=pop(site(0x1f667fu));s_.gp[6]=pop(site(0x1f6680u));}
    void write_result(const Progress&d){Base::vector_store(site(0x1f6681u),s_.gp[0],packed(d.result));}
    void complete_vector_store(std::uint32_t pc,std::uint32_t address,__m128 value){Base::vector_store(pc,address,value);}
    void finish(){auto&g=s_.gp;g[3]=pop(site(0x1f6684u));g[4]=g[5];g[5]=pop(site(0x1f6687u));pop(site(0x1f6688u));g[4]+=8u;}
};
template<bool Actual=true,class State,class Before>void support_retained(State&s,Before&before){
    native_raw_x87_detail::Scope scope{s};Frame<State,Before,Actual> frame{s,before,scope};Progress data;support(frame,data);
}
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State>void ordered_store(State&s,std::uint32_t pc,std::uint32_t address,__m128 value){
    if(s.image==SourcePhysicsContactImage::MapPack11){native_polygon_support_legacy_detail::ordered_store(s,pc,address,value);return;}
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)){
        physics_step_detail::vw(s.memory,s.observe,pc,address,value);return;
    }
    native_raw_x87_detail::Scope scope{s};Noop before;Frame frame{s,before,scope};
    frame.complete_vector_store(pc,address,value);
}
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_POLYGON_SUPPORT");return p&&*p;}();return enabled;}
template<class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){
        const auto*p=s.instruction_cursor;
        if(!s.literal_service_sites||!p||p!=&s.services.instruction_cursor||!p->position||!p->callback_type||
           s.services.instruction.target_type()!=*p->callback_type||p->executor!=std::this_thread::get_id()||
           !p->literal_service_sites||p->service_sites.size()!=3||p->service_sites[0]!=0x38846eu||
           p->service_sites[1]!=0x21f590u||p->service_sites[2]!=0x33b595u)return false;
        if((p->first_trace&&*p->first_trace)||(p->second_trace&&*p->second_trace))return false;
    }
    if(s.observe&&(!s.instruction_cursor||!s.instruction_cursor->observation_only_stores))return false;
    return true;
#endif
}
template<class State>bool bridge(State&s){
    if(s.image==SourcePhysicsContactImage::MapPack11)return native_polygon_support_legacy_detail::bridge(s);
    if(!admitted(s))return false;Noop before;
    if constexpr(requires{s.selected_observation_pcs;})support_retained<true>(s,before);
    else support_retained<false>(s,before);
    return true;
}
}
#else
#include "physics/runtime/physics_step_internal.hpp"
namespace halo2::native_polygon_support_detail {
// Unsupported backends retain the existing generated owner. Its exact
// arithmetic spellings must still parse without any GNU raw-frame headers.
template<class State>__m128 ordered_product(State&,__m128 left,__m128 right){return _mm_mul_ps(left,right);}
template<class State>__m128 ordered_sum(State&,__m128 left,__m128 right){return _mm_add_ss(left,right);}
template<class State>std::uint32_t ordered_shift(State&s,std::uint32_t value){return s.flags.shift(value,4u,4);}
template<class State>void ordered_store(State&s,std::uint32_t pc,std::uint32_t address,__m128 value){physics_step_detail::vw(s.memory,s.observe,pc,address,value);}
template<class State>bool bridge(State&){return false;}
}
#endif
#include "physics/runtime/native_tu11_polygon_support_frame.hpp"
