#pragma once
#include "halo2/native_tu15_particle_type_filter_kernel.hpp"
#include "gameplay/objects/native_tu15_object_body_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_particle_type_filter_detail {
using namespace physics_world_runtime_detail;

template<bool Ordered,class State> std::uint32_t multiply_row(State& s,std::uint32_t stride) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(Ordered)
        return native_object_body_detail::arithmetic<0>(s,s.gp[0],stride);
#endif
    return s.flags.product(s.gp[0],stride);
}

template<bool Literal,bool Ordered,class State,class Lookup> class SourceFrame {
    State& s;
    Lookup lookup;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {return r(s.memory,address,width);}
    void leave(std::uint32_t saved,std::uint32_t exit) {
        step(saved);s.gp[3]=s.pop();
        step(exit);s.pop();s.gp[4]+=4u;
    }
public:
    SourceFrame(State& state,Lookup local_lookup):s(state),lookup(local_lookup) {}
    void lookup_body() {
        auto& g=s.gp;
        step(0x17de60u);g[1]=read(0x4f720cu);
        step(0x17de66u);g[2]=read(g[1]+0x44u);
        step(0x17de69u);g[0]=s.flags.logic(g[0]&0xffffu,4);
        step(0x17de6eu);g[0]=multiply_row<Ordered>(s,0x190u);
        step(0x17de74u);g[0]=read(g[0]+g[2]+0x48u);
        step(0x17de78u);s.push(0x17de78u,g[3]);
        step(0x17de79u);s.push(0x17de79u,3u);
        step(0x17de7bu);s.set(3,s.flags.logic(0u,1),1,0);
        step(0x17de7du);s.push(0x17de7du,0x17de82u);lookup(s);
    }
    bool has_body() {
        step(0x17de82u);s.flags.logic(s.gp[0],4);
        step(0x17de84u);return !s.flags.zf;
    }
    bool has_definition() {
        auto& g=s.gp;
        step(0x17de86u);g[0]=read(g[0]+0x13cu);
        step(0x17de8cu);s.flags.minus(g[0],0xffffffffu,4);
        step(0x17de8fu);return !s.flags.zf;
    }
    void compare_definition_type() {
        auto& g=s.gp;
        step(0x17de91u);g[1]=read(0x4f54f4u);
        step(0x17de97u);g[2]=read(g[1]+0x44u);
        step(0x17de9au);g[1]=read(g[4]+8u);
        step(0x17de9eu);g[0]=s.flags.logic(g[0]&0xffffu,4);
        step(0x17dea3u);g[0]=multiply_row<Ordered>(s,0x21cu);
        step(0x17dea9u);g[0]=(read(g[0]+g[2]+0x28u,2)^0x8000u)-0x8000u;
        step(0x17deaeu);s.flags.minus(g[0],g[1],4);
    }
    void return_comparison() {
        step(0x17deb0u);s.set(0,s.flags.zf,1,0);
        leave(0x17deb3u,0x17deb4u);
    }
    void return_missing() {
        step(0x17deb7u);s.set(0,s.reg(3,1,0),1,0);
        leave(0x17deb9u,0x17debau);
    }
};

template<bool Literal,bool Ordered,class State,class Lookup> void run_source_frame(State& state,Lookup lookup) {
    SourceFrame<Literal,Ordered,State,Lookup> frame{state,lookup};filter(frame);
}
} // namespace halo2::native_particle_type_filter_detail
