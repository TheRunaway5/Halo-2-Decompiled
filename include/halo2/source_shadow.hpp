#pragma once
#include "halo2/cache.hpp"
#include "halo2/transform.hpp"

namespace halo2 {
struct Transform;
struct ShadowCasterDefinition {
    std::string name;
    float bounding_radius{};
    Vec3 bounding_offset;
    std::uint16_t mode{}; // Source object definition +0x18: 0 or 2 admits a caster.
    std::uint32_t model=0xffffffffu;
    bool model_present{};
    float fade_end{}, fade_start{}; // Root model +0x28/+0x2C, 4BAF0.
    std::array<float,5> render_cutoffs{}; // Model +0x34..+0x44.
    std::uint16_t render_lod_count{}; // Model +0x4C.
};
struct ShadowCasterSphere {Vec3 center;float radius{};};
// BDEF0's regular branch for player/weapon types0/1/2. The source node0
// world matrix and actual object+A0 scale are separate inputs. Type11's
// optional physics override and node/attachment production stay with callers.
ShadowCasterSphere source_shadow_sphere(const ShadowCasterDefinition &,
                                        const Transform &node0,float object_scale);
// BDC40 constructs the object frame before BD090 composes the animated
// node0. Attached objects retain their own scale; the parent's scale affects
// the attachment translation. Both reflection flags are actual caller state.
Transform source_object_transform(Vec3 position,Vec3 forward,Vec3 up,
    float object_scale,bool mirrored,const Transform *parent_node=nullptr,
    bool parent_mirrored=false);
struct ObjectAttachmentMarker {
    std::int8_t region=-1,permutation=-1;
    std::uint8_t node{};
    Transform local;
    float authored_scale{}; // mode marker+20, retained separately by 1D8F50.
};
struct WeaponAttachmentDefinition {
    std::uint32_t parent_marker{},child_marker{},preferred_child_marker{};
    std::vector<ObjectAttachmentMarker> parent,child,preferred_child;
};
struct WeaponAttachmentMarkers {ObjectAttachmentMarker parent,child;bool named_child{};};
// D0870/B8D30/1D8F50 select the first marker admitted by the actual runtime
// region permutations. Empty permutation spans reproduce the source's null
// filter; callers with a live variant supply its original permutation bank.
// A missing group uses B8D30's authored identity marker at node0.
WeaponAttachmentMarkers select_weapon_attachment(const WeaponAttachmentDefinition &,
    std::span<const std::uint8_t> parent_permutations,
    std::span<const std::uint8_t> child_permutations);
struct ObjectAttachmentFrame {Vec3 position,forward,up;};
// B8EE0/B92D0 followed by B91D0. The parent marker is its actual world frame.
// A named child supplies its actual animated world marker; an unnamed child
// supplies B8D30's identity local marker. Current orientation supplies the
// degenerate normalization fallback. No first-person offset participates.
ObjectAttachmentFrame source_object_attachment_frame(ObjectAttachmentFrame current_child,
    const Transform &parent_marker_world,const Transform &child_marker,bool named_child);
// B93B0 stores an attached object's position/forward/up relative to the
// actual parent node, after marker alignment and before BD090 composes its
// animated pose. The degenerate fallback remains the child's current world
// orientation, as in B91D0; no animated weapon node is subtracted here.
ObjectAttachmentFrame source_object_attachment_local_frame(ObjectAttachmentFrame world_child,
    const Transform &parent_node);
struct SourceShadowFrame {
    Vec3 center,direction,up,left;
    Vec3 virtual_eye,volume_center,volume_half_extent;
    float transverse_radius{},axial_radius{},width{},near_offset{},far_offset{};
    float virtual_distance{},cone_angle{},opacity{};
    // Camera-space clip distances include the original virtual light origin.
    float near_distance()const{return near_offset+virtual_distance;}
    float far_distance()const{return far_offset+virtual_distance;}
    std::array<float,29> source_values()const;
};
struct SourceShadowUniforms {
    // 3A550 submits c31..39; the fixed object-shadow programs consume c31..36.
    std::array<ShaderVector,6> projection;
    // 48EE0's separate nine-vector submission starts at c40.
    std::array<ShaderVector,9> constants;
    unsigned texture_width=128,texture_height=128;
    float opacity{}; // Original pixel scalar extern 17 (32350 +3290A).
};
// Fixed object-shadow definition 47FDA0, mask 0x14. This is not the dynamic
// light producer, whose adaptive dimensions and authored definition differ.
// Feature bit 10 controls the original filter state; it is a caller input.
SourceShadowUniforms source_shadow_uniforms(const SourceShadowFrame &,
                                           bool shadow_depth_feature);
enum class SourceShadowPhase {Generate,Apply};
struct SourceShadowMaterial {
    Material material;
    // Renderer-provided original target indices, zero for resident bitmaps.
    std::array<unsigned,4> external_targets{};
};
// 15D70/30CD0/30D10 for the fixed object-shadow mask 0x14 only. A missing
// layer or source-suppressed generation returns an empty vector. Unsupported
// target/implementation inputs throw instead of substituting a different LOD.
std::vector<SourceShadowMaterial> prepare_shadow_material(const Cache &,
    const Material &authored,SourceShadowPhase,bool model_geometry,
    bool geometry_shadow_modifier,float opacity);
// 4BAF0's second output byte, multiplied by the source 1/255 constant.
// Supply the source root model definition for attached objects. The caller's
// original draw metric already includes the observer's distance multiplier.
float source_shadow_distance_fade(const ShadowCasterDefinition &,float draw_metric,
                                  float current_radius,bool force_highest_lod);
std::uint8_t source_shadow_distance_fade_byte(const ShadowCasterDefinition &,
    float draw_metric,float current_radius,bool force_highest_lod);
struct SourceShadowRootFadeInput {
    std::uint16_t definition_mode;
    std::uint8_t object_type,header_kind,model_fade;
    bool no_shadow,dying;
    float seconds_since_death,camouflage;
};
struct SourceShadowRootFade {bool eligible;float fade;};
// 3D7F0 for an unattached root. Parent fallback/recursive attachment handling
// stays with the caller. Supply the actual object/header/death/camo state.
SourceShadowRootFade source_shadow_root_fade(SourceShadowRootFadeInput);
struct SourceShadowCandidate {
    // Four original flag bytes. Identity/owner/PVS/coverage are caller inputs.
    std::uint32_t flags{},identity{};
    std::int32_t owner=-1;
    float requested_fade{},coverage{};
};
struct SourceShadowSelectionEntry {
    SourceShadowCandidate candidate;
    float slope{},fade{};
    std::int32_t last_seen_frame{},created_frame{};
};
struct SourceShadowSelectionCache {
    std::array<SourceShadowSelectionEntry,32> entries{};
    unsigned count{};
};
// Original4A7C0/4AC30/4B160. Returned records include zero-opacity entries.
// Object shadows use preferred budget4, acceleration.005; dynamic lights
// use budget3, acceleration.01 with their separately retained cache.
std::vector<SourceShadowCandidate> source_shadow_select(SourceShadowSelectionCache &,
    std::span<const SourceShadowCandidate>,std::int32_t frame,unsigned preferred_budget,
    float fade_acceleration,bool cinematic_bypass);
// 33400/31910 projected diameter. The plane is the original inverse-view
// matrix's depth row; pixel_scale is (2FB70 vertical_scale * canonical outer
// viewport height) *.5, written by 2FD90 at output+BC (global4B9ED0).
float source_shadow_coverage(Vec3 center,float current_radius,
                             ShaderVector depth_plane,float pixel_scale);
// 31F50/32350/ C1930/C1A80. Spheres must contain the actual root/child/sibling
// hierarchy in original traversal order, with current transformed centers and
// radii. Opacity/fade are source runtime values, including camouflage/death/LOD.
// Eligibility/PVS and source selection inputs remain the caller's responsibility.
std::optional<SourceShadowFrame> source_shadow_frame(ShadowCasterSphere root,
    std::span<const ShadowCasterSphere> hierarchy,Vec3 shadow_direction,
    float cached_opacity,float runtime_fade);
}
