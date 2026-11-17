#pragma once
#include <cstdint>
#include <cstdlib>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "native_tu15_object_function_scalar_frame.hpp"
#endif

namespace halo2::native_object_function_scalar_owner_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_OBJECT_FUNCTION_SCALAR");
        return value && *value;
    }();
    return enabled;
}
inline std::uint64_t& activity() {
    thread_local std::uint64_t entries{};
    return entries;
}
template<class Cache,class Type,class Row,class Inherited> struct Children {
    Cache cache;
    Type type;
    Row row;
    Inherited inherited;
};

template<bool Literal,class State,class Routes> bool bridge(State& state,Routes children) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (state.image!=SourcePhysicsContactImage::TitleUpdate15 || reference()) return false;
    ++activity();
    native_object_function_scalar_detail::run_source<Literal,true>(state,children);
    return true;
#else
    (void)state;
    (void)children;
    return false;
#endif
}
} // namespace halo2::native_object_function_scalar_owner_detail
