#pragma once

#include "halo2/physics_contact_runtime.hpp"
#include <cstdlib>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/vehicles/native_tu15_vehicle_effect_ratio_source_frame.hpp"
#endif

namespace halo2::native_vehicle_effect_ratio_owner_detail {

inline bool reference() {
    static const bool selected=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_VEHICLE_EFFECT_RATIO");
        return value&&*value;
    }();
    return selected;
}

// Private source admission. The original entry remains available on refusal;
// admitted operations keep the real source callbacks and current providers.
template<bool Literal,class State> bool bridge(State& s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||reference()||
       s.active_mmx||s.fp.count>8u||s.fp.top>7u||(s.fp.status&0x8080u))return false;
    std::uint16_t cw{},sw{};
    asm volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));
    const auto csr=_mm_getcsr();
    if((cw&0x3fu)!=0x3fu||(sw&0x8080u)||(csr&0x1f80u)!=0x1f80u)return false;
    native_vehicle_effect_ratio_detail::run_source_private<Literal,true>(s);
    return true;
#else
    (void)s;
    return false;
#endif
}

} // namespace halo2::native_vehicle_effect_ratio_owner_detail
