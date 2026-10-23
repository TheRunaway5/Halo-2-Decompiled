#include "halo2/world_material.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
nlohmann::json read(const char *path){std::ifstream file(path);check(bool(file),"Missing original camouflage state evidence");return nlohmann::json::parse(file);}
std::uint32_t word(const std::string &hex,std::size_t at) {
    std::uint32_t value=0;
    for(unsigned n=0;n<4;++n)value|=std::uint32_t(std::stoul(hex.substr((at+n)*2,2),nullptr,16))<<(n*8);
    return value;
}
std::vector<ShaderRemap> mappings(const nlohmann::json &rows) {
    std::vector<ShaderRemap> result;
    for(const auto &row:rows) {
        const auto raw=word(row,0);
        result.push_back({std::uint16_t(raw),std::uint8_t(raw>>16),std::uint8_t(raw>>24)});
    }
    return result;
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply original state compositions and original pass banks");
    const auto binder=read(argv[1]),passes=read(argv[2]);
    unsigned cases=0;
    for(const auto &row:binder.at("state_cases")) {
        const auto original_case=row.at("prior_case").get<unsigned>();
        const auto source_case=passes.contains("case_indices")?passes.at("case_indices").at(original_case).get<unsigned>():original_case;
        const auto &prior=passes.at("cases").at(source_case);
        Material material;material.forward_pass=true;material.forward_pass_index=0;material.passes.emplace_back();
        auto &pass=material.passes[0];
        const auto initial=prior.at("initial_vertex_hex").get<std::string>();
        const auto expected=prior.at(row.at("layer")==18?"vertex_hex":"initial_vertex_hex").get<std::string>();
        for(unsigned bank=0;bank<2;++bank)for(unsigned reg=0;reg<16;++reg)for(unsigned axis=0;axis<4;++axis)
            pass.constants[bank][reg][axis]=std::bit_cast<float>(word(initial,bank*256+reg*16+axis*4));
        material.program_constants=pass.constants;
        pass.pixel_externs=mappings(prior.at("mappings").at("pixel"));
        // Other camera/fog extern providers retain separate native owners.
        for(const auto &mapping:mappings(prior.at("mappings").at("vertex")))
            if(mapping.source==37)pass.vertex_externs.push_back(mapping);
        pass.state_externs=mappings(row.at("mappings"));
        for(unsigned n=0;n<60;++n)pass.program.words[n]=word(prior.at("initial_pixel_hex"),n*4);
        material.forward_program=pass.program;
        material.forward_blend_color=row.at("initial_state75");
        // A pass which does not request color40 leaves that provider bank
        // untouched. Scalar37 is the camouflage input for the pixel/vertex
        // consumers, including passes with an empty render-state range.
        const auto input=std::bit_cast<float>(prior.at("scalar_bits").at("37").get<std::uint32_t>());
        auto alias_program=material.forward_program;auto alias_constants=material.program_constants;
        auto alias_blend=material.forward_blend_color;
        bind_object_camouflage_constants(alias_program,alias_constants,alias_blend,pass,
            material.shader_switches,input,row.at("requested_fpcw"));
        bind_object_camouflage(material,input,row.at("requested_fpcw"));
        check(alias_blend==row.at("state75") && alias_program.words==material.forward_program.words &&
            !std::memcmp(&alias_constants,&material.program_constants,sizeof(alias_constants)),
            "Independent draw camouflage state differs from original composition");
        check(material.forward_blend_color==row.at("state75"),"Native camouflage state75 differs from whole original composition");
        for(unsigned bank=0;bank<2;++bank)for(unsigned reg=0;reg<16;++reg)for(unsigned axis=0;axis<4;++axis) {
            const auto bits=word(expected,bank*256+reg*16+axis*4);
            check(std::bit_cast<std::uint32_t>(material.program_constants[bank][reg][axis])==bits &&
                std::bit_cast<std::uint32_t>(pass.constants[bank][reg][axis])==bits,
                "Native camouflage live/retained vertex bank differs");
        }
        for(unsigned n=0;n<60;++n) {
            const auto bits=word(prior.at("pixel_hex"),n*4);
            check(material.forward_program.words[n]==bits && pass.program.words[n]==bits,
                "Native camouflage live/retained pixel bank differs");
        }
        ++cases;
    }
    check(cases==5040,"Native camouflage state composition coverage changed");
    std::cout<<cases<<" original color40/state75 compositions and scalar37 live/retained banks passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
