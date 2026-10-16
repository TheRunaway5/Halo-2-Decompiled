#pragma once
#include "halo2/particle_vertex.hpp"
#include "halo2/physics_contact_record.hpp"
#include <array>
namespace halo2 {
struct SourceParticleDeclarationTu15 {
    std::uint32_t vertex_datum{},classification{},input_mask{};
    std::array<std::array<std::uint32_t,4>,16> elements{};
    bool operator==(const SourceParticleDeclarationTu15 &) const = default;
};
// Whole original1C2C0/1C690/1C310 with the actual tag/classification and the
// packed world-particle declaration selected by original247F7F. This produces
// input metadata only; no Xbox vertex buffer, device or boot history is supplied.
SourceParticleDeclarationTu15 source_particle_declaration_tu15(
    SourcePhysicsContactMemory &,const ParticleVertexSelection &);
}
