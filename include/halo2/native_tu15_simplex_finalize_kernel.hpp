#pragma once
#include "halo2/native_tu15_simplex_direction_kernel.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
enum class NativeTu15SimplexFinalizeFrame { normalization, constant };
enum class NativeTu15SimplexFinalizePhase {
    first, second, axis, plane_argument, fresh_axis, length,
    estimate, estimate_store, estimate_reload, three, three_reload,
    half, half_reload, plane, distance, distance_reload, cache_argument,
    plane_distance, first_count_byte, first_count_store,
    second_count_byte, second_count_store, begin_first_keys,
    first_key, first_key_store, first_bound, advance_first_keys,
    begin_second_keys, second_key, second_key_store, second_bound,
    advance_second_keys
};
namespace native_simplex_finalize_numeric {
using Vector=__m128;
inline Vector multiply_low(Vector a,Vector b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
    throw std::runtime_error("Exact native simplex finalization backend unavailable");
#endif
}
inline Vector subtract_low(Vector a,Vector b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
    throw std::runtime_error("Exact native simplex finalization backend unavailable");
#endif
}
inline Vector scalar(std::uint32_t bits){Vector value{};std::memcpy(&value,&bits,4);return value;}
inline std::uint32_t low(Vector value){std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
}
struct NativeTu15SimplexFinalize {
    using Vector=__m128;
    std::uint32_t work{},second_count_address{},axis_address{},plane{},cache{},
        normalization{},constant{},distance_word{},first_count_byte{},second_count_byte{},
        index{},key_source{},key_target{},key{},bound{},first_written{},result{};
    Vector first{},second{},axis{},difference{},fresh_axis{},squared{},xy{},z{},length{},
        estimated{},inverse{},scaled_length{},three{},correction{},half{},half_inverse{},
        projection_terms{},reciprocal{},plane_vector{},projection_xy{},projection_sum{},distance{};
};
// Normalization, cache serialization and the parent epilogue continuation.
// estimate() must preserve the owning CPU-input policy/journal. It is not a
// supplied contact-plane outcome. Raw result() acquires the current reducer
// word after all preceding cache stores; typed callers retain their computed
// native result under their separate DATA facade contract.
template<class Access>
std::uint32_t native_tu15_finalize_simplex_contact(Access&memory,NativeTu15SimplexFinalize&s){
    using P=NativeTu15SimplexFinalizePhase;
    using F=NativeTu15SimplexFinalizeFrame;
    namespace N=native_simplex_direction_numeric;
    namespace L=native_simplex_finalize_numeric;
    const auto read=[&](P phase,std::uint32_t address,unsigned width=4u){return memory.word_read(phase,s,address,width);};
    const auto write=[&](P phase,std::uint32_t address,std::uint32_t value,unsigned width=4u){memory.word_write(phase,s,address,value,width);};
    s.first=memory.vector_read(P::first,s,s.work+0x20u);
    s.second=memory.vector_read(P::second,s,s.work+0xa0u);
    s.axis=memory.vector_read(P::axis,s,s.axis_address);
    s.plane=memory.argument(P::plane_argument,s);
    s.difference=N::subtract(s.second,s.first);
    s.fresh_axis=memory.vector_read(P::fresh_axis,s,s.axis_address);
    s.squared=N::multiply(s.fresh_axis,s.fresh_axis);
    s.xy=N::add_low(N::shuffle<0x55>(s.squared),s.squared);
    s.z=N::shuffle<0xaa>(s.squared);s.length=N::add_low(s.z,s.xy);
    s.normalization=memory.frame_address(F::normalization,s);
    s.constant=memory.frame_address(F::constant,s);
    memory.vector_write(P::length,s,s.normalization,s.length);
    const auto estimate=memory.estimate(s,L::low(s.length));
    s.estimated=s.xy;std::memcpy(&s.estimated,&estimate,4);
    write(P::estimate_store,s.normalization,estimate);
    s.inverse=memory.vector_read(P::estimate_reload,s,s.normalization);
    s.scaled_length=L::multiply_low(L::multiply_low(s.length,s.inverse),s.inverse);
    write(P::three,s.constant,0x40400000u);
    s.three=L::scalar(read(P::three_reload,s.constant));s.correction=L::subtract_low(s.three,s.scaled_length);
    write(P::half,s.constant,0x3f000000u);
    s.half=L::scalar(read(P::half_reload,s.constant));s.half_inverse=L::multiply_low(s.half,s.inverse);
    s.projection_terms=N::multiply(s.difference,s.axis);
    s.reciprocal=L::multiply_low(s.half_inverse,s.correction);
    s.plane_vector=N::multiply(N::shuffle<0>(s.reciprocal),s.axis);
    memory.vector_write(P::plane,s,s.plane,s.plane_vector);
    s.projection_xy=N::add_low(N::shuffle<0x55>(s.projection_terms),s.projection_terms);
    s.projection_sum=N::add_low(N::shuffle<0xaa>(s.projection_terms),s.projection_xy);
    s.distance=L::multiply_low(s.projection_sum,s.reciprocal);
    write(P::distance,s.constant,L::low(s.distance));
    s.distance_word=read(P::distance_reload,s.constant);
    s.cache=memory.argument(P::cache_argument,s);
    write(P::plane_distance,s.plane+12u,s.distance_word);
    s.first_count_byte=read(P::first_count_byte,s.work,1u);
    write(P::first_count_store,s.cache+8u,s.first_count_byte,1u);
    s.second_count_byte=read(P::second_count_byte,s.second_count_address,1u);
    write(P::second_count_store,s.cache+9u,s.second_count_byte,1u);
    s.index=0u;s.key_source=s.work+0x2cu;memory.prepare(P::begin_first_keys,s);
    do{
        s.key=read(P::first_key,s.key_source,2u);
        write(P::first_key_store,s.cache+s.index*2u,s.key,2u);
        s.bound=read(P::first_bound,s.work);
        ++s.index;s.key_source+=16u;memory.prepare(P::advance_first_keys,s);
    }while(std::bit_cast<std::int32_t>(s.index)<std::bit_cast<std::int32_t>(s.bound));
    s.first_written=s.index;s.index=0u;s.key_target=s.cache+s.first_written*2u;
    s.key_source=s.work+0xecu;memory.prepare(P::begin_second_keys,s);
    do{
        s.key=read(P::second_key,s.key_source,2u);
        write(P::second_key_store,s.key_target,s.key,2u);
        s.bound=read(P::second_bound,s.second_count_address);
        s.key_target+=2u;++s.index;s.key_source+=16u;memory.prepare(P::advance_second_keys,s);
    }while(std::bit_cast<std::int32_t>(s.index)<std::bit_cast<std::int32_t>(s.bound));
    s.result=memory.result(s);memory.complete(s);return s.result;
}
} // namespace halo2
