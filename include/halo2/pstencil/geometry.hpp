#pragma once
#include "halo2/menu_world_source_binding.hpp"
#include <vector>

namespace halo2::pstencil {
struct ShadowVertex {
    Vec3 position;
    float extruded{};
    std::uint32_t stipple_level=16;
};
static_assert(sizeof(ShadowVertex)==20);
// Current raw indices and current palette, before camera projection. The
// adapter executes the authored program's actual position/skin prefix. It
// neither chooses a pose nor welds unrelated source vertices.
std::vector<Vec3> world_positions(const MenuWorldSourceBinding&);
// The ordinary model draw can contain triangles omitted by Apply's spatial
// range filter. Its same immutable section/current native palette still owns
// every original part vertex. This computes that bank without changing the
// binding's actual submitted range or constructing additional draw packets.
std::vector<Vec3> complete_world_positions(const MenuWorldSourceBinding&);
// Offline arithmetic-reference boundary reconstruction; test-only linkage.
VertexProgram position_prefix(const VertexProgram&);
// Complete source identity selects a fixed AOT C++ prefix function. This is
// the production CPU-side position conduit; it never executes program words.
void execute_native_position_prefix(const VertexProgram&,VertexProgramState&);
// Current raw geometry lowered to the existing world-class material conduit.
// Basis skinning retains the same descending influence/rounded arithmetic as
// the source model renderer, with the source program's exact posed positions.
std::vector<Vertex> posed_triangles(const MenuWorldSourceBinding&,unsigned material);
// Closed triangle prisms use the 2003 doubled-point convention and capped
// Carmack counting. Shared internal edges cancel in stencil rather than
// requiring 2003-only precomputed ISQ adjacency in retail map resources.
// This changes the amount of submitted geometry, not the shadow surface.
void append_volume(std::vector<ShadowVertex>&,std::span<const Vec3>,
    std::span<const std::array<std::uint16_t,3>>,Vec3 light,unsigned stipple_level=16);
void append_volume(std::vector<ShadowVertex>&,const MenuWorldSourceBinding&,Vec3 light,unsigned stipple_level=16);
} // namespace halo2::pstencil
