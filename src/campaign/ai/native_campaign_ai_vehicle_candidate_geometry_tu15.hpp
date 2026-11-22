#pragma once
#include "campaign/ai/native_campaign_ai_goal_port_tu15.hpp"
#include "halo2/native_tu15_look_vector_angle.hpp"

namespace halo2::vehicle_candidates_tu15 {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
using goal_tu15::u32;

struct Planar {
    u32 x;
    u32 y;
};

using DoubleOperand = std::array<std::byte, 8>;

struct Magnitude {
    u32 rounded;
    bool comparison;
};

inline Magnitude compare_length(NativeTu15PhysicsFrameFp &fp, Planar value, u32 bound) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 length;
    unsigned char larger;
    asm volatile("flds %[y]\n\tfmuls %[y]\n\tflds %[x]\n\tfmuls %[x]\n\tfaddp\n\tfsqrt\n\tfsts %[length]"
                 "\n\tflds %[bound]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[larger]"
        : [length] "=m"(length), [larger] "=q"(larger)
        : [x] "m"(value.x), [y] "m"(value.y), [bound] "m"(bound)
        : "cc", "memory", "st", "st(1)");
    return {length, larger != 0};
}

inline Magnitude compare_displaced_length(NativeTu15PhysicsFrameFp &fp, Planar end, Planar start, u32 bound) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 length;
    unsigned char larger;
    // Source 2A2C00 keeps both differences extended until the root spills.
    asm volatile("flds %[ex]\n\tfsubs %[sx]\n\tflds %[ey]\n\tfsubs %[sy]"
                 "\n\tfld %%st(0)\n\tfmul %%st(1)\n\tfld %%st(2)\n\tfmul %%st(3)\n\tfaddp\n\tfsqrt"
                 "\n\tfstp %%st(2)\n\tfstp %%st(0)\n\tfsts %[length]"
                 "\n\tflds %[bound]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[larger]"
        : [length] "=m"(length), [larger] "=q"(larger)
        : [ex] "m"(end.x), [ey] "m"(end.y), [sx] "m"(start.x), [sy] "m"(start.y), [bound] "m"(bound)
        : "cc", "memory", "st", "st(1)", "st(2)", "st(3)");
    return {length, larger != 0};
}

inline Magnitude compare_ratio(NativeTu15PhysicsFrameFp &fp, u32 numerator, u32 denominator,
                              const DoubleOperand &bound) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 value;
    unsigned char above_bound;
    asm volatile("flds %[numerator]\n\tfdivs %[denominator]\n\tfsts %[value]\n\tfldl %[bound]"
                 "\n\tfxch %%st(1)\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[above]"
        : [value] "=m"(value), [above] "=q"(above_bound)
        : [numerator] "m"(numerator), [denominator] "m"(denominator), [bound] "m"(bound)
        : "cc", "memory", "st", "st(1)");
    return {value, above_bound != 0};
}

inline Magnitude compare_projection(NativeTu15PhysicsFrameFp &fp, Planar a, Planar b,
                                    const DoubleOperand &bound) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 value;
    unsigned char above_bound;
    asm volatile("flds %[ay]\n\tfmuls %[by]\n\tflds %[ax]\n\tfmuls %[bx]\n\tfaddp"
                 "\n\tfsts %[value]\n\tfldl %[bound]\n\tfxch %%st(1)\n\tfcomip %%st(1),%%st"
                 "\n\tfstp %%st(0)\n\tseta %[above]"
        : [value] "=m"(value), [above] "=q"(above_bound)
        : [ax] "m"(a.x), [ay] "m"(a.y), [bx] "m"(b.x), [by] "m"(b.y), [bound] "m"(bound)
        : "cc", "memory", "st", "st(1)");
    return {value, above_bound != 0};
}

inline bool nonnegative_projection(NativeTu15PhysicsFrameFp &fp, Planar a, Planar b,
                                    const DoubleOperand &zero) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    unsigned char greater_equal;
    asm volatile("flds %[ax]\n\tfmuls %[bx]\n\tflds %[ay]\n\tfmuls %[by]\n\tfaddp"
                 "\n\tfldl %[zero]\n\tfxch %%st(1)\n\tfcomip %%st(1),%%st"
                 "\n\tfstp %%st(0)\n\tsetae %[above]"
        : [above] "=q"(greater_equal)
        : [ax] "m"(a.x), [ay] "m"(a.y), [bx] "m"(b.x), [by] "m"(b.y), [zero] "m"(zero)
        : "cc", "memory", "st", "st(1)");
    return greater_equal != 0;
}

inline bool bound_exceeds_word(NativeTu15PhysicsFrameFp &fp, const DoubleOperand &bound, u32 value) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    unsigned char larger;
    asm volatile("flds %[value]\n\tfldl %[bound]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[larger]"
        : [larger] "=q"(larger) : [value] "m"(value), [bound] "m"(bound)
        : "cc", "memory", "st", "st(1)");
    return larger != 0;
}

inline u32 subtract_extended(NativeTu15PhysicsFrameFp &fp, u32 a, u32 b) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 result;
    asm volatile("flds %1\n\tfsubs %2\n\tfstps %0"
        : "=m"(result) : "m"(a), "m"(b) : "memory", "st");
    return result;
}

inline Planar rotate_candidates(NativeTu15PhysicsFrameFp &fp, Planar basis, u32 angle, bool reverse_sine) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    Planar result;
    // The first source pass negates only the sine input. Use the separate
    // original numeric instructions and avoid a fused sincos or mul/add.
    asm volatile("flds %[angle]" :: [angle] "m"(angle) : "memory", "st");
    if (reverse_sine) asm volatile("fchs" ::: "st");
    asm volatile("fsin\n\tflds %[angle]\n\tfcos\n\tfld %%st(0)\n\tfmuls %[x]\n\tfld %%st(2)"
                 "\n\tfmuls %[y]\n\t.byte 0xde,0xe9\n\tfstps %[out_x]"
                 "\n\tfmuls %[y]\n\tfxch %%st(1)\n\tfmuls %[x]\n\tfaddp\n\tfstps %[out_y]"
        : [out_x] "=m"(result.x), [out_y] "=m"(result.y)
        : [angle] "m"(angle), [x] "m"(basis.x), [y] "m"(basis.y)
        : "memory", "st", "st(1)", "st(2)", "st(3)");
    return result;
}
#endif
} // namespace halo2::vehicle_candidates_tu15
