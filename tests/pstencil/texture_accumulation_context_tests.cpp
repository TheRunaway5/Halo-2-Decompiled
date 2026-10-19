#include "halo2/pstencil/texture_accumulation_2003.hpp"
#include "halo2/pstencil/detail.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
void require(bool v,const char* why){if(!v)throw FormatError(why);}
int main(int argc,char** argv)try {
    require(argc==2,"Supply actual current texture bindings");nlohmann::json rows;std::ifstream(argv[1])>>rows;
    const auto found=std::find_if(rows.begin(),rows.end(),[](const auto& r){return r["shader"]=="objects\\characters\\elite\\shaders\\torso";});
    require(found!=rows.end(),"Actual Elite texture program missing");Material m;
    m.name=(*found)["shader"];m.forward_pass=true;m.forward_vertex=3;m.forward_pass_index=0;
    m.vertex_program={std::byte{1}};m.forward_program.words=(*found)["program"].template get<std::array<std::uint32_t,60>>();m.passes.resize(1);
    m.passes[0].vertex_shader=(*found)["vertex"];m.passes[0].name="shaders\\shader_passes\\texture_mapping\\default_texture_two_change_color_zwrite";
    const Texture sample{1,1,11,std::vector<std::byte>{std::byte{29},std::byte{34},std::byte{39},std::byte{73}}, {}};
    m.diffuse=m.detail=m.auxiliary=sample;const auto detail=pstencil::reference_neutral_detail_stage(m);
    require(detail==3,"Actual source-backed Elite neutral-detail selector missing");
    const auto converted=pstencil::adapt_texture_accumulation_2003(m,detail);
    const std::vector<std::byte> neutral{std::byte{128},std::byte{128},std::byte{128},std::byte{255}};
    require(converted.detail.pixels==neutral,"Converted Elite pass lost completed source neutral-detail policy");
    require(m.detail.pixels==sample.pixels,"Conversion mutated Original material detail");
    auto ordinary=m;ordinary.name="objects\\characters\\dervish\\shaders\\dervish_armor";
    const auto preserved=pstencil::adapt_texture_accumulation_2003(ordinary,pstencil::reference_neutral_detail_stage(ordinary));
    require(preserved.detail.pixels==sample.pixels,"Elite detail policy escaped to Arbiter armor");
    std::cout<<"Converted Elite preserves primary RGBA128/255 detail input; Original and Arbiter detail remain authored\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
