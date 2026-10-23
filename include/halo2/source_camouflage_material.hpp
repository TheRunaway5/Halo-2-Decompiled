#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// 2CBF0 stores trunc(primary*256), clamped to a byte, at context+76.
std::uint8_t source_camouflage_fade_byte(float primary);
// 4CBF0 layer18, for a present object-lighting context. The original
// shader's selected LOD, rather than the override's LOD, owns bit15.
unsigned source_camouflage_implementation(std::uint32_t original_layers,
    std::uint8_t fade);
// Exact global override implementation. Extern26 resolves through the
// original part shader; extern17 is a declared live scene-copy dependency.
Material prepare_camouflage_material(const Cache &,const Material &original,
    unsigned layer,unsigned implementation,float primary=0.f);
//133AB0's clone queues the global override as its draw shader. Retain that
//shader's authored variants alongside the replaced root. They are submitted
//only by the camouflage group; ordinary late overlays must omit them.
void prepare_camouflage_materials(const Cache &,Material &);
}
