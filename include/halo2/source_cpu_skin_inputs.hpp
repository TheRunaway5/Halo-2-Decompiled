#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// The caller supplies the literal 1CAA0/1CB20 class. This is an adapter from
// CPU-posed streams to a selected world program, never raw skin indices.
SourceWorldVertexInputTransform source_cpu_skin_input_transform(
    std::span<const std::byte> world_program,std::span<const std::byte> source_program,
    unsigned source_class);
// Copy-on-write children keep each selected pass's own original behavior.
// Returns whether any pass needs a distinct material input binding.
bool prepare_cpu_skin_material_inputs(const Cache &,Material &,unsigned source_class);
// Literal DPH o9/o10.xyz in the proved local/skinned offset program. The
// remaining outputs already come from its matching world program over the
// current CPU pose. This does not select a family or normalize its raw normal.
void apply_cpu_skin_texture_coordinates(SourceWorldVertexInputTransform,
    Vec3 local_position,std::span<const ShaderVector> constants,
    std::array<ShaderVector,16> &outputs);
}
