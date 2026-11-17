#pragma once
#include "halo2/native_tu15_shape_collection_kernel.hpp"
#include "physics/runtime/native_tu15_ray_filter_frame.hpp"
namespace halo2::native_shape_collection_detail {
template<class State,class Before,Field Kind>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_ray_filter_detail::HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_ray_filter_detail::HostIo>;
    using Base::s_;using Base::at;
    static constexpr auto entry=Kind==Field::shape?0x306220u:0x306230u;
public:
    Frame(State&s,Before&before):Base{s,before,native_ray_filter_detail::HostIo{}}{}
    std::uint32_t rows(){at(entry);return s_.gp[0]=Base::word(s_.gp[1]+12u);}
    std::uint32_t key(){at(entry+3u);return s_.gp[1]=Base::word(s_.gp[4]+4u);}
    static constexpr std::uint32_t offset(){return Kind==Field::shape?0u:4u;}
    std::uint32_t word(std::uint32_t address){at(entry+7u);return s_.gp[0]=Base::word(address);}
    void finish(){Base::pop(Kind==Field::shape?0x30622au:0x30623bu);s_.gp[4]+=Kind==Field::shape?8u:4u;}
};
template<Field Kind,class State,class Before>void run_private(State&s,Before&before){Frame<State,Before,Kind>view{s,before};element(view);view.finish();}
} // namespace halo2::native_shape_collection_detail
