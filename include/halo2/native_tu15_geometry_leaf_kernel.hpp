#pragma once
#include "halo2/native_tu15_geometry.hpp"
#include <bit>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#include <xmmintrin.h>

namespace halo2::native_geometry_leaf_detail {
using Vector=NativeTu15GeometryVector;
inline __m128 lanes(const Vector&value){return std::bit_cast<__m128>(value);}
inline Vector words(__m128 value){return std::bit_cast<Vector>(value);}
inline __m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
inline __m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
struct Direction {
    __m128 basis_z{},basis_y{},basis_x{},input{},z_low{},z_high{},xy_low{},xy_high{};
    __m128 y_axis{},y_term{},x_axis{},x_broadcast{},x_term{},z_axis{},z_broadcast{},z_term{},result{};
};
struct Position {
    __m128 input{},translation{},basis_z{},basis_y{},basis_x{},z_term{},translated_z{},y_term{},x_broadcast{},x_term{},result{};
};
// Existing actual2E47B0 DATA policy. It neither owns an instruction callback,
// a register bank nor a caller frame; Access owns its actual memory phases.
template<class Access>void transposed_direction(Access&storage){
    Direction d;
    d.basis_z=lanes(storage.basis_column(2));
    d.basis_y=lanes(storage.basis_column(1));
    d.basis_x=lanes(storage.basis_column(0));
    d.input=lanes(storage.source_column(0));
    d.z_low=_mm_shuffle_ps(d.basis_z,d.basis_z,0x44);
    d.z_high=_mm_shuffle_ps(d.basis_z,d.basis_z,0xee);
    d.xy_low=_mm_shuffle_ps(d.basis_x,d.basis_y,0x44);
    d.xy_high=_mm_shuffle_ps(d.basis_x,d.basis_y,0xee);
    d.y_axis=_mm_shuffle_ps(d.xy_low,d.z_low,0xdd);
    d.y_term=multiply(d.y_axis,_mm_shuffle_ps(d.input,d.input,0x55));
    d.x_axis=_mm_shuffle_ps(d.xy_low,d.z_low,0x88);
    d.x_broadcast=_mm_shuffle_ps(d.input,d.input,0);
    d.x_term=multiply(d.x_axis,d.x_broadcast);
    d.z_axis=_mm_shuffle_ps(d.xy_high,d.z_high,0x88);
    d.z_broadcast=_mm_shuffle_ps(d.input,d.input,0xaa);
    d.z_term=multiply(d.z_axis,d.z_broadcast);
    d.result=add(add(d.x_term,d.y_term),d.z_term);
    if constexpr(requires{storage.computed(d);})storage.computed(d);
    storage.destination_column(0,words(d.result),NativeTu15GeometryPublication::transposed_direction);
}
// Existing actual2E4690 DATA policy, including the authored association and
// lane W. Input/translation/Z/Y/X stay at their lazy storage phases.
template<class Access>void transformed_position(Access&storage){
    Position d;
    d.input=lanes(storage.source_column(0));
    d.translation=lanes(storage.basis_column(3));
    d.basis_z=lanes(storage.basis_column(2));
    d.z_term=multiply(d.basis_z,_mm_shuffle_ps(d.input,d.input,0xaa));
    if constexpr(requires{storage.z_term_ready(d);})storage.z_term_ready(d);
    d.basis_y=lanes(storage.basis_column(1));
    d.translated_z=add(d.z_term,d.translation);
    d.y_term=multiply(d.basis_y,_mm_shuffle_ps(d.input,d.input,0x55));
    d.x_broadcast=_mm_shuffle_ps(d.input,d.input,0);
    if constexpr(requires{storage.y_term_ready(d);})storage.y_term_ready(d);
    d.basis_x=lanes(storage.basis_column(0));
    d.x_term=multiply(d.basis_x,d.x_broadcast);
    d.result=add(add(d.x_term,d.y_term),d.translated_z);
    if constexpr(requires{storage.computed(d);})storage.computed(d);
    storage.destination_column(0,words(d.result),NativeTu15GeometryPublication::transformed_position);
}
} // namespace halo2::native_geometry_leaf_detail
#endif
