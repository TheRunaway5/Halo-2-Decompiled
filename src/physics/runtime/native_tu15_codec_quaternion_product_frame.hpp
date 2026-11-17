#pragma once
#include "halo2/native_tu15_codec_quaternion_product_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"

namespace halo2::native_codec_quaternion_product_detail {
inline constexpr std::array reads{
    std::array{0x12022au,0x12022fu,0x120233u,0x120238u,0x120241u,0x120246u,0x12024eu,0x120253u},
    std::array{0x120260u,0x120265u,0x120269u,0x12026eu,0x120277u,0x12027cu,0x120285u,0x120289u},
    std::array{0x120297u,0x12029cu,0x1202a1u,0x1202a6u,0x1202aeu,0x1202b3u,0x1202bcu,0x1202c0u},
    std::array{0x1202ceu,0x1202d3u,0x1202d8u,0x1202dcu,0x1202e4u,0x1202e9u,0x1202f2u,0x1202f7u},
};
inline constexpr std::array stores{0x12025cu,0x120292u,0x1202c9u,0x120300u};

struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io {
    Scope* scope;
    template<class Callback>void operator()(Callback&& callback)const {scope->io(std::forward<Callback>(callback));}
};

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;
    void publish(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        Base::store(pc,address,value,[]{});
    }
    template<unsigned Register>void stage() {
        constexpr bool left=Register==1u;
        constexpr std::array load_pcs=left?std::array{0x1201e8u,0x1201eeu,0x1201f5u,0x1201f8u}:
            std::array{0x12020bu,0x120211u,0x120218u,0x12021bu};
        constexpr std::array store_pcs=left?std::array{0x1201eau,0x1201f1u,0x1201ffu,0x1201fbu}:
            std::array{0x12020du,0x120214u,0x120222u,0x12021eu};
        auto& g=s_.gp;
        for(unsigned lane=0;lane<2u;++lane) {
            at(load_pcs[lane]);g[6]=word(g[Register]+lane*4u);
            publish(store_pcs[lane],g[4]+4u+lane*4u,g[6]);
        }
        at(load_pcs[2]);g[6]=word(g[Register]+8u);
        at(load_pcs[3]);g[Register]=word(g[Register]+12u);
        publish(store_pcs[3],g[4]+16u,g[Register]);
        publish(store_pcs[2],g[4]+12u,g[6]);
        g[Register]=g[4]+4u;
    }
public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}}{}
    bool begin() {
        auto& g=s_.gp;
        g[4]=s_.flags.minus(g[4],16u,4u);
        s_.flags.minus(g[1],g[2],4u);
        Base::push(0x1201e5u,g[6]);
        return s_.flags.zf;
    }
    void stage_left(){stage<1u>();}
    bool right_aliases_output(){s_.flags.minus(s_.gp[0],s_.gp[2],4u);return s_.flags.zf;}
    void stage_right(){stage<0u>();}
    __m128& accumulator(){return s_.xmm[0];}
    __m128& term(){return s_.xmm[1];}
    __m128 read(unsigned component,unsigned operand,Field field) {
        const auto member=unsigned(field);
        at(reads[component][operand]);
        const auto value=word(s_.gp[member<4u?1u:0u]+(member&3u)*4u);
        return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});
    }
    static __m128 multiply(__m128 left,__m128 right) {
        asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    static __m128 add(__m128 left,__m128 right) {
        asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    static __m128 subtract(__m128 left,__m128 right) {
        asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    void store_component(unsigned component,__m128 result) {
        publish(stores[component],s_.gp[2]+component*4u,std::bit_cast<std::array<std::uint32_t,4>>(result)[0]);
    }
    void finish() {
        s_.gp[6]=Base::pop(0x120305u);
        s_.gp[4]=s_.flags.plus(s_.gp[4],16u,4u);
        (void)Base::pop(0x120309u);
    }
};

// Private physical publication; retained PUSH and provider mutation are
// separate from this CPU-boundary view.
template<class State,class Before=Noop>void run_private(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};Frame view{state,before,scope};quaternion_product(view);
}
} // namespace halo2::native_codec_quaternion_product_detail
#endif
