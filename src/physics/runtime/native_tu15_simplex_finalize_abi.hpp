#pragma once
#include "halo2/native_tu15_simplex_finalize_kernel.hpp"
#include "physics/runtime/native_tu15_simplex_direction_abi.hpp"

namespace halo2::native_simplex_finalize_detail {
using Data=NativeTu15SimplexFinalize;
using P=NativeTu15SimplexFinalizePhase;
using F=NativeTu15SimplexFinalizeFrame;
using Noop=native_simplex_direction_detail::Noop;
template<class State,class Before=Noop>struct Access {
    State&s;native_raw_x87_detail::Scope<State>&fp;
    native_simplex_direction_detail::Access<State,Before>io;
    std::uint32_t frame_address(F slot,const Data&)const{return s.gp[4]+(slot==F::normalization?0x70u:0x4cu);}
    std::uint32_t argument(P phase,const Data&){
        const auto pc=phase==P::plane_argument?0x30f5e5u:0x30f687u;
        const auto address=s.gp[5]+(phase==P::plane_argument?0x18u:0x14u);
        io.at(pc);std::uint32_t value;fp.io([&]{value=physics_step_detail::r(s.memory,address,4);});
        s.gp[phase==P::plane_argument?0u:1u]=value;return value;
    }
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        std::uint32_t pc{};unsigned r{};
        switch(phase){
        case P::first:pc=0x30f5d7u;r=0;break;
        case P::second:pc=0x30f5dbu;r=1;break;
        case P::axis:pc=0x30f5e2u;r=3;break;
        case P::fresh_axis:s.xmm[0]=s.xmm[1]=d.difference;pc=0x30f5eeu;r=1;break;
        case P::estimate_reload:pc=0x30f61cu;r=2;break;
        default:throw std::runtime_error("Invalid simplex finalization vector read");
        }
        io.at(pc);if(address&15u)throw std::runtime_error("Native simplex finalization MOVAPS load alignment");
        __m128 value;fp.io([&]{value=physics_step_detail::v(s.memory,address);});s.xmm[r]=value;return value;
    }
    void vector_write(P phase,const Data&d,std::uint32_t address,__m128 value){
        std::uint32_t pc{};
        if(phase==P::length){s.xmm[1]=d.length;s.xmm[2]=d.xy;s.xmm[4]=d.z;pc=0x30f60du;}
        else if(phase==P::plane){s.xmm[0]=d.projection_terms;s.xmm[1]=d.reciprocal;s.xmm[2]=d.plane_vector;pc=0x30f65eu;}
        else throw std::runtime_error("Invalid simplex finalization vector store");
        io.at(pc);if(address&15u)throw std::runtime_error("Native simplex finalization MOVAPS store alignment");
        std::array<std::byte,16>written{};std::memcpy(written.data(),&value,16);
        io.bytes(pc,address,std::span(written).first(8));io.bytes(pc,address+8u,std::span(written).last(8));
    }
    std::uint32_t estimate(const Data&,std::uint32_t input){
        io.at(0x30f612u);
        if(!s.services.reciprocal_sqrt)throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
        std::uint32_t value;
        // Retain the real production SourceExecutionInput journal/policy.
        // The callback owns an input observation, rather than a contact result.
        fp.io([&]{value=s.services.reciprocal_sqrt(input);});return value;
    }
    std::uint32_t word_read(P phase,const Data&,std::uint32_t address,unsigned width){
        std::uint32_t pc{};
        switch(phase){
        case P::three_reload:pc=0x30f631u;break;
        case P::half_reload:pc=0x30f643u;break;
        case P::distance_reload:pc=0x30f683u;break;
        case P::first_count_byte:pc=0x30f68du;break;
        case P::second_count_byte:pc=0x30f692u;break;
        case P::first_key:pc=0x30f6a0u;break;
        case P::first_bound:pc=0x30f6a7u;break;
        case P::second_key:pc=0x30f6c0u;break;
        case P::second_bound:pc=0x30f6c6u;break;
        default:throw std::runtime_error("Invalid simplex finalization word read");
        }
        io.at(pc);std::uint32_t value;fp.io([&]{value=physics_step_detail::r(s.memory,address,width);});
        switch(phase){
        case P::three_reload:s.xmm[4]=native_simplex_finalize_numeric::scalar(value);break;
        case P::half_reload:s.xmm[1]=native_simplex_finalize_numeric::scalar(value);break;
        case P::distance_reload:s.gp[2]=value;break;
        case P::first_count_byte:s.set(0,value,1,0);break;
        case P::second_count_byte:s.set(2,value,1,0);break;
        case P::first_key:s.set(3,value,2,0);break;
        case P::first_bound:s.gp[3]=value;break;
        case P::second_key:s.set(1,value,2,0);break;
        case P::second_bound:s.gp[1]=value;break;
        default:break;
        }
        return value;
    }
    void word_write(P phase,const Data&d,std::uint32_t address,std::uint32_t value,unsigned width){
        std::uint32_t pc{};
        switch(phase){
        case P::estimate_store:s.xmm[2]=d.estimated;pc=0x30f616u;break;
        case P::three:s.xmm[1]=d.scaled_length;pc=0x30f629u;break;
        case P::half:s.xmm[4]=d.correction;pc=0x30f63bu;break;
        case P::distance:s.xmm[2]=d.projection_xy;s.xmm[3]=d.distance;s.gp[1]=d.constant;pc=0x30f67fu;break;
        case P::plane_distance:pc=0x30f68au;break;
        case P::first_count_store:pc=0x30f68fu;break;
        case P::second_count_store:pc=0x30f694u;break;
        case P::first_key_store:pc=0x30f6a3u;break;
        case P::second_key_store:pc=0x30f6c3u;break;
        default:throw std::runtime_error("Invalid simplex finalization word store");
        }
        io.at(pc);std::array<std::byte,4>written{};std::memcpy(written.data(),&value,4);io.bytes(pc,address,std::span(written).first(width));
    }
    void prepare(P phase,const Data&d){
        auto&g=s.gp;
        switch(phase){
        case P::begin_first_keys:g[0]=s.flags.logic(0u,4);g[2]=d.work+0x2cu;break;
        case P::advance_first_keys:
            g[0]=s.flags.increment(g[0],4,false);g[2]=s.flags.plus(g[2],16u,4);s.flags.minus(g[0],g[3],4);break;
        case P::begin_second_keys:
            g[2]=s.flags.logic(0u,4);g[0]=g[1]+d.first_written*2u;g[6]=s.flags.plus(g[6],0xecu,4);break;
        case P::advance_second_keys:
            g[0]=s.flags.plus(g[0],2u,4);g[2]=s.flags.increment(g[2],4,false);g[6]=s.flags.plus(g[6],16u,4);s.flags.minus(g[2],g[1],4);break;
        default:throw std::runtime_error("Invalid simplex finalization loop phase");
        }
    }
    std::uint32_t result(const Data&){
        io.at(0x30f6d3u);std::uint32_t value;fp.io([&]{value=physics_step_detail::r(s.memory,s.gp[4]+0x20u,4);});s.gp[0]=value;return value;
    }
    void complete(const Data&){
        for(auto [pc,reg]:{std::pair{0x30f6d7u,7u},std::pair{0x30f6d8u,6u},std::pair{0x30f6d9u,3u}}){
            io.at(pc);std::uint32_t value;fp.io([&]{value=s.pop();});s.gp[reg]=value;
        }
        s.gp[4]=s.gp[5];io.at(0x30f6dcu);std::uint32_t saved;fp.io([&]{saved=s.pop();});s.gp[5]=saved;
        io.at(0x30f6ddu);fp.io([&]{s.pop();});s.gp[4]+=20u;
    }
};
template<class State,class Before=Noop>
std::uint32_t run_private_stage(State&s,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};
    Access<State,Before>memory{s,fp,{s,fp,std::move(before)}};
    Data data;data.work=s.gp[6];data.second_count_address=s.gp[7];data.axis_address=s.gp[3];
    return native_tu15_finalize_simplex_contact(memory,data);
}
} // namespace halo2::native_simplex_finalize_detail
