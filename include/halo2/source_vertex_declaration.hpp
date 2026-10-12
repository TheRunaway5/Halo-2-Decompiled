#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace halo2 {
// Literal vrtx class+8 semantic IDs. Original1C330 assigns a vertex register
// by ordinal in this list; a shader's name does not determine that register.
enum class SourceVertexSemantic : std::uint16_t {
    Position=0, UV=3, Normal=4, Tangent=5, Binormal=6, Incident=8,
    LightmapUV=9, Radiance=14, PrtCluster=19, PrtWeights=20
};
struct SourceVertexDeclaration {
    unsigned class_index{};
    std::vector<std::uint16_t> semantics;
    bool operator==(const SourceVertexDeclaration&) const = default;
};
// A proved local/skinned compiled variant can require an operation which is
// absent from its world variant. This modifier is independent of the raw
// declaration and is applied only after the source CPU skin matrix blend.
enum class SourceWorldVertexInputTransform : std::uint8_t {
    Preserve, NormalizeSkinNormal, OffsetLocalPosition
};
// Sixteen byte register selectors. Bit7 of byte0 marks an authored mapping;
// Bit6 on POSITION requests the proved offset program's local noise outputs;
// bit6 on NORMAL requests the proved skinned-normal normalization.
// FF leaves that register's constant attribute unchanged. All-FF is the
// legacy/procedural adapter. Ordinary supported semantic IDs are below128.
using PackedSourceVertexDeclaration=std::array<std::uint32_t,4>;
inline constexpr PackedSourceVertexDeclaration legacy_source_vertex_declaration{
    0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu};
bool source_world_vertex_declaration_supported(const SourceVertexDeclaration&);
PackedSourceVertexDeclaration pack_source_vertex_declaration(
    const std::optional<SourceVertexDeclaration>&,
    SourceWorldVertexInputTransform=SourceWorldVertexInputTransform::Preserve);
} // namespace halo2
