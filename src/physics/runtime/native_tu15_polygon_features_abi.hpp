#pragma once
#include "halo2/native_tu15_polygon_features_kernel.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_simplex_candidate_abi.hpp"
#endif

namespace halo2::native_polygon_features_detail {
using Data=NativeTu15PolygonFeatures;
using P=NativeTu15PolygonFeaturesPhase;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Noop=native_simplex_direction_detail::Noop;
template<bool Actual,class State,class Before=Noop>struct Access {
    State&s;native_raw_x87_detail::Scope<State>&fp;
    native_simplex_direction_detail::Access<State,Before>io;
    static constexpr std::uint32_t site(std::uint32_t pc){return Actual?pc:pc-0x48f0u;}
    void at(std::uint32_t pc){io.at(site(pc));}
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width){
        at(pc);std::uint32_t value;fp.io([&]{value=physics_step_detail::r(s.memory,address,width);});return value;
    }
    std::uint32_t count(const Data&){return s.gp[0]=read(0x1f6690u,s.gp[4]+8u,4u);}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);fp.io([&]{s.push(site(pc),value);});}
    void prepare(P phase,Data&d){
        auto&g=s.gp;
        switch(phase){
        case P::count:g[0]=s.flags.increment(g[0],4,true);break;
        case P::begin_rows:
            push(0x1f6697u,g[6]);g[6]=read(0x1f6698u,g[4]+0x10u,4u);
            g[2]=native_simplex_candidate_detail::shift4(s,g[0]);g[2]=s.flags.plus(g[2],g[6],4u);
            g[6]=read(0x1f66a3u,g[4]+8u,4u);d.ids=g[6];d.output=g[2];d.shape=g[1];push(0x1f66a7u,g[7]);break;
        case P::advance:
            g[0]=s.flags.increment(g[0],4,true);g[2]=s.flags.minus(g[2],16u,4u);s.flags.logic(g[0],4u);break;
        case P::finish:
            if(!((d.count-1u)&0x80000000u)){
                at(0x1f66c9u);std::uint32_t value;fp.io([&]{value=s.pop();});g[7]=value;
                at(0x1f66cau);fp.io([&]{value=s.pop();});g[6]=value;
            }
            at(0x1f66cbu);fp.io([&]{s.pop();});g[4]+=12u;break;
        default:throw std::runtime_error("Invalid polygon feature phase");
        }
    }
    std::uint32_t feature(const Data&,std::uint32_t address){return s.gp[7]=read(0x1f66b0u,address,2u);}
    std::array<std::byte,16>point(const Data&,std::uint32_t address){
        s.gp[7]=s.flags.plus(s.gp[7],2u,4u);s.gp[7]=native_simplex_candidate_detail::shift4(s,s.gp[7]);
        at(0x1f66bau);if(address&15u)throw std::runtime_error("Native polygon feature MOVAPS load alignment");
        __m128 value;fp.io([&]{value=physics_step_detail::v(s.memory,address);});s.xmm[0]=value;
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16u);return bytes;
    }
    void publish(const Data&,std::uint32_t address,const std::array<std::byte,16>&value){
        at(0x1f66beu);if(address&15u)throw std::runtime_error("Native polygon feature MOVAPS store alignment");
        io.bytes(site(0x1f66beu),address,std::span(value).first(8));io.bytes(site(0x1f66beu),address+8u,std::span(value).last(8));
    }
};
template<bool Actual=true,class State,class Before=Noop>
void run_private_owner(State&s,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};
    Access<Actual,State,Before>memory{s,fp,{s,fp,std::move(before)}};
    Data data;native_tu15_expand_polygon_features(memory,data);
}
#endif
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_POLYGON_FEATURES");return p&&*p;}();return enabled;}
struct Activity{std::uint64_t actual_calls{},canonical_calls{};};
inline Activity&activity(){thread_local Activity value;return value;}
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
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(!admitted(s))return false;
    if constexpr(requires{s.selected_observation_pcs;}){++activity().actual_calls;run_private_owner<true>(s);}
    else{++activity().canonical_calls;run_private_owner<false>(s);}
    return true;
#endif
}
} // namespace halo2::native_polygon_features_detail
