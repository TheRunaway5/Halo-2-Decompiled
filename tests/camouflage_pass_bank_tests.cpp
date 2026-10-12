#include "halo2/world_material.hpp"
#include "halo2/source_camouflage_context.hpp"
#include "halo2/source_shader_fog.hpp"
#include "halo2/source_shader_texture.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const std::string &message){if(!value)throw FormatError(message);}
std::uint32_t word(const std::string &hex,std::size_t at) {
    std::uint32_t value=0;
    for(unsigned n=0;n<4;++n)value|=std::uint32_t(std::stoul(hex.substr((at+n)*2,2),nullptr,16))<<(n*8);
    return value;
}
float number(const nlohmann::json &bits){return std::bit_cast<float>(bits.get<std::uint32_t>());}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply Lockout and complete original binder report");
    Cache cache(argv[1]);const auto global=cache.load_material(source_camouflage_shader(cache));
    std::ifstream input(argv[2]);check(bool(input),"Original binder report missing");
    const auto report=nlohmann::json::parse(input);
    check(global.animation.parameters.empty(),"Whole-bank corpus has unbound animated parameters");
    const auto &object=report.at("inputs").at("object");
    const auto datum=object.at("datum").get<std::uint32_t>();
    const auto body=object.at("body_hex").get<std::string>();
    const auto type=std::uint8_t(std::stoul(body.substr(0xaa*2,2),nullptr,16));
    const auto parent=word(body,0x14);
    unsigned index=0,complete=0,fog_cases=0;
    for(const auto &row:report.at("cases")) {
        const auto label="Original whole binder case "+std::to_string(index++);
        if(!row.at("complete_original_return").get<bool>())continue;
        auto material=global;const unsigned layer=row.at("layer"),implementation=row.at("implementation");
        const auto range=material.layers.at(material.lods.at(0).layers.first()+layer);
        const auto selected=std::find_if(material.passes.begin(),material.passes.end(),[&](const auto &pass){
            return pass.template_pass>=range.first() && pass.template_pass-range.first()<range.count() &&
                pass.implementation==implementation;
        });
        check(selected!=material.passes.end(),label+": authored pass missing");
        material.forward_pass=true;material.forward_pass_index=unsigned(selected-material.passes.begin());
        auto &pass=material.passes[material.forward_pass_index];
        const auto before=row.at("initial_bank_hex").get<std::string>();
        const auto after=row.at("final_bank_hex").get<std::string>();
        // Whole16B90 preserves unmentioned vertex words from its caller's
        // bank. Start from that declared original bank, then apply actual
        // authored parameter remaps rather than assuming zero registers.
        for(unsigned bank=0;bank<2;++bank)for(unsigned reg=0;reg<16;++reg)for(unsigned axis=0;axis<4;++axis)
            material.program_constants[bank][reg][axis]=std::bit_cast<float>(word(before,0x1530+bank*256+reg*16+axis*4));
        remap_vertex_constants(material.program_constants,material.vertex_constants,pass.vertex_remaps);
        pass.constants=material.program_constants;material.forward_program=pass.program;
        material.forward_blend_color=row.at("state75_at_binder_entry");
        for(const auto &state:pass.states)if(state.index==75)material.forward_blend_color=state.value;
        const SourceCamouflageContextServices owners{
            [&](std::uint32_t value)->std::optional<SourceCamouflageObjectView>{
                if(value==datum)return SourceCamouflageObjectView{type,parent};return {};
            },{},[&](std::uint32_t value){check(value==datum,label+": wrong primary owner");return number(row.at("primary_bits"));}
        };
        bind_object_camouflage(material,source_object_camouflage(datum,owners));
        const auto &record=row.at("fog_context_before");
        SourceShaderFogContext fog{record.at("datum"),number(record.at("near_bits")),number(record.at("far_bits")),{}, {}};
        for(unsigned axis=0;axis<3;++axis) {
            fog.camera_forward[axis]=number(record.at("forward_bits").at(axis));
            fog.default_color[axis]=std::bit_cast<float>(word(record.at("default_rgb_hex"),axis*4));
        }
        bind_camouflage_fog_pass(material,fog);
        if(layer==19 && fog.datum!=0xffffffffu)++fog_cases;
        check(material.forward_blend_color==row.at("state75"),label+": state75 differs");
        for(unsigned n=0;n<60;++n) {
            const auto expected=word(after,0x1424+n*4);
            check(material.forward_program.words[n]==expected && pass.program.words[n]==expected,
                label+": live/retained pixel word "+std::to_string(n)+" differs");
        }
        for(unsigned bank=0;bank<2;++bank)for(unsigned reg=0;reg<16;++reg)for(unsigned axis=0;axis<4;++axis) {
            const auto expected=word(after,0x1530+bank*256+reg*16+axis*4);
            check(std::bit_cast<std::uint32_t>(material.program_constants[bank][reg][axis])==expected &&
                std::bit_cast<std::uint32_t>(pass.constants[bank][reg][axis])==expected,
                label+": live/retained vertex register differs");
        }
        ++complete;
    }
    check(complete==64 && fog_cases==4,"Whole original binder bank coverage changed");
    std::cout<<complete<<" complete original pass banks matched all live/retained pixel/vertex words and state75; "
        <<fog_cases<<" valid fog contexts used native scalar39/color41 providers\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
