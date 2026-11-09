#pragma once
#include "halo2/native_tu15_simplex_initialize_kernel.hpp"
#include "physics/runtime/native_tu15_simplex_direction_abi.hpp"

namespace halo2::native_simplex_initialize_detail {
using Data=NativeTu15SimplexInitialize;
using P=NativeTu15SimplexInitializePhase;
using Noop=native_simplex_direction_detail::Noop;
template<class State,class Features,class Before=Noop>struct Access {
    State&s;native_raw_x87_detail::Scope<State>&fp;
    native_simplex_direction_detail::Access<State,Before>io;Features invoke_features;
    void push(std::uint32_t pc,std::uint32_t value){io.at(pc);fp.io([&]{s.push(pc,value);});}
    void begin(Data&){
        auto&g=s.gp;push(0x30e930u,g[5]);g[5]=g[4];g[4]=s.flags.logic(g[4]&0xfffffff0u,4);
        g[4]=s.flags.minus(g[4],0x74u,4);push(0x30e939u,g[3]);
    }
    std::uint32_t argument(P phase,const Data&){
        std::uint32_t pc,offset;unsigned reg;
        switch(phase){
        case P::cache_argument:pc=0x30e93au;offset=0x14u;reg=3;break;
        case P::first_argument:pc=0x30e956u;offset=8u;reg=1;break;
        case P::second_argument:pc=0x30e970u;offset=12u;reg=1;break;
        case P::transform_argument:pc=0x30e9a4u;offset=16u;reg=0;break;
        default:throw std::runtime_error("Invalid simplex initialization argument");
        }
        io.at(pc);std::uint32_t value;fp.io([&]{value=physics_step_detail::r(s.memory,s.gp[5]+offset,4);});s.gp[reg]=value;return value;
    }
    std::uint32_t word_read(P phase,const Data&,std::uint32_t address,unsigned width){
        std::uint32_t pc;unsigned reg;
        switch(phase){
        case P::first_count:pc=0x30e93du;reg=0;break;
        case P::second_count:pc=0x30e946u;reg=1;break;
        case P::current_first:pc=0x30e94fu;reg=0;break;
        case P::first_table:pc=0x30e959u;reg=2;break;
        case P::second_table:pc=0x30e973u;reg=2;break;
        case P::current_second:pc=0x30e97cu;reg=0;break;
        case P::second_first:pc=0x30e97fu;reg=0;break;
        case P::transform_count:pc=0x30e988u;reg=0;break;
        case P::first_owner:pc=0x30e96du;reg=8;break;
        case P::second_owner:pc=0x30e985u;reg=8;break;
        default:throw std::runtime_error("Invalid simplex initialization word read");
        }
        io.at(pc);std::uint32_t value;fp.io([&]{value=physics_step_detail::r(s.memory,address,width);});
        if(reg!=8u)s.gp[reg]=width==1u?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value)))):value;
        return value;
    }
    std::uint32_t frame(P phase,const Data&)const{
        return s.gp[4]+(phase==P::iteration?0x28u:phase==P::epsilon?0x20u:0x20u);
    }
    void word_write(P phase,const Data&,std::uint32_t address,std::uint32_t value){
        const auto pc=phase==P::publish_first?0x30e944u:phase==P::publish_second?0x30e954u:
            phase==P::iteration?0x30e95du:phase==P::epsilon?0x30e965u:0x30e9f3u;
        io.at(pc);std::array<std::byte,4>written{};std::memcpy(written.data(),&value,4);io.bytes(pc,address,written);
    }
    void prepare(P phase,Data&d){
        auto&g=s.gp;
        switch(phase){
        case P::save_work:push(0x30e941u,g[6]);g[6]=g[1];d.work=g[6];break;
        case P::save_count_pointer:
            push(0x30e94au,g[7]);g[0]=g[6]+0x20u;d.output=g[0];push(0x30e94eu,g[0]);break;
        case P::publish_second:g[7]=g[6]+4u;d.count_address=g[7];break;
        case P::first_arguments:push(0x30e95bu,g[0]);d.ids=g[3];push(0x30e95cu,g[3]);break;
        case P::second_output:g[0]=g[6]+0xe0u;d.output=g[0];push(0x30e97bu,g[0]);break;
        case P::second_count_argument:push(0x30e97eu,g[0]);break;
        case P::second_ids:g[0]=g[3]+g[0]*2u;d.ids=g[0];push(0x30e984u,g[0]);break;
        case P::transform_advance:
            g[1]=s.flags.plus(g[1],16u,4);g[3]=s.flags.increment(g[3],4,true);d.source=g[1];d.remaining=g[3];break;
        default:throw std::runtime_error("Invalid simplex initialization preparation");
        }
    }
    void features(P phase,Data&d){
        const auto pc=phase==P::first_features?0x30e96du:0x30e985u;
        // The target operand was read before the real CALL return-word store.
        // Real dispatch owns the complete child ABI, not a DATA handoff.
        fp.io([&]{s.push(pc,phase==P::first_features?0x30e970u:0x30e988u);});
        fp.child([&]{invoke_features(s,d.owner,pc);});
        d.work=s.gp[6];d.count_address=s.gp[7];d.cache=s.gp[3];
    }
    bool transform_begin(Data&d){
        auto&g=s.gp;g[0]=s.flags.increment(g[0],4,true);g[2]=g[6]+0xa0u;g[1]=g[6]+0xe0u;
        d.source=g[1];if(s.flags.sf)return false;
        g[3]=g[6]+0xe0u;g[2]=s.flags.minus(g[2],g[3],4);g[3]=g[0]+1u;
        d.destination_offset=g[2];d.remaining=g[3];return true;
    }
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        std::uint32_t pc;unsigned reg;
        switch(phase){
        case P::point:pc=0x30e9b0u;reg=0;break;
        case P::matrix_x:pc=0x30e9b3u;reg=1;break;
        case P::matrix_y:pc=0x30e9b6u;reg=3;break;
        case P::matrix_z:
            s.xmm[1]=d.x_term;s.xmm[3]=d.y_term;s.xmm[2]=native_simplex_direction_numeric::shuffle<0xaa>(d.point);
            pc=0x30e9d5u;reg=0;break;
        case P::translation:s.xmm[0]=d.z_term;pc=0x30e9dcu;reg=2;break;
        default:throw std::runtime_error("Invalid simplex initialization vector read");
        }
        io.at(pc);if(address&15u)throw std::runtime_error("Native simplex initialization MOVAPS load alignment");
        __m128 value;fp.io([&]{value=physics_step_detail::v(s.memory,address);});s.xmm[reg]=value;return value;
    }
    void vector_write(P,const Data&d,std::uint32_t address,__m128 value){
        s.xmm[0]=d.zt;s.xmm[1]=value;io.at(0x30e9e9u);
        if(address&15u)throw std::runtime_error("Native simplex initialization MOVAPS store alignment");
        std::array<std::byte,16>written{};std::memcpy(written.data(),&value,16);
        io.bytes(0x30e9e9u,address,std::span(written).first(8));io.bytes(0x30e9e9u,address+8u,std::span(written).last(8));
    }
    void complete(const Data&){
        if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x30e9fbu;
    }
};
template<class State,class Features,class Before=Noop>
void run_private_stage(State&s,Features features,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};
    Access<State,Features,Before>memory{s,fp,{s,fp,std::move(before)},std::move(features)};
    Data data;native_tu15_initialize_simplex(memory,data);
}
} // namespace halo2::native_simplex_initialize_detail
