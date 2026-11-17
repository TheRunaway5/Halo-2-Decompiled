#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <cstdint>

namespace halo2::native_object_ratio_caller_detail {
// Preserve the older model for MapPack and platforms without the exact x86 backend.
template<bool Ordered,class State> void shift_definition_index(State& state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(Ordered){
        if(state.image==SourcePhysicsContactImage::TitleUpdate15){
            auto input=state.gp[1];std::uint32_t output;
            asm volatile("shll $4,%%ecx; movl %%ecx,%%edx; seto %%cl; lahf; movb %%ah,%%ch"
                :"+c"(input),"=d"(output)::"eax","cc");
            state.gp[1]=output;
            state.flags.load(((input>>8u)&0xd5u)|((input&1u)<<11u));
            return;
        }
    }
#endif
    state.gp[1]=state.flags.shift(state.gp[1],4u,4);
}

template<bool Ordered,class State> void divide_clock_digit(State& state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(Ordered){
        if(state.image==SourcePhysicsContactImage::TitleUpdate15){
            auto eax=state.gp[0],edx=state.gp[2];
            std::uint32_t quotient;
            unsigned char overflow;
            asm volatile("idivl %%ecx; seto %[overflow]; movl %%eax,%[quotient]; lahf; movzbl %%ah,%%eax"
                :"+a"(eax),"+d"(edx),[quotient]"=&r"(quotient),[overflow]"=&qm"(overflow)
                :"c"(state.gp[1]):"cc");
            state.gp[0]=quotient;state.gp[2]=edx;
            state.flags.load((eax&0xd5u)|(std::uint32_t(overflow)<<11u));
            return;
        }
    }
#endif
    state.quotient(state.gp[1],true);
}
} // namespace halo2::native_object_ratio_caller_detail
