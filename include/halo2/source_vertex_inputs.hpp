#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_vertex_declaration.hpp"

namespace halo2 {
SourceVertexDeclaration decode_source_vertex_declaration(
    const Bytes&,std::size_t root,std::int64_t metadata_magic,unsigned classification);
// Decoded world-space streams, after the already-proved geometry/placement
// transformations. UVs are already decompressed (the matching c172 is1,1,0,0).
// This does not replace local, boned or skinned declarations/pose producers.
std::array<ShaderVector,16> source_world_vertex_inputs(
    const SourceVertexDeclaration&, const Vertex&,
    const std::array<ShaderVector,16>& constant_attributes,
    SourceWorldVertexInputTransform=SourceWorldVertexInputTransform::Preserve);
// Immediate1AB0 overrides register11 after declaration-selected position/UV
// inputs. This is selected explicitly by the submitting source draw owner.
std::array<ShaderVector,16> source_immediate_color11_vertex_inputs(
    const SourceVertexDeclaration&,const Vertex&,
    const std::array<ShaderVector,16>& constant_attributes);
} // namespace halo2
