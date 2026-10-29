#pragma once
#include "halo2/model_skin_submission.hpp"

namespace halo2::path_tracing {
struct MaterialRange {std::uint32_t first{},count{};};
struct Scene;
// Runtime-owned full-body provenance, copied alongside the completed native
// WORLD geometry. Carried weapons have their own material/vertex banks and
// never acquire a body owner. This metadata does not select or generate pose.
struct SecondaryBodyOwner {
    std::uint32_t root=~0u,definition=~0u,render_model=~0u,source_tick{};
    unsigned lod{};
    MaterialRange materials;
    std::uint32_t first_vertex{},vertex_count{};
    std::vector<unsigned> sections;
};
struct SecondaryBodyCount {
    std::uint32_t root{},definition{},render_model{},source_tick{};
    unsigned lod{};
    std::uint64_t triangles{},eligible_triangles{};
};
// Count only final BVH triangles matching the designated body's actual bank,
// material and ordered positions. Reordering the BVH cannot change ownership;
// filtering, weapons and primary partial-body leftovers cannot inflate it.
std::vector<SecondaryBodyCount> secondary_body_counts(const Scene&,
    std::span<const Vertex> native_bank,std::span<const SecondaryBodyOwner>,
    std::span<const Material>,std::span<const MaterialRange> replaced);
// Append complete WORLD triangles for secondary rays. Explicit full-body
// owners can replace the ordinary/first-person body ranges without changing
// primary visibility. Material identity, alpha coverage and pose stay authored.
void append_secondary_geometry(std::vector<Vertex>&,std::span<const Vertex>,
    std::span<const Material>,std::span<const MaterialRange> replaced={});
// Own a WORLD-space copy for CPU BVH construction. GPU-deferred ranges use
// their actual current palettes and the shared native skin/FP placement graph;
// the caller's source bank and immutable submissions remain unchanged.
std::vector<Vertex> posed_vertices(std::span<const Vertex>,
    std::span<const ModelSkinSubmission>);
}
