#pragma once
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_object_presentation.hpp"
#include "halo2/source_model_fade.hpp"
#include "halo2/source_shader_lod.hpp"

namespace halo2 {
struct SourceModelBuildWorld;
// Bind133790/133AB0's typed collector to whole native4B5D0 over the same
// caller-owned memory/cache/arena banks. Only collector+4 onward is copied;
// collector owner+0 is retained by its caller. No callee output is supplied.
void source_object_build_collector(SourceObjectDrawCollector&,
    const SourceObjectBuildRequest&,const SourceModelBuildWorld&);
struct SourceRenderCollectorFlagsContext {
    std::uint8_t query_flags{}; // Current query+2A60; only its sign is tested.
    std::int32_t fog_classification{}; //4B9EF0.
    std::uint32_t planar_datum=0xffffffffu; //4B9F8C.
    float eye_fraction{}; //4B9FFC.
    bool planar_only{}; //4BA004.
    // Required only if the original planar definition read is reached.
    std::optional<std::uint8_t> planar_flags;
};
// Whole1332F0, including query sign, ordered equality and planar fog gates.
std::uint32_t source_render_collector_flags(std::uint16_t,
    const SourceRenderCollectorFlagsContext&);
struct SourceObjectRenderSphere {Vec3 center;float radius{};};
struct SourceObjectRenderEntryContext {
    std::uint32_t mode{};
    Vec3 eye;
    float distance_scale{},coverage_multiplier{}; //4B9DCC,4BA028.
    SourceShaderCoverageCamera coverage;
    SourceRenderCollectorFlagsContext flags;
};
struct SourceObjectRenderEntryRequest {
    std::uint32_t alternate{},section_counter{},section_group{},children{};
};
struct SourceObjectRenderEntryServices {
    // Current direct-low16 object body+30/+3C. No inferred mesh sphere.
    std::function<SourceObjectRenderSphere(std::uint32_t)> sphere;
    // Bind to whole native4BAF0 over the current object/parent/model banks.
    std::function<SourceModelFadeBytes(std::uint32_t,float)> fade;
    // Bind to whole native4B5D0. Owner+0 must survive the call.
    std::function<void(SourceObjectDrawCollector&,const SourceObjectBuildRequest&)> build;
    // Bind to whole native133AB0, sharing current section/draw/cache banks.
    std::function<void(SourceObjectDrawCollector&,std::uint32_t,std::uint32_t,std::uint32_t)> present;
};
// Whole133790. The collector starts with the caller's retained local storage;
// only source-written bytes change. Row association/flags remain literal
// inputs and the first fade byte owns queue lowbit0. Returns the original AL
// first-person byte, used by133720 to repeat a mode1 owner as world geometry.
// Missing deep owners reject; this routine never substitutes build records.
std::uint8_t source_object_render_entry(SourceBspAdmissionRecord&,
    SourceObjectDrawCollector&,const SourceObjectRenderEntryRequest&,
    const SourceObjectRenderEntryContext&,const SourceObjectRenderEntryServices&);
}
