#pragma once
#include "halo2/native_tu15_object_ratio_caller_high_kernel.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_ordered.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_object_ratio_caller_detail {
using namespace physics_world_runtime_detail;

template<bool Literal,class State> class SourceRatioFrame {
protected:
    State& s;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {return r(s.memory,address,width);}
    __m128 scalar(std::uint32_t address) {return _mm_set_ss(f(read(address)));}
    std::uint32_t symbol(std::uint32_t pc,std::uint32_t key) {
        return source_object_function_compiled_symbol_tu15(s.memory,pc,key);
    }
    void compare_key(std::uint32_t pc,std::uint32_t key) {
        step(pc);s.flags.minus(s.gp[6],symbol(pc,key),4);
    }
    bool below_or_equal() const {return s.flags.cf||s.flags.zf;}
    void restore_end(std::uint32_t ebx,std::uint32_t ecx,std::uint32_t ret) {
        step(ebx);s.gp[3]=s.pop();
        step(ecx);s.gp[1]=s.pop();
        step(ret);s.pop();s.gp[4]+=16u;
    }
public:
    explicit SourceRatioFrame(State& state):s(state) {}

    void clamp_result() {
        auto& x=s.xmm;
        step(0x104ce6u);s.flags.scalar_compare(x[3],x[2],false);
        step(0x104ce9u);
        if(!below_or_equal()){
            step(0x104cebu);x[2]=x[3];step(0x104ceeu);return;
        }
        step(0x104cf0u);s.flags.scalar_compare(x[2],x[4],false);
        step(0x104cf3u);if(!below_or_equal()){step(0x104cf5u);x[2]=x[4];}
    }
    void finish_result() {
        auto& g=s.gp;
        step(0x104cf8u);s.flags.scalar_compare(s.xmm[2],s.xmm[3],false);
        step(0x104cfbu);g[2]=read(g[4]+0x20u);
        step(0x104cffu);w(s.memory,s.observe,0x104cffu,g[2],4,u(_mm_cvtss_f32(s.xmm[2])));
        step(0x104d03u);
        if(!below_or_equal()){
            step(0x104d05u);g[1]=read(g[4]+0x24u);
            step(0x104d09u);g[7]=s.pop();
            step(0x104d0au);g[6]=s.pop();
            step(0x104d0bu);g[0]=1u;
            step(0x104d10u);g[5]=s.pop();
            step(0x104d11u);w(s.memory,s.observe,0x104d11u,g[1],1,1u);
            restore_end(0x104d13u,0x104d14u,0x104d15u);
        }else{
            step(0x104d18u);g[1]=read(g[4]+0x24u);
            step(0x104d1cu);g[7]=s.pop();
            step(0x104d1du);g[6]=s.pop();
            step(0x104d1eu);g[0]=s.flags.logic(0u,4);
            step(0x104d20u);g[5]=s.pop();
            step(0x104d21u);w(s.memory,s.observe,0x104d21u,g[1],1,0u);
            step(0x104d23u);s.set(0,1u,1,0);
            restore_end(0x104d25u,0x104d26u,0x104d27u);
        }
    }
    void finish_unknown() {
        auto& g=s.gp;
        step(0x1049d0u);g[7]=s.pop();
        step(0x1049d1u);g[6]=s.pop();
        step(0x1049d2u);g[5]=s.pop();
        step(0x1049d3u);s.set(0,s.flags.logic(0u,1),1,0);
        restore_end(0x1049d5u,0x1049d6u,0x1049d7u);
    }
};

