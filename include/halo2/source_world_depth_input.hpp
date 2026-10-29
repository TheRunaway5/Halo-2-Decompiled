#pragma once
#include "halo2/source_live_shadow_phase_inputs_tu15.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/world_material_uniform.hpp"
#include "halo2/source_main_camera_receipt_tu15.hpp"
#include "halo2/native_world_kernel.hpp"
#include "halo2/bitmap_frame_animation.hpp"
#include <memory>
#include <string>
#include <variant>

namespace halo2 {
struct SourceWorldDepthMaterialIdentity {
    unsigned root_slot{};
    std::string child_path;
    auto operator<=>(const SourceWorldDepthMaterialIdentity&) const=default;
};
struct SourceWorldDepthRaster {
    std::array<ShaderVector,12> camera;
    ShaderVector clip_range;
    std::array<float,2> surface_size;
    std::array<float,2> logical_surface_size; // Actual retained native viewport extent before temporal scaling.
    std::array<std::uint32_t,4> surface_clip;
    // Literal NV owner order {xmin,xmax,ymin,ymax}, maxima INCLUSIVE.
    std::array<std::int32_t,4> window_inclusive;
    std::array<std::int32_t,2> surface_scale;
    std::array<float,2> factor_offset;
    std::uint32_t cull{},front_face{};
    bool w_clamp{};
};
struct SourceWorldDepthImageUpload {
    const Texture* shape{};
    std::uint64_t pixel_fingerprint{};
    std::uint32_t mip_count{};
    bool source_image{},mutated{};
};
class Renderer;
class SourceWorldDepthInput;
// An actual Renderer frame binding, issued after the bitmap descriptor
// producer commits. Copies retain no ownership of the synchronous upload
// epoch, so neither a later draw nor a recycled frame index can reuse it.
class SourceWorldDepthFrameReceipt {
public:
    const SourceWorldDepthMaterialIdentity& identity() const {return identity_;}
    const Material* source_material() const {return material_;}
    unsigned expanded_slot() const {return expanded_slot_;}
    const std::array<SourceWorldDepthImageUpload,4>& images() const {return images_;}
    const std::array<std::uint32_t,4>& frames() const {return bound_;}
    const std::array<ShaderTextureState,4>& samplers() const {return samplers_;}
private:
    friend class Renderer;
    friend class SourceWorldDepthInput;
    SourceWorldDepthFrameReceipt(const SourceWorldDepthInput* owner,
        const RendererTargets::Use& use,const SourceWorldDepthMaterialIdentity& identity,
        const Material* material,unsigned expanded_slot,const std::shared_ptr<const unsigned>& epoch,
        std::uint64_t descriptor,std::array<std::uint32_t,4> wanted,
        std::array<std::uint32_t,4> bound,std::array<SourceWorldDepthImageUpload,4> images,
        std::array<ShaderTextureState,4> samplers,
        std::array<std::array<std::uint64_t,3>,4> views,
        std::array<std::uint64_t,4> gpu_samplers,
        std::span<const BitmapFrameAnimation> animations)
        :owner_(owner),device_(use.device_address()),generation_(use.bank().generation),
         identity_(identity),material_(material),expanded_slot_(expanded_slot),epoch_(epoch),
         descriptor_(descriptor),wanted_(wanted),bound_(bound),images_(images),samplers_(samplers),
         views_(views),gpu_samplers_(gpu_samplers),animations_(animations){}
    const SourceWorldDepthInput* owner_{};
    NativeRuntimeAddress device_;
    std::uint64_t generation_{};
    SourceWorldDepthMaterialIdentity identity_;
    const Material* material_{};
    unsigned expanded_slot_{};
    std::weak_ptr<const unsigned> epoch_;
    std::uint64_t descriptor_{};
    std::array<std::uint32_t,4> wanted_,bound_;
    std::array<SourceWorldDepthImageUpload,4> images_;
    std::array<ShaderTextureState,4> samplers_;
    // Actual sampler and 2D/cube/volume views committed by the descriptor
    // producer, including the appropriate dimensional fallback view.
    std::array<std::array<std::uint64_t,3>,4> views_;
    std::array<std::uint64_t,4> gpu_samplers_;
    std::span<const BitmapFrameAnimation> animations_;
};
struct SourceWorldDepthCompletedMaterial {
    SourceWorldDepthMaterialIdentity identity;
    unsigned expanded_slot{};
    std::span<const std::byte> material_uniform,vertex_bank_uniform;
    const Material* source_material{}; // Actual Renderer constructor owner.
    const NativeWorldKernelEntry* native_kernel{};
    std::array<std::uint32_t,60> native_controls{};
};
// Complete native source input admission for ordinary BSP W depth.
// The source memory, complete Mesh and real target use must outlive it.
class SourceWorldDepthInput {
public:
    // Access checks the live World/borrow/camera/device. The Renderer audits
    // complete immutable owners at admission, final upload and recording
    // boundaries; individual draw checks do not rescan unrelated texture banks.
    const SourceWorldDepthRaster& raster(const RendererTargets::Use&) const;
    std::span<const SourceWorldDepthMaterialIdentity> identities() const {return identities_;}
    // Complete owner coverage is mandatory. No partial root/child success is
    // returned; callers validate before uploading or recording anything.
    void validate_completed(const RendererTargets::Use&,
        std::span<const SourceWorldDepthCompletedMaterial>) const;
    void validate_source_owners(const RendererTargets::Use&) const;
    // Packed BSP geometry preserves the actual ordinary instance stipple;
    // this does not change the separate raw-shadow fade admission guards.
    void validate_geometry(const RendererTargets::Use&,std::span<const Vertex>) const;
    void validate_resident_atlas(const RendererTargets::Use&,const SourceBspResidentPlan&,
        std::span<const Vertex> actually_uploaded) const;
    void validate_resident_view(const RendererTargets::Use&,
        const SourceBspResidentPlan&,std::span<const SourceBspPartBinding>,
        const SourceBspMergedDrawPlan&) const;
    void validate_resident_visibility(const RendererTargets::Use&,
        const SourceBspAdmission&) const;
    bool admits_draw(const SourceBspDrawKey&) const;
    bool independent_draw(const SourceBspDrawKey&) const;
    void validate_draw(const RendererTargets::Use&,const SourceBspDrawKey&,
        std::span<const Vertex>,bool resident) const;
    void validate_uploaded_material(const RendererTargets::Use&,
        const SourceWorldDepthMaterialIdentity&,const Material*,
        const std::array<std::uint32_t,60>& actual_pipeline_controls,
        std::span<const SourceWorldDepthImageUpload,4>,
        const std::array<ShaderTextureState,4>&,
        const std::array<std::uint32_t,4>&,
        const SourceWorldDepthFrameReceipt* = nullptr,unsigned expanded_slot=~0u) const;
    void validate_uploaded_lightmaps(const RendererTargets::Use&,
        std::span<const SourceWorldDepthImageUpload>) const;
    void validate_camera(const RendererTargets::Use&,std::array<float,3> eye,
        std::array<float,3> forward,std::array<float,3> up) const;
    void validate_camera_push(const RendererTargets::Use&,ShaderVector forward,
        ShaderVector eye,ShaderVector viewport) const;
    std::span<const Vertex> selected_geometry() const {return selected_geometry_;}
    const Cache& cache_owner()const {return *cache_;}
private:
    void validate_lifetime(const RendererTargets::Use&) const;
    explicit SourceWorldDepthInput(SourceLiveShadowPhaseInputsTu15 camera_owner)
        :camera_owner_(std::move(camera_owner)){}
    explicit SourceWorldDepthInput(SourceMainCameraRasterUseTu15 camera_owner)
        :camera_owner_(std::move(camera_owner)){}
    std::array<ShaderVector,12> camera(const RendererTargets::Use&) const;
    static SourceWorldDepthInput complete_owners(SourceWorldDepthInput,
        const RendererTargets::Use&,const Cache&,const Mesh&,
        const SourceBspResidentPlan&,const SourceBspResidentViewPlan&);
    friend SourceWorldDepthInput source_world_depth_input_completed_main_tu15(
        SourceMainCameraRasterUseTu15,const RendererTargets::Use&,const Cache&,
        const Mesh&,const SourceBspResidentPlan&,const SourceBspResidentViewPlan&);
    friend SourceWorldDepthInput source_world_depth_input_tu15(
        const SourceRuntimeMemory&,const RendererTargets::Use&,const Cache&,
        const Mesh&,const SourceBspResidentPlan&,const SourceBspResidentViewPlan&,std::array<std::uint32_t,2>);
    struct MaterialOwner {
        SourceWorldDepthMaterialIdentity identity;
        const Material* material{};
        std::uint32_t shader{},lod{},template_pass{},implementation{};
        unsigned forward_vertex{},blend{};
        ForwardRasterState raster;
        VertexProgram vertex;
        std::vector<std::byte> tokens;
        PackedSourceVertexDeclaration declaration;
        ShaderConstants constants;
        std::array<ShaderTextureState,4> textures;
        std::array<std::uint32_t,4> frames;
        std::array<Texture,4> images;
        std::array<std::vector<Texture>,4> animated_images;
        std::vector<BitmapFrameAnimation> bitmap_animations;
        PixelProgram pixel;
        std::array<bool,5> target_bindings;
    };
    std::variant<SourceLiveShadowPhaseInputsTu15,SourceMainCameraRasterUseTu15> camera_owner_;
    SourceWorldDepthRaster raster_;
    const Mesh* mesh_{};
    const Cache* cache_{};
    std::uint64_t generation_{};
    NativeRuntimeAddress device_;
    std::vector<SourceBspShaderPart> parts_;
    std::vector<SourceBspShaderPart> independent_parts_;
    const SourceBspResidentPlan* resident_{};
    std::vector<Vertex> resident_atlas_,selected_geometry_;
    std::vector<SourceBspPartBinding> bindings_;
    SourceBspMergedDrawPlan expected_bsp_;
    std::vector<SourceBspDrawKey> selected_draws_;
    std::vector<SourceBspDrawKey> independent_draws_;
    struct WaterOwner {
        unsigned root{};
        const Material* material{};
        std::uint32_t shader{},lod{},pass{},implementation{},forward_vertex{};
        std::string vertex_shader;
        std::vector<std::byte> tokens;
    };
    std::vector<WaterOwner> independent_water_;
    const SourceRuntimeMemory* memory_{};
    std::array<float,3> eye_,forward_,up_;

    std::vector<SourceWorldDepthMaterialIdentity> identities_;
    std::vector<MaterialOwner> materials_;
    std::vector<Texture> lightmaps_;
};
// The production caller obtains use from its actual initialized
// NativeTu15GraphicsDevice::target_resources(). A standalone diagnostic may
// bind retained source camera bytes to a genuine Renderer lease explicitly;
// that proves this bounded contract, not the captured game's device identity.
SourceWorldDepthInput source_world_depth_input_tu15(const SourceRuntimeMemory&,
    const RendererTargets::Use&,const Cache&,const Mesh&,
    const SourceBspResidentPlan&,const SourceBspResidentViewPlan& actual_selection,
    std::array<std::uint32_t,2> logical_surface={});
// The capability is issued by a completed actual main call, and bound only
// inside a genuine same-World/same-frame read-only borrow. A copy cannot extend
// that callback lifetime. This branch never rereads or republishes late source
// camera globals; actual target/material/geometry owners remain mandatory.
SourceWorldDepthInput source_world_depth_input_completed_main_tu15(
    SourceMainCameraRasterUseTu15,const RendererTargets::Use&,const Cache&,
    const Mesh&,const SourceBspResidentPlan&,const SourceBspResidentViewPlan&);
} // namespace halo2
