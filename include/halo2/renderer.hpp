#pragma once
#include "halo2/pstencil/lightmap.hpp"
#include "halo2/pstencil/point_caster.hpp"
#include "halo2/renderer_device_lease.hpp"
#include "halo2/renderer_palette.hpp"
#include "halo2/renderer_constant_mode.hpp"
#include "halo2/renderer_depth_mode.hpp"
#include "halo2/renderer_particle_program.hpp"
#include "halo2/renderer_particle_view.hpp"
#include "halo2/renderer_output_state.hpp"
#include "halo2/renderer_texture_coordinates.hpp"
#include "halo2/renderer_targets.hpp"
#include "halo2/renderer_texture_pairs.hpp"
#include "halo2/renderer_aux_textures.hpp"
#include "halo2/renderer_debug_texture.hpp"
#include "halo2/source_lens_query_gpu.hpp"
#include "halo2/source_lens_draw_gpu.hpp"
#include "halo2/cache.hpp"
#include "halo2/appearance.hpp"
#include "halo2/display_settings.hpp"
#include "halo2/scene.hpp"
#include "halo2/cinematic_crossfade.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/water.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/scope_copy.hpp"
#include "halo2/light_volume.hpp"
#include "halo2/source_light_volume_draw.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/popup_blur.hpp"
#include "halo2/source_shadow_plan.hpp"
#include "halo2/source_shadow_raw_plan.hpp"
#include "halo2/renderer_timing.hpp"
#include "halo2/source_view_clear.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/source_transparent_draw.hpp"
#include "halo2/source_weather_point_render.hpp"
#include "halo2/source_weather_line_render.hpp"
#include "halo2/source_weather_plate_render.hpp"
#include "halo2/source_decal_draw.hpp"
#include "halo2/source_contrail_draw.hpp"
#include <SDL.h>
#include <array>
#include <functional>
#include <memory>

