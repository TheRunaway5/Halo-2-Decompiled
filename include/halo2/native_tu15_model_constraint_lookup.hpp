#pragma once
#include "halo2/native_tu15_body_constraint_update.hpp"
namespace halo2 {
struct NativeTu15ModelConstraintLookupStats {
    std::uint64_t calls{},candidates{},key_matches{},pose_queries{},body_visibility_queries{},found{};
};
// Sole general22E0D0 integer visibility predicate; also used by1DD710.
bool native_tu15_model_constraint_bodies_visible(const SourcePhysicsContactMemory&,
    std::uint32_t first_component,std::uint32_t second_component,
    NativeTu15ModelConstraintLookupStats* = nullptr);
// Actual1DD710 with22DEF0/22E040/22E000/22E0D0. Operates on real request DATA
// and existing instance lists; never creates rows or receives CPU postimages.
std::uint32_t native_tu15_find_model_constraint(const SourcePhysicsContactMemory&,
    NativeTu15PhysicsComponent,NativeTu15ModelConstraintRequest,std::uint32_t flags,
    NativeTu15PhysicsFrameFp&,NativeTu15ModelConstraintLookupStats* = nullptr);
} // namespace halo2
