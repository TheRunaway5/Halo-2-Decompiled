#pragma once
#include "halo2/native_tu15_object_definition_mode_query_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_object_definition_mode_query_detail {
using namespace physics_world_runtime_detail;
template<bool Literal,class State> class SourceFrame {
    State& s;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {return r(s.memory,address,width);}
public:
    explicit SourceFrame(State& state):s(state) {}
    bool mode_is_nine() {
        step(0x165590u);s.set(2,read(0x4f3218u),4,0);
        step(0x165596u);s.push(0x165596u,s.reg(6,4,0));
        step(0x165597u);s.set(6,read(s.gp[2]+0x180u),4,0);
        step(0x16559du);s.set(0,s.flags.logic(0u,1),1,0);
        step(0x16559fu);s.flags.minus(s.reg(6,4,0),9u,4);
        step(0x1655a2u);s.set(6,s.pop(),4,0);
        step(0x1655a3u);return s.flags.zf;
    }
    bool has_datum() {
        step(0x1655a5u);s.flags.minus(s.reg(1,4,0),0xffffffffu,4);
        step(0x1655a8u);return !s.flags.zf;
    }
    bool definition_word_is_one() {
        auto& g=s.gp;
        step(0x1655aau);s.set(2,read(0x4ecbd0u),4,0);
        step(0x1655b0u);s.set(2,read(g[2]+0x44u),4,0);
        step(0x1655b3u);s.set(1,s.flags.logic(g[1]&0xffffu,4),4,0);
        step(0x1655b9u);s.set(1,g[1]+g[1]*2u,4,0);
        step(0x1655bcu);s.set(1,read(g[2]+g[1]*4u+8u),4,0);
        step(0x1655c0u);s.set(2,read(g[1]),4,0);
        step(0x1655c2u);s.set(1,read(0x4f0414u),4,0);
        step(0x1655c8u);s.set(2,s.flags.logic(g[2]&0xffffu,4),4,0);
        step(0x1655ceu);s.set(2,s.flags.shift(g[2],4u,4),4,0);
        step(0x1655d1u);s.set(2,read(g[2]+g[1]+8u),4,0);
        step(0x1655d5u);s.flags.minus(read(g[2]+0x290u,2),1u,2);
        step(0x1655ddu);return s.flags.zf;
    }
    void accept() {step(0x1655dfu);s.set(0,1u,1,0);}
    bool finish() {step(0x1655e1u);s.pop();return s.reg(0,1,0)!=0;}
};

template<bool Literal,class State> bool run_source_frame(State& state) {
    SourceFrame<Literal,State> view{state};return query(view);
}
} // namespace halo2::native_object_definition_mode_query_detail
