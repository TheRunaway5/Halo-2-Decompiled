#pragma once
#include "halo2/source_control.hpp"

namespace halo2 {
struct ObjectLightSpatialBsp;
struct SourceObjectSpatialPool;
struct SourceObjectLocation;
struct SourceObjectSpatialQueryState;
// The first eight bytes of original151110's action record. 151320 compares
// action as a SIGNED short and only copies these two words on replacement.
struct SourcePlayerActionTarget {
    std::int16_t action{}, index{};
    std::uint32_t object=0xffffffffu;
    bool operator==(const SourcePlayerActionTarget &)const=default;
};
// These are independent source fields/services, not a camera adapter.
// CAFC0 includes animation/observer and world-contact corrections; callers
// must supply its actual result. The ray is the actual unit+168 vector.
struct SourceControlTargetQuery {
    std::uint32_t unit_parent=0xffffffffu;
    Vec3 unit_bound_center;
    float unit_bound_radius{};
    Vec3 unit_center, unit_forward, unit_root;
};
struct SourceControlTargetCandidate {
    std::uint32_t object=0xffffffffu;
    Vec3 bound_center;
    float bound_radius{};
    Vec3 root;
    const SourceDeviceGroup *position{}, *power{};
    std::uint32_t device_flags{}, control_flags{}; // object+12C/+1CC.
    // Actual B8D30(name0500008F, maximum1) result, after region/node posing.
    std::uint16_t marker_count{};
    Vec3 marker_forward;
};
//151110's actual BB050 call, including parent rejection, mask, capacity and
// radius operand order. Location and independent pools must come from the
// current registered source owners; returned objects still need their typed
// action branches and actual bound/center/marker services below.
std::vector<std::uint32_t> source_control_spatial_candidates(
    const SourceControlTargetQuery &,const SourceObjectLocation &,
    const ObjectLightSpatialBsp &,const SourceObjectSpatialPool &collidable,
    const SourceObjectSpatialPool &noncollidable,SourceObjectSpatialQueryState &);
// Complete scalar gates of107870 and1078F0. A present position group requires
// an actual power group; NONE is not replaced by a fabricated powered state.
bool source_device_usable(const SourceDeviceGroup *position,
                          const SourceDeviceGroup *power,std::uint32_t flags);
bool source_control_side_allowed(bool typed_control,std::uint32_t flags,
                                 std::uint16_t marker_count,Vec3 marker_forward,
                                 Vec3 unit_forward);
//11B980 initializes control+1CC from placement+3C bits0/4, after allocation.
std::uint32_t source_control_initial_flags(std::uint32_t placement_flags);
//151320, including its distinct operand order for the two squared distances.
// Root positions are actual B9DD0 results, with parent transforms resolved.
bool source_player_action_target_replace(SourcePlayerActionTarget &current,
    const SourcePlayerActionTarget &candidate,Vec3 unit_root,
    Vec3 current_root,Vec3 candidate_root);
//1516C0 for an already admitted candidate. This omits151110's parent/bounds
// filters; use the outer function below when applying those filters as well.
bool source_control_action_candidate(const SourceControlTargetQuery &,
    const SourceControlTargetCandidate &,SourcePlayerActionTarget &current,
    Vec3 current_root);
//151110's control bound gate followed by complete1516C0's scalar decisions.
// The caller must first enumerate an actual BB050 candidate (FFFFEBCF,
// capacity64), preserving that enumeration order. This function does not
// manufacture broadphase membership, marker transforms, or unit-center data.
bool source_control_target_candidate(const SourceControlTargetQuery &,
    const SourceControlTargetCandidate &,SourcePlayerActionTarget &current,
    Vec3 current_root);
}
