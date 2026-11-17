#pragma once
#include "halo2/native_tu15_codec_model_selector_setup_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_model_mode_frame.hpp"

namespace halo2::native_codec_model_selector_setup_detail {
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
    std::uint32_t signed_half(std::uint32_t pc,std::uint32_t address) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(load(pc,address,2u)))));
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
    void align_tracks() {
        auto& g=s_.gp;
        g[2]=s_.flags.plus(g[2],31u,4u);g[2]=s_.flags.right(g[2],3u,4u,true);
        g[2]=s_.flags.logic(g[2]&0xfffffffcu,4u);
    }
public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}}{}
    bool has_override() {
        auto& g=s_.gp;
        s_.flags.minus(load(0x283815u,0x510f58u),g[3],4u);
        store(0x28381bu,0x510f34u,g[3]);g[1]=1u;
        return !s_.flags.zf;
    }
    void publish_override() {
        auto& g=s_.gp;
        g[0]=load(0x283828u,g[4]+28u);
        s_.flags.logic(load(0x28382cu,g[0]+28u,1u)&(g[1]&255u),1u);
        g[2]=!s_.flags.zf;g[2]=s_.flags.increment(g[2],4u,false);
        store(0x283838u,0x510f34u,g[2]);g[0]=load(0x28383eu,g[7]+19u,1u);
        store(0x283842u,0x510f3cu,g[0]);
    }
    void publish_default_limit() {
        auto& g=s_.gp;
        g[0]=load(0x283849u,g[7]+19u,1u);g[2]=load(0x28384du,0x510f40u);
        s_.flags.minus(g[2],g[0],4u);store(0x283855u,0x510f3cu,g[0]);
        if(s_.flags.zf||s_.flags.sf!=s_.flags.of)store(0x28385cu,0x510f3cu,g[2]);
    }
    void select_weight() {
        auto& g=s_.gp;
        s_.flags.scalar_compare(s_.xmm[4],s_.xmm[3],false);
        store(0x283865u,0x510f30u,g[3]);
        s_.set(0u,s_.flags.cf||s_.flags.zf?s_.flags.logic(0u,1u):g[1],1u,0u);
    }
    bool is_blended_model() {
        auto& g=s_.gp;
        g[7]=load(0x283873u,g[4]+20u);
        s_.flags.minus(load(0x283877u,g[7]+16u,1u),g[1]&255u,1u);
        return s_.flags.zf;
    }
    void publish_blended_mode() {
        auto& g=s_.gp;
        s_.set(0u,s_.flags.minus(0u,g[0]&255u,1u),1u,0u);
        g[0]=s_.flags.borrow(g[0],g[0],4u);g[0]=s_.flags.plus(g[0],3u,4u);
        store(0x283883u,0x510f30u,g[0]);
    }
    bool uses_zero_weight_mode() {
        s_.flags.minus(s_.gp[0]&255u,s_.gp[3]&255u,1u);return s_.flags.zf;
    }
    void publish_zero_weight_mode(){store(0x28388eu,0x510f30u,s_.gp[1]);}
    void publish_model_weight() {
        store(0x283894u,0x510f50u,std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[4])));
    }
    bool needs_primary_selector() {
        const auto& g=s_.gp;
        s_.flags.minus(load(0x28389cu,g[7]+16u,1u),g[3]&255u,1u);
        return s_.flags.zf;
    }
    bool has_primary_stream() {
        auto& g=s_.gp;g[1]=load(0x2838a5u,g[6]+4u);
        s_.flags.minus(load(0x2838a8u,g[1]+6u,2u),g[3]&65535u,2u);
        return !s_.flags.zf;
    }
    void publish_decoder_table() {
        auto& g=s_.gp;
        g[0]=load(0x2838b2u,g[6]);store(0x2838b4u,0x510f60u,g[0]);
        g[0]=load(0x2838b9u,g[0],1u);g[2]=g[0]*5u;g[0]=0x48c33cu+g[2]*8u;
        g[1]=load(0x2838c6u,g[0]);g[2]=load(0x2838c8u,g[0]+4u);g[0]=load(0x2838cbu,g[0]+8u);
        store(0x2838ceu,0x510f68u,g[2]);store(0x2838d4u,0x510f64u,g[1]);store(0x2838dau,0x510f6cu,g[0]);
    }
    void publish_stream_offsets() {
        auto& g=s_.gp;
        g[0]=load(0x2838dfu,g[6]+4u);g[1]=signed_half(0x2838e2u,g[0]+6u);
        g[1]=s_.flags.plus(g[1],load(0x2838e6u,g[0]+12u),4u);
        g[1]=s_.flags.plus(g[1],load(0x2838e9u,g[6]),4u);store(0x2838ebu,0x510f70u,g[1]);

        g[2]=load(0x2838f1u,g[6]+8u,1u);g[0]=load(0x2838f5u,g[6]+4u);
        g[1]=signed_half(0x2838f8u,g[0]+6u);align_tracks();
        g[2]=s_.flags.plus(g[2],g[1],4u);g[1]=load(0x283907u,g[0]+12u);
        g[0]=load(0x28390au,g[6]);g[2]=s_.flags.plus(g[2],g[1],4u);g[2]=s_.flags.plus(g[2],g[0],4u);
        store(0x283910u,0x510f74u,g[2]);

        g[2]=load(0x283916u,g[6]+8u,1u);g[0]=load(0x28391au,g[6]+4u);
        g[1]=signed_half(0x28391du,g[0]+6u);align_tracks();
        g[2]=g[1]+g[2]*2u;g[1]=load(0x28392du,g[0]+12u);g[0]=load(0x283930u,g[6]);
        g[2]=s_.flags.plus(g[2],g[1],4u);g[2]=s_.flags.plus(g[2],g[0],4u);
        store(0x283936u,0x510f78u,g[2]);
    }
    void publish_selector_channel(){store_byte(0x28393cu,0x510f38u,s_.gp[3]&255u);}
};

template<class State,class Before=Noop>Boundary run_private_prefix(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    native_codec_model_initialize_detail::Frame initial{state,before,scope};
    const auto branch=native_codec_model_initialize_detail::initialize(initial);
    if(branch==native_codec_model_initialize_detail::Boundary::returned)return Boundary::returned;
    native_codec_model_mode_detail::Frame mode{state,before,scope};
    native_codec_model_mode_detail::compose_mode(mode,branch);
    Frame setup{state,before,scope};return prepare_selector(setup);
}
} // namespace halo2::native_codec_model_selector_setup_detail
#endif
