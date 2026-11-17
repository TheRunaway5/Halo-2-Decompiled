#pragma once
#include "halo2/native_tu15_map_world_property_query_kernel.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_map_world_property_query_detail {
// Match the authored scalar instruction when a live source callback supplies
// a NaN in the destination before a multiply by a nonfinite scale.
inline __m128 ordered_multiply(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulss %1,%0":"+x"(destination):"x"(source));return destination;
#else
    return _mm_mul_ss(destination,source);
#endif
}

template<bool Literal,class State> class SourceFrame {
    State& s_;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s_.instruction_literal(pc);
        else if(s_.services.instruction)s_.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {
        return physics_world_runtime_detail::r(s_.memory,address,width);
    }
    __m128 scalar(std::uint32_t address) {
        return _mm_set_ss(std::bit_cast<float>(read(address)));
    }
    std::uint32_t finish(std::uint32_t pc) {
        step(pc);s_.pop();s_.gp[4]+=4u;return s_.gp[0];
    }
public:
    explicit SourceFrame(State& state):s_(state) {}
    Selection select() {
        auto& ecx=s_.gp[1];
        step(0x1db800u);ecx=s_.flags.minus(ecx,
            source_object_function_compiled_symbol_tu15(s_.memory,0x1db800u,0xa000693u),4);
        step(0x1db806u);s_.xmm[1]=_mm_xor_ps(s_.xmm[1],s_.xmm[1]);
        step(0x1db809u);s_.set(0,1u,1,0);
        step(0x1db80bu);if(s_.flags.zf)return Selection::flag;
        step(0x1db80du);ecx=s_.flags.minus(ecx,0x2fffffeu,4);
        step(0x1db813u);if(s_.flags.zf)return Selection::second_byte;
        step(0x1db815u);ecx=s_.flags.increment(ecx,4,true);
        step(0x1db816u);return s_.flags.zf?Selection::first_byte:Selection::unsupported;
    }
    std::uint32_t unsupported() {
        step(0x1db818u);s_.set(0,s_.flags.logic(0u,1),1,0);return finish(0x1db81au);
    }
    void flag_value() {
        step(0x1db841u);s_.gp[1]=read(s_.gp[6]+4u);
        step(0x1db844u);s_.flags.logic(s_.reg(1,1,8)&0x20u,1);
        step(0x1db847u);s_.xmm[0]=scalar(0x46a0d0u);
        step(0x1db84fu);
        if(s_.flags.zf){step(0x1db851u);s_.xmm[0]=s_.xmm[1];}
    }
    void byte_value(bool second) {
        step(second?0x1db82fu:0x1db81du);s_.gp[1]=read(s_.gp[6]+(second?0x1bu:0x1au),1);
        step(second?0x1db833u:0x1db821u);
        s_.xmm[0]=_mm_cvtsi32_ss(s_.xmm[0],std::bit_cast<std::int32_t>(s_.gp[1]));
        step(second?0x1db837u:0x1db825u);s_.xmm[0]=ordered_multiply(s_.xmm[0],scalar(0x46a0c4u));
        step(second?0x1db83fu:0x1db82du);
    }
    std::uint32_t publish() {
        auto& gp=s_.gp;
        step(0x1db854u);s_.flags.scalar_compare(s_.xmm[0],s_.xmm[1],false);
        step(0x1db857u);physics_world_runtime_detail::w(s_.memory,s_.observe,0x1db857u,gp[2],4,
            std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0])));
        step(0x1db85bu);
        if(s_.flags.cf||s_.flags.zf) {
            step(0x1db86bu);gp[2]=read(gp[4]+4u);
            step(0x1db86fu);gp[1]=s_.flags.logic(0u,4);
            step(0x1db871u);physics_world_runtime_detail::w(s_.memory,s_.observe,0x1db871u,gp[2],1,s_.reg(1,1,0));
            return finish(0x1db873u);
        }
        step(0x1db85du);gp[2]=read(gp[4]+4u);
        step(0x1db861u);gp[1]=1u;
        step(0x1db866u);physics_world_runtime_detail::w(s_.memory,s_.observe,0x1db866u,gp[2],1,s_.reg(1,1,0));
        return finish(0x1db868u);
    }
};

template<bool Literal,class State> std::uint32_t run_source_frame(State& state) {
    SourceFrame<Literal,State> view{state};return query(view);
}
} // namespace halo2::native_map_world_property_query_detail
