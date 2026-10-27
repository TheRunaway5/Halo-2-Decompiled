#pragma once
#include "halo2/source_bsp_shader_scene.hpp"
#include <compare>
#include <optional>

namespace halo2 {
class SourceTextureCameraBindingTu15;
// Same lexicographic key as Renderer::DrawKey. Material remains a draw binding;
// the world vertex declaration does not expose Vertex::material as an input.
struct SourceBspDrawKey {
    std::uint32_t material{},lightmap{};
    int geometry=-1;
    std::uint32_t section=0xffffffffu,part=0xffffffffu;
    auto operator<=>(const SourceBspDrawKey &) const=default;
};
struct SourceBspResidentPart {
    SourceBspDrawKey original;
    std::uint32_t first{},count{};
    std::optional<std::uint32_t> uniform_stipple;
};
enum class SourceBspPartDecision {untouched,accepted,suppressed};
enum class SourceBspPartSuppression {none,zero_alpha,rejected_lod};
// The caller establishes original2D000's lighting classification. The default
// retains the existing static-lightmap import; classification3 uses the
// authored default-lighting implementation for every available lighting LOD.
enum class SourceBspResidentLighting { static_lightmap, classification3 };
struct SourceBspShaderLodAuxiliary {
    bool present{},byte5{}; // Original510C50 pointer and cinematic byte+5.
};
struct SourceBspPartDecisionRecord {
    SourceBspPartDecision state=SourceBspPartDecision::untouched;
    SourceBspPartSuppression suppression=SourceBspPartSuppression::none;
    // Present only when the original selection path actually accepted a LOD.
    std::optional<SourceBspShaderPart> selection;
};
// An absent material means the original part is removed. An absent stipple
// override preserves the raw vertex values; it does not invent an alpha.
struct SourceBspPartBinding {
    std::optional<std::uint32_t> material;
    std::optional<std::uint32_t> stipple;
};
struct SourceBspResidentViewPlan {
    // Catalogue order, including accepted, suppressed and untouched parts.
    std::vector<SourceBspPartBinding> bindings;
    std::vector<SourceBspPartDecisionRecord> decisions;
    // Exact old select() result order, including duplicate accepted requests.
    std::vector<SourceBspShaderPart> selected_parts;
};
enum class SourceBspVertexStorage {resident,dynamic,objects,object_runs};
struct SourceBspDrawToken {
    SourceBspDrawKey key;
    SourceBspVertexStorage storage=SourceBspVertexStorage::dynamic;
    std::uint32_t first{},count{};
    // The old all-dynamic packed offset, retained for differential consumers.
    std::uint32_t logical_first{};
    // Only object_runs storage uses this count; first indexes borrowed_runs.
    std::uint32_t run_count{};
};
struct SourceBspDescriptorRequest {
    std::uint32_t material{},lightmap{};
    auto operator<=>(const SourceBspDescriptorRequest &) const=default;
};
struct SourceBspObjectRun {std::uint32_t first{},count{};};
struct SourceBspMergedDrawPlan {
    // A single sorted list for every world/fog/environment/water/overlay/bloom
    // consumer. Buffer storage must not split or reorder that token stream.
    std::vector<SourceBspDrawToken> draws;
    std::vector<SourceBspDescriptorRequest> descriptors;
    std::vector<Vertex> dynamic_vertices;
    std::vector<SourceBspObjectRun> borrowed_runs;
    std::uint32_t logical_vertices{},borrowed_object_vertices{};
};
// Isolated ordinary BSP resident-bank/selection consumer. Constructor matches
// SourceBspShaderScene's material import and retains immutable raw geometry.
// It does not create a GPU buffer, descriptor, device or render success.
class SourceBspResidentPlan {
public:
    SourceBspResidentPlan(const Cache &,Mesh &,std::size_t bsp,
        SourceBspResidentLighting=SourceBspResidentLighting::static_lightmap);
    SourceBspResidentPlan(const Cache &,Mesh &,std::size_t bsp,SourceBspResidentLighting,
        const SourceTextureCameraBindingTu15 *texture_camera,bool live_texture_camera=false);
    SourceBspResidentViewPlan select(const SourceBspShaderView &) const;
    SourceBspResidentViewPlan select(const SourceBspShaderView &,
        SourceBspShaderLodAuxiliary) const;
    const std::vector<SourceBspResidentPart> &parts() const {return resident_parts_;}
    // Packed exactly as the existing Renderer constructor's encounter-order
    // catalogue, with stable raw input order within each complete key.
    const std::vector<Vertex> &resident_vertices() const {return packed_vertices_;}
    // Complete reference expansion in original input order; intended for
    // differential checks/fallback, not mandatory per-frame construction.
    std::vector<Vertex> expand(std::span<const SourceBspPartBinding>) const;
    SourceBspMergedDrawPlan merge(std::span<const SourceBspPartBinding>,
        std::span<const Vertex> dynamic_objects,unsigned material_count,unsigned lightmap_count,
        std::uint32_t dynamic_first=0,std::uint32_t logical_first=0) const;
    // Rebuild all current tokens and bytes while retaining only output capacity.
    // With borrow_objects, objects tokens address the supplied immutable input
    // span, which must outlive consumption of this frame plan. The default
    // keeps the standalone owned-output contract used by merge().
    void merge_into(SourceBspMergedDrawPlan &,std::span<const SourceBspPartBinding>,
        std::span<const Vertex> dynamic_objects,unsigned material_count,unsigned lightmap_count,
        std::uint32_t dynamic_first=0,std::uint32_t logical_first=0,bool borrow_objects=false) const;
private:
    SourceBspAdmissionBsp geometry_;
    SourceTransparentBspParts parts_;
    std::map<std::uint32_t,SourceShaderLodDefinition> shaders_;
    std::vector<std::vector<unsigned>> materials_;
    std::vector<std::uint32_t> source_shaders_;
    std::map<std::pair<int,unsigned>,unsigned> source_materials_;
    std::vector<Vertex> raw_vertices_,packed_vertices_;
    std::vector<unsigned> raw_part_;
    std::vector<SourceBspResidentPart> resident_parts_;
};
}
