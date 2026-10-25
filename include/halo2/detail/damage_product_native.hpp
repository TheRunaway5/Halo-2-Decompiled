#pragma once

namespace halo2::detail {
inline float damage_product(float modifier, float accumulated) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // TU1.5 D81B1 has the modifier in the destination. Commuting MULSS
    // changes the selected payload when both operands are distinct NaNs.
    asm volatile("mulss %1,%0" : "+x"(modifier) : "x"(accumulated));
    return modifier;
#else
    // Retain the previous scalar backend until separately qualified.
    return modifier * accumulated;
#endif
}
}
