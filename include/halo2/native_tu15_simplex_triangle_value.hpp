#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_triangle_kernel.hpp"
namespace halo2::native_simplex_triangle_value_detail {
using Data=NativeTu15SimplexTriangle;
using P=NativeTu15SimplexTrianglePhase;
// Preserve the existing value API's owned local temporary and sixteen-byte
// DATA transaction contract. Raw callers instead mount the actual private
// frame and use the independently qualified staged FP/GP adapter.
template<class Memory>class View {
    Memory&memory_;std::uint32_t work_,point_,vertices_,orient_;__m128 scratch_{};
public:
    View(Memory&memory,std::uint32_t work,std::uint32_t point,std::uint32_t vertices,std::uint32_t orient)
        :memory_(memory),work_(work),point_(point),vertices_(vertices),orient_(orient){}
    void begin(Data&d){d.work=work_;}
    std::uint32_t argument(P p,const Data&)const{return p==P::vertices_argument?vertices_:p==P::point_argument?point_:orient_;}
    std::uint32_t scratch_address(const Data&)const{return 0u;}
    __m128 vector_read(P p,const Data&,std::uint32_t address){return p==P::scratch_read?scratch_:memory_.vector(address);}
    void vector_write(P p,const Data&,std::uint32_t address,__m128 value){
        if(p==P::scratch_store){scratch_=value;return;}
        const auto pc=p==P::weights?0x30d51eu:p==P::normal?0x30d52eu:p==P::flipped_normal?0x30d578u:
            p==P::publish_first?0x30d586u:p==P::publish_second?0x30d589u:p==P::publish_payload_first?0x30d595u:0x30d599u;
        memory_.vector(pc,address,value);
    }
    void prepare_weights(const Data&){}void prepare_normal(const Data&){}void prepare_orientation(const Data&){}
    void load_weight(const Data&d){const auto value=memory_.word(d.work+0x160u);asm volatile("flds %0"::"m"(value):"memory","st");}
    std::uint32_t word_read(P,const Data&,std::uint32_t address){return memory_.word(address);}
    void word_write(P,const Data&,std::uint32_t address,std::uint32_t value){memory_.word(0x30d5a9u,address,value);}
    void store_weight(const Data&d){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");memory_.word(0x30d5afu,d.work+0x164u,value);}
    void finish(const Data&,bool){}
};
} // namespace halo2::native_simplex_triangle_value_detail
#endif
