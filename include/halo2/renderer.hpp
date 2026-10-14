#pragma once
#include "halo2/renderer_device_lease.hpp"
#include "halo2/renderer_palette.hpp"
#include "halo2/renderer_constant_mode.hpp"
#include "halo2/cache.hpp"
#include "halo2/appearance.hpp"
#include "halo2/display_settings.hpp"
#include "halo2/scene.hpp"
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
class DebugPanel;
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
};
// SDL owns the platform window/input; only this module knows Vulkan handles.
class Renderer {
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
             const std::filesystem::path &shader_directory = {});
    ~Renderer();
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
    // No CPU world/cache/view owner or ImGui callback participates.
    bool draw_loading(unsigned resident_material_base,std::span<const Material>,
                      std::span<const Vertex>,const std::filesystem::path& screenshot = {});
    // Called on the UI owner before transferring graphics to the loader.
    void prepare_loading();
    // Caller has excluded rendering. Release this SDL window's swapchain
    // before a different Vulkan device acquires presentation ownership.
    void release_presentation();
    void resized();
    std::string validate_display_settings(const DisplaySettings &) const;
    // Lazy optional F1 overlay; closed panels submit no graphics commands.
    DebugPanel& debug_panel();
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
