#pragma once
#include "halo2/native_tu15_codec_model_initialize_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_quaternion_product_frame.hpp"

namespace halo2::native_codec_model_initialize_detail {
using native_codec_quaternion_product_detail::Io;
using native_codec_quaternion_product_detail::Noop;

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;
    std::uint32_t load(std::uint32_t pc,std::uint32_t address,unsigned width=4u) {
        at(pc);return word(address,width);
    }
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_set_ss(std::bit_cast<float>(load(pc,address)));
    }
    std::uint32_t scalar_bits(unsigned lane)const {
        return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[lane]));
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        Base::store(pc,address,value,[]{});
    }
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        const std::array bytes{std::byte(value)};io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}}{}
    bool begin() {
        auto& g=s_.gp;
        s_.xmm[2]=scalar(0x2835b0u,0x46a0ecu);
        s_.flags.scalar_compare(s_.xmm[4],s_.xmm[2],false);
        s_.set(2,load(0x2835bbu,g[4]+32u,1u),1u,0u);
        Base::push(0x2835bfu,g[3]);g[3]=load(0x2835c0u,g[4]+16u);
        Base::push(0x2835c4u,g[7]);g[7]=load(0x2835c5u,g[4]+16u);
        return !s_.flags.cf&&!s_.flags.zf;
    }
    void finish_low_weight() {
        auto& g=s_.gp;g[7]=Base::pop(0x283a3du);g[3]=Base::pop(0x283a3eu);
        (void)Base::pop(0x283a3fu);g[4]+=40u;
    }
    void acquire_index() {
        auto& g=s_.gp;
        Base::push(0x2835cfu,g[5]);store(0x2835d0u,g[4]+44u,scalar_bits(1u));
        s_.xmm[0]=scalar(0x2835d6u,g[4]+44u);
        asm volatile("cvttss2si %1,%0":"=r"(g[0]):"x"(s_.xmm[0]));
        const auto count=std::uint16_t(load(0x2835e0u,g[7]+20u,2u));
        g[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(count)));
        g[5]=s_.flags.logic(0u,4u);
    }
    void clear_first_planes() {
        for(unsigned n=0;n<22u;++n)store(0x2835e6u+n*6u,0x56e000u+n*4u,s_.gp[5]);
    }
    void publish_pair_and_finish_planes() {
        auto& g=s_.gp;
        store(0x28366au,0x510f3cu,g[3]);store(0x283670u,0x510f40u,g[3]);
        g[3]=s_.flags.logic(0u,4u);g[1]=s_.flags.increment(g[1],4u,true);
        s_.flags.minus(g[0],g[3],4u);
        store(0x28367bu,0x56e058u,g[5]);store(0x283681u,0x56e05cu,g[5]);
    }
    bool negative_index()const{return s_.flags.sf!=s_.flags.of;}
    void zero_index(){s_.gp[0]=s_.flags.logic(0u,4u);}
    bool above_last_index() {
        s_.flags.minus(s_.gp[0],s_.gp[1],4u);
        return !s_.flags.zf&&s_.flags.sf==s_.flags.of;
    }
    void use_last_index(){s_.gp[0]=s_.gp[1];}
    void publish_decoder_inputs() {
        auto& g=s_.gp;
        g[5]=load(0x283693u,g[4]+28u);s_.xmm[3]=scalar(0x283697u,0x46a0d0u);
        s_.xmm[0]=_mm_setzero_ps();
        store(0x2836a2u,0x510f58u,g[5]);g[5]=load(0x2836a8u,g[4]+32u);
        store(0x2836acu,0x510f5cu,g[5]);g[5]=s_.flags.logic(0u,4u);
        s_.flags.minus(g[2]&255u,g[3]&255u,1u);
        store(0x2836b6u,0x510f64u,g[5]);store(0x2836bcu,0x510f68u,g[5]);
        store(0x2836c2u,0x510f6cu,g[5]);g[5]=load(0x2836c8u,g[4]+36u);
        store(0x2836ccu,0x510f50u,scalar_bits(3u));s_.xmm[3]=scalar(0x2836d4u,0x46a188u);
        store(0x2836dcu,0x510f44u,g[0]);store(0x2836e1u,0x510f48u,g[0]);
        store(0x2836e6u,0x510f4cu,scalar_bits(0u));store(0x2836eeu,0x510f54u,g[7]);
        store(0x2836f4u,0x510f60u,g[3]);store(0x2836fau,0x510f70u,g[3]);
        store(0x283700u,0x510f74u,g[3]);store(0x283706u,0x510f78u,g[3]);
        store(0x28370cu,0x510f7cu,g[5]);store_byte(0x283712u,0x510f80u,g[2]&255u);
    }
    bool has_secondary()const{return !s_.flags.zf;}
};

template<class State,class Before=Noop>Boundary run_private_prefix(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};Frame view{state,before,scope};
    return initialize(view);
}
} // namespace halo2::native_codec_model_initialize_detail
#endif
