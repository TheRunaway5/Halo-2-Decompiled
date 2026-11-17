#pragma once
#include "halo2/native_tu15_codec_inverse_transform_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"

namespace halo2::native_codec_inverse_transform_detail {
enum class Address {input,output,fixed};
struct Operand {std::uint32_t pc;Address base;std::uint32_t offset;};
inline constexpr auto input=Address::input,output=Address::output,fixed=Address::fixed;
inline constexpr std::array reads{
    Operand{0x120740u,fixed,0x46a11cu},Operand{0x12074eu,fixed,0x46a0d0u},
    Operand{0x12076cu,output,12u},Operand{0x120771u,fixed,0x46a0dcu},
    Operand{0x12077eu,input,16u},Operand{0x120783u,output,0u},
    Operand{0x120787u,input,20u},Operand{0x12078cu,output,4u},
    Operand{0x120791u,input,16u},Operand{0x120796u,output,4u},
    Operand{0x12079bu,input,20u},Operand{0x1207a0u,output,8u},
    Operand{0x1207a9u,output,8u},Operand{0x1207aeu,input,24u},
    Operand{0x1207b7u,output,0u},Operand{0x1207bbu,input,24u},
    Operand{0x1207d3u,output,8u},Operand{0x1207d8u,input,16u},
    Operand{0x1207e1u,input,20u},Operand{0x1207e6u,output,0u},
    Operand{0x1207eeu,output,4u},Operand{0x1207f3u,input,24u},
    Operand{0x1207fcu,input,16u},Operand{0x120811u,output,0u},
    Operand{0x120822u,input,20u},Operand{0x120827u,output,4u},
    Operand{0x120845u,output,8u},Operand{0x12084au,input,24u},
    Operand{0x120867u,input,28u},Operand{0x120873u,fixed,0x46a348u},
    Operand{0x12087du,input,28u},Operand{0x120887u,input,28u},
    Operand{0x12088fu,output,16u},Operand{0x120899u,output,20u},
    Operand{0x1208a7u,output,24u},Operand{0x1208b5u,fixed,0x46a0dcu},
    Operand{0x1208bdu,output,16u},Operand{0x1208cbu,output,20u},
    Operand{0x1208d9u,output,24u},
};
inline constexpr std::array stores{
    Operand{0x120779u,output,12u},Operand{0x12081du,output,16u},
    Operand{0x120840u,output,20u},Operand{0x120862u,output,24u},
    Operand{0x120894u,output,16u},Operand{0x1208a2u,output,20u},
    Operand{0x1208b0u,output,24u},Operand{0x1208c6u,output,16u},
    Operand{0x1208d4u,output,20u},Operand{0x1208e2u,output,28u},
    Operand{0x1208e7u,output,24u},
};
static_assert(reads.size()==unsigned(Read::signed_z)+1u && stores.size()==unsigned(Store::signed_z)+1u);

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

    std::uint32_t address(Operand operand)const {
        return operand.offset+(operand.base==input?s_.gp[2]:operand.base==output?s_.gp[1]:0u);
    }
    static __m128 scalar(std::uint32_t bits){return std::bit_cast<__m128>(std::array<std::uint32_t,4>{bits,0,0,0});}
    static std::uint32_t bits(__m128 scalar){return std::bit_cast<std::array<std::uint32_t,4>>(scalar)[0];}
    void publish_word(std::uint32_t pc,std::uint32_t destination,std::uint32_t value) {
        Base::store(pc,destination,value,[]{});
    }
public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}}{}
    __m128& norm(){return s_.xmm[0];}
    __m128& dot(){return s_.xmm[1];}
    __m128& conjugated_w(){return s_.xmm[2];}
    __m128& two_or_cross_y(){return s_.xmm[3];}
    __m128& cross_z(){return s_.xmm[4];}
    __m128& reciprocal(){return s_.xmm[5];}
    __m128& cross_x(){return s_.xmm[6];}
    __m128& scratch(){return s_.xmm[7];}
    __m128 zero()const{return _mm_setzero_ps();}

    static __m128 add(__m128 left,__m128 right) {
        asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    static __m128 subtract(__m128 left,__m128 right) {
        asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    static __m128 multiply(__m128 left,__m128 right) {
        asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    static __m128 divide(__m128 left,__m128 right) {
        asm volatile("divss %1,%0":"+x"(left):"x"(right):"memory");return left;
    }

    __m128 read(Read source) {
        const auto operand=reads[unsigned(source)];at(operand.pc);return scalar(word(address(operand)));
    }
    void store(Store destination,__m128 value) {
        const auto operand=stores[unsigned(destination)];publish_word(operand.pc,address(operand),bits(value));
    }
    void copy_quaternion() {
        auto& g=s_.gp;
        Base::push(0x120748u,g[6]);Base::push(0x120749u,g[7]);
        g[0]=g[2];at(0x12074cu);g[7]=word(g[0]);
        reciprocal()=read(Read::one);g[6]=g[1];
        publish_word(0x120758u,g[6],g[7]);
        at(0x12075au);g[7]=word(g[0]+4u);publish_word(0x12075du,g[6]+4u,g[7]);
        at(0x120760u);g[7]=word(g[0]+8u);publish_word(0x120763u,g[6]+8u,g[7]);
        at(0x120766u);g[0]=word(g[0]+12u);publish_word(0x120769u,g[6]+12u,g[0]);
    }
    void test_scale(__m128 left,Read source) {
        const auto right=read(source);
        s_.flags.scalar_compare(left,right,true);
        s_.set(0,s_.flags.bits()|2u,1u,8u);
        s_.flags.logic((s_.gp[0]>>8u)&0x44u,1u);
    }
    bool scale_test_passed()const{return s_.flags.pf;}
    void restore_saved_registers() {
        s_.gp[7]=Base::pop(0x12086fu);s_.gp[6]=Base::pop(0x120870u);
    }
    void finish(){(void)Base::pop(0x1208ecu);}
};

// This entry uses SSE but owns no x87/MMX instruction. Keep the full opaque
// virtual register file and fence incidental host effects at each provider.
template<class State,class Before=Noop>void run_private(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};Frame view{state,before,scope};inverse_transform(view);
}
} // namespace halo2::native_codec_inverse_transform_detail
#endif
