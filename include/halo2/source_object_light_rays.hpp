#pragma once
#include "halo2/source_render_contact.hpp"
#include "halo2/object_lighting.hpp"
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
struct SourceObjectLightRayRequest {
    std::uint32_t ordinal{},excluded=0xffffffffu;
    bool initial_query{}; //D2A50's separate byte argument, not its flags.
    Vec3 origin,delta; //Origin already includes the source .001 start offset.
};
struct SourceObjectLightRaysInput {
    bool bank_valid{}; //Actual active-lightmap/bitmap/checksum guard.
    std::uint32_t flags{},excluded=0xffffffffu;
    bool initial_query{};
    Vec3 origin; //Actual supplied owner center, not a camera proxy.
    std::optional<Vec3> custom_delta; //Bit2 selects exactly one when bit0 is clear.
    SourceRenderContactPacket caller_contact{}; //Original local uninitialized words.
};
//Complete14AF40 service: false can still partially write the contact record.
using SourceObjectLightRayContact=std::function<bool(
    const SourceObjectLightRayRequest&,SourceRenderContactPacket&)>;
//CompleteD1E10 service, including all original statuses and partial writes.
//flags and the separate initial-query byte are forwarded without conflation.
using SourceObjectLightRaySample=std::function<std::int32_t(
    const SourceRenderContactPacket&,std::uint32_t flags,bool initial_query,
    ObjectLightmapSample&)>;
struct SourceObjectLightRayStep {
    SourceObjectLightRayRequest request;
    bool contacted{};
    std::optional<std::int32_t> sample_status;
    SourceRenderContactPacket contact;
};
struct SourceObjectLightRaysResult {
    std::int32_t status=2;
    SourceRenderContactPacket contact;
    std::vector<SourceObjectLightRayStep> steps;
};
//CompleteD2A50 orchestration. It stops at the first sample status other than2;
//there is no ray averaging or weighting. A status2 sample can write output
//before another ray runs. D1850 cache acceptance is a separate outer service.
//Unknown contact/sample services must throw, not invent status2 or a sample.
SourceObjectLightRaysResult source_object_light_rays(const SourceObjectLightRaysInput&,
    const SourceObjectLightRayContact&,const SourceObjectLightRaySample&,
    ObjectLightmapSample& caller_output);
}
