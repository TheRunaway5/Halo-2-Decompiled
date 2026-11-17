#pragma once
#include "halo2/native_tu15_codec_transform_composition_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_quaternion_product_frame.hpp"

namespace halo2::native_codec_transform_composition_detail {
using native_codec_quaternion_product_detail::Io;
using native_codec_quaternion_product_detail::Noop;

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Product=native_codec_quaternion_product_detail::Frame<State,Before>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;
    Scope& scope_;

    static __m128 scalar(std::uint32_t bits) {
        return std::bit_cast<__m128>(std::array<std::uint32_t,4>{bits,0,0,0});
    }
    std::uint32_t address(Field field,unsigned offset) const {
        using enum Field;
        switch(field) {
        case left:return s_.gp[3]+offset;
        case right:return s_.gp[5]+offset;
        case output:return s_.gp[2]+offset;
        case two:return 0x46a11cu;
        case one:return 0x46a0d0u;
        }
        throw std::runtime_error("Invalid composition field");
    }
    __m128 read_scalar(std::uint32_t pc,Field field,unsigned offset) {
        at(pc);return scalar(word(address(field,offset)));
    }
    void stage(unsigned input,std::uint32_t pc) {
        auto& g=s_.gp;
        g[6]=g[input];g[1]=8u;g[7]=g[4]+16u;
        while(g[1]) {
            at(pc);const auto value=word(g[6]);
            Base::store(pc,g[7],value,[&]{
                const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
                g[6]+=delta;g[7]+=delta;--g[1];
            },true);
        }
        g[input]=g[4]+16u;
    }
public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}},scope_(scope){}
    bool begin() {
        auto& g=s_.gp;g[4]=s_.flags.minus(g[4],32u,4u);
        Base::push(0x120573u,g[3]);at(0x120574u);g[3]=word(g[4]+40u);
        s_.flags.minus(g[3],g[2],4u);
        Base::push(0x12057au,g[5]);at(0x12057bu);g[5]=word(g[4]+48u);
        Base::push(0x12057fu,g[6]);Base::push(0x120580u,g[7]);
        return s_.flags.zf;
    }
    void stage_left(){stage(3u,0x12058eu);}
    bool right_aliases_output(){s_.flags.minus(s_.gp[5],s_.gp[2],4u);return s_.flags.zf;}
    void stage_right(){stage(5u,0x1205a3u);}
    void quaternion_product() {
        auto& g=s_.gp;g[0]=g[5];g[1]=g[3];
        Base::push(0x1205adu,0x1205b2u);
        scope_.child([&] {
            native_codec_quaternion_product_detail::run_private(s_,std::ref(before_));
        });
    }
    void load(unsigned destination,std::uint32_t pc,Field field,unsigned offset) {
        s_.xmm[destination]=read_scalar(pc,field,offset);
    }
    void multiply(unsigned destination,std::uint32_t pc,Field field,unsigned offset) {
        const auto operand=read_scalar(pc,field,offset);
        s_.xmm[destination]=Product::multiply(s_.xmm[destination],operand);
    }
    void add(unsigned destination,std::uint32_t pc,Field field,unsigned offset) {
        const auto operand=read_scalar(pc,field,offset);
        s_.xmm[destination]=Product::add(s_.xmm[destination],operand);
    }
    void multiply(unsigned destination,unsigned source) {
        s_.xmm[destination]=Product::multiply(s_.xmm[destination],s_.xmm[source]);
    }
    void add(unsigned destination,unsigned source) {
        s_.xmm[destination]=Product::add(s_.xmm[destination],s_.xmm[source]);
    }
    void subtract(unsigned destination,unsigned source) {
        s_.xmm[destination]=Product::subtract(s_.xmm[destination],s_.xmm[source]);
    }
    void copy(unsigned destination,unsigned source){s_.xmm[destination]=s_.xmm[source];}
    void store(std::uint32_t pc,unsigned offset,unsigned source) {
        const auto bits=std::bit_cast<std::array<std::uint32_t,4>>(s_.xmm[source])[0];
        Base::store(pc,s_.gp[2]+offset,bits,[]{});
    }
    bool scale_differs_from_one() {
        s_.flags.scalar_compare(s_.xmm[0],s_.xmm[5],true);
        s_.set(0,s_.flags.bits()|2u,1u,8u);
        s_.flags.logic((s_.gp[0]>>8u)&0x44u,1u);
        return s_.flags.pf;
    }
    void copy_right_scale() {
        at(0x1206f7u);s_.gp[0]=word(s_.gp[5]+28u);
        Base::store(0x1206fau,s_.gp[2]+28u,s_.gp[0],[]{});
    }
    void restore_index_registers() {
        s_.gp[7]=Base::pop(0x120716u);s_.gp[6]=Base::pop(0x120717u);
    }
    void restore_frame_register(){s_.gp[5]=Base::pop(0x120727u);}
    void finish() {
        auto& g=s_.gp;g[3]=Base::pop(0x12072du);
        g[4]=s_.flags.plus(g[4],32u,4u);(void)Base::pop(0x120731u);g[4]+=8u;
    }
};

// Private raw caller: actual 1201E0 runs from the real call packet and its
// returned state is used by every later input/output acquisition.
template<class State,class Before=Noop>void run_private(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    Frame view{state,before,scope};compose_transform(view);
}
} // namespace halo2::native_codec_transform_composition_detail
#endif
