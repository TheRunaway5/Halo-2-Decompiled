#pragma once
#include <cstdint>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"
#endif

namespace halo2::native_object_function_scalar_ordered_detail {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<unsigned Count,class State> std::uint32_t observed_right(State& state,std::uint32_t value) {
    static_assert(Count==9u || Count==26u);
    const auto context=state.flags.bits();
    auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
    unsigned overflow{};
    asm volatile("addb $127,%%al;sahf;shrl %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(overflow),"+r"(value):"I"(Count):"cc");
    state.flags.load((flags&0xd5u)|((overflow&1u)<<11u));
    return value;
}
#endif

template<unsigned Count,bool Left,class State> std::uint32_t shift_for_image(State& state,std::uint32_t value) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (state.image==decltype(state.image)::TitleUpdate15) {
        if constexpr (Left) return native_mopp_aabb_abi_detail::shifted_immediate<Count>(state,value);
        else return observed_right<Count>(state,value);
    }
#endif
    if constexpr (Left) return state.flags.shift(value,Count,4);
    else return state.flags.right(value,Count,4,false);
}
} // namespace halo2::native_object_function_scalar_ordered_detail
