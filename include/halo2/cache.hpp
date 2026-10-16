#pragma once
#include "halo2/render_program.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/tags/directory.hpp"
#include "halo2/tags/binary.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace halo2 {
class CacheFileSnapshot;
struct SourceDeviceMotionDefinition;
struct LevelContent;
struct SourceGameVariant;
struct ParticleSpriteDefinition;
struct AuthoredMuzzleEffect;
struct ParticleSystemRenderDefinition;
struct ParticleVertexSelection;
struct ParticleMaterial;
struct BspContent;
struct PredictedResource;
struct EncodedSound;
struct RenderModel;
struct Transform;
struct ScenarioObject;
struct SceneryDefinition;
struct SceneryLightprobe;
struct AnimationClip;
struct UnitAnimationDefinition;
class UnitAnimationGraphMetadata;
class SourceTagMetadata;
struct FrontendModelGrip;
struct FrontendModelPrimary;
struct SourceSceneryIdlePhaseDefinition;
struct FirstPersonIkDefinition;
struct FrontendModelWeaponIk;
struct LoopingSound;
struct FrontendContent;
struct HudContent;
struct FullScoreboardAssets;
struct BspAtmosphere;
struct SourcePortalBsp;
struct SourcePortalMaskDefinition;
struct SourceMachinePortalDefinition;
struct SourceBspAdmissionBsp;
struct SourceTransparentPart;
struct SourceTransparentBspParts;
struct SourceTransparentModelParts;
struct SourceModelPaletteDefinition;
struct SourceShaderLodDefinition;
struct BspWater;
struct ObjectLightingDefinition;
struct ObjectSkyLightingDefinition;
struct ObjectLightContactDefinition;
struct ObjectLightRenderLeaves;
struct ObjectLightSpatialBsp;
struct ObjectLightSpatialDefinition;
struct SourceInitialModelDefinition;
struct SourceInitialModelSelection;
struct SourceObjectConstructorMetadata;
struct SourceObjectColorDefinition;
struct SourceGeometryLodDefinition;
struct ShadowCasterDefinition;
struct WeaponAttachmentDefinition;
struct SourceMachineDefinition;
struct SourceControlDefinition;
struct SourceWeatherDefinition;
struct SourceWeatherBsp;
struct SourcePropDefinition;
struct SourceWorldWeaponDefinition;
struct SourceMachinePlacement;
struct SourcePrtDefinition;
struct SourcePrtLighting;
struct SourcePrtVertex;
struct BspSurfaceLighting;
struct OriginalData;
struct FirstPersonBindingDefinition;
struct ModelCollision;
struct TerrainCollision;
struct PickupCapsuleDefinition;
struct SoundVariationDefinition;
struct MultiplayerAudioBsp;
struct ItemPointDefinition;
struct LightVolumeDefinition;
struct LightVolumeAttachment;
struct FormatError : std::runtime_error { using std::runtime_error::runtime_error; };

// Xbox files are little endian. All offset/size checks happen before access.
class Bytes {
public:
    explicit Bytes(std::span<const std::byte> data) : data_(data) {}
    void require(std::size_t offset, std::size_t size) const;
    std::uint16_t u16(std::size_t offset) const;
    std::uint32_t u32(std::size_t offset) const;
    float f32(std::size_t offset) const;
    std::string string(std::size_t offset, std::size_t max_size) const;
    std::span<const std::byte> slice(std::size_t offset, std::size_t size) const;
    std::size_t size() const { return data_.size(); }
private:
    std::span<const std::byte> data_;
};

