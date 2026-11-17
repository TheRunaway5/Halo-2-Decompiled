#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
namespace halo2::native_codec_rotation_interpolation_detail {
// Keep the original destination/source order even for two distinct NaNs.
// Unsupported native backends retain the prior intrinsic fallback policy.
inline __m128 ordered_add(SourcePhysicsContactImage image,__m128 left,__m128 right){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(image==SourcePhysicsContactImage::TitleUpdate15){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
#endif
    (void)image;
    return _mm_add_ps(left,right);
}
inline __m128 ordered_multiply(SourcePhysicsContactImage image,__m128 left,__m128 right){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(image==SourcePhysicsContactImage::TitleUpdate15){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
#endif
    (void)image;
    return _mm_mul_ps(left,right);
}
inline __m128 ordered_subtract(SourcePhysicsContactImage image,__m128 left,__m128 right){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(image==SourcePhysicsContactImage::TitleUpdate15){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
#endif
    (void)image;
    return _mm_sub_ps(left,right);
}
} // namespace halo2::native_codec_rotation_interpolation_detail
