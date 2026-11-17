#pragma once
#include "gameplay/objects/native_tu15_object_body_source_owner.hpp"
#include "gameplay/objects/native_tu15_object_body_canonical_sites.hpp"
namespace halo2::native_object_body_canonical_owner_detail {
template<bool Literal=true,class State>std::uint32_t bridge(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return native_object_body_owner_detail::bridge<Literal>(s,native_object_body_detail::CanonicalSites{});
#else
    (void)s;return 0u;
#endif
}
}
