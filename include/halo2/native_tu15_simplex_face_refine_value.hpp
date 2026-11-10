#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_face_refine_kernel.hpp"
namespace halo2::native_simplex_face_refine_value_detail {
using Data=NativeTu15SimplexFaceRefine;
using P=NativeTu15SimplexFaceRefinePhase;
// The existing DATA facade publishes four completed dot products together.
// Its local result buffer preserves that observable value contract while raw
// caller-frame access publishes each product at its physical source boundary.
// Neither view supplies a geometric outcome to the common policy.
template<class Memory>class View {
    Memory&memory_;std::uint32_t vertices_,point_,scores_,scratch_;
    std::array<std::uint32_t,4>products_{};
    static std::uint16_t compare(std::uint32_t bits){std::uint16_t status;asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;}
public:
    View(Memory&memory,std::uint32_t vertices,std::uint32_t point,std::uint32_t scores,std::uint32_t scratch):memory_(memory),vertices_(vertices),point_(point),scores_(scores),scratch_(scratch){}
    void begin(Data&d){d.vertices=vertices_;d.scores=scores_;memory_.word(0x30d1b0u,scratch_+12u,vertices_);}
    std::uint32_t index(P,const Data&,std::uint32_t address){return std::uint32_t(std::int32_t(std::int8_t(memory_.word(address,1))));}
    bool nonnegative(const Data&d,bool second){native_simplex_projection_numeric::load(memory_.word(d.scores+(second?d.second:d.first)*4u));return !(compare(memory_.word(0x46a0e8u))&0x100u);}
    std::uint32_t cursor(const Data&){return memory_.word(scratch_+12u);}
    __m128 vector_read(P,const Data&,std::uint32_t address){return memory_.vector(address);}
    std::uint32_t point_argument(const Data&)const{return point_;}
    void score_write(P phase,const Data&,std::uint32_t bits){
        const auto slot=phase==P::along_first?0u:phase==P::along_second?1u:phase==P::apex_first?2u:3u;products_[slot]=bits;
        if(slot!=3u)return;
        constexpr std::array pc{0x30d25eu,0x30d277u,0x30d28du,0x30d2abu},offset{0x14u,0x1cu,0x18u,0x10u};
        for(unsigned n=0;n<4u;++n)memory_.word(pc[n],scratch_+offset[n],products_[n]);
    }
    bool compare_products(const Data&){namespace X=native_simplex_projection_numeric;X::load(memory_.word(scratch_+0x10u));X::multiply(memory_.word(scratch_+0x14u));X::load(memory_.word(scratch_+0x18u));X::multiply(memory_.word(scratch_+0x1cu));return !(X::compare_pop_two()&0x4100u);}
    void invalidate(P phase,const Data&d){const bool second=phase==P::invalidate_second;memory_.word(second?0x30d2c8u:0x30d2d1u,d.scores+(second?d.second:d.first)*4u,0xbf800000u);}
    void advance(const Data&){const auto cursor=memory_.word(scratch_+12u);memory_.word(0x30d2e3u,scratch_+12u,cursor+16u);}
    bool positive(const Data&d,unsigned face){native_simplex_projection_numeric::load(memory_.word(d.scores+face*4u));return !(compare(memory_.word(0x46a0e8u))&0x4100u);}
    void finish(std::uint32_t){}
};
} // namespace halo2::native_simplex_face_refine_value_detail
#endif
