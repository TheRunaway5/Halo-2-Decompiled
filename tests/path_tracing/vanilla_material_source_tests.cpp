#include "halo2/path_tracing/surface.hpp"
#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/object_lighting.hpp"
#include "../scalar_combiners.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
}
int main(int argc,char** argv)try {
    require(argc==3,"Supply current opening cache and report path");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);std::mt19937 random(0x1211a);
    std::uniform_real_distribution<float> value(0,1);
    nlohmann::json rows=nlohmann::json::array();unsigned cases{},families[3]{};
    for(const auto& tag:cache.tags()) {
        if(tag.group!="shad" || !tag.address || !tag.name.starts_with("objects\\characters\\"))continue;
        auto material=cache.load_material(tag.datum);material.selected_shader_lod=0;
        if(!prepare_dynamic_material(cache,material,{},true,0.f))continue;
        const auto surface=pt::qualified_surface(material);
        if(!surface || unsigned(surface->family)<8 || unsigned(surface->family)>10)continue;
        ++families[unsigned(surface->family)-8];
        const auto before=material.forward_program.words;
        for(unsigned n=0;n<128;++n) {
            std::array<ShaderVector,4> samples;for(auto& sample:samples)for(auto& c:sample)c=value(random);
            const auto expected=halo2_test::scalar_combiners(before,samples,material.forward_diffuse,material.forward_specular);
            const auto actual=pt::surface_color(surface->family,pt::surface_constants(material,surface->family),samples);
            for(unsigned c=0;c<4;++c)require(std::abs(actual[c]-expected[c])<2.e-6f,
                "Opening character unlit albedo does not match its current complete native shader");
            ++cases;
        }
        rows.push_back({{"shader",tag.name},{"pass",material.passes[material.forward_pass_index].name},
            {"family",unsigned(surface->family)},{"source_program",before}});
    }
    require(families[0] && families[1] && families[2],"Opening scene did not positively cover texture/detail and two/four change colors");
    nlohmann::json report{{"passed",true},{"cache",argv[1]},{"mcc",cache.is_mcc()},
        {"source_cases",cases},{"families",families},{"materials",rows},
        {"scope","Current opening character unlit texture/detail/change-color programs; complete native controls and vertex UV identity. Native source images/runtime unchanged. Does not establish unknown shader-family parity."}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';
    std::cout<<"PASS opening current native material sources: "<<rows.size()<<" shaders, "<<cases<<" scalar cases\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
