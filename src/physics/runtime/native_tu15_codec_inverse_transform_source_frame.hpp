#pragma once
#include "physics/runtime/native_tu15_codec_inverse_transform_frame.hpp"
#include "physics/runtime/native_tu15_codec_copy_source_io.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_inverse_transform_detail {

inline constexpr std::array read_successor{
    0x120748u,0x120756u,0x120771u,0x120779u,0x120783u,0x120787u,0x12078cu,0x120791u,
    0x120796u,0x12079bu,0x1207a0u,0x1207a5u,0x1207aeu,0x1207b3u,0x1207bbu,0x1207c0u,
    0x1207d8u,0x1207ddu,0x1207e6u,0x1207eau,0x1207f3u,0x1207f8u,0x120801u,0x120815u,
    0x120827u,0x12082cu,0x12084au,0x12084fu,0x12086bu,0x12087bu,0x120881u,0x12088cu,
    0x120894u,0x12089eu,0x1208acu,0x1208bdu,0x1208c2u,0x1208d0u,0x1208deu,
};
inline constexpr std::array store_successor{
    0x12077eu,0x120822u,0x120845u,0x120867u,0x120899u,0x1208a7u,0x1208b5u,0x1208cbu,
    0x1208d9u,0x1208e7u,0x1208ecu,
};
static_assert(read_successor.size()==reads.size() && store_successor.size()==stores.size());

// Raw CPU and retained provider publication are distinct. The authored MOVSS
// or memory arithmetic owns the complete XMM assignment before any handoff.
// No source view is admitted for arbitrary GP/XMM or FP metadata mutation.
template<bool Ordered,class State,class Checkpoint>class SourceFrame:
    public native_codec_copy_source_detail::Io<State,Checkpoint> {
    using Base=native_codec_copy_source_detail::Io<State,Checkpoint>;
    using Base::s_;using Base::at;using Base::word;using Base::read_gp;using Base::write;using Base::retire;
    using OrderedMath=Frame<State,Noop>;
    std::uint32_t pending_{};

    class Lane {
        SourceFrame& owner_;
        unsigned index_;
    public:
        Lane(SourceFrame& owner,unsigned index):owner_(owner),index_(index){}
        operator __m128()const{return owner_.s_.xmm[index_];}
        Lane& operator=(__m128 value) {
            owner_.s_.xmm[index_]=value;
            if(const auto next=std::exchange(owner_.pending_,0u))owner_.retire(next);
            return *this;
        }
        Lane& operator=(const Lane& value){return *this=static_cast<__m128>(value);}
    };
    std::array<Lane,8> lanes_;

    std::uint32_t address(Operand operand)const {
        return operand.offset+(operand.base==input?s_.gp[2]:operand.base==output?s_.gp[1]:0u);
    }
    static __m128 scalar(std::uint32_t bits){return std::bit_cast<__m128>(std::array<std::uint32_t,4>{bits,0,0,0});}
    static std::uint32_t bits(__m128 scalar){return std::bit_cast<std::array<std::uint32_t,4>>(scalar)[0];}
    void push(std::uint32_t pc,std::uint32_t next,std::uint32_t value) {
        at(pc);s_.push(pc,value);retire(next);
    }
    void pop(std::uint32_t pc,std::uint32_t next,unsigned reg) {
        at(pc);s_.gp[reg]=s_.pop();retire(next);
    }
public:
    SourceFrame(State& state,Checkpoint& checkpoint):Base{state,checkpoint},
        lanes_{{{*this,0u},{*this,1u},{*this,2u},{*this,3u},{*this,4u},{*this,5u},{*this,6u},{*this,7u}}}{}
    Lane& norm(){return lanes_[0];}
    Lane& dot(){return lanes_[1];}
    Lane& conjugated_w(){return lanes_[2];}
    Lane& two_or_cross_y(){return lanes_[3];}
    Lane& cross_z(){return lanes_[4];}
    Lane& reciprocal(){return lanes_[5];}
    Lane& cross_x(){return lanes_[6];}
    Lane& scratch(){return lanes_[7];}
    __m128 zero()const{return _mm_setzero_ps();}

    static __m128 add(__m128 left,__m128 right) {
        if constexpr(Ordered)return OrderedMath::add(left,right);
        return _mm_add_ss(left,right);
    }
    static __m128 subtract(__m128 left,__m128 right) {
        if constexpr(Ordered)return OrderedMath::subtract(left,right);
        return _mm_sub_ss(left,right);
    }
    static __m128 multiply(__m128 left,__m128 right) {
        if constexpr(Ordered)return OrderedMath::multiply(left,right);
        return _mm_mul_ss(left,right);
    }
    static __m128 divide(__m128 left,__m128 right) {
        if constexpr(Ordered)return OrderedMath::divide(left,right);
        return _mm_div_ss(left,right);
    }

    __m128 read(Read source) {
        const auto operand=reads[unsigned(source)];at(operand.pc);const auto value=word(address(operand));
        pending_=read_successor[unsigned(source)];return scalar(value);
    }
    void store(Store destination,__m128 value) {
        const auto operand=stores[unsigned(destination)];
        write(operand.pc,store_successor[unsigned(destination)],address(operand),bits(value));
    }
    void copy_quaternion() {
        auto& g=s_.gp;
        push(0x120748u,0x120749u,g[6]);push(0x120749u,0x12074au,g[7]);
        g[0]=g[2];read_gp(7u,0x12074cu,0x12074eu,g[0]);
        reciprocal()=read(Read::one);g[6]=g[1];
        write(0x120758u,0x12075au,g[6],g[7]);
        read_gp(7u,0x12075au,0x12075du,g[0]+4u);write(0x12075du,0x120760u,g[6]+4u,g[7]);
        read_gp(7u,0x120760u,0x120763u,g[0]+8u);write(0x120763u,0x120766u,g[6]+8u,g[7]);
        read_gp(0u,0x120766u,0x120769u,g[0]+12u);write(0x120769u,0x12076cu,g[6]+12u,g[0]);
    }
    void test_scale(__m128 left,Read source) {
        const auto operand=reads[unsigned(source)];at(operand.pc);
        s_.flags.scalar_compare(left,scalar(word(address(operand))),true);
        retire(read_successor[unsigned(source)]);
        s_.set(0,s_.flags.bits()|2u,1u,8u);
        s_.flags.logic((s_.gp[0]>>8u)&0x44u,1u);
    }
    bool scale_test_passed()const{return s_.flags.pf;}
    void restore_saved_registers() {
        pop(0x12086fu,0x120870u,7u);pop(0x120870u,0x120871u,6u);
    }
    void finish(){Base::finish_at(0x1208ecu);}
};

template<bool Ordered,class State,class Continue>
std::uint32_t run_private_source(State& state,Continue continuation) {
    return native_codec_copy_source_detail::run(state,continuation,[&](auto& checkpoint) {
        SourceFrame<Ordered,State,std::remove_reference_t<decltype(checkpoint)>> view{state,checkpoint};
        inverse_transform(view);
    });
}

} // namespace halo2::native_codec_inverse_transform_detail
#endif
