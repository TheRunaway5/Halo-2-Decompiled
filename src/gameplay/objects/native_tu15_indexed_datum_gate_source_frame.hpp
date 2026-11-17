#pragma once
#include "halo2/native_tu15_indexed_datum_gate_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_indexed_datum_gate_detail {
using namespace physics_world_runtime_detail;

template<bool Literal,Globals Addresses,class State> class SourceFrame {
    State& s;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {return r(s.memory,address,width);}
public:
    explicit SourceFrame(State& state):s(state) {}

    bool valid_datum() {
        step(0x3eb10u);s.set(0,s.flags.logic(0u,1),1,0);
        step(0x3eb12u);s.flags.minus(s.gp[2],0xffffffffu,4);
        step(0x3eb15u);return !s.flags.zf;
    }
    bool enabled() {
        step(0x3eb17u);s.set(1,read(Addresses.enabled,1),1,0);
        step(0x3eb1du);s.flags.logic(s.reg(1,1,0),1);
        step(0x3eb1fu);return !s.flags.zf;
    }
    void save_index_register() {step(0x3eb21u);s.push(0x3eb21u,s.gp[6]);}
    bool has_index() {
        step(0x3eb22u);s.gp[6]=read(Addresses.index);
        step(0x3eb28u);s.flags.minus(s.gp[6],0xffffffffu,4);
        step(0x3eb2bu);return !s.flags.zf;
    }
    bool has_selection() {
        step(0x3eb2du);s.flags.minus(read(Addresses.selection,2),0xffffu,2);
        step(0x3eb35u);return !s.flags.zf;
    }
    bool has_rows() {
        auto& g=s.gp;
        step(0x3eb37u);g[1]=read(Addresses.header);
        step(0x3eb3du);s.push(0x3eb3du,g[7]);
        step(0x3eb3eu);g[7]=read(g[1]+0xacu);
        step(0x3eb44u);s.flags.logic(g[7],4);
        step(0x3eb46u);g[7]=s.pop();
        step(0x3eb47u);return !s.flags.zf&&s.flags.sf==s.flags.of;
    }
    bool matches_row() {
        auto& g=s.gp;
        step(0x3eb49u);g[1]=read(g[1]+0xb0u);
        step(0x3eb4fu);g[1]=(read(g[1]+g[6]*2u,2)^0x8000u)-0x8000u;
        step(0x3eb53u);s.flags.minus(g[1],g[2],4);
        step(0x3eb55u);return s.flags.zf;
    }
    void accept() {step(0x3eb57u);s.set(0,1u,1,0);}
    bool finish(bool saved) {
        if(saved){step(0x3eb59u);s.gp[6]=s.pop();}
        step(0x3eb5au);s.pop();return s.reg(0,1,0)!=0;
    }
};

template<bool Literal,Globals Addresses=title_update_15,class State> bool run_source_frame(State& state) {
    SourceFrame<Literal,Addresses,State> view{state};return query(view);
}
} // namespace halo2::native_indexed_datum_gate_detail
