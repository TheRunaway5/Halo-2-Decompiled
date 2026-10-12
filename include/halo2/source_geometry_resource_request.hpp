#pragma once
#include "halo2/source_geometry_resource.hpp"

namespace halo2 {
// Addresses of the current owner cells, not cached values of their pointers.
// Request-side writes and service completion must share memory with fixups.
struct SourceGeometryResourceGlobals {
    std::uint32_t tags,physical,inflight,backing,disable_blocking,
        policy,policy_filter,missing_special,pending_failure;
    static constexpr SourceGeometryResourceGlobals retail() {
        return {0x4e3b44,0x4e649c,0x4e648c,0x4e6494,0x468c4c,
            0x4e64a0,0x4e64a4,0x4e64a8,0x4e64b0};
    }
    static constexpr SourceGeometryResourceGlobals map_pack_1_1() {
        return {0x4eb114,0x4eda6c,0x4eda5c,0x4eda64,0x4700ec,
            0x4eda70,0x4eda74,0x4eda78,0x4eda80};
    }
};
struct SourceGeometryResourceRequestServices {
    // Whole12E3A0 (TU1.1 12FB10) must own admission, allocation and real IO.
    // Its AL is unconsumed. The request rereads actual header+20 afterwards.
    std::function<void(std::uint32_t header,bool blocking,bool selective,
        std::int32_t priority)> acquire;
    // Whole120CE0 (TU1.1 122400), including literal promotion priority7.
    std::function<void(std::uint32_t job,std::int32_t priority)> promote;
    std::function<void()> yield; //2D2084 / TU1.1 2D84C4
    std::function<void()> pump;  //125D60 / TU1.1 1274B0
};
// Whole retail12DE70 / TU1.1 12F5E0. Existing linked banks touch physical
// age; pending requests consume policy and real completion, then join native
//223C70. No readiness result, allocation, completion or cache is invented.
// Missing reached services throw after the original preceding mutations.
bool source_geometry_resource_request(std::uint32_t header,std::uint32_t flags,
    const SourceGeometryResourceGlobals &,const SourceGeometryResourceMemory &,
    const SourceGeometryResourceRequestServices &);
}