namespace halo2 {
class MainmenuWorldSourcePlan;
class SourceBspResidentPlan;
struct MainmenuBspAdmissionFrame;
// Retained source frame and current animated material bank for the title world.
// This handoff does not establish wider-than-canonical visibility admission.
struct MainmenuWorldDraw {
    const MainmenuBspAdmissionFrame &frame;
    MainmenuWorldSourcePlan &source;
    const SourceBspResidentPlan &resident;
    std::span<const Material> materials;
};
struct SourceBspAdmission;
class SourceBspResidentPlan;
struct SourceBspPartBinding;
struct ObjectLightingBinding {
    unsigned first_material{}, material_count{};
    ObjectLightingState state;
};
struct SourceViewShaderConstants {
    std::array<ShaderVector,7> world{},reflection{}; // Originalc0..6.
    std::array<ShaderVector,4> foreground{}; // Original12D50 BL=1 c0..3.
};
// Logical-device state used by the raw original-world shader admission. API
// support alone does not establish that the required features were enabled.
struct SourceWorldDeviceState {
    std::uint32_t instance_api_version{}, physical_api_version{};
    bool geometry_shader_enabled{}, geometry_point_size_enabled{};
    bool depth_clamp_enabled{}, demote_to_helper_enabled{};
    bool anisotropy_enabled{}, custom_border_colors_enabled{};
    float maximum_anisotropy{}, maximum_lod_bias{};
};
struct ModelObjectDraw {
    std::span<const Vertex> vertices;
    // This owner shares resident material images, while these constants must
    // remain independent even when several owners use the same shader slots.
    std::optional<ObjectLightingState> lighting;
    std::optional<ObjectColors> colors;
    ShaderInput shader_input; // Evaluated into this owner's independent constant bank.
    std::span<const SourcePrtMaterialBank> prt_banks;
};
// Read-only CPU metadata for the material image cache at uniform upload.
// A draw may instead bind a baked lightmap, accumulation/scope/water target,
// or owner alias; this observation is not a per-draw descriptor/pipeline.
struct RendererObservedMaterialPage {
    std::uint32_t width{},height{},depth{},layers{},mip_count{},format{};
    // Some source cohorts retain these bytes; absence remains unobserved.
    std::optional<std::uint64_t> source_pixel_fingerprint; // Retained FNV1a identity.
    bool source_image{};
    std::optional<bool> source_mutated;
    std::uint64_t image_identity{},view_identity{},cube_view_identity{},sampler_identity{};
    std::span<const TextureMip> source_mips;
};
struct RendererCompletedMaterialUpload {
    std::size_t root_material_count{},expanded_material_count{},root_slot{},material_slot{};
    std::uint32_t source_shader{},selected_shader_lod{};
    std::uint64_t target_generation{};
    std::optional<std::uint64_t> source_frames_submitted;
    std::array<std::uint32_t,4> wanted_frames{},bound_frames{};
    std::array<ShaderTextureState,4> texture_states{};
    std::array<RendererObservedMaterialPage,4> cached_material_pages{};
    bool baked_lightmap{},accumulation{},scope{};
    std::array<bool,2> water_targets{};
    // Borrowed only during the synchronous callback, after actual per-view
    // completion and before upload. The observer must copy any retained bytes.
    std::span<const std::byte> material_uniform,vertex_bank_uniform;
};
// Capture-only facts copied from actual Vulkan creation/write/bind/draw calls.
// These record command inputs, not surviving samples or original-frame parity.
struct RendererObservedImageBinding {
    std::uint64_t sampler{},view{},image{};
    std::uint32_t layout{},width{},height{},depth{},layers{},mip_count{},format{};
    bool present{},image_owner_known{};
};
struct RendererObservedDescriptor {
    std::uint64_t handle{};
    // Actual descriptor bindings0..3,5[0..3],6[0..3]. No active-table guess.
    std::array<std::array<RendererObservedImageBinding,4>,3> images{};
    std::array<std::array<std::uint64_t,3>,2> buffers{}; // bindings4/7: buffer/offset/range.
};
struct RendererObservedPipelineShader {
    std::uint32_t stage{};
    std::uint64_t module{};
    std::vector<std::uint32_t> words;
    std::vector<std::array<std::uint32_t,3>> specialization_entries;
    std::vector<std::byte> specialization_bytes;
};
struct RendererObservedPipeline {
    std::uint64_t handle{},layout{},render_pass{};
    std::array<std::uint32_t,6> raster{};
    std::array<float,4> raster_floats{};
    std::array<std::uint32_t,5> depth{};
    std::array<float,2> depth_bounds{};
    std::vector<std::uint32_t> sample_mask,dynamic_states;
    std::array<std::array<std::uint32_t,7>,2> stencil{};
    std::array<std::uint32_t,8> blend{};
    std::array<std::uint32_t,4> samples{};
    float minimum_sample_shading{};
    std::array<std::uint32_t,2> assembly{};
    std::vector<std::array<std::uint32_t,3>> vertex_bindings;
    std::vector<std::array<std::uint32_t,4>> vertex_attributes;
    std::vector<RendererObservedPipelineShader> shaders;
};
struct RendererObservedTargetTransition {
    unsigned phase{};std::size_t transitions{};bool complete{};
    std::uint64_t render_pass{},framebuffer{};
    std::array<std::int64_t,4> render_area{};
    std::vector<std::array<std::uint32_t,4>> clear_values;
};
enum class RendererBspDrawPhase { scene_base,accumulation_base,environment,overlay_before,overlay_after,texture_mapping,decal,planar_fog };
// Read-only facts from the actual owner-geometry vkCmdDraw. Command submission
// does not establish visible samples, GPU completion or Xbox pixel parity.
struct RendererObservedObjectDraw {
    std::size_t object_index{};
    RendererBspDrawPhase phase{};
    std::array<std::uint32_t,2> source_material{};
    unsigned material_slot{},parent_material{};
    std::uint64_t pipeline{},descriptor{},vertex_buffer{};
    std::uint32_t first{},count{};
    bool owner_constants{};
};
struct SourceBspResidentPart;
struct SourceBspDrawToken;
struct SourceBspPartBinding;
struct RendererObservedBspDraw {
    std::uint64_t serial{},target_generation{},render_pass{},framebuffer{};
    RendererBspDrawPhase phase{};
    unsigned root_slot{},material_slot{},parent_material{},pipeline_index{};
    std::array<std::uint32_t,2> source_material{};
    std::span<const RendererObservedTargetTransition> target_transitions;
    std::span<const std::byte> camera_push; // Exactly the currently commanded112 bytes.
    std::array<float,6> viewport{};
    std::array<std::int64_t,4> scissor{};
    const RendererObservedPipeline* pipeline{};
    const RendererObservedDescriptor* descriptor{};
    std::uint32_t dynamic_offset{},lightmap{};
    std::array<float,4> blend_constants{};
    std::uint64_t vertex_buffer{},vertex_offset{};
    std::uint32_t vertex_stride{},first{},count{},instances{},first_instance{};
    int geometry{};
    std::uint32_t section{},part{};
    bool source_resident{};
    const SourceBspDrawToken* source_token{};
    const SourceBspPartBinding* source_binding{};
    const SourceBspResidentPart* resident_part{};
    std::span<const Vertex> source_vertices; // Proven source-resident uploaded atlas range.
};
// A clipped model viewport, ordered amongst the UI's textured triangles.
struct ModelView {
    Camera camera;
    float left{}, top{}, width{}, height{}; // 640x480 UI coordinates.
    std::size_t before_ui_vertex{};
    std::span<const Vertex> vertices;
    bool full_surface{}; // Split-screen world: coordinates cover the entire drawable.
    std::span<const Vertex> foreground;
    std::span<const Vertex> background;
    // Per-view source particle submissions. Each bank owns its current camera
    // constants and is clipped to this view, including split-screen views.
    std::span<const ParticleDraw> world_particles,foreground_particles;
    // Actual per-view projected MGS2 draws. They join effect sorting through
    // their retained42850 records and use this view's clip on GPU submission.
    std::span<const SourceLightVolumeDraw> light_volumes;
    std::span<const SourceContrailDraw> contrails;
    std::function<void(const SourceContrailDraw &)> contrail_observer;
    std::function<void(const SourceLightVolumeDraw &)> light_volume_observer;
    std::span<const SourceWeatherPointDraw> weather_points;
    std::span<const SourceWeatherLineDraw> weather_lines;
    std::span<const SourceWeatherPlateDraw> weather_plates;
    // Complete ordinary-world and deferred-FP1E370 drains. Supplying a bank
    // replaces that phase's unordered layer15/particle submission. Commands
    // from all producers share its capacity, ordinal sorter and callback order.
    std::optional<SourceTransparentDrawBank> world_transparent,foreground_transparent;
    std::optional<BspFogSample> atmosphere;
    const BspAtmosphere *atmosphere_source{};
    std::span<const WaterFrame> water;
    std::span<const Vertex> water_reflection_background;
    Camera water_reflection_camera;
    float effect_time{};
    PlayerScreenFlash screen_flash;
    std::optional<std::size_t> screen_flash_after_ui_vertex;
    std::span<const ObjectLightingBinding> object_lighting;
    std::span<const ModelObjectDraw> objects;
    std::optional<ScopeCopyFrame> scope;
    // Actual 2C490 bank for original world/first-person pass scheduling.
    std::optional<SourceGeometryLodPolicy> source_render_policy;
    // Actual134390 per-view queues, constructed from source camera/portal
    // frustums. The caller retains this bank through Renderer::draw.
    const SourceBspAdmission *source_bsp_admission{};
    // The explicit plan owns the immutable BSP bank; vertices contains only
    // dynamic objects when this ordinary-world path is selected.
    const SourceBspResidentPlan *source_bsp_resident_plan{};
    std::span<const SourceBspPartBinding> source_bsp_bindings;
    std::optional<SourceViewShaderConstants> source_view_shader_constants;
    // Explicit ordered source records only; the renderer does not invent
    // casters/receivers or activate a runtime shadow selection service.
    // Nonempty roots require world W-depth and source_view_shader_constants
    // whose c0..6 exactly match the plan's camera bank.
    std::optional<SourceShadowPlan> source_shadows;
    // Original indexed class0..6 geometry and current palettes. Supplying
    // both shadow representations in one view is rejected before recording.
    std::optional<SourceShadowRawPlan> raw_source_shadows;
    // Optional live reference owner. Checked AFTER swapchain recreation and
    // before raw planning/recording; a stale native lease cannot bind this
    // plan to replacement attachments. Caller retains it through draw().
    const RendererTargets::Use* raw_source_shadow_targets{};
    // Completed source layer12 submissions, ordered after335E0 shadows by
    //2BCD0/31C80. Each light/view owns independent pixel and c31..39 banks.
    std::span<const EffectLightDraw> effect_lights;
    // Raw class0/1 alternative with original indexed geometry and palettes.
    // One view supplies one ordered representation; mixing both would lose
    // the source interleaving and is rejected.
    std::span<const EffectLightRawDraw> raw_effect_lights;
    // Original 2B0BAA/2C560 UI cameras use near/far 0.015625/1024.
    // Other views retain their existing projection planes when unset.
    std::optional<Vec2> projection_clip_planes;
    // Admitted captured132D0 initialization, distinct from native defaults.
    // Only target0 RGBA+depth+stencil initialization is currently supported.
    std::optional<SourceViewClearState> source_view_clear;
    // PC UI model widgets supply their separately truncated physical bounds.
    // Keeping those words avoids a second native canvas projection/rounding.
    std::optional<std::array<std::int32_t,4>> source_ui_viewport;
    // Completed retained decal cache pages, in original layer/cluster order.
    std::span<const SourceDecalDraw> source_decals;
    // An independent first-person lens keeps hands, weapon geometry and
    // deferred foreground draws in the same projection as their shader bank.
    std::optional<float> foreground_projection_vertical_angle;
    // Original query quads, submitted against world depth before foreground.
    std::span<const SourceLensQueryDraw> lens_queries;
    std::span<const SourceLensDraw> lens_draws;
    // Capture-only observer of slots already collected for this view. It
    // neither supplies a uniform nor adds a draw, visibility or binding.
    std::function<void(const RendererCompletedMaterialUpload&)> completed_material_upload_observer;
    std::function<void(const RendererObservedBspDraw&)> completed_bsp_draw_observer;
    std::function<void(const RendererObservedObjectDraw&)> completed_object_draw_observer;
    std::span<const pstencil::LightmapCaster> pstencil_lightmap_casters;
    std::span<const pstencil::PointCaster> pstencil_point_casters;
    bool pstencil_native_point_casters=false;
    // Exact configured4922F8 clip/effect distance, independent of the
    // submitted camera far in projection_clip_planes (491E7C).
    std::optional<float> source_w_distance;
};
// SDL owns the platform window/input; only this module knows Vulkan handles.
class Renderer {
    struct LensQueryRetirementData;
public:
    // The optional shadow table names actual root Mesh slots allocated before
    // construction. Its authored records must outlive this Renderer.
    // A scene handoff borrows presentation_owner only during construction and
    // retains its window instance/surface independently of the departed scene.
    Renderer(SDL_Window* window, const Mesh& mesh, bool validation,
             std::span<const SourceShadowPlanMaterial> shadow_materials = {},
             const SourceBspResidentPlan *bsp_resident_plan = nullptr,
             const std::function<void()> &before_presentation = {},
             const Renderer *presentation_owner = nullptr,
             const std::filesystem::path &shader_directory = {},
             std::span<const SourceShadowRawPlanMaterial> raw_shadow_materials = {});
    ~Renderer();
    // Frontend worlds and model widgets retain native resolution while
    // players edit their saved world-upscaling choices.
    void upscale_world(bool enabled);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    bool draw(const Camera& camera, const std::filesystem::path& screenshot = {},
              std::span<const Vertex> debug = {}, bool reticle = false,
              std::span<const Vertex> first_person = {}, ScreenEffects effects = {},
              std::span<const Vertex> ui = {}, std::span<const ModelView> views = {},
              std::span<const Vertex> background = {},
              std::span<const UiBackdrop> backdrops = {},
              // Actual current mainmenu aggregate, owned through this draw.
              // This does not admit canonical visibility for wider culling.
              const BspFogSample *mainmenu_fog = nullptr,
              const MainmenuWorldDraw *mainmenu_world = nullptr,
              bool ui_full_surface_bounds = false);
    // Isolated loading submission: only resident loading image descriptors,
    // supplied original triangles/constants, and frame synchronization are used.
    // No CPU world/cache/view owner participates.
    bool draw_loading(unsigned resident_material_base,std::span<const Material>,
                      std::span<const Vertex>,const std::filesystem::path& screenshot = {});
    // Called on the UI owner before transferring graphics to the loader.
    void prepare_loading();
    // Caller has excluded rendering. Release this SDL window's swapchain
    // before a different Vulkan device acquires presentation ownership.
    void release_presentation();
    void resized();
    std::string validate_display_settings(const DisplaySettings &) const;
    const DisplaySettings& display_settings() const;
    void display_settings(DisplaySettings);
    // The desktop session outlives every renderer using its preferences.
    void share_display_settings(DisplaySettings&);
    // Opt-in read-only profiling; disabled draws do not read the host clock.
    void measure_frames(bool);
    RendererFrameTiming frame_timing() const;
    // Read-only residency diagnostic; excludes scene/render-target images.
    std::size_t resident_material_images() const;
    // Update an existing 2D RGBA8 material image without recreating descriptors.
    void update_image(unsigned material, const Texture &);
    // Expand through this renderer's actual stage palette, then use the
    // existing single-level image update/fence/upload contract and shaders.
    // Palette::decode retains complete mip chains for ordinary static uploads.
    void update_palettized_image(unsigned material,unsigned stage,std::span<const std::byte>,
        unsigned width,unsigned height,bool swizzled,unsigned additional_mips=0);
    RendererPaletteFactory palette_factory();
    RendererConstantMode constant_mode_control();
    RendererDepthMode depth_mode_control();
    RendererParticleProgram particle_program_control();
    RendererParticleView particle_view_inputs();
    RendererParticleView prepare_particle_views(std::span<const RendererParticleViewInput>);
    RendererOutputState output_state_control();
    RendererTextureCoordinates texture_coordinate_control();
    RendererTargets target_resources();
    RendererTexturePairFactory texture_pair_factory();
    RendererAuxTextureFactory aux_texture_factory();
    RendererDebugTextureFactory debug_texture_factory();
    std::vector<SourceLensQueryResult> take_lens_query_results();
    // Scene/query-owner retirement waits for actual GPU work before releasing
    // identities. Completed results from the retiring scene are discarded.
    void retire_lens_queries() noexcept;
    class PreparedLensQueryRetirement {
    public:
        ~PreparedLensQueryRetirement();
        PreparedLensQueryRetirement(PreparedLensQueryRetirement &&) noexcept;
        PreparedLensQueryRetirement &operator=(PreparedLensQueryRetirement &&) noexcept;
    private:
        friend class Renderer;
        PreparedLensQueryRetirement();
        std::unique_ptr<LensQueryRetirementData> data_;
    };
    // Executor-local completed-frame staging; exclude rendering/loading and
    // retain the SDL window through token destruction. Preparation leaves old
    // query work/results intact. Any intervening draw, resize, harvest or
    // immediate retirement invalidates readiness, including declined draws.
    PreparedLensQueryRetirement prepare_lens_query_retirement();
    bool lens_query_retirement_ready(const PreparedLensQueryRetirement &) const noexcept;
    // No Vulkan call or allocation: move old GPU ownership/results into this
    // token and consume its commit eligibility. Keep the token alive across
    // the enclosing native commit. Destroy/move-assign it afterward, outside
    // session locks: that later retirement can wait for actual GPU work.
    bool commit_lens_query_retirement(PreparedLensQueryRetirement &&) noexcept;
    // Admission succeeds only for the requested actual uploaded Mesh bitmap.
    // The caller must retain device_lease() through its source ABI publication.
    void require_lens_bitmap(unsigned material,const Texture &) const;
    // Queue constants and resident bitmap frames after the previous draw's fence.
    void update_material_constants(std::span<const Material>);
    void update_material_constants(std::span<const Material>, std::span<const unsigned> active_roots);
    void scene_clear_color(Vec3);
    void scene_atmosphere(MenuAtmosphere, BspAtmosphere);
    void scene_bsp_atmosphere(const BspAtmosphere &);
    void source_fog_controls(SourceFogDrawControls);
    std::optional<BspPostprocessState> bsp_postprocess_state() const;
    void restore_bsp_postprocess_state(const BspPostprocessState &);
    void scene_bloom_threshold(float);
    void campaign_cinematic_bloom(bool,const CinematicBloomOverride &);
    void campaign_cinematic_crossfade(const CinematicCrossfadeFrame &);
    bool cinematic_crossfade_captured()const;
    void scene_color_filter(std::optional<PixelProgram>);
    void scene_light_volumes(std::vector<LightVolumeInstance>,
                             std::shared_ptr<const ShaderFunctionTables>);
    void scene_particles(std::vector<ParticleDraw>);
    std::string device_name() const;
    SourceWorldDeviceState source_world_device_state() const;
    RendererDeviceLease device_lease();
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    RendererDeviceLease device_lease_;
    RendererPaletteFactory palette_factory_;
};
} // namespace halo2
