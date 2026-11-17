#pragma once
#include "halo2/native_tu15_device_property_query_kernel.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include <bit>
#include <cmath>

namespace halo2::native_device_property_query_detail {
using namespace physics_world_runtime_detail;
template<bool Literal,bool Fresh,class State,class Spill> class SourceFrame {
    State& s;
    Spill& spill_word;
    void step(std::uint32_t pc) {
        // HALO2_CENSUS_NATIVE_BRIDGES: this complete native skips its
        // original-PC reports under full callbacks without a cursor.
        if(native_census_detail::skip_report(s.services,pc))return;
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {return r(s.memory,address,width);}
    __m128 scalar(std::uint32_t address) {return _mm_set_ss(f(read(address)));}
    std::uint32_t key(std::uint32_t site,std::uint32_t original) {
        return source_object_function_compiled_symbol_tu15(s.memory,site,original);
    }
    bool below_or_equal() const {return s.flags.cf||s.flags.zf;}
    void lahf_test(std::uint32_t lahf,std::uint32_t test) {
        step(lahf);s.set(0,s.flags.bits()|2u,1,8);
        step(test);s.flags.logic(s.reg(0,1,8)&0x44u,1);
    }
    void clear_c1() {if constexpr(Fresh)s.fp.status&=~0x200u;}
    void push_memory(std::uint32_t pc,unsigned reg,std::uint32_t offset) {
        step(pc);s.fp.push(ext(f(read(s.gp[reg]+offset))));clear_c1();
    }
    void push_threshold(std::uint32_t pc) {
        step(pc);s.fp.push(ext(f(read(0x46a0ecu))));clear_c1();
    }
    void abs_value(std::uint32_t pc) {
        step(pc);s.fp.st(0)=std::fabs(s.fp.st(0));clear_c1();
    }
    void pop_value(std::uint32_t pc) {
        step(pc);s.fp.st(0)=s.fp.st(0);clear_c1();s.fp.pop();
    }
    void compare_pop(std::uint32_t pc) {step(pc);s.compare_and_pop(s.fp.st(1));}
    void spill_reload(std::uint32_t store,std::uint32_t load) {
        step(store);
        if constexpr(Fresh)w(s.memory,s.observe,store,8u+s.gp[4],4,spill_word(s.fp.st(0),s.fp.status));
        else w(s.memory,s.observe,store,8u+s.gp[4],4,u(spill(s.fp.st(0))));
        // Retained source observers run before the modeled pop.
        s.fp.pop();step(load);s.xmm[0]=scalar(8u+s.gp[4]);
    }
    std::uint32_t finish(std::uint32_t restore,std::uint32_t ret) {
        step(restore);s.set(3,s.pop(),4,0);
        step(ret);s.pop();s.gp[4]+=16u;return s.gp[0];
    }
    Key high_selector() {
        step(0x10bf78u);s.flags.minus(s.reg(0,4,0),0x8000080u,4);
        step(0x10bf7du);if(s.flags.zf)return Key::scalar_140;
        step(0x10bf83u);s.flags.minus(s.reg(0,4,0),key(0x10bf83u,0xf00055eu),4);
        step(0x10bf88u);if(s.flags.zf)return Key::change_power;
        step(0x10bf8eu);s.flags.minus(s.reg(0,4,0),key(0x10bf8eu,0x1200055fu),4);
        step(0x10bf93u);return s.flags.zf?Key::change_position:Key::unknown;
    }
public:
    SourceFrame(State& state,Spill& spill_policy):s(state),spill_word(spill_policy) {}
    Key select() {
        auto&g=s.gp;
        step(0x10be20u);s.set(0,read(g[4]+4u),4,0);
        step(0x10be24u);s.set(1,read(0x4ecbd0u),4,0);
        step(0x10be2au);s.set(2,read(g[1]+0x44u),4,0);
        step(0x10be2du);s.xmm[1]=_mm_xor_ps(s.xmm[1],s.xmm[1]);
        step(0x10be30u);s.xmm[2]=scalar(0x46a0d0u);
        step(0x10be38u);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0x10be3du);s.set(0,g[0]+g[0]*2u,4,0);
        step(0x10be40u);s.set(1,read(8u+g[2]+g[0]*4u),4,0);
        step(0x10be44u);s.set(0,read(g[1]),4,0);
        step(0x10be46u);s.set(2,read(0x4f0414u),4,0);
        step(0x10be4cu);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0x10be51u);s.set(0,s.flags.shift(s.reg(0,4,0),4u,4),4,0);
        step(0x10be54u);s.set(2,read(8u+g[0]+g[2]),4,0);
        step(0x10be58u);s.set(0,read(g[4]+8u),4,0);
        step(0x10be5cu);s.flags.minus(s.reg(0,4,0),key(0x10be5cu,0x6000560u),4);
        step(0x10be61u);s.push(0x10be61u,s.reg(3,4,0));
        step(0x10be62u);s.set(3,1u,1,0);
        step(0x10be64u);s.xmm[0]=s.xmm[1];
        step(0x10be67u);if(!s.flags.zf&&s.flags.sf==s.flags.of)return high_selector();
        step(0x10be6du);if(s.flags.zf)return Key::locked;
        step(0x10be73u);s.flags.minus(s.reg(0,4,0),key(0x10be73u,0x40005abu),4);
        step(0x10be78u);if(s.flags.zf)return Key::loop;
        step(0x10be7au);s.flags.minus(s.reg(0,4,0),0x5000081u,4);
        step(0x10be7fu);if(s.flags.zf)return Key::scalar_134;
        step(0x10be81u);s.flags.minus(s.reg(0,4,0),key(0x10be81u,0x5000561u),4);
        step(0x10be86u);return s.flags.zf?Key::delay:Key::unknown;
    }
    std::uint32_t unknown() {
        step(0x10bf95u);s.set(0,s.flags.logic(0u,1),1,0);return finish(0x10bf97u,0x10bf98u);
    }
    Output delay() {
        auto&g=s.gp;auto*x=s.xmm;
        step(0x10be8cu);x[3]=scalar(g[2]+0x10cu);
        step(0x10be94u);s.flags.scalar_compare(x[3],x[1],false);
        step(0x10be97u);if(below_or_equal())return Output::direct;
        step(0x10be9du);s.set(0,read(0x51df54u),4,0);
        step(0x10bea2u);
        const auto count=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(read(g[1]+0x148u,2)));
        s.set(1,std::bit_cast<std::uint32_t>(std::int32_t(count)),4,0);
        step(0x10bea9u);x[0]=scalar(g[0]+4u);
        step(0x10beaeu);x[0]=_mm_div_ss(x[0],x[3]);
        step(0x10beb2u);x[3]=_mm_cvtsi32_ss(x[3],si(s.reg(1,4,0)));
        step(0x10beb6u);x[0]=_mm_mul_ss(x[0],x[3]);
        step(0x10bebau);return Output::clamped;
    }
    void loop() {
        auto*x=s.xmm;
        step(0x10beccu);x[0]=scalar(0x46a194u);
        step(0x10bed4u);s.flags.scalar_compare(x[0],scalar(s.gp[1]+0x140u),false);
        step(0x10bedbu);
        if(s.flags.cf){
            step(0x10beebu);s.set(0,s.flags.logic(0u,4),4,0);
            step(0x10beedu);x[0]=_mm_cvtsi32_ss(x[0],si(s.reg(0,4,0)));
            step(0x10bef1u);
        }else{
            step(0x10beddu);s.set(0,1u,4,0);
            step(0x10bee2u);x[0]=_mm_cvtsi32_ss(x[0],si(s.reg(0,4,0)));
            step(0x10bee6u);
        }
    }
    void scalar_134() {step(0x10bebfu);s.xmm[0]=scalar(s.gp[1]+0x134u);step(0x10bec7u);}
    void scalar_140() {step(0x10c064u);s.xmm[0]=scalar(s.gp[1]+0x140u);}
    Output locked() {
        auto&g=s.gp;auto*x=s.xmm;
        step(0x10bef6u);x[3]=scalar(g[1]+0x134u);
        step(0x10befeu);s.flags.scalar_compare(x[3],x[1],true);
        lahf_test(0x10bf01u,0x10bf02u);
        step(0x10bf05u);if(!s.flags.pf){step(0x10bf07u);x[0]=x[2];}
        step(0x10bf0au);s.flags.minus(read(g[1]+0xaau,1),7u,1);
        step(0x10bf11u);if(!s.flags.zf)return Output::clamped;
        step(0x10bf17u);s.set(0,read(g[1]+0x13cu),4,0);
        step(0x10bf1du);s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
        step(0x10bf20u);if(s.flags.zf)return Output::clamped;
        step(0x10bf26u);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0x10bf2bu);s.set(2,g[0]+g[0]*2u,4,0);
        step(0x10bf2eu);s.set(0,read(0x4ecbf8u),4,0);
        step(0x10bf33u);s.set(0,read(g[0]+0x44u),4,0);
        step(0x10bf36u);s.set(0,g[0]+g[2]*4u,4,0);
        step(0x10bf39u);s.set(2,read(g[1]+0x1ccu),4,0);
        step(0x10bf3fu);s.flags.logic(s.reg(2,1,0)&0xdu,1);
        step(0x10bf42u);if(!s.flags.zf){step(0x10bf44u);x[0]=x[2];}
        step(0x10bf47u);s.set(0,read(g[0]+2u,2),2,0);
        step(0x10bf4bu);s.flags.logic(s.reg(0,1,0)&1u,1);
        step(0x10bf4du);
        if(!s.flags.zf){
            step(0x10bf4fu);s.flags.logic(s.reg(0,1,0)&2u,1);
            step(0x10bf51u);if(!s.flags.zf){step(0x10bf53u);x[0]=x[2];}
        }
        step(0x10bf56u);x[3]=scalar(g[1]+0x140u);
        step(0x10bf5eu);s.flags.scalar_compare(x[3],x[2],true);
        lahf_test(0x10bf61u,0x10bf62u);
        step(0x10bf65u);
        if(s.flags.pf){
            step(0x10bf67u);s.flags.logic(s.reg(2,1,0)&0x10u,1);
            step(0x10bf6au);if(s.flags.zf)return Output::clamped;
        }
        step(0x10bf70u);x[0]=x[1];step(0x10bf73u);return Output::direct;
    }
    Output change_position() {
        auto&g=s.gp;auto*x=s.xmm;
        step(0x10bf9bu);x[3]=scalar(g[1]+0x144u);
        step(0x10bfa3u);s.flags.scalar_compare(x[3],x[1],true);
        lahf_test(0x10bfa6u,0x10bfa7u);
        step(0x10bfaau);if(!s.flags.pf)return Output::direct;
        step(0x10bfb0u);s.set(0,read(g[1]+0x12cu),4,0);
        step(0x10bfb6u);s.flags.logic(s.reg(0,1,0)&8u,1);
        step(0x10bfb8u);x[0]=x[1];
        step(0x10bfbbu);
        if(!s.flags.zf) {
            step(0x10bfbdu);s.flags.logic(s.reg(0,1,0)&0x20u,1);
            step(0x10bfbfu);if(!s.flags.zf)return Output::direct;
            push_memory(0x10bfc5u,1,0x164u);abs_value(0x10bfcbu);
            step(0x10bfcdu);s.fp.push(s.fp.st(0));clear_c1();
            push_threshold(0x10bfcfu);compare_pop(0x10bfd5u);pop_value(0x10bfd7u);
            step(0x10bfd9u);
            if(!below_or_equal()){pop_value(0x10c080u);return Output::direct;}
            push_memory(0x10bfdfu,1,0x144u);abs_value(0x10bfe5u);
            step(0x10bfe7u);s.fp.st(0)=divide(s.fp.st(0),s.fp.st(1));
            spill_reload(0x10bfe9u,0x10bfedu);pop_value(0x10bff3u);
            step(0x10bff5u);return Output::clamped;
        }
        push_memory(0x10bff7u,2,0xc8u);abs_value(0x10bffdu);
        push_threshold(0x10bfffu);compare_pop(0x10c005u);pop_value(0x10c007u);
        step(0x10c009u);if(!below_or_equal())return Output::direct;
        push_memory(0x10c00bu,1,0x144u);abs_value(0x10c011u);
        step(0x10c013u);s.fp.st(0)=divide(s.fp.st(0),ext(f(read(g[2]+0xc8u))));
        spill_reload(0x10c019u,0x10c01du);step(0x10c023u);return Output::clamped;
    }
    Output change_power() {
        auto&g=s.gp;auto*x=s.xmm;
        step(0x10c025u);x[3]=scalar(g[1]+0x138u);
        step(0x10c02du);s.flags.scalar_compare(x[3],x[1],true);
        lahf_test(0x10c030u,0x10c031u);
        step(0x10c034u);if(!s.flags.pf)return Output::direct;
        step(0x10c036u);x[3]=scalar(g[2]+0xc0u);
        step(0x10c03eu);s.flags.scalar_compare(x[3],x[1],true);
        lahf_test(0x10c041u,0x10c042u);
        step(0x10c045u);x[0]=x[1];
        step(0x10c048u);if(!s.flags.pf)return Output::direct;
        push_memory(0x10c04au,1,0x138u);abs_value(0x10c050u);
        step(0x10c052u);s.fp.st(0)=divide(s.fp.st(0),ext(f(read(g[2]+0xc0u))));
        spill_reload(0x10c058u,0x10c05cu);step(0x10c062u);return Output::clamped;
    }
    void clamp() {
        auto*x=s.xmm;
        step(0x10c06cu);s.flags.scalar_compare(x[1],x[0],false);
        step(0x10c06fu);
        if(!below_or_equal()){step(0x10c071u);x[0]=x[1];step(0x10c074u);return;}
        step(0x10c076u);s.flags.scalar_compare(x[0],x[2],false);
        step(0x10c079u);
        if(!below_or_equal()){step(0x10c07bu);x[0]=x[2];step(0x10c07eu);}
    }
    std::uint32_t publish() {
        auto&g=s.gp;auto*x=s.xmm;
        step(0x10c082u);s.flags.scalar_compare(x[0],x[1],false);
        step(0x10c085u);s.set(1,read(g[4]+16u),4,0);
        step(0x10c089u);w(s.memory,s.observe,0x10c089u,g[1],4,u(_mm_cvtss_f32(x[0])));
        step(0x10c08du);
        if(below_or_equal()){
            step(0x10c0a0u);s.set(2,read(g[4]+20u),4,0);
            step(0x10c0a4u);s.set(0,s.flags.logic(0u,4),4,0);
            step(0x10c0a6u);w(s.memory,s.observe,0x10c0a6u,g[2],1,s.reg(0,1,0));
            step(0x10c0a8u);s.set(0,s.reg(3,1,0),1,0);
            return finish(0x10c0aau,0x10c0abu);
        }
        step(0x10c08fu);s.set(2,read(g[4]+20u),4,0);
        step(0x10c093u);s.set(0,1u,4,0);
        step(0x10c098u);w(s.memory,s.observe,0x10c098u,g[2],1,s.reg(0,1,0));
        step(0x10c09au);s.set(0,s.reg(3,1,0),1,0);
        return finish(0x10c09cu,0x10c09du);
    }
};

template<bool Literal,bool Fresh,class State,class Spill>
std::uint32_t run_source_frame(State& state,Spill spill_policy) {
    SourceFrame<Literal,Fresh,State,Spill> view{state,spill_policy};return query(view);
}
} // namespace halo2::native_device_property_query_detail
