#pragma once
#include "gameplay/objects/native_tu15_object_body_frame.hpp"

namespace halo2::native_particle_record_detail {
// 1E4D7 IMUL EAX,EAX,0x70 and 1E526 SHR ECX,2 leave flags the architecture
// does not define; the next memory access publishes them. Both execute the
// original instruction on the caller's flag context.
template<class State> std::uint32_t multiply_slot(State& s,std::uint32_t value) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return native_object_body_detail::arithmetic<0>(s,value,0x70u);
#else
    return s.flags.product(value,0x70u);
#endif
}
template<class State> std::uint32_t shift_count(State& s,std::uint32_t value) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);unsigned overflow{};
    asm volatile("addb $127,%%al;sahf;shrl $2,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(overflow),"+r"(value)::"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
#else
    return s.flags.right(value,0x2u,4,false);
#endif
}
} // namespace halo2::native_particle_record_detail
