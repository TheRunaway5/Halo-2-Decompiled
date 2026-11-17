#pragma once
#include "halo2/native_tu15_codec_scale_kernel.hpp"
#include "physics/runtime/native_tu15_codec_copy_source_io.hpp"

namespace halo2::native_codec_scale_row_detail {
template<class State,class Checkpoint>class SourceFrame:
    public native_codec_copy_source_detail::Io<State,Checkpoint> {
    using Base=native_codec_copy_source_detail::Io<State,Checkpoint>;
    using Base::s_;using Base::at;using Base::word;using Base::read_gp;using Base::write;using Base::retire;
public:
    using Base::Base;
    NativeTu15WordCopy scale(){
        auto&g=s_.gp;
        read_gp(0,0x2cec60u,0x2cec65u,0x510f60u);
        read_gp(1,0x2cec65u,0x2cec68u,g[0]+28u);
        at(0x2cec68u);g[1]=s_.flags.product(s_.reg(1,4,0),word(0x510f9cu));retire(0x2cec6fu);
        read_gp(2,0x2cec6fu,0x2cec75u,0x510f44u);g[1]+=g[2]*4u;
        at(0x2cec78u);g[1]=s_.flags.plus(s_.reg(1,4,0),word(g[0]+16u),4);retire(0x2cec7bu);
        return{g[1]+g[0],0u,1u,0u};
    }
    std::uint32_t read_word(std::uint32_t address){return read_gp(2,0x2cec7bu,0x2cec7eu,address);}
    void write_word(std::uint32_t,std::uint32_t){
        read_gp(0,0x2cec7eu,0x2cec83u,0x510fa0u);
        write(0x2cec83u,0x2cec86u,s_.gp[0]+28u,s_.gp[2]);
    }
    void advanced(const NativeTu15WordCopy&){}
    void finish(){Base::finish_at(0x2cec86u);}
};
template<class State,class Continue>std::uint32_t run_source(State&s,Continue continuation){
    return native_codec_copy_source_detail::run(s,continuation,[&](auto&checkpoint){
        SourceFrame<State,std::remove_reference_t<decltype(checkpoint)>>view{s,checkpoint};
        native_codec_scale_detail::decode(view);
    });
}
} // namespace halo2::native_codec_scale_row_detail