struct Vec3 { float x{}, y{}, z{}; };
struct Vec2 { float x{}, y{}; };
struct Vertex {
    Vec3 position; Vec3 color; Vec2 uv; std::uint32_t material{};
    Vec2 lightmap_uv{};
    Vec3 normal{}, tangent{}, binormal{}, incident{};
    std::uint32_t lightmap{}; // Zero is unbound; remaining indices address Mesh::lightmaps + 1.
    // >=0: authored cluster; -1: non-BSP geometry; <=-2: -2-instance ordinal.
    // Instances retain every containing cluster in BspAtmosphere for admission.
    std::int32_t bsp_cluster=-1;
    // Original PRT stream: signed scaled SHORT1 cluster and SNORM SHORT4 PCA
    // weights. These replace v5/v6 only for a pass with an explicit PRT bank.
    float prt_cluster{};
    ShaderVector prt_weights{};
    // Triangle expansion keeps the original geometry section and 72-byte
    // part ordinal. These CPU identities are not shader vertex attributes.
    std::uint32_t source_section=0xffffffffu,source_part=0xffffffffu;
    // Original instance alpha selects a 0..16 screen stipple pattern. Other
    // geometry retains the fully populated source pattern.
    std::uint32_t source_stipple_level=16;
    // Original decompressed model position before the current pose. The
    // plasma-shield local/skinned programs retain this for noise coordinates.
    Vec3 source_local_position{};
    // Immediate source attribute11 alpha, used only by the explicit simple
    // world color adapter. Existing model/BSP programs retain uniform v11.
    float source_color_alpha=1;
    // Literal raw72-byte part+0/+2, retained only as CPU provenance. Posing
    // and material selection must preserve these independently of shader LOD.
    std::uint16_t source_part_type=0xffffu,source_part_flags{};
};
struct TextureMip { std::uint32_t width{}, height{}; std::size_t offset{}, size{}; std::uint32_t depth=1; };
struct Texture {
    std::uint32_t width{}, height{};
    std::uint16_t format{}; // 11: RGBA8, 14/15/16: BC1/BC2/BC3.
    std::vector<std::byte> pixels;
    std::vector<TextureMip> mips; // Authored mip chain, contiguous in pixels; empty means one level.
    std::uint32_t layers = 1; // Six for +X/-X/+Y/-Y/+Z/-Z cubemaps; mips are face-major.
    std::uint32_t depth{}; // Zero: 2D/cube. Positive: authored volume, including 1x1x1.
};
struct BitmapResource {
    std::uint16_t width{}, height{}, depth{}, type{}, format{};
    std::array<std::vector<std::byte>,3> levels;
};
// Original bitmap+60/+64/+68..6E descriptor in map-header158/15C storage.
// This is independent of the three hardware partitions and preserves padding.
struct BitmapLowDetailResource {
    std::uint16_t width{},height{},depth{},type{},format{},flags{};
    std::uint32_t shared_offset{};
    std::vector<std::byte> pixels;
};
struct ForwardRasterState {
    std::uint32_t color_write = 0x1010101;
    std::uint32_t depth_enable = 2, depth_write = 0, depth_function = 0x203;
    std::uint32_t cull = 0x900; // Retail 1BBF0 material reset: D3DCULL_CW.
    std::uint32_t alpha_test = 0, alpha_function = 0x207, alpha_reference = 0;
    auto operator<=>(const ForwardRasterState &) const = default;
};
enum class UiFrameExtension : std::uint8_t { None, Left, Right };
// PC loading.bin has four ps_2_0 programs, independent of the Xbox combiner
// banks. The explicit native adapter retains their constants and blend state.
enum class VistaLoadingMaterialBlend : std::uint8_t { OneOne, DestinationColorZero, SourceAlphaInverseSourceAlpha };
struct VistaLoadingMaterial {
    std::uint32_t shader{};
    VistaLoadingMaterialBlend blend{};
    ShaderVector constant0{};
    // Framebuffer-pixel viewport translation supplied by the raster adapter.
    // The original DXVK vertex program retains unadjusted clip coordinates.
    Vec2 raster_offset_pixels{};
};
struct Material {
    std::string name; Texture diffuse;
    std::string template_name;
    std::vector<ShaderBitmap> bitmaps;
    std::vector<std::uint32_t> pixel_constants;
    std::vector<ShaderVector> vertex_constants;
    std::vector<ShaderLod> lods;
    std::vector<ShaderRange> layers, pass_implementations;
    std::vector<MaterialPass> passes;
    ShaderAnimation animation;
    std::array<std::int32_t,8> shader_switches{};
    float specular_brightness{}, ambient_brightness{};
    // Supported static lightmap pass, selected explicitly; other passes remain
    // available for the pass scheduler instead of being collapsed into diffuse.
    Texture detail, bump;
    std::array<ShaderVector, 3> texture_transforms{{{1,1,0,0},{1,1,0,0},{1,1,0,0}}};
    PixelProgram lightmap_program;
    std::vector<std::byte> vertex_program;
    ShaderConstants program_constants{};
    std::shared_ptr<const VertexProgramBank> vertex_bank;
    std::optional<SourceVertexDeclaration> vertex_declaration;
    SourceWorldVertexInputTransform world_vertex_input_transform=SourceWorldVertexInputTransform::Preserve;
    std::array<ShaderTextureState,4> texture_states{};
    bool static_lightmap_pass{};
    // Frontend forward passes retain the original combiner and authored
    // texture/color bindings and authored animated constants. Remaining
    // extern/pass scheduling gaps are retained in rendering_gaps.
    bool forward_pass{};
    bool hud_pass{}; // Procedural HUD quad; pixel program is still authored.
    // Native multiplayer text/cells share the full drawable's coordinate
    // conduit while retaining their ordinary UI pipeline and texture program.
    bool ui_full_surface{};
    std::optional<VistaLoadingMaterial> vista_loading;
    bool light_volume_pass{}; // Procedural 4FE20 glow; no material-template pass/remaps.
    std::uint32_t forward_pass_index = 0xffffffffu;
    unsigned forward_vertex{}; // 1: transparent; 2: emissive; 3: texture; 4: environment; 5: planar fog; 6: world lighting; 7/8: water; 9: PRT; 10: shadow; 11: projected decal; 12: immediate world RGBA11.
    unsigned forward_blend{}; // 1: alpha; 2: premultiplied; 3: additive; 5: planar constant color; 10: sphere RGB additive/alpha replace; 12: component max.
    std::uint32_t forward_blend_color{}; // Original dynamic state75, ARGB bytes.
    ForwardRasterState forward_raster;
    PixelProgram forward_program;
    Texture auxiliary;
    std::array<ShaderVector,8> forward_transforms{};
    ShaderVector forward_diffuse{1,1,1,1}, forward_specular{};
    // Authored bitmap frames remain separate images with their original mips.
    std::array<std::vector<Texture>,4> animated_textures;
    std::array<std::uint32_t,4> texture_frames{};
    // A model's base-color pass feeds the original projected texture extern 3.
    // Keep the environment pass separate so its depth test, cube lookup and
    // gel combiners run after the base geometry, including first-person views.
    std::shared_ptr<Material> environment_pass;
    // Original layer 16 for the bounded main-menu BSP. Scene objects keep
    // their separate visibility/admission path.
    std::shared_ptr<Material> planar_fog_pass;
    bool accumulation_texture{}; // Stage 0 is the active base-color target.
    bool world_pass{}, per_vertex_lighting{}, baked_lightmap_texture{};
    std::vector<std::shared_ptr<Material>> world_overlays_before, world_overlays_after;
    bool world_bloom_pass{};
    std::vector<std::shared_ptr<Material>> world_bloom_passes;
    std::array<std::shared_ptr<Material>,3> world_planar_passes;
    bool water_reflection_texture{},water_refraction_texture{},water_pass{};
    bool scope_texture{}; // Original texture extern27, target16 during first person.
    int water_definition=-1;
    std::vector<std::string> rendering_gaps;
    std::uint32_t source_shader_datum=0xffffffffu;
    unsigned selected_shader_lod{}; // Source1BF50 template index; geometry LOD is separate.
    std::optional<std::array<Vec3,4>> object_render_colors; // Packed/unpacked3DD10 draw bank.
    bool ui_emblem_pass{}; // 22A664/slot20 quad, centered frontend UI viewport.
    // Native aspect extension for decorative strips/dimming only. The shell
    // frame itself keeps its authored center and extends separate edge strips.
    bool ui_widescreen_background{};
    bool ui_frame_center{}; // Native horizontal-extension boundary sampling only.
    UiFrameExtension ui_frame_extension{};
};
struct BspLightingBucket {
    std::vector<std::uint32_t> incident;
    std::vector<std::array<std::uint8_t,3>> radiance;
    std::vector<Vec3> incident_vectors; // Vista FLOAT3; preserve authored precision.
};
struct BspLightingReference {
    std::int32_t bucket=-1;
    std::vector<std::uint16_t> section_offsets;
};
struct BspLightingScenery {
    std::uint32_t unique_id{}, checksum{};
    std::int16_t origin_bsp=-1;
    std::int8_t type{}, source{};
    BspLightingReference reference;
};
struct BspLighting {
    bool structure_matches{};
    std::uint32_t bitmap_datum=0xffffffffu; // Original 2D000 bank+1C admission.
    std::vector<Texture> images;
    std::vector<std::uint32_t> clusters, instances;
    std::vector<BspLightingBucket> buckets;
    std::vector<BspLightingReference> instance_buckets;
    std::vector<BspLightingScenery> scenery;
};
struct Spawn {
    Vec3 position; float facing{}; std::uint16_t bsp{};
    int team=8, player_type=2;
    std::array<int,4> game_types{12,0,0,0}, spawn_types{};
};
struct AtmosphericFog {
    Vec3 color;
    float maximum_density{}, start{}, opaque{};
};
struct PlanarFog {
    ShaderVector plane{};
    Vec3 color;
    float maximum_density{}, opaque_distance{}, opaque_depth{}, eye_offset{};
};
struct PatchyFog {
    Vec3 color;
    std::array<float,2> density{}, distance{};
    std::uint32_t flags{};
    float rotation{}, strafe{}, zoom{}, noise_scale{};
    float vertical_forward{}, vertical_up{}, opacity_up{}, animation_period{};
    Vec3 constant_wind;
    std::vector<Texture> noise;
};
struct MenuAtmosphere {
    AtmosphericFog atmospheric;
    PlanarFog planar;
    Texture gradient{}; // Rasterizer globals +0x14, bitmap 0 (25E50).
    std::optional<PatchyFog> patchy;
    std::uint32_t gradient_datum=0xffffffffu; // Same authored tag, bitmap 1 for extern 15.
};
struct SkyDefinition {
    std::string name;
    std::uint32_t model{};
    float scale{}, movement{}, yaw{};
    std::optional<Vec3> clear;
    std::optional<AtmosphericFog> atmospheric_fog;
};
struct Bsp { std::uint32_t file_offset{}, size{}, memory_address{}, datum{}; std::string name; };
struct Mesh {
    std::vector<Vertex> vertices; // Expanded triangle list; no Xbox GPU data crosses the renderer boundary.
    Vec3 minimum{}, maximum{};
    std::size_t clusters{}, instances{};
    std::vector<Material> materials;
    std::vector<Texture> lightmaps;
    std::vector<std::string> material_warnings;
    // Resident shader alternatives retain the source geometry's exact
    // material/lightmap pairs even before a view selects those alternatives.
    std::vector<std::array<std::uint32_t,2>> resident_material_lightmaps;
};

