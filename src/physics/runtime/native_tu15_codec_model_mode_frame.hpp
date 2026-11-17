#pragma once
#include "halo2/native_tu15_codec_model_mode_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_model_initialize_frame.hpp"

namespace halo2::native_codec_model_mode_detail {
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
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        Base::store(pc,address,value,[]{});
    }
    void scalar_store(std::uint32_t pc,std::uint32_t address,unsigned lane) {
        store(pc,address,std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[lane])));
    }
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        const std::array bytes{std::byte(value)};io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
    bool signed_less()const{return s_.flags.sf!=s_.flags.of;}
    bool signed_greater()const{return !signed_less()&&!s_.flags.zf;}
    bool above()const{return !s_.flags.cf&&!s_.flags.zf;}
    static void from_signed(__m128& destination,std::uint32_t value) {
        asm volatile("cvtsi2ssl %1,%0":"+x"(destination):"r"(value));
    }
    static void subtract(__m128& left,__m128 right) {
        asm volatile("subss %1,%0":"+x"(left):"x"(right));
    }
public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}}{}
    void acquire_secondary_index() {
        auto& g=s_.gp;
        s_.xmm[2]=scalar(0x28371eu,g[4]+48u);scalar_store(0x283724u,g[4]+44u,2u);
        s_.xmm[0]=scalar(0x28372au,g[4]+44u);
        asm volatile("cvttss2si %1,%0":"=r"(g[0]):"x"(s_.xmm[0]));
        s_.flags.minus(g[0],g[3],4u);
        if(signed_less())g[2]=s_.flags.logic(0u,4u);
        else {
            s_.flags.minus(g[0],g[1],4u);g[2]=g[1];
            if(!signed_greater())g[2]=g[0];
        }
    }
    void publish_secondary_interval() {
        auto& g=s_.gp;
        g[0]=s_.flags.increment(g[0],4u,false);s_.flags.minus(g[0],g[3],4u);
        store(0x283747u,0x510f84u,g[2]);
        if(signed_less())store(0x28374fu,0x510f88u,g[3]);
        else {
            s_.flags.minus(g[0],g[1],4u);store(0x283759u,0x510f88u,g[1]);
            if(!signed_greater())store(0x283761u,0x510f88u,g[0]);
        }
    }
    void publish_secondary_fraction() {
        auto& g=s_.gp;
        from_signed(s_.xmm[0],g[2]);g[2]=load(0x28376au,0x510f44u);
        g[0]=g[2]+1u;s_.flags.minus(g[0],g[3],4u);subtract(s_.xmm[2],s_.xmm[0]);
        scalar_store(0x283779u,0x510f8cu,2u);
        if(signed_less())store(0x283783u,0x510f48u,g[3]);
        else {
            s_.flags.minus(g[0],g[1],4u);store(0x28378du,0x510f48u,g[1]);
            if(!signed_greater())store(0x283795u,0x510f48u,g[0]);
        }
        g[7]=load(0x28379au,0x510f54u);from_signed(s_.xmm[0],g[2]);
        subtract(s_.xmm[1],s_.xmm[0]);s_.xmm[0]=scalar(0x2837a8u,g[4]+52u);
        scalar_store(0x2837aeu,0x510f4cu,1u);scalar_store(0x2837b6u,0x510f90u,0u);
    }
    bool late_primary_mode() {
        const auto& g=s_.gp;
        s_.flags.minus(load(0x2837c0u,g[4]+40u,1u),g[3]&255u,1u);
        return !s_.flags.zf;
    }
    bool at_last_primary_index() {
        s_.flags.minus(s_.gp[0],s_.gp[1],4u);return s_.flags.zf;
    }
    void publish_primary_ceiling() {
        auto& g=s_.gp;
        g[2]=g[0]+1u;s_.flags.minus(g[2],g[3],4u);
        if(signed_less())store(0x2837d1u,0x510f48u,g[3]);
        else {
            s_.flags.minus(g[2],g[1],4u);store(0x2837dbu,0x510f48u,g[1]);
            if(!signed_greater())store(0x2837e3u,0x510f48u,g[2]);
        }
    }
    void publish_primary_fraction() {
        from_signed(s_.xmm[5],s_.gp[0]);subtract(s_.xmm[1],s_.xmm[5]);
        s_.flags.scalar_compare(s_.xmm[2],s_.xmm[1],false);
        scalar_store(0x2837f4u,0x510f4cu,1u);
    }
    bool below_primary_epsilon()const{return above();}
    bool above_primary_limit() {
        s_.flags.scalar_compare(s_.xmm[1],s_.xmm[3],false);return above();
    }
    void promote_primary_ceiling(){store(0x283803u,0x510f44u,s_.gp[2]);}
    void clear_primary_fraction(){scalar_store(0x283809u,0x510f4cu,0u);}
    void clear_primary_mode(){store_byte(0x283811u,s_.gp[4]+40u,s_.gp[3]&255u);}
};

// First phase and both live modes through the common 283815 boundary.
// This never invokes a selector, transform, or supplied child outcome.
template<class State,class Before=Noop>Boundary run_private_prefix(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    native_codec_model_initialize_detail::Frame init{state,before,scope};
    const auto boundary=native_codec_model_initialize_detail::initialize(init);
    Frame view{state,before,scope};compose_mode(view,boundary);
    return boundary;
}
} // namespace halo2::native_codec_model_mode_detail
#endif
