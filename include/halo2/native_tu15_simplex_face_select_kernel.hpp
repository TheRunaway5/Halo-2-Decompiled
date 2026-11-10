#pragma once
#include "halo2/native_tu15_simplex_initialize_kernel.hpp"
#include "halo2/native_tu15_simplex_projection_kernel.hpp"

namespace halo2 {
enum class NativeTu15SimplexFaceSelectPhase {
    first,origin,apex,second,scale_argument,point,apex_fresh,scale_store,scale_load,
    normal_first,normal_second,normal_third,absolute_mask,signed_products,
    normal,zero,squared_length,squared_length_load,face_product,denominator,score,
    rank_second,rank_first,rank_first_word,rank_first_store,rank_old_store,
    rank_reload,rank_third_compare,rank_temporary_store,rank_temporary_read,
    rank_third_load,rank_third_store,rank_first_reload,rank_first_retry,
    threshold_third,threshold_constant,rank_factor,rank_third_again
};
struct NativeTu15SimplexFaceSelect {
    using Vector=native_simplex_direction_numeric::Vector;
    std::uint32_t vertices{},point_address{},scratch{},scale{},choice{1u},face{};
    Vector first{},origin{},apex{},second{},edge_first{},edge_second{},apex_origin{},apex_first{},apex_second{},second_first{};
    Vector negative_first{},positive_first{},negative_second{},positive_second{},negative_third{},positive_third{};
    Vector third_zxy{},third_yzx{},point{},apex_fresh{},normal_first{},normal_second{},normal_third{},delta{},scaled{};
    Vector low_third{},high_third{},high_pair{},low_pair{},scale_z{},rows_z{},product_z{},scale_y{},rows_y{},product_y{},scale_x{},rows_x{},product_x{},absolute_mask{},triples{},absolute_triples{},signed_products{};
    Vector normal{},squares{},squared_xy{},squared_z{},squared_length{};
    std::uint16_t relation{};
};
namespace native_simplex_face_select_numeric {
inline __m128 broadcast(std::uint32_t bits){__m128 value{};std::memcpy(&value,&bits,4u);return _mm_shuffle_ps(value,value,0);}
inline std::uint16_t compare_next_pop(){std::uint16_t status;asm volatile("fcomp %%st(1);fnstsw %0":"=a"(status)::"memory","st","st(1)");return status;}
inline std::uint16_t compare_unordered_pop_two(){std::uint16_t status;asm volatile("fucompp;fnstsw %0":"=a"(status)::"memory","st","st(1)");return status;}
inline std::uint16_t compare_memory(std::uint32_t bits,bool pop){std::uint16_t status;if(pop)asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");else asm volatile("fcoms %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;}
inline void divide(std::uint32_t bits){asm volatile("fdivs %0"::"m"(bits):"memory","st");}
inline void multiply_next(){asm volatile("fmul %%st(1),%%st(0)":::"memory","st","st(1)");}
inline void discard(){asm volatile("fstp %%st(0)":::"memory","st");}
inline bool greater(std::uint16_t status){return !(status&0x4100u);}
}
// Private actual30D5D0 policy. Geometric rows, scratch and arguments are acquired
// at semantic boundaries. Access transports genuine native x87/provider phases
// and the real refinement child; it never supplies a classification outcome.
template<class Access>
std::uint32_t native_tu15_select_simplex_faces(Access&memory,NativeTu15SimplexFaceSelect&d){
    using P=NativeTu15SimplexFaceSelectPhase;
    namespace N=native_simplex_direction_numeric;
    namespace X=native_simplex_projection_numeric;
    namespace F=native_simplex_face_select_numeric;
    memory.begin(d);
    d.first=memory.vector_read(P::first,d,d.vertices+0x10u);
    d.origin=memory.vector_read(P::origin,d,d.vertices);
    d.apex=memory.vector_read(P::apex,d,d.vertices+0x30u);
    d.second=memory.vector_read(P::second,d,d.vertices+0x20u);
    d.edge_first=N::subtract(d.first,d.origin);d.edge_second=N::subtract(d.origin,d.second);
    d.apex_origin=N::subtract(d.apex,d.origin);d.apex_first=N::subtract(d.apex,d.first);d.apex_second=N::subtract(d.apex,d.second);d.second_first=N::subtract(d.second,d.first);
    d.negative_first=N::multiply(N::shuffle<0xd2>(d.second_first),N::shuffle<0xc9>(d.apex_first));
    d.scale=memory.word_read(P::scale_argument,d);
    d.positive_first=N::multiply(N::shuffle<0xc9>(d.second_first),N::shuffle<0xd2>(d.apex_first));
    d.negative_second=N::multiply(N::shuffle<0xd2>(d.edge_second),N::shuffle<0xc9>(d.apex_second));
    d.positive_second=N::multiply(N::shuffle<0xc9>(d.edge_second),N::shuffle<0xd2>(d.apex_second));
    d.negative_third=N::multiply(N::shuffle<0xd2>(d.edge_first),N::shuffle<0xc9>(d.apex_origin));
    d.third_zxy=N::shuffle<0xd2>(d.apex_origin);d.third_yzx=N::shuffle<0xc9>(d.edge_first);
    d.point=memory.vector_read(P::point,d,d.point_address);
    d.positive_third=N::multiply(d.third_yzx,d.third_zxy);
    d.apex_fresh=memory.vector_read(P::apex_fresh,d,d.vertices+0x30u);
    d.normal_third=N::subtract(d.positive_third,d.negative_third);d.delta=N::subtract(d.point,d.apex_fresh);
    d.normal_first=N::subtract(d.positive_first,d.negative_first);d.normal_second=N::subtract(d.positive_second,d.negative_second);
    memory.word_write(P::scale_store,d,d.scale);
    const auto scale=memory.word_read(P::scale_load,d);d.scaled=N::multiply(F::broadcast(scale),d.delta);
    memory.vector_write(P::normal_first,d,d.scratch+0x30u,d.normal_first);
    d.high_pair=_mm_shuffle_ps(d.normal_first,d.normal_second,0xee);d.low_third=N::shuffle<0x44>(d.normal_third);d.high_third=N::shuffle<0xee>(d.normal_third);
    memory.vector_write(P::normal_second,d,d.scratch+0x40u,d.normal_second);
    memory.vector_write(P::normal_third,d,d.scratch+0x50u,d.normal_third);
    d.low_pair=_mm_shuffle_ps(d.normal_first,d.normal_second,0x44);d.scale_z=N::shuffle<0xaa>(d.scaled);
    d.rows_z=_mm_shuffle_ps(d.high_pair,d.high_third,0x88);d.product_z=N::multiply(d.rows_z,d.scale_z);
    d.scale_y=N::shuffle<0x55>(d.scaled);d.rows_y=_mm_shuffle_ps(d.low_pair,d.low_third,0xdd);d.product_y=N::multiply(d.rows_y,d.scale_y);
    d.scale_x=N::shuffle<0>(d.scaled);d.rows_x=_mm_shuffle_ps(d.low_pair,d.low_third,0x88);d.product_x=N::multiply(d.rows_x,d.scale_x);
    d.absolute_mask=memory.vector_read(P::absolute_mask,d,0x461950u);
    d.triples=native_simplex_initialize_numeric::add(native_simplex_initialize_numeric::add(d.product_x,d.product_y),d.product_z);
    d.absolute_triples=_mm_and_ps(d.triples,d.absolute_mask);d.signed_products=N::multiply(d.triples,d.absolute_triples);
    memory.vector_write(P::signed_products,d,d.scratch+0x20u,d.signed_products);
    memory.begin_scores(d);
    for(d.face=0;d.face<3u;++d.face){
        d.normal=memory.vector_read(P::normal,d,d.scratch+0x30u+d.face*16u);
        memory.require_capacity(2u);X::load(memory.word_read(P::zero,d));
        d.squares=N::multiply(d.normal,d.normal);d.squared_xy=N::add_low(N::shuffle<0x55>(d.squares),d.squares);d.squared_z=N::shuffle<0xaa>(d.squares);d.squared_length=N::add_low(d.squared_z,d.squared_xy);
        memory.word_write(P::squared_length,d,X::low(d.squared_length));X::load(memory.word_read(P::squared_length_load,d));
        d.relation=F::compare_unordered_pop_two();memory.relation(d,0x44u);
        const auto equal_bits=d.relation&0x4400u;
        if(equal_bits==0x4000u||equal_bits==0x0400u)memory.maximum_score(d);
        else{X::load(memory.word_read(P::face_product,d));F::divide(memory.word_read(P::denominator,d));memory.store_score(d);}
        memory.advance_score(d);
    }
    X::load(memory.word_read(P::rank_second,d));d.choice=1u;memory.choice(d);X::load(memory.word_read(P::rank_first,d));
    d.relation=F::compare_next_pop();memory.relation(d,0x41u);
    if(F::greater(d.relation)){
        const auto old=memory.word_read(P::rank_first_word,d);memory.store_rank(P::rank_first_store,d);
        memory.word_write(P::rank_old_store,d,old);X::load(memory.word_read(P::rank_reload,d));d.choice=0u;memory.choice(d);
    }
    d.relation=F::compare_memory(memory.word_read(P::rank_third_compare,d),false);memory.relation(d,0x41u);
    if(F::greater(d.relation)){
        memory.store_rank(P::rank_temporary_store,d);const auto old=memory.word_read(P::rank_temporary_read,d);
        X::load(memory.word_read(P::rank_third_load,d));memory.word_write(P::rank_third_store,d,old);X::load(memory.word_read(P::rank_first_reload,d));
        d.relation=F::compare_next_pop();memory.relation(d,0x41u);
        if(F::greater(d.relation)){F::discard();X::load(memory.word_read(P::rank_first_retry,d));}
    }else{d.choice=2u;memory.choice(d);}
    X::load(memory.word_read(P::threshold_third,d));d.relation=F::compare_memory(memory.word_read(P::threshold_constant,d),true);memory.relation(d,5u);
    if((d.relation&0x500u)==0x100u){F::discard();memory.finish(d,0xffffffffu);return 0xffffffffu;}
    memory.require_capacity(2u);X::load(memory.word_read(P::rank_factor,d));F::multiply_next();X::load(memory.word_read(P::rank_third_again,d));d.relation=X::compare_pop_two();memory.ranking_compared(d);F::discard();memory.ranking_discarded(d);
    if(F::greater(d.relation)){memory.finish(d,d.choice);return d.choice;}
    return memory.refine(d);
}
} // namespace halo2
