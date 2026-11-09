#pragma once
#include "halo2/native_tu15_simplex_direction_kernel.hpp"
#include <cstring>

namespace halo2 {
enum class NativeTu15SimplexProjectionPhase {
    axis, first, fresh_axis, squared_length, second, projection,
    projection_load, projection_square, projection_cube, epsilon_load,
    threshold_scale, threshold_length, length_load, iteration, tolerance_scale,
    parity, tolerance
};
enum class NativeTu15SimplexProjectionNext { reduce,first_support,second_support };
struct NativeTu15SimplexProjection {
    using Vector=native_simplex_direction_numeric::Vector;
    std::uint32_t work{},reducer_status{},iteration{},parity{};
    Vector axis{},squares{},length_xy{},length_z{},first{},length{},fresh_axis{},second{};
    Vector difference{},products{},projection_xy{},projection_z{},projection{};
    std::uint16_t relation{};
    NativeTu15SimplexProjectionNext next{NativeTu15SimplexProjectionNext::reduce};
};
namespace native_simplex_projection_numeric {
inline std::uint32_t low(__m128 value){std::uint32_t bits;std::memcpy(&bits,&value,4u);return bits;}
inline void load(std::uint32_t bits){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("flds %0"::"m"(bits):"memory","st");
#else
    throw std::runtime_error("Exact native simplex projection backend unavailable");
#endif
}
inline void multiply(std::uint32_t bits){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fmuls %0"::"m"(bits):"memory","st");
#else
    throw std::runtime_error("Exact native simplex projection backend unavailable");
#endif
}
inline std::uint16_t compare_pop_two(){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t status;asm volatile("fcompp;fnstsw %0":"=a"(status)::"memory","st","st(1)");return status;
#else
    throw std::runtime_error("Exact native simplex projection backend unavailable");
#endif
}
}
// Private full-controller continuation. Access owns the active FP environment,
// protects providers while real x87 intermediates are live, and stages FSTP
// publication. This has no production admission before its raw proof completes.
template<class Access>
NativeTu15SimplexProjectionNext native_tu15_project_simplex_direction(Access&memory,NativeTu15SimplexProjection&d){
    using P=NativeTu15SimplexProjectionPhase;
    namespace N=native_simplex_direction_numeric;
    namespace X=native_simplex_projection_numeric;
    d.next=NativeTu15SimplexProjectionNext::reduce;memory.begin(d);
    d.axis=memory.vector_read(P::axis,d,d.work+0x120u);
    d.squares=N::multiply(d.axis,d.axis);
    d.length_xy=N::add_low(N::shuffle<0x55>(d.squares),d.squares);
    d.length_z=N::shuffle<0xaa>(d.squares);
    d.first=memory.vector_read(P::first,d,d.work+0x20u);
    d.length=N::add_low(d.length_z,d.length_xy);
    d.fresh_axis=memory.vector_read(P::fresh_axis,d,d.work+0x120u);
    memory.word_write(P::squared_length,d,X::low(d.length));
    d.second=memory.vector_read(P::second,d,d.work+0xa0u);
    d.difference=N::subtract(d.second,d.first);
    d.products=N::multiply(d.difference,d.fresh_axis);
    d.projection_xy=N::add_low(N::shuffle<0x55>(d.products),d.products);
    d.projection_z=N::shuffle<0xaa>(d.products);
    d.projection=N::add_low(d.projection_z,d.projection_xy);
    memory.word_write(P::projection,d,X::low(d.projection));
    if(!d.reducer_status){
        memory.require_capacity(2u);
        X::load(memory.scalar_read(P::projection_load,d));
        X::multiply(memory.scalar_read(P::projection_square,d));
        X::multiply(memory.scalar_read(P::projection_cube,d));
        X::load(memory.scalar_read(P::epsilon_load,d));
        X::multiply(memory.scalar_read(P::threshold_scale,d));
        X::multiply(memory.scalar_read(P::threshold_length,d));
        d.relation=X::compare_pop_two();memory.relation(d);
        if(d.relation&0x100u){
            X::load(memory.scalar_read(P::length_load,d));
            d.iteration=memory.scalar_read(P::iteration,d);
            X::multiply(memory.scalar_read(P::tolerance_scale,d));
            d.parity=d.iteration&1u;memory.word_write(P::parity,d,d.parity);
            memory.store_tolerance(d);
            d.next=d.parity?NativeTu15SimplexProjectionNext::first_support:NativeTu15SimplexProjectionNext::second_support;
        }
    }
    memory.complete(d);return d.next;
}
} // namespace halo2
