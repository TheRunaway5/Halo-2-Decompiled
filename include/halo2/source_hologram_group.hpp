#pragma once
#include "halo2/source_hologram_material.hpp"
#include "halo2/source_transparent_group_parts.hpp"
#include "halo2/menu_world_source_binding.hpp"
#include "halo2/menu_world_raster.hpp"
#include <map>
#include <tuple>

namespace halo2 {
struct SourceHologramCameraPose {
    Vec3 position,forward;
};
// The binder retains the genuine submitted raw pose alongside its unchanged
// c0..11 bank. c6 has already undergone2FD90 normalization and cannot recover
// the raw forward bits used by the actual ordinary camera push.
class SourceHologramCameraOwner {
public:
    const SourceHologramCameraPose& pose()const {return pose_;}
    bool matches(std::span<const ShaderVector,12>)const;
private:
    friend class SourceHologramGroupBindings;
    SourceHologramCameraOwner(SourceHologramCameraPose,const MenuWorldSourceConstants&);
    SourceHologramCameraPose pose_;
    std::array<ShaderVector,12> camera_;
};
// Only the actual group binder can issue this immutable Cache/LOD/section
// witness. A supplied RenderModel pointer is not an original-ordinal owner.
class SourceHologramBodyGeometry {
public:
    const Cache& cache_owner()const {return *cache_;}
    std::uint32_t render_model()const {return render_model_;}
    unsigned lod()const {return lod_;}
    unsigned section()const {return section_;}
    const RenderModel& model()const {return *model_;}
private:
    friend class SourceHologramGroupBindings;
    SourceHologramBodyGeometry(const Cache& cache,std::uint32_t model,unsigned lod,unsigned section,
        std::shared_ptr<const RenderModel> geometry)
        :cache_(&cache),render_model_(model),lod_(lod),section_(section),model_(std::move(geometry)){}
    const Cache* cache_{};
    std::uint32_t render_model_{};
    unsigned lod_{},section_{};
    std::shared_ptr<const RenderModel> model_;
};
// One descriptor from the completed423B0 group. Ordinary parts retain their
// existing owner-constant material binding; layer21 retains original indexed
// geometry, the actual current c50 palette and the post-cull index ranges.
struct SourceHologramGroupPart {
    unsigned layer{},command{},sorted{},base_ordinal{},section{},part{},vertex_class{};
    std::uint32_t object{};
    std::shared_ptr<const MenuWorldSourcePass> pass;
    unsigned hologram_material=~0u;
    std::vector<MenuWorldSourceDraw> hologram_ranges;
    unsigned body_vertices{}; // Proved unfiltered whole-part nondegenerate expansion.
    // Actual base-unit LOD/palette and immutable source ordinals. These are
    // body inputs, independent of the layer21 spatial ranges.
    unsigned geometry_lod{};
    std::optional<MenuWorldSourceDraw> body_source;
    std::shared_ptr<const SourceHologramBodyGeometry> body_geometry;
};
struct SourceHologramGroupDraw {
    const Cache *cache{};
    std::shared_ptr<const SourceHologramCameraOwner> camera_owner;
    SourceTransparentCommand command;
    double shader_clock{};
    std::vector<std::uint32_t> owners;
    std::vector<SourceHologramGroupPart> parts;
    MenuWorldSourceConstants constants{};
    MenuWorldRasterPacket raster;
    std::array<float,4> clip_range{}; // {minZ,maxZ,minW,maxW}; W bounds remain separate from raster jitter.
    std::array<float,2> fog_parameters{},surface_size{};
};
// Command-side observation is deliberately separate from GPU completion and
// framebuffer parity. A copy event must precede every body/override event.
struct SourceHologramGroupSubmission {
    enum class Kind { scene_copy,part } kind{};
    unsigned group{},descriptor{},layer{},range{},index_count{};
    std::uint32_t object{},shader{};
    std::uint64_t pipeline{},descriptor_set{};
    bool indexed{};
    std::uint64_t index_buffer{},index_offset{};
    unsigned first_index{};
    int vertex_offset{};
};
class SourceHologramGroupBindings {
public:
    SourceHologramGroupBindings(const Cache &,Mesh &,std::span<const std::uint32_t> definitions);
    void rebase_materials(unsigned previous_geometry_count,unsigned next_geometry_count);
    std::vector<SourceHologramGroupDraw> draw(
        std::span<const SourceLiveTransparentGroupTu15>,std::span<const SourceTransparentGroupParts>,
        std::span<const SourceLiveModelBaseDrawTu15>,const MenuWorldSourceConstants &,
        MenuWorldRasterPacket,std::array<float,4> clip,std::array<float,2> fog,
        std::array<float,2> surface,std::optional<SourceHologramCameraPose> submitting_camera={});
private:
    const Cache &cache_;
    std::map<std::pair<std::uint32_t,unsigned>,unsigned> materials_;
    std::map<std::pair<std::uint32_t,unsigned>,std::shared_ptr<const MenuWorldSourceSection>> geometry_;
    std::map<std::tuple<std::uint32_t,unsigned,unsigned>,std::shared_ptr<const SourceHologramBodyGeometry>> model_origins_;
};
// Validate a completed per-group command ledger against the actual descriptor
// bank. Empty filtered ranges are explicit; missing callbacks never pass.
void validate_source_hologram_group_submissions(std::span<const SourceHologramGroupDraw>,
    std::span<const SourceHologramGroupSubmission>);
}
