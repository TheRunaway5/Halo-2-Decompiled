#pragma once
#include "halo2/native_tu15_activation_services.hpp"
namespace halo2 {
struct NativeTu15ActivationPayloadFilterServices {
    std::function<std::uint32_t(std::uint32_t motion,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)>motion_kind;
};
struct NativeTu15ActivationPayloadFilterStats {
    std::uint64_t calls{},motion_queries{},native_motion_queries{},
        unconverted_motion_queries{},property_queries{};
};
// Actual 1D7530: the first property whose key is 2001, or source datum zero.
std::uint32_t native_tu15_activation_body_datum(const SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody);
// Actual 281F10. Its filter receiver is unused. Preserve the source's actual
// first-component object/header selection and subsequent fresh byte reads.
// Unknown motion owners remain explicit typed dependencies.
std::uint32_t native_tu15_activation_payload_filter(SourcePhysicsContactMemory&,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15PhysicsFrameFp&,const NativeTu15ActivationPayloadFilterServices& = {},
    NativeTu15ActivationPayloadFilterStats* = nullptr);
} // namespace halo2
