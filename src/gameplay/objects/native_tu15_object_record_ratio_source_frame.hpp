#pragma once
#include "halo2/native_tu15_object_record_ratio_kernel.hpp"
#include "gameplay/objects/native_tu15_object_record_ratio_ordered.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_object_record_ratio_detail {
using namespace physics_world_runtime_detail;
template<bool Literal,bool Ordered,class State> class SourceFrame {
    State& s;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {return r(s.memory,address,width);}
    template<unsigned Count> std::uint32_t shift_eax() {
        if constexpr(Ordered)return shift_eax_for_image<Count>(s,s.reg(0,4,0));
        else return s.flags.shift(s.reg(0,4,0),Count,4);
    }
    void finish_at(std::uint32_t pop_pc,std::uint32_t ret_pc) {
        step(pop_pc);s.set(6,s.pop(),4,0);
        step(ret_pc);s.pop();s.gp[4]+=4u;
    }
public:
    explicit SourceFrame(State& state):s(state) {}
    void open_record_table() {
        auto& g=s.gp;
        step(0x107140u);s.set(1,read(0x4ecbd0u),4,0);
        step(0x107146u);s.set(2,read(g[1]+0x44u),4,0);
        step(0x107149u);s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
        step(0x10714cu);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0x107151u);s.set(0,g[0]+g[0]*2u,4,0);
        step(0x107154u);s.set(1,read(g[2]+g[0]*4u+8u),4,0);
        step(0x107158u);s.set(0,read(g[1]),4,0);
        step(0x10715au);s.set(2,read(0x4f0414u),4,0);
        step(0x107160u);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0x107165u);s.set(0,shift_eax<4u>(),4,0);
        step(0x107168u);s.set(2,read(g[0]+g[2]+8u),4,0);
        step(0x10716cu);s.set(0,signed_word(read(g[4]+4u,2)),4,0);
        step(0x107171u);s.push(0x107171u,s.reg(6,4,0));
    }
    bool index_below_count() {
        step(0x107172u);s.flags.minus(read(s.gp[2]+0x2c8u),s.reg(0,4,0),4);
        step(0x107178u);return !s.flags.zf&&s.flags.sf==s.flags.of;
    }
    Record record_kind() {
        auto& g=s.gp;
        step(0x10717au);s.set(6,g[0]+g[0]*2u,4,0);
        step(0x10717du);s.set(1,g[1]+g[6]*4u+0x20cu,4,0);
        step(0x107184u);s.set(6,(read(g[1],1)^0x80u)-0x80u,4,0);
        step(0x107187u);s.set(6,s.flags.increment(s.reg(6,4,0),4,true),4,0);
        step(0x107188u);if(s.flags.zf)return Record::ratio;
        step(0x10718au);s.set(6,s.flags.increment(s.reg(6,4,0),4,true),4,0);
        step(0x10718bu);return s.flags.zf?Record::constant:Record::other;
    }
    void load_constant() {step(0x10718du);s.xmm[0]=_mm_set_ss(f(read(0x46a0d0u)));}
    void subtract_scaled_ratio() {
        auto& g=s.gp;
        step(0x107199u);s.set(1,signed_word(read(g[1]+2u,2)),4,0);
        step(0x10719du);s.set(2,read(g[2]+0x2ccu),4,0);
        step(0x1071a3u);s.xmm[0]=_mm_set_ss(f(read(0x46a0d0u)));
        step(0x1071abu);s.set(0,shift_eax<6u>(),4,0);
        step(0x1071aeu);s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(s.reg(1,4,0)));
        step(0x1071b2u);s.xmm[1]=_mm_div_ss(s.xmm[1],_mm_set_ss(f(read(g[0]+g[2]+0x1cu))));
        step(0x1071b8u);s.set(0,read(0x51df54u),4,0);
        step(0x1071bdu);s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(read(g[0]+4u))));
        step(0x1071c2u);s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
    }
    __m128 finish_constant() {finish_at(0x107195u,0x107196u);return s.xmm[0];}
    __m128 finish() {finish_at(0x1071c6u,0x1071c7u);return s.xmm[0];}
private:
    static std::uint32_t signed_word(std::uint32_t value) {return (value^0x8000u)-0x8000u;}
};

template<bool Literal,bool Ordered=false,class State> __m128 run_source_frame(State& state) {
    SourceFrame<Literal,Ordered,State> view{state};return query(view);
}
} // namespace halo2::native_object_record_ratio_detail
