#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// These exact typed owners are prepared by prepare_water_material or the
// original layer6 water-edge branch. A water flag on an ordinary shader does
// not move it out of ordinary source-depth admission. Cache/pass identity is
// independently checked by SourceWorldDepthInput before retaining this role.
inline bool source_world_depth_independent_water(const Material& material) {
    if(!material.water_pass || !material.forward_pass ||
       material.forward_pass_index>=material.passes.size())return false;
    const auto& shader=material.passes[material.forward_pass_index].vertex_shader;
    if(material.passes[material.forward_pass_index].implementation!=0)return false;
    return (material.forward_vertex==7 &&
            (shader.ends_with("\\water_pool") || shader.ends_with("\\water_pool_static"))) ||
           (material.forward_vertex==8 &&
            (shader.ends_with("\\water") || shader.ends_with("\\water_static"))) ||
           (material.forward_vertex==3 && shader.ends_with("\\water_edge_blend"));
}
}
