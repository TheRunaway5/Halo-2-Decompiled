#pragma once
#include <cstdint>

namespace halo2::native_object_record_ratio_detail {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<unsigned Count,class State>
std::uint32_t observed_shift_eax(State& state,std::uint32_t value) {
    static_assert(Count==4u||Count==6u);
    // The two original instructions are C1 E0 with their respective
    // immediates. Keep the observed AF/OF at their following boundaries.
    std::uint32_t result;
    asm volatile("shll $%c2,%%eax; movl %%eax,%%edx; seto %%al; lahf"
        :"+a"(value),"=d"(result):"i"(Count):"cc");
    state.flags.load(((value>>8u)&0xd5u)|((value&1u)<<11u));
    return result;
}
#endif

template<unsigned Count,class State>
std::uint32_t shift_eax_for_image(State& state,std::uint32_t value) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image==decltype(state.image)::TitleUpdate15)
        return observed_shift_eax<Count>(state,value);
#endif
    return state.flags.shift(value,Count,4);
}
} // namespace halo2::native_object_record_ratio_detail
