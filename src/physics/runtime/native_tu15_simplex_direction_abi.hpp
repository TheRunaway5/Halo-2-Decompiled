#pragma once
#include "halo2/native_tu15_simplex_direction_kernel.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <utility>

namespace halo2::native_simplex_direction_detail {
using P=NativeTu15SimplexDirectionPhase;
using Data=NativeTu15SimplexDirection;
using Side=NativeTu15SimplexDirectionSide;
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{};};
// Private retained continuation, not a whole30E930 admission or default.
// Vector stores preserve the current runtime's two8-byte bank/observer
// protocol. This does not qualify physical MOVAPS page-fault atomicity.
template<class State,class Before=Noop>struct Access {
    State&s;native_raw_x87_detail::Scope<State>&fp;Before before{};
    static bool first(const Data&d){return d.side==Side::first_edge;}
    void at(std::uint32_t pc){
        if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;
        fp.io([&]{before(s,pc);});
    }
    std::uint32_t vector_pc(P phase,const Data&d)const{
        const bool a=first(d);
        switch(phase){
        case P::endpoint:return a?0x30f175u:0x30f066u;
        case P::first:return a?0x30f179u:0x30f06du;
        case P::opposite:return a?0x30f17du:0x30f074u;
        case P::payload:return a?0x30f245u:0x30f139u;
        case P::promote_endpoint:return a?0x30f249u:0x30f140u;
        case P::promote_payload:return a?0x30f24du:0x30f147u;
        case P::edge_direction:return a?0x30f22eu:0x30f122u;
        case P::point_second:return 0x30f253u;
        case P::point_first:return 0x30f25au;
        case P::point_direction:return 0x30f261u;
        default:throw std::runtime_error("Invalid native simplex vector phase");
        }
    }
    void projected(const Data&d){
        auto&x=s.xmm;const auto&p=d.projection;
        x[0]=p.first_xy;x[1]=p.first_terms;x[2]=p.edge;
        x[3]=p.from_endpoint;x[4]=p.from_first;x[5]=d.endpoint;
        x[6]=p.first_sum;x[7]=p.endpoint_sum;
        s.gp[0]=s.flags.logic(p.endpoint_sign,4);
        const unsigned r=first(d)?1u:2u;
        s.gp[r]=s.flags.logic(p.first_sign,4);
        s.flags.minus(s.gp[0],s.gp[r],4);
    }
    void interior(const Data&d){
        auto&x=s.xmm;const auto&p=d.projection;
        x[4]=native_simplex_direction_numeric::shuffle<0xc9>(p.from_endpoint);
        if(first(d)){
            x[0]=p.direction;
            x[1]=native_simplex_direction_numeric::shuffle<0xd2>(p.inner_cross);
            x[2]=p.edge;x[3]=p.outer_negative;
        }else{
            x[0]=p.inner_cross;
            x[1]=native_simplex_direction_numeric::shuffle<0xd2>(p.edge);
            x[2]=p.direction;x[3]=p.outer_negative;
        }
    }
    void prepare(P phase,const Data&d){
        if(phase==P::projected)projected(d);
        else if(phase==P::select_endpoint)s.flags.logic(s.gp[0],4);
        else throw std::runtime_error("Invalid native simplex preparation phase");
    }
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        const auto pc=vector_pc(phase,d);at(pc);
        if(address&15u)throw std::runtime_error("Native simplex MOVAPS load alignment");
        __m128 value;fp.io([&]{value=physics_step_detail::v(s.memory,address);});
        unsigned r;
        switch(phase){
        case P::endpoint:r=5;break;
        case P::first:case P::payload:case P::point_first:r=0;break;
        case P::opposite:case P::point_second:r=1;break;
        default:throw std::runtime_error("Invalid native simplex vector load phase");
        }
        s.xmm[r]=value;return value;
    }
    void bytes(std::uint32_t pc,std::uint32_t address,std::span<const std::byte>written){
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s.observe){event.before.resize(written.size());fp.io([&]{s.memory.read(address,event.before);});}
        fp.io([&]{s.memory.write(address,written);});
        if(s.observe){event.written.assign(written.begin(),written.end());fp.io([&]{s.observe(event);});}
    }
    void vector_write(P phase,const Data&d,std::uint32_t address,__m128 value){
        if(phase==P::edge_direction)interior(d);
        else if(phase==P::point_direction)s.xmm[1]=d.direction;
        const auto pc=vector_pc(phase,d);at(pc);
        if(address&15u)throw std::runtime_error("Native simplex MOVAPS store alignment");
        std::array<std::byte,16>written{};std::memcpy(written.data(),&value,16);
        bytes(pc,address,std::span(written).first(8));
        bytes(pc,address+8u,std::span(written).last(8));
    }
    std::uint32_t word_pc(P phase,const Data&d)const{
        if(phase==P::decrement_count)return first(d)?0x30f251u:0x30f14eu;
        if(phase==P::second_count){
            if(d.side==Side::points||d.projection.endpoint_sign==d.projection.first_sign)return 0x30f268u;
            return first(d)?0x30f235u:0x30f129u;
        }
        if(phase==P::publish_count){
            if(d.side==Side::points||d.projection.endpoint_sign==d.projection.first_sign)return 0x30f26au;
            return first(d)?0x30f237u:0x30f12bu;
        }
        throw std::runtime_error("Invalid native simplex word phase");
    }
    std::uint32_t word_read(P phase,const Data&d,std::uint32_t address){
        at(word_pc(phase,d));std::uint32_t value;
        fp.io([&]{value=physics_step_detail::r(s.memory,address,4);});
        if(phase==P::second_count)s.gp[2]=value;
        return value;
    }
    void word_write(P phase,const Data&d,std::uint32_t address,std::uint32_t value){
        // Preserve the retained C++ RMW protocol. Physical pre-write flags
        // atomicity needs its own injected-fault proof before default binding.
        if(phase==P::decrement_count)s.flags.increment(value+1u,4,true);
        const auto pc=word_pc(phase,d);
        if(phase!=P::decrement_count)at(pc);
        std::array<std::byte,4>written{};std::memcpy(written.data(),&value,4);bytes(pc,address,written);
    }
    void complete(P phase,const Data&d){
        s.gp[0]=s.flags.logic(0u,4);
        const auto pc=phase==P::finish_points?0x30f26fu:(first(d)?0x30f23cu:0x30f130u);
        if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;
    }
};
template<class State,class Before=Noop>
void run_private_stage(State&s,Side side,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};
    Access<State,Before>memory{s,fp,std::move(before)};
    Data data;data.side=side;data.work=s.gp[6];data.second_count_address=s.gp[7];
    native_tu15_reduce_simplex_direction(memory,data);
}
} // namespace halo2::native_simplex_direction_detail
