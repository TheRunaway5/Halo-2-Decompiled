#pragma once
#include "halo2/cache.hpp"

namespace halo2::detail {
// 2BCD0 composites atmospheric fog before authored layer15. Keep the legacy
// blend fallback only for procedural passes without a selected template layer.
inline bool material_after_fog(const Material &material) {
    bool late=material.forward_blend!=0;
    if(material.forward_pass && !material.lods.empty()) {
        if(material.selected_shader_lod>=material.lods.size())
            throw FormatError("fog scheduling shader LOD outside authored template");
        const auto &lod=material.lods[material.selected_shader_lod];
        if(material.forward_pass_index>=material.passes.size())
            throw FormatError("fog scheduling pass outside material");
        const auto pass=material.passes[material.forward_pass_index].template_pass;
        bool selected=false;
        for(unsigned layer=0;layer<lod.layers.count() && layer<32;++layer) {
            if(!(lod.available_layers & (1u<<layer)))continue;
            const auto index=lod.layers.first()+layer;
            if(index>=material.layers.size())throw FormatError("fog scheduling layer outside template");
            const auto range=material.layers[index];
            if(pass>=range.first() && pass<range.first()+range.count()) {
                if(selected && late!=(layer==15))
                    throw FormatError("fog scheduling pass belongs to incompatible layers");
                late=layer==15;selected=true;
            }
        }
    }
    return late;
}
}
