#include "halo2/pstencil/specular_mask.hpp"
#include "halo2/pstencil/texture_accumulation_2003.hpp"
#include <iostream>
using namespace halo2;
void require(bool v,const char* why){if(!v)throw FormatError(why);}
int main()try {
    Material current;current.source_shader_datum=42;current.forward_pass=true;current.forward_vertex=3;
    current.forward_pass_index=0;current.selected_shader_lod=2;current.passes.resize(1);
    current.passes[0].vertex_shader="verified\\stencil_texture";
    auto low=current;low.selected_shader_lod=0;
    auto combined=current;combined.forward_vertex=6;
    auto converted=low;converted.forward_blend=pstencil::texture_accumulation_blend_2003;
    auto foreign=converted;foreign.source_shader_datum=43;
    auto transformed=converted;transformed.world_vertex_input_transform=SourceWorldVertexInputTransform::NormalizeSkinNormal;
    const std::array original{current,low,combined};
    const auto old=pstencil::texture_mask_materials(original);
    require(old==std::vector<unsigned>{0,1,1},"Current route changed independent authored LOD ownership");
    const std::array all{current,low,combined,foreign,transformed,converted};
    const auto result=pstencil::texture_mask_materials(all);
    require(result==std::vector<unsigned>{5,5,5,3,4,5},"Converted same-shader pass did not reach both independent and combined roots");
    std::cout<<"Independent and combined roots select recovered texture accumulation; foreign shader/CPU transforms stay separate; current route preserves authored LODs\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
