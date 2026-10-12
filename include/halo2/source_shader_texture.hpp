#pragma once
#include "halo2/source_tag_metadata_store.hpp"
#include <compare>
#include <functional>

namespace halo2 {
class Cache;
struct SourceShaderTextureReference {
    std::uint32_t bitmap{};
    std::uint16_t frame{}; // Exact CX bits supplied to original143C0.
    auto operator<=>(const SourceShaderTextureReference &) const = default;
};
// Original22590/4BCC0: the active matg rasterizer reference at+CC/+D0.
std::uint32_t source_camouflage_shader(const SourceTagMetadataProviders &,
    std::uint32_t globals);
std::uint32_t source_camouflage_shader(const Cache &);
// Original19530 extern1 and26, through the actual bitmap binder's entry.
// Extern26 retains the signed bump-property index and the sole -1 fallback
// through the global camouflage shader. No texture/cache readiness is implied.
SourceShaderTextureReference source_shader_texture_reference(
    const SourceTagMetadataProviders &,std::uint32_t globals,
    std::uint32_t original_shader,unsigned external);
SourceShaderTextureReference source_shader_texture_reference(
    const Cache &,std::uint32_t original_shader,unsigned external);
}
