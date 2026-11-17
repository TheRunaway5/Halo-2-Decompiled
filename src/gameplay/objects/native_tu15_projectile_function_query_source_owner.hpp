#pragma once
#include "gameplay/objects/native_tu15_projectile_function_query_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_projectile_function_query_owner_detail {
inline bool reference(){
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_PROJECTILE_FUNCTION_QUERY");
        return value&&*value;
    }();
    return enabled;
}
inline std::uint64_t& activity(){thread_local std::uint64_t count{};return count;}

// Both source callback formats keep their complete live instruction policy.
// Refusal leaves the entire frame and its providers untouched.
template<bool Literal,class State>bool bridge(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||reference())return false;
    ++activity();
    native_projectile_function_query_detail::run_source_frame<Literal>(s);
    return true;
#else
    (void)s;
    return false;
#endif
}
} // namespace halo2::native_projectile_function_query_owner_detail
