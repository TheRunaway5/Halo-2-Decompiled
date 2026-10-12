#pragma once
#include "halo2/object_light_spatial.hpp"
#include "halo2/posed_collision.hpp"
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
// B67C0 initializes the object data array with literal maximum 0x800.
inline constexpr std::size_t source_object_vector_slot_capacity=2048;
// Literal owner/header inputs for 169510 and its child 168A10 admission.
// Lookup uses the low 16-bit slot; the requested salt is not substituted.
struct SourceObjectVectorOwner {
    std::uint32_t full_handle=0xffffffffu,flags{};
    std::uint8_t header_flags{},header_type{};
    std::uint32_t sibling=0xffffffffu,child_head=0xffffffffu;
    std::uint8_t unit_flags_10a{},biped_flags_348{};
    std::uint32_t prop_flags_12c{};
    ObjectLightSpatialSphere primitive_sphere; // object+40/+4C, not +30/+3C.
};
struct SourceObjectVectorInput {
    // Actual forwarded 1697C0 flags, including its default 1FFF0 type mask.
    // This routine does not normalize a caller's original query flags.
    std::uint32_t flags{},lower_flags{}; // 169510 parameters 3 and 4, separately.
    std::uint32_t excluded_a=0xffffffffu,excluded_b=0xffffffffu;
    Vec3 origin,delta;
};
struct SourceObjectVectorContact {std::uint32_t object=0xffffffffu;PosedCollisionHit target;};
struct SourceObjectVectorResult {
    std::optional<SourceObjectVectorContact> contact;
    std::vector<std::uint32_t> targets; // Admitted source 1EFB40 target order.
};
// Exact 168A10 direct live-field admission. Unlike stored primitive filtering,
// it reads the header type and actual current object flags for descendants.
bool source_object_vector_child_admitted(std::uint32_t requested_handle,
    const SourceObjectVectorOwner&,const SourceObjectVectorInput&);
// The target service owns 1EFB40/1EFF40 and the 168B40 world-plane conversion.
// It returns the complete posed contact packet in world coordinates.
// It must query the whole source segment independently of the shared fraction,
// using actual collision metadata, WORLD nodes and active region state. NONE
// means missing model/collision or a source geometry miss. No inferred pose.
using SourceObjectVectorTarget=std::function<std::optional<PosedCollisionHit>(
    std::uint32_t requested_handle,const SourceObjectVectorOwner&,
    const SourceObjectVectorInput&)>;
// Original 169510: first_admitted bypasses direct admission for every sibling
// in the initial chain; descendant chains use 168A10 and 11E5E0. Child-before-
// sibling order, strict closer replacement and flag 10000000 exit are literal.
// maximum_fraction is the actual current shared packet fraction. The caller
// retains that packet unless this result supplies a strictly closer target.
SourceObjectVectorResult source_object_vector_contact(
    std::span<const SourceObjectVectorOwner>,std::uint32_t first,
    bool first_admitted,const SourceObjectVectorInput&,float maximum_fraction,
    const SourceObjectVectorTarget&);
}
