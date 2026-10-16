#pragma once
#include "halo2/atmosphere.hpp"
#include "halo2/bloom.hpp"
namespace halo2 {
struct BspPostprocessState {
    std::array<PatchyFogState,4> motion;
    std::uint32_t random{};
};
struct BspPatchyDefinition {
    std::uint32_t datum=0xffffffffu;
    PatchyFog fog;
    std::array<float,2> wind_velocity{},wind_period{};
    float wind_acceleration{},wind_perpendicular{};
};
struct BspPatchyFrame {
    PatchyFogFrame shading;
    std::array<unsigned,4> bitmaps{};
    std::array<float,4> interpolation{};
    float depth_scale=16777215.f;
    bool quantized_composite{};
    // 3A5C0 -> 27520 -> 350E0: integer W of the direct fog quad.
    // Zero preserves the original disabled-Z/fallback branch.
    float composite_minimum_depth{};
};
struct BspBloomFrame {
    MenuBloom programs;
    // Gamma is the aggregate sky state; 13870 itself consumes only the
    // threshold and strength. Keep zero when no sky bloom override exists.
    float threshold{},strength{},gamma{};
};
// The retail fog state is camera/cluster dependent. Keep it independent of
// renderer ownership so every local-player viewport queries its own cluster.
struct BspFogSample {
    int cluster = -1, visible_sky = -1;
    unsigned classification{};
    AtmosphericFog primary{}, secondary{}, sky_fog{};
    PlanarFog planar{};
    std::uint32_t planar_datum = 0xffffffffu;
    unsigned planar_flags{};
    std::uint32_t patchy_datum=0xffffffffu;
    float eye_height{}, eye_fraction{}, eye_offset{}, transition = 1, interpolation{};
    Vec3 sky_color_premultiplied{};
    float sky_opacity{};
    bool planar_only{};
    std::optional<Vec3> clear;
    std::optional<PatchyFog> patchy;
    std::array<float,4> bloom{};
};
struct BspAtmosphere {
    struct Node { unsigned plane{}, negative{}, positive{}; };
    struct Cluster {
        int sky = -1, visible_sky = -1, atmospheric = -1;
        int planar_designator = -1, visible_plane = -1;
        std::vector<unsigned> portals;
        unsigned flags{};
    };
    struct Portal { int back = -1, front = -1; unsigned plane{}; Vec3 center; float radius{}; unsigned flags{}; };
    struct Sky {
        unsigned flags{};
        float spread{};
        AtmosphericFog primary{}, secondary{}, sky_fog{};
        std::optional<Vec3> clear;
        std::optional<PatchyFog> patchy;
        std::uint32_t patchy_datum=0xffffffffu;
        std::array<float,4> bloom{};
        // Scenario sky ordinals may contain a NONE tag reference. Preserve
        // those slots without treating them as present, empty-data skies.
        bool present=true;
        // Independent14B360 AL control, separate from flag32's RGB override.
        bool model_present{};
    };
    // Authored scnr+340 rows, not sky tags. TU1.5 131AC0 selects this
    // branch before sky aggregation; 131610 owns live mixer preprocessing.
    struct AtmosphericPalette {
        float spread{};
        AtmosphericFog primary{}, secondary{};
        std::array<float,6> planar_override{}; // RGB, density, bias, amount.
        std::optional<PatchyFog> patchy;
        Vec3 patchy_secondary{};
        std::uint32_t patchy_datum=0xffffffffu;
        std::array<float,4> bloom{};
        unsigned mixer_count{}, immersion_flags{};
    };
    struct FogPlane { int palette = -1; ShaderVector plane{}; };
    struct FogPalette {
        std::uint32_t datum = 0xffffffffu;
        unsigned flags{};
        PlanarFog fog{};
        std::array<float,2> transition{};
        std::optional<PatchyFog> patchy;
        float patchy_minimum_depth{};
        std::uint32_t patchy_datum=0xffffffffu;
    };
    std::vector<Node> nodes;
    std::vector<ShaderVector> planes;
    std::vector<int> leaves;
    std::vector<Cluster> clusters;
    // 16DE20/16E150: row-major potential and closed/base bitsets. Runtime
    // portal/frustum clipping can only remove entries from potential PVS.
    std::vector<std::uint32_t> cluster_pvs,cluster_closed_pvs;
    // 134390 ORs fog admission over the visible clusters that reference an
    // instance. The current all-geometry path supplies every authored member.
    std::vector<std::vector<unsigned>> instance_membership;
    std::vector<Portal> portals;
    std::vector<Sky> skies;
    std::vector<AtmosphericPalette> atmospheric_palette;
    std::vector<FogPlane> fog_planes;
    std::vector<FogPalette> fog_palette;
    std::vector<BspPatchyDefinition> patchy_definitions;
    unsigned scenario_flags{};
    int default_sky = -1;
    Texture gradient;
    std::uint32_t gradient_datum = 0xffffffffu;
    struct CameraLocation { int leaf = -1, cluster = -1; };
    // Literal14A280 root query and leaf->cluster mapping. This exposes no
    // retained-location policy; the2B720 caller owns that separate history.
    CameraLocation location_at(Vec3 eye) const;
    int cluster_at(Vec3 eye) const;
    bool contains_planar(int cluster, std::uint32_t datum) const;
    // Negative authored instance encoding is -2-instance_index. Geometry
    // with no authored cluster (-1) remains admitted by this coarse filter.
    bool geometry_visible(int cluster,std::span<const std::uint32_t> visible) const;
    bool contains_planar(int cluster,std::uint32_t datum,
        std::span<const std::uint32_t> visible) const;
    // Source-exact conservative upper bound before camera portal clipping.
    std::vector<std::uint32_t> potentially_visible_clusters(int camera_cluster) const;
    // Exact 16DE20 dynamic-door PVS when the original open-portal mask is
    // available. An empty mask means all dynamic doors are closed.
    std::vector<std::uint32_t> visible_clusters(int camera_cluster,
        std::span<const std::uint32_t> open_portals) const;
    const BspPatchyDefinition *patchy_definition(std::uint32_t datum) const;
    // Explicit cluster is for an independently determined camera leaf and
    // differential fixtures; omission performs the original BSP3D traversal.
    BspFogSample sample(Vec3 eye, Vec3 forward, std::optional<int> cluster = {}) const;
};
// Original 25A10, 336F0/33980 and 25E50 inputs after 130DE0 classification.
std::array<ShaderVector,6> bsp_fog_constants(const BspFogSample &, Vec3 eye, Vec3 forward);
MenuFogExterns bsp_fog_externs(const BspFogSample &);
MenuFogComposite bsp_fog_composite(const BspFogSample &, float far_clip);
struct SourceFogDrawControls { bool suppressed{},force_atmospheric{}; };
enum class SourceFogDrawKind { Atmospheric, Sky };
// Whole TU1.5 25D30 selection before its GPU submission/state cleanup.
std::optional<SourceFogDrawKind> source_fog_draw_selection(unsigned classification,
    SourceFogDrawControls = {});
std::optional<MenuFogComposite> bsp_fog_draw(const BspFogSample &,float far_clip,
    SourceFogDrawControls = {});

void prepare_bsp_planar_fog(const Cache &, Mesh &, const BspAtmosphere &);
std::optional<unsigned> bsp_planar_fog_implementation(const BspFogSample &, bool containing_geometry);
PixelProgram bsp_planar_fog_program(const MaterialPass &, const BspFogSample &);
void apply_bsp_fog_pixel_externs(PixelProgram &, const MaterialPass &,
    const BspFogSample &, std::span<const std::int32_t> switches = {});
BspPatchyFrame bsp_patchy_fog_frame(const BspPatchyDefinition &,const PatchyFog &sample,
    PatchyFogState &,std::uint32_t &random_state,const Transform &world_to_view,
    float forward_z,float vertical_fov,float height_over_width,float time,float far_clip=1024.f,
    float near_clip=.015625f);
// 137A0/13870: initial cinematic amount/brightness are both 0.5 (22590).
// Sky aggregation supplies the blend amount, threshold, brightness and gamma.
BspBloomFrame bsp_bloom_frame(const BspFogSample &,float cinematic_amount=.5f,
    float cinematic_brightness=.5f,float screen_effect_threshold_factor=1.f);
} // namespace halo2
