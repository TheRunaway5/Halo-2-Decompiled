#pragma once
#include "halo2/cache.hpp"
#include <functional>
namespace halo2 {
// Original BF9A0 admission used by B7B40 to initialize object flag bit31.
// A collision tag alone is insufficient: the render model must carry both
// PRT data and its authored collision-to-render leaf maps.
struct ObjectLightContactDefinition {
    std::uint32_t datum=0xffffffffu,model=0xffffffffu;
    std::uint32_t render_model=0xffffffffu,collision=0xffffffffu;
    std::uint32_t prt_count{},section_render_leaves_count{};
    std::int16_t type=-1,scenery_policy=-1;
    bool model_attribution_available() const;
};
enum class ObjectLightContactAction {reject_scenery=0,attribute_model=1,fallback_map=2};
// 14AF40 after an admitted object collision. A rejected scenery contact may
// retry only under the authored double-sided surface flag; fallback_map uses
// source flags 0x04800005 and does not retry a generic host triangle.
ObjectLightContactAction source_object_light_contact_action(std::int16_t object_type,
    std::int16_t scenery_policy,bool model_attribution_available);
// Exact input bytes for retail16B430. The three type-specific fields retain
// their source offsets because their runtime producers are separate systems.
struct ObjectLightSpatialState {
    std::uint32_t object_flags{},type11_flags_12c{};
    std::uint8_t handle_flags{},object_type{},unit_flags_10a{},biped_flags_348{};
};
std::uint16_t source_object_light_spatial_flags(const ObjectLightSpatialState&);
std::uint16_t source_object_light_collision_exclusion_mask(std::uint32_t query_flags);
// Retail1697C0's stored spatial-type/flags filter, before source11E5E0's
// segment/sphere test and the posed target query. Input ordering stays explicit.
bool source_object_light_spatial_contact_admitted(std::uint32_t query_flags,
    std::uint8_t spatial_type,std::uint16_t spatial_flags,std::uint32_t object,
    std::uint32_t excluded_a=0xffffffffu,std::uint32_t excluded_b=0xffffffffu);
// Authored mode+7C maps in geometry NodeMap order. These do not establish a
// runtime collision owner, active permutation or current object node pose.
struct ObjectLightRenderLeaves {
    struct Reference {
        std::uint16_t strip{},lightmap_triangle{};
        std::int32_t collision_node=-1;
    };
    struct Leaf {
        std::int16_t cluster=-1,count{};
        std::int32_t first{};
    };
    struct NodeMap {std::vector<Leaf> leaves;std::vector<Reference> references;};
    struct Section {std::vector<std::uint8_t> nodes;std::vector<NodeMap> maps;};
    std::uint32_t render_model=0xffffffffu;
    std::vector<Section> sections;
};
struct ObjectLightRenderReference {
    std::int32_t index{};
    std::uint16_t strip{},lightmap_triangle{};
};
// The exact ordered reference scan from 14A8E0. `accept_triangle` is its
// 23AC30/11E800 service: the caller must supply the original current model
// coordinate transform and decoded render positions, not a collision fan.
// NONE references reject immediately, even if later references would match.
std::optional<ObjectLightRenderReference> source_object_light_render_reference(
    const ObjectLightRenderLeaves::Section&,std::int16_t collision_node,
    std::uint32_t collision_leaf_count,std::uint32_t collision_leaf,
    std::uint32_t collision_plane,std::span<const std::uint16_t> node_planes,
    const std::function<bool(const ObjectLightRenderReference&)>& accept_triangle);
} // namespace halo2