template<bool Literal,bool Ordered,class State> class SourceHighFrame:public SourceRatioFrame<Literal,State> {
    using Base=SourceRatioFrame<Literal,State>;
    using Base::s;using Base::step;using Base::read;using Base::scalar;using Base::symbol;using Base::compare_key;
    bool upper_middle_{};
public:
    explicit SourceHighFrame(State& state):Base{state} {}

    bool exact_high_key() {step(0x104bfeu);return s.flags.zf;}
    bool subtract_high_key(std::uint32_t key) {
        upper_middle_=key==0x14000570u;
        const auto mov=upper_middle_?0x104bcbu:0x104c04u;
        const auto sub=upper_middle_?0x104bcdu:0x104c06u;
        step(mov);s.gp[0]=s.gp[6];
        step(sub);s.gp[0]=s.flags.minus(s.gp[0],symbol(sub,key),4);
        step(upper_middle_?0x104bd2u:0x104c0bu);
        return s.flags.zf;
    }
    bool subtract_high_offset(std::uint32_t value) {
        const bool near=value==8u;
        step(upper_middle_?0x104bd8u:near?0x104c0du:0x104c12u);
        s.gp[0]=s.flags.minus(s.gp[0],value,4);
        step(upper_middle_?0x104bdbu:near?0x104c10u:0x104c17u);
        return s.flags.zf;
    }
    bool sample_record(bool second) {
        auto& g=s.gp;
        step(second?0x104cc5u:0x104c6au);g[1]=read(g[3]+0x2d0u);
        step(second?0x104ccbu:0x104c70u);g[0]=s.flags.logic(0u,4);
        compare_key(second?0x104ccdu:0x104c72u,second?0x1500056eu:0x14000570u);
        step(second?0x104cd3u:0x104c78u);s.set(0,!s.flags.zf,1,0);
        step(second?0x104cd6u:0x104c7bu);s.flags.minus(g[0],g[1],4);
        step(second?0x104cd8u:0x104c7du);
        if(s.flags.sf==s.flags.of)return false;
        step(second?0x104cdau:0x104c7fu);g[0]=s.flags.product(g[0],0x34u);
        step(second?0x104cddu:0x104c82u);s.xmm[2]=scalar(g[0]+g[5]+(second?0x1b8u:0x1b4u));
        if(!second)step(0x104c8bu);
        return true;
    }
    void sample_flag(bool upper_middle) {
        const auto reg=upper_middle?0u:2u;
        step(upper_middle?0x104be1u:0x104c4bu);s.set(reg,read(s.gp[5]+0x16eu,1),1,0);
        step(upper_middle?0x104be7u:0x104c51u);s.set(reg,s.flags.right(s.reg(reg,1,0),upper_middle?1u:2u,1,false),1,0);
        step(upper_middle?0x104be9u:0x104c54u);s.flags.logic(s.reg(reg,1,0)&1u,1);
        step(upper_middle?0x104bebu:0x104c57u);
        if(!s.flags.zf){
            step(upper_middle?0x104bf1u:0x104c5du);s.xmm[2]=scalar(0x46a0d0u);
            step(upper_middle?0x104bf9u:0x104c65u);
        }
    }
    bool sample_clock_digit(bool tens) {
        auto& g=s.gp;auto& x=s.xmm;
        step(tens?0x104c8du:0x104c1du);g[0]=read(g[3]+0x2c0u);
        step(tens?0x104c93u:0x104c23u);s.flags.logic(g[0],4);
        step(tens?0x104c95u:0x104c25u);
        if(s.flags.zf||s.flags.sf!=s.flags.of)return false;
        step(tens?0x104c97u:0x104c2bu);
        g[tens?1u:0u]=(read(g[5]+0x22cu,2)^0x8000u)-0x8000u;
        if(tens){
            step(0x104c9eu);g[0]=0x66666667u;
            step(0x104ca3u);
            const auto product=std::int64_t(si(g[0]))*si(g[1]);
            g[0]=std::uint32_t(product);g[2]=std::uint32_t(std::uint64_t(product)>>32u);
            s.flags.cf=s.flags.of=product!=std::int64_t(si(g[0]));
            step(0x104ca5u);g[2]=s.flags.right(g[2],2u,4,true);
            step(0x104ca8u);g[0]=g[2];
            step(0x104caau);g[0]=s.flags.right(g[0],31u,4,false);
            step(0x104cadu);g[0]=s.flags.plus(g[0],g[2],4);
        }
        step(tens?0x104cafu:0x104c32u);g[2]=si(g[0])<0?0xffffffffu:0u;
        step(tens?0x104cb0u:0x104c33u);g[1]=10u;
        step(tens?0x104cb5u:0x104c38u);divide_clock_digit<Ordered>(s);
        step(tens?0x104cb7u:0x104c3au);x[2]=_mm_cvtsi32_ss(x[2],si(g[2]));
        step(tens?0x104cbbu:0x104c3eu);x[2]=_mm_mul_ss(x[2],scalar(0x46a178u));
        step(tens?0x104cc3u:0x104c46u);
        return true;
    }
};
} // namespace halo2::native_object_ratio_caller_detail
