#include "halo2/source_render_policy.hpp"

namespace halo2 {
bool source_render_layer_enabled(const SourceGeometryLodPolicy &policy,unsigned layer) {
    if(layer==2)return policy.feature_bytes[3]!=0;
    if(layer==8)return policy.feature_bytes[2]!=0;
    if(layer==17)return policy.feature_bytes[4]!=0;
    return true;
}
std::optional<unsigned> source_material_layer(const Material &material) {
    if(!material.forward_pass || material.lods.empty())return {};
    if(material.forward_pass_index>=material.passes.size())
        throw FormatError("source layer pass outside material");
    if(material.selected_shader_lod>=material.lods.size())
        throw FormatError("source layer shader LOD outside authored template");
    const auto &lod=material.lods[material.selected_shader_lod];
    const auto pass=material.passes[material.forward_pass_index].template_pass;
    std::optional<unsigned> selected;
    for(unsigned layer=0;layer<lod.layers.count() && layer<32;++layer) {
        if(!(lod.available_layers&(1u<<layer)))continue;
        const auto index=lod.layers.first()+layer;
        if(index>=material.layers.size())throw FormatError("source layer outside template");
        const auto range=material.layers[index];
        if(pass>=range.first() && pass<range.first()+range.count()) {
            if(selected && *selected!=layer)
                throw FormatError("prepared source pass belongs to multiple layers");
            selected=layer;
        }
    }
    return selected;
}
}
