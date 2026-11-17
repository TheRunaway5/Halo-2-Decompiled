#pragma once
#include "halo2/native_tu15_codec_row_copy_kernel.hpp"
#include "physics/runtime/native_tu15_codec_copy_source_io.hpp"

namespace halo2::native_codec_row_copy_detail {
template<Row Profile,class State,class Checkpoint>class SourceFrame:
    public native_codec_copy_source_detail::Io<State,Checkpoint> {
    using Base=native_codec_copy_source_detail::Io<State,Checkpoint>;
    using Base::s_;using Base::at;using Base::word;using Base::read_gp;using Base::write;using Base::retire;
    static constexpr bool quaternion=Profile==Row::quaternion;
    static constexpr unsigned count=quaternion?4u:3u;
    static constexpr auto reads=quaternion?std::array{0x2963f5u,0x2963f9u,0x2963ffu,0x296405u}:std::array{0x29642eu,0x29643eu,0x296444u,0u};
    static constexpr auto writes=quaternion?std::array{0x2963f7u,0x2963fcu,0x296402u,0x296408u}:std::array{0x29643cu,0x296441u,0x296447u,0u};
    static constexpr auto after_write=quaternion?std::array{0x2963f9u,0x2963ffu,0x296405u,0x29640bu}:std::array{0x29643eu,0x296444u,0x29644au,0u};
    unsigned ordinal_{};
public:
    using Base::Base;
    NativeTu15WordCopy acquire(){
        auto&g=s_.gp;
        if constexpr(quaternion){
            read_gp(1,0x2963d0u,0x2963d6u,0x510f60u);
            read_gp(2,0x2963d6u,0x2963d9u,g[1]+20u);
            at(0x2963d9u);g[2]=s_.flags.product(s_.reg(2,4,0),word(0x510f94u));retire(0x2963e0u);
            read_gp(0,0x2963e0u,0x2963e5u,0x510f44u);g[2]=s_.flags.plus(g[2],g[1],4);
            read_gp(1,0x2963e7u,0x2963edu,0x510fa0u);
            g[0]=s_.flags.plus(g[0],2u,4);g[0]=s_.flags.shift(g[0],4u,4);g[0]=s_.flags.plus(g[0],g[2],4);
        }else{
            read_gp(1,0x296410u,0x296416u,0x510f60u);
            read_gp(2,0x296416u,0x296419u,g[1]+24u);
            at(0x296419u);g[2]=s_.flags.product(s_.reg(2,4,0),word(0x510f98u));retire(0x296420u);
            read_gp(0,0x296420u,0x296425u,0x510f44u);g[0]=g[0]+g[0]*2u;g[0]=g[2]+g[0]*4u;
            at(0x29642bu);g[0]=s_.flags.plus(s_.reg(0,4,0),word(g[1]+12u),4);retire(0x29642eu);
            read_gp(2,0x29642eu,0x296431u,g[0]+g[1]);g[0]=s_.flags.plus(g[0],g[1],4);
            read_gp(1,0x296433u,0x296439u,0x510fa0u);g[1]=s_.flags.plus(g[1],16u,4);
        }
        return{g[0],g[1],count,0u};
    }
    std::uint32_t read_word(std::uint32_t){
        if constexpr(!quaternion)if(!ordinal_)return s_.gp[2];
        return read_gp(ordinal_+1u==count?0u:2u,reads[ordinal_],writes[ordinal_],s_.gp[0]+ordinal_*4u);
    }
    void write_word(std::uint32_t,std::uint32_t){
        write(writes[ordinal_],after_write[ordinal_],s_.gp[1]+ordinal_*4u,s_.gp[ordinal_+1u==count?0u:2u]);
    }
    void advanced(const NativeTu15WordCopy&){++ordinal_;}
    void finish(){Base::finish_at(quaternion?0x29640bu:0x29644au);}
};
template<Row Profile,class State,class Continue>std::uint32_t run_source(State&s,Continue continuation){
    return native_codec_copy_source_detail::run(s,continuation,[&](auto&checkpoint){
        SourceFrame<Profile,State,std::remove_reference_t<decltype(checkpoint)>>view{s,checkpoint};
        native_codec_row_copy_detail::decode(view);
    });
}
} // namespace halo2::native_codec_row_copy_detail
