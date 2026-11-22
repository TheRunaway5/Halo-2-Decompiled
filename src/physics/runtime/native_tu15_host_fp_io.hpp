#pragma once
#include <array>
#include <bit>
#include <cstdlib>
#include <type_traits>
#include <utility>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#endif

namespace halo2::native_host_fp_io_detail {
// Preserve the host's opaque FP image across an observation/provider callback.
// This owner neither imports nor publishes State's software x87/MMX file.
// Genuine child execution belongs outside this boundary so its returned FP
// outcome remains visible to the caller.
template<class Callback>void preserve(Callback&&callback){
    static_assert(std::is_void_v<std::invoke_result_t<Callback>>);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto image=native_raw_x87_detail::capture();
    try{std::forward<Callback>(callback)();}
    catch(...){native_raw_x87_detail::restore(image);throw;}
    native_raw_x87_detail::restore(image);
#else
    std::forward<Callback>(callback)();
#endif
}
// The same preservation with native_raw_x87_detail::Scope::io's exact
// empty-stack fast path. A callback entered with an empty host register file
// at TOP 0 (every ordinary x86-64 call boundary) has nothing to park: FXSAVE
// samples CW/SW/tags/MXCSR, and after the callback any difference restores
// exactly that environment. Empty register payloads and FIP/FDP/FOP are
// unobservable. Any other image, and HALO2_REFERENCE_SCOPED_ACCESS=1, keep the
// full FNSAVE/FRSTOR preserve above.
template<class Callback>void preserve_empty(Callback&&callback){
    static_assert(std::is_void_v<std::invoke_result_t<Callback>>);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!native_raw_x87_detail::scoped_access_reference()){
        alignas(16) std::array<std::byte,512> area;
        asm volatile("fxsave %0":"=m"(area)::"memory");
        std::uint16_t control{},status{};std::uint32_t mxcsr{};
        std::memcpy(&control,area.data(),2);std::memcpy(&status,area.data()+2,2);std::memcpy(&mxcsr,area.data()+24,4);
        if(!std::to_integer<unsigned>(area[4])&&!(status&0x3800u)){
            const auto settle=[&]{
                alignas(16) std::array<std::byte,512> after;
                asm volatile("fxsave %0":"=m"(after)::"memory");
                if(std::memcmp(after.data(),area.data(),6)||std::memcmp(after.data()+24,area.data()+24,4)){
                    native_raw_x87_detail::Image source{};source.word(0,control);source.word(4,status);source.word(8,0xffffu);
                    asm volatile("fninit; fldenv %0"::"m"(source.x87):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
                    _mm_setcsr(mxcsr);
                }
            };
            try{std::forward<Callback>(callback)();}
            catch(...){settle();throw;}
            settle();return;
        }
    }
#endif
    preserve(std::forward<Callback>(callback));
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline bool reference_simplex_iteration_projection_io() noexcept {
    static const bool reference=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_SIMPLEX_ITERATION_PROJECTION_IO");
        return value && *value;
    }();
    return reference;
}

// The retained iteration and projection stages keep source x87 in their
// State. These transports have no host FP effects. A refused access still
// reaches its original scoped provider and publishes the same fault prefix.
template<class Memory>
bool read_simplex_projection_vector(const Memory& memory,std::uint32_t address,__m128& output) noexcept {
    if(reference_simplex_iteration_projection_io())return false;
    if(const auto* page=memory.direct_page(address,sizeof(output))) {
        std::memcpy(&output,page->bytes+(address&4095u),sizeof(output));
        return true;
    }
    return false;
}

template<class State>
bool write_simplex_retained_word(State& state,std::uint32_t address,std::uint32_t value) noexcept {
    if(state.observe || reference_simplex_iteration_projection_io())return false;
    const auto bytes=std::bit_cast<std::array<std::byte,4>>(value);
    return state.memory.try_callback_free_write(address,bytes);
}
#endif
} // namespace halo2::native_host_fp_io_detail
