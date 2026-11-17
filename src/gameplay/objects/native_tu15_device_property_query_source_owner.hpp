#pragma once
#include "gameplay/objects/native_tu15_device_property_query_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_device_property_query_owner_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_DEVICE_PROPERTY_QUERY");
        return value&&*value;
    }();
    return enabled;
}
inline std::uint64_t& activity() {thread_local std::uint64_t count{};return count;}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class PreserveHostC1 {
    static constexpr std::uint32_t c1=0x200u;
    static std::uint16_t status() noexcept {
        std::uint16_t value;
        asm volatile("fnstsw %0":"=a"(value)::"memory");
        return value;
    }
    const std::uint16_t incoming_=status();
public:
    ~PreserveHostC1() {
        if(!((status()^incoming_)&c1))return;
        // Modeled guest C1 stays in the frame. Preserve the host caller's C1
        // across compiler-generated x87 spills, including on unwind.
        std::array<std::uint32_t,7> environment;
        asm volatile("fnstenv %0":"=m"(environment)::"memory");
        environment[1]=(environment[1]&~c1)|(incoming_&c1);
        asm volatile("fldenv %0"::"m"(environment):"memory");
    }
};
#endif

template<bool Literal,bool Fresh,class State,class Spill> bool bridge(State&state,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference())return false;
    const PreserveHostC1 host_c1;
    ++activity();
    native_device_property_query_detail::run_source_frame<Literal,Fresh>(state,spill);
    return true;
#else
    (void)state;(void)spill;
    return false;
#endif
}
} // namespace halo2::native_device_property_query_owner_detail