struct SourceRuntimeCacheTagImage;
struct SourceRuntimeCacheBspImage;
struct SourceRuntimeCacheAnimationImage;
class Cache {
public:
    using AnimationResourceReader=std::function<std::vector<std::byte>(std::uint32_t,std::uint32_t)>;
    // The same object -> hlmt -> jmad operand used by load_model, without
    // reading unrelated mesh resources merely to select an animation graph.
    std::uint32_t load_model_animation_graph(std::uint32_t datum) const;
    PickupCapsuleDefinition load_pickup_capsule_definition(std::uint32_t biped) const;
    FirstPersonBindingDefinition load_first_person_binding(std::uint32_t weapon,
        std::int32_t representation) const;
    FirstPersonBindingDefinition load_first_person_binding(std::uint32_t weapon,
        std::int32_t representation,const AnimationResourceReader &) const;
    explicit Cache(const std::filesystem::path& path,
                   const std::filesystem::path& shared_directory = {},
                   std::shared_ptr<const OriginalData> gameplay_data = {});
    // Ordered writes applied to this private cache copy before gameplay import.
    std::size_t multiplayer_balance_writes() const { return multiplayer_balance_writes_; }
    std::size_t multiplayer_netgame_writes() const { return multiplayer_netgame_writes_; }
    const std::string& name() const { return name_; }
    const std::string& build() const { return build_; }
    bool is_vista() const { return vista_; }
    bool is_mcc() const { return mcc_; }
    const std::string& scenario() const { return scenario_; }
    std::int16_t load_source_geometry_scenario_type() const; // Literal primaryscnr+10 for2F970.
    const std::filesystem::path& shared_directory() const { return shared_directory_; }
    const std::filesystem::path& path() const { return path_; }
    const std::vector<Tag>& tags() const { return tags_; }
    const std::vector<TagGroupDefinition>& tag_groups() const { return tag_groups_; }
    const TagGroupDefinition* tag_group(TagGroup) const;
    const Tag* find_tag(TagHandle, std::optional<TagGroup> expected = {}) const;
    const Tag* query_tag(TagQuery) const;
    TagRange select_tags(TagQuery query = {}) const { return {tags_, query}; }
    const Tag& tag_at_index(std::size_t) const;
    // Borrowed resident bytes, with source-address conversion in one place.
    std::span<const std::byte> read_tag_bytes(std::uint32_t address, std::size_t size) const;
    std::span<const std::byte> tag_payload(TagHandle) const;
    TagHandle next_tag(TagIterator&) const;
    TagHandle scenario_tag() const { return scenario_tag_; }
    TagHandle globals_tag() const { return globals_tag_; }
    const std::vector<Bsp>& bsps() const { return bsps_; }
    const std::vector<Spawn>& spawns() const { return spawns_; }
    Mesh load_bsp(std::size_t index, bool textures = true) const;
    BspLighting load_bsp_lighting(std::size_t index = 0) const;
    BspSurfaceLighting load_bsp_surface_lighting(std::size_t index = 0) const;
    // Apply the original 10A8B0 scenery identity/checksum admission and
    // 4D300/4DC60 section-indexed baked vertex streams to a placement copy.
    bool apply_scenery_lighting(RenderModel &, const BspLighting &,
                               const ScenarioObject &) const;
    // Single visible-sky BSPs; ambiguous per-cluster sky selection is explicit.
    std::optional<SkyDefinition> load_visible_sky(std::size_t bsp) const;
    std::optional<ObjectSkyLightingDefinition> load_object_sky_lighting(std::size_t ordinal) const;
    // 14B360's availability guard for the selected camera/render sky. This
    // does not choose an object-containing cluster or manufacture a sky.
    bool object_sky_lighting_available(std::size_t scenario_sky) const;
    std::optional<ObjectLightContactDefinition> load_object_light_contact_definition(std::uint32_t object) const;
    ObjectLightRenderLeaves load_object_light_render_leaves(std::uint32_t render_model) const;
    ObjectLightSpatialBsp load_object_light_spatial_bsp(std::size_t bsp=0) const;
    ObjectLightSpatialDefinition load_object_light_spatial_definition(std::uint32_t object) const;
    std::optional<SkyDefinition> load_sky(std::size_t scenario_sky) const;
    // Bounded main-menu fog path; unsupported regional fog is explicit.
    MenuAtmosphere load_menu_atmosphere() const;
    // Authored title desaturation effect, confirmed by the original GPU pass.
    PixelProgram load_menu_scene_filter() const;
    BspAtmosphere load_bsp_atmosphere(std::size_t bsp) const;
    SourcePortalBsp load_source_portal_bsp(std::size_t bsp) const;
    SourcePortalMaskDefinition load_source_portal_mask_definition(std::size_t bsp) const;
    SourceMachinePortalDefinition load_source_machine_portal_definition(std::uint32_t datum) const;
    SourceBspAdmissionBsp load_source_bsp_admission(std::size_t bsp) const;
    SourceTransparentBspParts load_source_transparent_bsp_parts(std::size_t bsp) const;
    SourceTransparentModelParts load_source_transparent_model_parts(
        std::uint32_t render_model,std::span<const unsigned> actual_ordered_sections) const;
    SourceModelPaletteDefinition load_source_model_palette_definition(std::uint32_t render_model) const;
    SourceTransparentPart load_source_transparent_shader(std::uint32_t shader) const;
    std::vector<std::int16_t> load_source_transparent_model_sort_orders(
        std::uint32_t object_or_model,std::int32_t actual_variant_index) const;
    BspWater load_bsp_water(std::size_t bsp) const;
    ObjectLightingDefinition load_object_lighting_definition() const;
    ShadowCasterDefinition load_object_shadow_definition(std::uint32_t object) const;
    WeaponAttachmentDefinition load_weapon_attachment(std::uint32_t unit,
        std::uint32_t weapon,unsigned hand) const;
    SourceMachineDefinition load_source_machine_definition(std::uint32_t object) const;
    // Shared BD090/BDB60/107000 animation channels for mach and ctrl.
    SourceMachineDefinition load_source_device_pose_definition(std::uint32_t object) const;
    SourceControlDefinition load_source_control_definition(std::uint32_t object) const;
    SourceDeviceMotionDefinition load_source_device_motion_definition(std::uint32_t object) const;
    SourcePropDefinition load_source_prop_definition(std::uint32_t object) const;
    SourceWorldWeaponDefinition load_source_world_weapon_definition(std::uint32_t object) const;
    SourceMachinePlacement load_source_machine_placement(unsigned ordinal) const;
    std::optional<SourcePrtDefinition> load_source_prt_definition(std::uint32_t render_model) const;
    SourcePrtLighting load_source_prt_lighting(std::size_t bsp=0) const;
    std::vector<std::uint8_t> source_prt_section_nodes(std::uint32_t render_model,unsigned section) const;
    bool apply_source_prt_vertices(RenderModel &,std::uint32_t render_model,unsigned source_lod=5) const;
    std::optional<std::vector<SourcePrtVertex>> load_source_prt_vertex_stream(
        std::uint32_t render_model,unsigned source_lod,unsigned section) const;
    std::optional<Texture> load_object_surface_texture(std::uint32_t shader) const;
    Material load_material(std::uint32_t shader, bool hud = false) const;
    // Original King2BE670 follows matg+16C/mulg runtime+534 settings pointer
    // and reads settings+C4. No tag-name or variant-name selection.
    std::uint32_t load_multiplayer_hill_shader() const;
    SourceShaderLodDefinition load_source_shader_lod_definition(std::uint32_t shader) const;
    LevelContent load_content(std::size_t index, bool multiplayer = false) const;
    LevelContent load_content(std::size_t index, bool multiplayer, const SourceGameVariant &) const;
    BspContent load_bsp_content(std::size_t index) const;
    ModelCollision load_model_collision(std::uint32_t model) const;
    TerrainCollision load_raw_terrain_collision(std::size_t index) const;
    std::vector<PredictedResource> structure_resources(std::size_t bsp,int section,
                                                       bool definition) const;
    EncodedSound load_sound(std::uint32_t datum, unsigned pitch = 0, unsigned permutation = 0) const;
    // Retail125F10's first-pitch/first-permutation compressed chunk request.
    // Xbox cache only; this performs no PCM decode or playback admission.
    std::optional<std::vector<std::byte>> load_sound_prediction_chunk(std::uint32_t datum) const;
    unsigned sound_pitch_range_count(std::uint32_t datum) const;
    unsigned sound_permutation_count(std::uint32_t datum,unsigned pitch=0) const;
    SoundVariationDefinition load_sound_variation(std::uint32_t datum, unsigned pitch = 0) const;
    // Literal resident records for the source sound request owners. These do
    // not choose a pitch/permutation or initialize scheduler/runtime state.
    std::array<std::byte,20> load_source_sound_definition(std::uint32_t datum) const;
    std::vector<std::array<std::byte,92>> load_source_sound_classes() const;
    ItemPointDefinition load_item_point_definition(std::uint32_t datum) const;
    LoopingSound load_looping_sound(std::uint32_t datum) const;
    MultiplayerAudioBsp load_multiplayer_audio_bsp(std::size_t bsp) const;
    RenderModel load_model(std::uint32_t datum, bool textures = true, std::uint32_t variant = 0) const;
    // Actual model-order bind hierarchy, without geometry/material resources.
    // Cinematic anchors and posed marker/query owners consume this same bank.
    RenderModel load_model_hierarchy(std::uint32_t datum) const;
    RenderModel load_model(std::uint32_t datum, bool textures, std::uint32_t variant,
                           const SourceInitialModelSelection& selected) const;
    // Resident source4C2B0 sections at the explicit authored LOD0..5. Region
    // order and repeated sections are retained; NONE returns empty geometry.
    // The older overloads retain their asset-preview LOD fallback behavior.
    RenderModel load_model(std::uint32_t datum, bool textures, std::uint32_t variant,
                           const SourceInitialModelSelection& selected,
                           unsigned geometry_lod) const;
    // Decode an explicit resident section bank using the same model decoder.
    // Runtime owners choose which prepared sections to submit each frame.
    RenderModel load_model_sections(std::uint32_t datum,bool textures,std::uint32_t variant,
                                   unsigned geometry_lod,std::span<const unsigned> sections)const;
    SourceGeometryLodDefinition load_source_geometry_lod_definition(std::uint32_t object) const;
    SourceInitialModelDefinition load_initial_model_definition(std::uint32_t datum) const;
    SourceInitialModelSelection load_initial_model_selection(std::uint32_t datum, std::uint32_t variant = 0) const;
    std::optional<SourceObjectConstructorMetadata> load_object_constructor_metadata(std::uint32_t datum) const;
    std::optional<SceneryDefinition> load_scenery_definition(std::uint32_t datum) const;
    std::vector<SceneryLightprobe> load_scenery_lightprobes(std::size_t bsp = 0) const;
    FirstPersonIkDefinition load_first_person_ik(std::uint32_t graph,
        std::uint32_t weapon_model, std::uint32_t arms_model,
        std::uint32_t mode=0x030000d9u,
        std::uint32_t weapon_class=0x030000d9u) const;
    std::array<std::uint32_t,2> load_weapon_animation_identifiers(std::uint32_t weapon) const;
    AnimationClip load_unit_weapon_action(std::uint32_t graph, std::uint32_t mode,
        std::uint32_t weapon_class, std::uint32_t weapon_type,
        std::uint32_t action, bool overlay=false) const;
    AnimationClip load_unit_weapon_action(std::uint32_t graph, std::uint32_t mode,
        std::uint32_t weapon_class, std::uint32_t weapon_type,
        std::uint32_t action, bool overlay,const AnimationResourceReader &) const;
    std::array<Vec3,4> load_object_colors(std::uint32_t datum,
                                        std::uint32_t variant = 0) const;
    SourceObjectColorDefinition load_source_object_color_definition(std::uint32_t datum) const;
    std::vector<ObjectFunction> load_object_functions(std::uint32_t datum) const;
    LightVolumeDefinition load_light_volume(std::uint32_t datum) const;
    std::vector<LightVolumeAttachment> load_light_volume_attachments(
        std::uint32_t object, const RenderModel &, float seconds = 0,
        const Transform *world = nullptr) const;
    Texture load_light_volume_default_bitmap() const;
    ParticleSpriteDefinition load_particle_sprite(std::uint32_t datum) const;
    AuthoredMuzzleEffect load_muzzle_effect(std::uint32_t datum) const;
    ParticleSystemRenderDefinition load_particle_system_render_definition(std::uint32_t owner,
        std::uint32_t event,unsigned system) const;
    // Original 2243C0/23EA80: first LOD, explicit layer/pass, style + 7.
    ParticleVertexSelection load_particle_vertex_selection(std::uint32_t datum,
        unsigned layer,unsigned layer_pass) const;
    ParticleMaterial load_particle_material(std::uint32_t datum,unsigned layer,
        unsigned layer_pass,unsigned implementation) const;
    // Classification is supplied by the source draw declaration, not guessed
    // from a shader filename. Empty resident classes return an empty payload.
    std::vector<std::byte> load_vertex_shader_code(std::uint32_t datum,unsigned classification) const;
    SourceVertexDeclaration load_source_vertex_declaration(std::uint32_t datum,unsigned classification) const;
    std::shared_ptr<const ShaderFunctionTables> shader_function_tables() const;
    std::shared_ptr<const OriginalData> original_data() const;
    SourceWeatherDefinition load_source_weather_definition(std::uint32_t datum) const;
    SourceWeatherBsp load_source_weather_bsp(std::size_t index = 0) const;
    Texture load_bitmap(std::uint32_t datum, std::uint32_t index,
                        std::span<const std::byte> palette = {}) const;
    // Actual Vista frontend RFC1950 resources, with tight per-face mip chains.
    // This explicit source path never substitutes an Xbox bitmap donor.
    Texture load_vista_frontend_bitmap(std::uint32_t datum, std::uint32_t index = 0) const;
    Texture load_scope_default_bitmap() const; // 19530/19B29, matg rasterizer +0x64.
    Texture load_effect_light_attenuation_bitmap() const; // 19530/19673, rasterizer +4.
    std::uint32_t bitmap_count(std::uint32_t datum) const;
    std::array<std::uint32_t,18> multiplayer_weapon_choices() const;
    std::uint32_t slayer_starting_weapon(unsigned option = 0,int source_engine = 2) const;
    BitmapResource load_bitmap_resource(std::uint32_t datum, unsigned index) const;
    std::optional<BitmapLowDetailResource> load_bitmap_low_detail_resource(
        std::uint32_t datum,unsigned index=0) const;
    // Separate authored software image; never changes hardware bytes or chooses
    // the current D15E0 cache/fallback result. Unsupported volume decode throws.
    std::optional<Texture> load_bitmap_low_detail(std::uint32_t datum,unsigned index=0) const;
    std::vector<std::vector<std::byte>> load_model_resources(std::uint32_t datum) const;
    std::vector<PredictedResource> model_bitmap_resources(std::uint32_t datum) const;
    std::vector<PredictedResource> object_resources(std::uint32_t datum) const;
    std::vector<std::vector<std::byte>> load_bsp_resources(std::size_t index) const;
    AnimationClip load_animation(std::uint32_t datum, unsigned animation) const;
    // Complete animation lookup can read from an admitted immutable resource
    // owner. Resident metadata and authored fallback order remain this Cache's.
    AnimationClip load_animation(std::uint32_t datum,unsigned animation,const AnimationResourceReader &) const;
    // Literal retained-channel metadata. The packed low signed short selects
    // an inherited graph; the high signed short selects its animation. This
    // imports event records, without decoding resources or choosing a clip.
    UnitAnimationDefinition load_source_unit_animation_definition(
        std::uint32_t graph, std::uint32_t packed) const;
    UnitAnimationGraphMetadata load_source_unit_animation_graph_metadata(
        std::uint32_t graph) const;
    // Literal resident tag payload with original VA words; separate BSP/ltmp
    // mappings and mutable runtime state are not supplied through this API.
    SourceTagMetadata load_source_tag_metadata(std::uint32_t datum) const;
    SourceRuntimeCacheTagImage load_source_runtime_cache_tag_image() const;
    SourceRuntimeCacheBspImage load_source_runtime_cache_bsp_image(std::size_t) const;
    // Actual jmad+AC descriptor and raw local/shared resource bytes. This
    // performs file I/O only; resource pools and residency are separate owners.
    SourceRuntimeCacheAnimationImage load_source_runtime_animation_image(std::uint32_t descriptor) const;
    SourceRuntimeCacheAnimationImage load_source_runtime_animation_image(std::uint32_t descriptor,
        const std::function<std::vector<std::byte>(std::uint32_t,std::uint32_t)> &resources) const;
    std::span<const std::byte> source_runtime_cache_bytes() const { return cache_bytes(); }
    // Literal local/shared file bytes at an encoded source resource offset.
    // The caller supplies the original I/O extent, including sector padding.
    std::vector<std::byte> load_source_runtime_resource(std::uint32_t encoded_offset,std::uint32_t bytes) const;
    // Literal owned BSP allocation, including its 16-byte allocation header.
    // source_address() is the BSP mapping base; the sbsp root is base + 16.
    // This does not admit sbsp/ltmp to the ordinary resident tag provider.
    SourceTagMetadata load_source_bsp_metadata(std::size_t index) const;
    FrontendModelPrimary load_frontend_model_primary_animation(std::uint32_t graph,
                                                              unsigned index) const;
    FrontendModelGrip load_frontend_model_grip_animation(std::uint32_t graph,
                                                       unsigned index) const;
    std::optional<FrontendModelGrip> load_frontend_model_grip(std::uint32_t unit) const;
    std::array<std::uint32_t,8> load_frontend_model_deferred_mask(std::uint32_t unit) const;
    AnimationClip load_named_animation(std::uint32_t datum, std::uint32_t name) const;
    std::optional<unsigned> find_local_animation_index(std::uint32_t graph,std::uint32_t name) const;
    // 1CB0D0's initialized default action, followed by the type6 SCEN
    // 10A3F0 named-idle override. Missing idle/NONE graph or incompatible
    // node count returns null; bad metadata
    // or animation resources throw. This does not select later live actions.
    std::optional<AnimationClip> load_initialized_object_idle(std::uint32_t object) const;
    // Original menu model-widget UI-mode request after its unit's authored
    // starting weapon is equipped. Missing UI action retains combat idle.
    std::optional<AnimationClip> load_frontend_model_animation(std::uint32_t object,
        bool request_ui = true) const;
    // OriginalCCAB0's single authored starting weapon for the supplied menu
    // bipeds. Multi-weapon active-slot production remains unsupported.
    std::optional<std::uint32_t> load_frontend_model_weapon(std::uint32_t unit) const;
    // B7B40/D5B60 initialize a menu biped's object+F0 fraction from the
    // first hlmt damage-info block's maximum shield. No damage tick follows
    // in the bounded main-menu model lifecycle.
    float load_frontend_model_initial_shield_fraction(std::uint32_t unit) const;
    FrontendModelWeaponIk load_frontend_model_weapon_ik(std::uint32_t unit,
        std::uint32_t controller_mode) const;
    FrontendModelWeaponIk load_unit_weapon_ik(std::uint32_t unit,
        std::uint32_t weapon,std::uint32_t controller_mode) const;
    SourceSceneryIdlePhaseDefinition load_source_scenery_idle_phase_definition(const AnimationClip &) const;
    std::uint32_t animation_name(std::uint32_t datum, const std::string &name) const;
    std::optional<std::uint32_t> find_animation_name(std::uint32_t datum,
                                                   const std::string &name) const;
    AnimationClip load_action(std::uint32_t graph, const std::string &mode,
        const std::string &weapon_class, const std::string &weapon_type,
        const std::string &action) const;
    AnimationClip load_action(std::uint32_t graph,const std::string &mode,
        const std::string &weapon_class,const std::string &weapon_type,
        const std::string &action,const AnimationResourceReader &) const;
    std::string string_id(std::uint32_t id) const;
    FrontendContent load_frontend() const;
    HudContent load_hud(std::uint32_t biped, std::uint32_t weapon) const;
    HudContent load_hud_definition(std::uint32_t biped,std::uint32_t hud_definition) const;
    std::vector<std::uint32_t> load_unit_seat_huds(std::uint32_t unit) const;
    FullScoreboardAssets load_full_scoreboard_assets() const;
    std::string describe() const;
private:
    HudContent load_hud(std::uint32_t biped,std::uint32_t weapon,std::uint32_t hud_definition) const;
    RenderModel load_model_with_initial_selection(std::uint32_t datum, bool textures,
        std::uint32_t variant, const SourceInitialModelSelection* selected,
        std::optional<unsigned> geometry_lod = {},
        std::optional<std::span<const unsigned>> sections = {}) const;
    std::filesystem::path path_;
    std::filesystem::path shared_directory_;
    std::vector<std::byte> storage_;
    std::shared_ptr<CacheFileSnapshot> snapshot_;
    std::string name_, build_, scenario_;
    std::vector<Tag> tags_;
    std::vector<TagGroupDefinition> tag_groups_;
    TagHandle scenario_tag_, globals_tag_;
    std::vector<Bsp> bsps_;
    std::vector<Spawn> spawns_;
    std::int64_t metadata_magic_{};
    mutable std::shared_ptr<const ShaderFunctionTables> function_tables_;
    mutable std::shared_ptr<const OriginalData> original_data_;
    std::size_t multiplayer_balance_writes_{};
    std::size_t multiplayer_netgame_writes_{};
    bool vista_{};
    bool mcc_{};
    std::vector<std::uint32_t> mcc_sound_gestalts_;
    std::vector<bool> mcc_native_shaders_;
    bool pc_geometry() const { return vista_ || mcc_; }
    std::size_t source_file_size_{};
    std::vector<std::pair<std::uint32_t,std::uint32_t>> vista_metadata_regions_;
    std::filesystem::path xbox_donor_root_;
    mutable std::vector<std::shared_ptr<const Cache>> vista_texture_donors_;
    std::pair<const Cache*,std::uint32_t> vista_texture_donor(std::uint32_t datum) const;
    std::span<const std::byte> cache_bytes() const;
    std::span<std::byte> mutable_cache_bytes();
    void initialize_vista();
    void initialize_mcc();
    void initialize_mcc_native_shaders();
    BitmapResource load_mcc_bitmap_resource(std::uint32_t,unsigned) const;
    Texture load_mcc_bitmap(std::uint32_t,unsigned,std::span<const std::byte>) const;
    EncodedSound load_mcc_sound(std::uint32_t,unsigned,unsigned) const;
    std::uint32_t mcc_sound_class_tag(bool) const;
    const Tag& mcc_sound_gestalt(std::uint32_t) const;
    void initialize_tag_directory(const Bytes&, std::size_t index, std::int64_t header_magic);
    void validate_tag_directory() const;
    friend struct TagMemory;
    std::size_t metadata_offset(std::uint32_t address, std::size_t size) const;
    TagBlockRange metadata_block(std::size_t descriptor, std::size_t stride, std::size_t limit = 65536) const;
    std::vector<std::size_t> metadata_block_offsets(std::size_t descriptor, std::size_t stride, std::size_t limit = 4096) const;
    std::vector<std::byte> raw_data(std::uint32_t pointer, std::uint32_t size) const;
    BspLighting load_bsp_lighting(const Bytes& metadata, std::int64_t magic) const;
};
} // namespace halo2
