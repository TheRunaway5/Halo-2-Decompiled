#pragma once
#include "halo2/native_tu15_node_function_progress_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>

namespace halo2::native_node_function_progress_detail {
using namespace physics_world_runtime_detail;
template<bool Literal,class State,class Dispatch,bool PhysicalStack=false> class SourceFrame {
    State& s;
    Dispatch& dispatch;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {return r(s.memory,address,width);}
    void push(std::uint32_t pc,std::uint32_t value) {
        if constexpr(PhysicalStack){
            const auto next=s.gp[4]-4u;
            w(s.memory,s.observe,pc,next,4,value);
            s.gp[4]=next;
        }else s.push(pc,value);
    }
    void load_one(std::uint32_t pc) {step(pc);s.xmm[1]=_mm_set_ss(f(read(0x46a0d0u)));}
public:
    SourceFrame(State& state,Dispatch& route):s(state),dispatch(route) {}
    bool has_object() {
        load_one(0xd3950u);
        step(0xd3958u);s.set(1,s.flags.logic(s.reg(1,4,0)|0xffffffffu,4),4,0);
        step(0xd395bu);s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4);
        step(0xd395du);s.xmm[0]=s.xmm[1];
        step(0xd3960u);return !s.flags.zf;
    }
    Route select() {
        auto&g=s.gp;
        step(0xd3962u);s.set(2,read(0x4ecbd0u),4,0);
        step(0xd3968u);s.set(2,read(g[2]+0x44u),4,0);
        step(0xd396bu);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0xd3970u);s.set(0,g[0]+g[0]*2u,4,0);
        step(0xd3973u);s.set(0,read(g[2]+g[0]*4u+8u),4,0);
        step(0xd3977u);push(0xd3977u,s.reg(6,4,0));
        step(0xd3978u);
        const auto offset=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(read(g[0]+0x12au,2)));
        s.set(6,std::bit_cast<std::uint32_t>(std::int32_t(offset)),4,0);
        step(0xd397fu);s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(0,4,0),4),4,0);
        step(0xd3981u);s.set(0,read(g[6]+0x7cu),4,0);
        step(0xd3984u);s.flags.minus(s.reg(0,4,0),0x700005cu,4);
        step(0xd3989u);if(s.flags.zf)return Route::direct;
        step(0xd399fu);s.flags.minus(s.reg(0,4,0),0x700005du,4);
        step(0xd39a4u);return s.flags.zf?Route::complement:Route::other;
    }
    template<bool Complement> bool valid_record() {
        step(Complement?0xd39a6u:0xd398bu);
        s.flags.minus(read(s.gp[6]),s.reg(1,4,0),4);
        step(Complement?0xd39a8u:0xd398du);s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
        step(Complement?0xd39abu:0xd3990u);if(s.flags.zf)return false;
        step(Complement?0xd39adu:0xd3992u);
        s.flags.minus(read(s.gp[6]+6u,2),s.reg(1,2,0),2);
        step(Complement?0xd39b1u:0xd3996u);return !s.flags.zf;
    }
    template<bool Complement> void progress() {
        constexpr auto pc=Complement?0xd39b3u:0xd3998u;
        step(pc);push(pc,Complement?0xd39b8u:0xd399du);
        dispatch(s,0x1cf0e0u,pc,false);
    }
    void reload_one() {load_one(0xd39b8u);}
    void complement() {
        step(0xd39c0u);s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[0]);
        step(0xd39c4u);s.xmm[0]=s.xmm[1];
    }
    void finish(Return route) {
        if(route!=Return::absent_object){
            step(route==Return::direct_child?0xd399du:0xd39c7u);s.set(6,s.pop(),4,0);
        }
        step(route==Return::direct_child?0xd399eu:0xd39c8u);s.pop();
    }
};

template<bool Literal,bool PhysicalStack=false,class State,class Dispatch> void run_frame(State& state,Dispatch dispatch) {
    SourceFrame<Literal,State,Dispatch,PhysicalStack> view{state,dispatch};run(view);
}
} // namespace halo2::native_node_function_progress_detail
