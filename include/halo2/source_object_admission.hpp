#pragma once
#include "halo2/object_light_spatial.hpp"
#include "halo2/source_bsp_admission.hpp"

namespace halo2 {
struct SourceObjectSpatialPool;
inline constexpr std::uint32_t source_object_main_queue_capacity=256;
// Actual live object-bank fields read by133050. The rendering sphere is the
// retained object+30/+3C cache; registration uses the separate +40/+4C sphere.
struct SourceObjectAdmissionObject {
    std::uint32_t full_handle=0xffffffffu,flags{}; // object+4; association gate31
    std::int8_t type{}; // object+AA, not an inferred material category
    ObjectLightSpatialSphere rendering_sphere;
};
struct SourceObjectAdmissionInput {
    std::uint32_t capacity=source_object_main_queue_capacity;
    // Verified ordinary main-mode0, context+2A78=0, auxNULL only. Optional
    // flags40(skip objects),80(force admission),200(plane),400(machine filter)
    // are literal source inputs. Flag10 affects separate light-volume services.
    std::uint32_t flags{};
    ShaderVector clipping_plane{};
    std::span<const std::uint8_t> containing_fog;
    // Explicit132220 sky-owner cluster, if its separate availability/map
    // producer has established one. NONE disables that branch. Unsupported
    // negative-index association writes are rejected rather than inferred.
    std::int16_t sky_cluster=-1;
};
struct SourceObjectAdmission {
    std::vector<SourceBspAdmissionRecord> objects;
    std::vector<std::array<std::uint32_t,4>> associations;
    bool association_capacity_reached{};
    const SourceBspAdmissionRecord *object(std::uint32_t full_handle) const;
};
// Exact ordinary-object132220→133050→132E60/165010/181800/134300/163C80.
// The two lists retain actual successful source registration chronology and
// full handles. This pass uses a fresh4DE2FC visited epoch, as1320F0 does after
// BSP134390, while preserving its association bank. Unregistered objects are
// absent. Light-volume132A30/C3950 services, aux163D20, object creation/location
// and allocator-pressure producers are outside this seam.
SourceObjectAdmission source_object_admission(
    const SourcePortalVisibilitySet&,const ObjectLightSpatialLists &collidable,
    const ObjectLightSpatialLists &noncollidable,
    std::span<const SourceObjectAdmissionObject>,const SourceObjectAdmissionInput&,
    std::span<const std::array<std::uint32_t,4>> prior_associations={});
// The same source traversal over complete independent data-array pools.
// Retains actual cluster head order and orphan entries after a reference
// allocation failure; no successful-only list reconstruction occurs.
SourceObjectAdmission source_object_admission(
    const SourcePortalVisibilitySet&,const SourceObjectSpatialPool &collidable,
    const SourceObjectSpatialPool &noncollidable,
    std::span<const SourceObjectAdmissionObject>,const SourceObjectAdmissionInput&,
    std::span<const std::array<std::uint32_t,4>> prior_associations={});
}
