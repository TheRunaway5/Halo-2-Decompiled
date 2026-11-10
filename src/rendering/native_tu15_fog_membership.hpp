#pragma once
#include "halo2/native_tu15_fog_membership_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#endif

namespace halo2::native_fog_membership_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_FOG_MEMBERSHIP");return p&&*p;}();return value;}
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
           (s.observe&&!c->observation_only_stores)||(c->first_trace&&*c->first_trace)||
           (c->second_trace&&*c->second_trace))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return native_sse_mmx_detail::supported(s);
    return native_raw_x87_detail::supported(s);
#else
    (void)s;return false;
#endif
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
struct RowOffset {std::uint32_t value{},flags{};};
inline RowOffset datum_offset(std::uint32_t index){
    std::uint32_t ah;unsigned char overflow;
    // The authored CMP(index,-1), then immediate SHL ECX,4 owns even the
    // shift-undefined physical flags exposed before the following data read.
    asm volatile("cmpl $-1,%%ecx;shll $4,%%ecx;lahf;seto %2":"+c"(index),"=a"(ah),"=qm"(overflow)::"cc","memory");
    return{index,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)|2u};
}
template<class State,class Before,class Scope>class View:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    bool saved_bx_{};
public:
    View(State&s,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}}{}
    bool available(){
        auto&g=s_.gp;at(0x171070u);g[2]=word(0x4ecc18u);
        push(0x171076u,g[6]);at(0x171077u);g[6]=word(0x4ecc20u);
        s_.set(0,s_.flags.logic(0u,1),1,0);s_.flags.logic(g[6],4);
        if(!g[6])return false;
        s_.flags.logic(g[2],4);if(!g[2])return false;
        s_.flags.minus(g[1],0xffffffffu,4);return g[1]!=0xffffffffu;
    }
    bool override_membership(){
        auto&g=s_.gp;push(0x17108cu,g[3]);saved_bx_=true;
        at(0x17108du);g[3]=word(0x4f2d98u);g[3]>>=4u;
        s_.flags.logic((g[3]&255u)&1u,1);return (g[3]&1u)!=0;
    }
    std::int32_t designator(){
        auto&g=s_.gp;at(0x17109bu);g[3]=word(g[2]+0xa0u);
        g[1]=s_.flags.plus(g[1]*0xb0u,g[3],4);
        at(0x1710a9u);g[1]=std::uint32_t(std::int32_t(std::int8_t(word(g[1]+0x70u,1))));
        return std::int32_t(g[1]);
    }
    bool no_designator(std::int32_t){s_.flags.minus(s_.gp[1]&255u,255u,1);return (s_.gp[1]&255u)==255u;}
    bool any_fog(){s_.flags.minus(s_.gp[7],0xffffffffu,4);return s_.gp[7]==0xffffffffu;}
    std::int32_t palette(std::int32_t designator){
        auto&g=s_.gp;s_.flags.logic(g[1]&255u,1);
        if(designator<0){
            at(0x1710bbu);g[2]=word(g[2]+0x68u);g[1]=s_.flags.logic(g[1]&127u,4);g[1]*=3u;
            at(0x1710c4u);g[1]=std::uint32_t(std::int32_t(std::int16_t(word(g[2]+g[1]*8u,2))));
        }else g[1]=s_.flags.logic(g[1]&127u,4);
        return std::int32_t(g[1]);
    }
    bool no_palette(std::int32_t){s_.flags.minus(s_.gp[1],0xffffffffu,4);return s_.gp[1]==0xffffffffu;}
    bool matches(std::int32_t){
        auto&g=s_.gp;at(0x1710d2u);g[2]=word(g[6]+0x34cu);
        const auto offset=datum_offset(g[1]);g[1]=offset.value;s_.flags.load(offset.flags);
        at(0x1710dbu);const auto datum=word(g[2]+g[1]+8u);s_.flags.minus(datum,g[7],4);return datum==g[7];
    }
    bool finish(bool contains){
        if(contains)s_.set(0,1u,1,0);
        if(saved_bx_)s_.gp[3]=pop(0x1710e3u);
        s_.gp[6]=pop(0x1710e4u);pop(0x1710e5u);return contains;
    }
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    const auto execute=[&](auto&scope){View view{s,before,scope};contains(view);};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};execute(scope);return true;}
    native_raw_x87_detail::Scope scope{s};execute(scope);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
// Only the complete authenticated 171070 fallback uses this spelling. It
// repairs the old shift helper's undefined-AF policy without changing any
// observation site or admitting a new image/backend to the native body.
template<class State>void retained_datum_offset(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image==SourcePhysicsContactImage::TitleUpdate15){
        const auto offset=datum_offset(s.reg(1,4,0));s.set(1,offset.value,4,0);s.flags.load(offset.flags);return;
    }
#endif
    s.set(1,s.flags.shift(s.reg(1,4,0),4u,4),4,0);
}
} // namespace halo2::native_fog_membership_detail
