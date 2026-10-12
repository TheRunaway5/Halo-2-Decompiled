// Generated from authenticated owned callers and retained public API declarations.
// Native void-call ABI preserves incoming registers; no original kernel clobber claim.
#pragma once
#include <array>
#include <cstdint>
namespace halo2::physics_platform_call_detail {
struct Binding { std::uint32_t instruction,target,return11,return15; };
inline constexpr std::array<Binding,3> bindings{{
    {0x2dacadu,0x80000115u,0x2dacb3u,0x2de763u},
    {0x2db25eu,0x80000126u,0x2db264u,0x2ded14u},
    {0x2db3d2u,0x80000126u,0x2db3d8u,0x2dee88u},
}};
} // namespace halo2::physics_platform_call_detail
