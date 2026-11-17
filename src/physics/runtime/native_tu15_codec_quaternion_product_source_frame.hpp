#pragma once
#include "physics/runtime/native_tu15_codec_quaternion_product_frame.hpp"
#include "physics/runtime/native_tu15_codec_copy_source_io.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_quaternion_product_detail {
inline constexpr std::array read_successors{
    std::array{0x12022fu,0x120233u,0x120238u,0x12023du,0x120246u,0x12024au,0x120253u,0x120258u},
    std::array{0x120265u,0x120269u,0x12026eu,0x120273u,0x12027cu,0x120281u,0x120289u,0x12028eu},
    std::array{0x12029cu,0x1202a1u,0x1202a6u,0x1202aau,0x1202b3u,0x1202b8u,0x1202c0u,0x1202c5u},
    std::array{0x1202d3u,0x1202d8u,0x1202dcu,0x1202e0u,0x1202e9u,0x1202eeu,0x1202f7u,0x1202fcu},
};
inline constexpr std::array store_successors{0x120260u,0x120297u,0x1202ceu,0x120305u};

// Retained source publication only. Checkpoints follow the guest assignment
// or complete authored write; physical PUSH publication is a separate view.
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
    };
    Lane result_{*this,0u},term_{*this,1u};

    template<unsigned Register>void stage() {
        constexpr bool left=Register==1u;
        constexpr std::array loads=left?std::array{0x1201e8u,0x1201eeu,0x1201f5u,0x1201f8u}:
            std::array{0x12020bu,0x120211u,0x120218u,0x12021bu};
        constexpr std::array stores=left?std::array{0x1201eau,0x1201f1u,0x1201ffu,0x1201fbu}:
            std::array{0x12020du,0x120214u,0x120222u,0x12021eu};
        auto& g=s_.gp;
        for(unsigned lane=0;lane<2u;++lane) {
            read_gp(6u,loads[lane],stores[lane],g[Register]+lane*4u);
            write(stores[lane],loads[lane+1u],g[4]+4u+lane*4u,g[6]);
        }
        read_gp(6u,loads[2],loads[3],g[Register]+8u);
        read_gp(Register,loads[3],stores[3],g[Register]+12u);
        write(stores[3],stores[2],g[4]+16u,g[Register]);
        write(stores[2],left?0x120203u:0x120226u,g[4]+12u,g[6]);
        g[Register]=g[4]+4u;
    }
public:
    SourceFrame(State& state,Checkpoint& checkpoint):Base{state,checkpoint}{}
    bool begin() {
        auto& g=s_.gp;g[4]=s_.flags.minus(g[4],16u,4u);s_.flags.minus(g[1],g[2],4u);
        at(0x1201e5u);s_.push(0x1201e5u,g[6]);retire(0x1201e6u);
        return s_.flags.zf;
    }
    void stage_left(){stage<1u>();}
    bool right_aliases_output(){s_.flags.minus(s_.gp[0],s_.gp[2],4u);return s_.flags.zf;}
    void stage_right(){stage<0u>();}
    Lane& accumulator(){return result_;}
    Lane& term(){return term_;}
    __m128 read(unsigned component,unsigned operand,Field field) {
        const auto member=unsigned(field);at(reads[component][operand]);
        const auto value=word(s_.gp[member<4u?1u:0u]+(member&3u)*4u);
        pending_=read_successors[component][operand];
        return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});
    }
    static __m128 multiply(__m128 left,__m128 right) {
        if constexpr(Ordered)return OrderedMath::multiply(left,right);
        return _mm_mul_ss(left,right);
    }
    static __m128 add(__m128 left,__m128 right) {
        if constexpr(Ordered)return OrderedMath::add(left,right);
        return _mm_add_ss(left,right);
    }
    static __m128 subtract(__m128 left,__m128 right) {
        if constexpr(Ordered)return OrderedMath::subtract(left,right);
        return _mm_sub_ss(left,right);
    }
    void store_component(unsigned component,__m128 value) {
        write(stores[component],store_successors[component],s_.gp[2]+component*4u,
              std::bit_cast<std::array<std::uint32_t,4>>(value)[0]);
    }
    void finish() {
        at(0x120305u);s_.gp[6]=s_.pop();retire(0x120306u);
        s_.gp[4]=s_.flags.plus(s_.gp[4],16u,4u);
        Base::finish_at(0x120309u);
    }
};

template<bool Ordered,class State,class Continue>
std::uint32_t run_private_source(State& state,Continue continuation) {
    return native_codec_copy_source_detail::run(state,continuation,[&](auto& checkpoint) {
        SourceFrame<Ordered,State,std::remove_reference_t<decltype(checkpoint)>> view{state,checkpoint};
        quaternion_product(view);
    });
}
} // namespace halo2::native_codec_quaternion_product_detail
#endif
