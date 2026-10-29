#include "halo2/cache.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include "halo2/mcc_particle_alpha_multiply_add.hpp"
#include "halo2/compiled_particle_vertex_signature.hpp"
#include "halo2/particle_material.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool value,const char* message) {if(!value)throw FormatError(message);}
void signature_controls() {
    constexpr std::array<unsigned,3> counts{51,63,72};
    constexpr std::array<std::uint64_t,3> hashes{
        0x2b67638b8d57aa48ull,0x93c69296e450b61cull,0x3e8f2d9052454432ull};
    for(unsigned classification=7;classification<=9;++classification) {
        const auto code=compiled_particle_vertex_signature(classification);
        std::uint64_t hash=14695981039346656037ull;
        for(auto byte:code)hash=(hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
        check(hash==hashes[classification-7] && decode_vertex_program(code).count==counts[classification-7],
            "Built-in particle signature differs from the existing custom Vulkan kernel");
    }
    check(compiled_particle_vertex_signature(6).empty() && compiled_particle_vertex_signature(10).empty(),
        "Built-in particle signature invented an unsupported vertex class");
}
void projection_controls() {
    MaterialPass pc;pc.name="shaders\\shader_passes\\transparent\\effect_shader_alpha_multiply_add";
    pc.bitmap_remaps={{0,0,0}};pc.vertex_remaps={{432,0,0},{7279,0,1}};
    pc.textures={{0,0,0,0},{0,0,1,0}};
    pc.states={{168,7},{27,1},{19,2},{20,6},{171,1},{14,0},{175,0},{15,1},{24,1},{25,5},{22,1}};
    pc.constants[0][0]={.25f,.5f,.75f,1.f};pc.constants[1][15]={4.f,3.f,2.f,1.f};
    std::array<std::byte,48> samplers{};
    for(unsigned stage=0;stage<2;++stage) {
        samplers[stage*24]=std::byte{0x20};samplers[stage*24+1]=std::byte{0x22};
        for(unsigned index=2;index<6;++index)samplers[stage*24+index]=std::byte{1};
    }
    const std::array<std::byte,15> defaults{std::byte{15},
        std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{24},
        std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{22},
        std::byte{2},std::byte{0},std::byte{0},std::byte{0}};
    const auto projected=project_mcc_particle_alpha_multiply_add_pass(pc,samplers,defaults);
    check(!native_fragment_kernel_path("particle",projected.program.words).empty(),
        "Qualified projection lost its compiled Vulkan fragment identity");
    check(projected.constants==pc.constants,
        "Particle projection replaced the imported remapped vertex constants");
    const auto rejects=[&](const auto& mutate) {
        auto changed=pc;auto altered=samplers;mutate(changed,altered);bool rejected=false;
        try {(void)project_mcc_particle_alpha_multiply_add_pass(changed,altered,defaults);}
        catch(const FormatError&) {rejected=true;}
        check(rejected,"Changed PC particle descriptor was admitted by the fixed Vulkan projection");
    };
    rejects([](auto& p,auto&) {p.definition.words[0]=1;});
    rejects([](auto& p,auto&) {p.program.words[54]=1;});
    rejects([](auto& p,auto&) {p.states[3].value=5;});
    rejects([](auto& p,auto&) {p.textures[1].parameter=1;});
    rejects([](auto& p,auto&) {p.vertex_remaps[1].destination^=1;});
    rejects([](auto& p,auto&) {p.pixel_externs.push_back({0,0,0});});
    rejects([](auto& p,auto&) {p.state_externs.push_back({0,0,0});});
    rejects([](auto&,auto& s) {s[24+2]^=std::byte{1};});
    auto changed_defaults=defaults;changed_defaults[11]=std::byte{1};
    bool rejected=false;
    try {(void)project_mcc_particle_alpha_multiply_add_pass(pc,samplers,changed_defaults);}
    catch(const FormatError&) {rejected=true;}
    check(rejected,"Changed PC default render states entered the fixed Vulkan projection");
    rejected=false;
    try {(void)project_mcc_particle_alpha_multiply_add_pass(pc,samplers,{});}
    catch(const FormatError&) {rejected=true;}
    check(rejected,"Missing PC default render states entered the fixed Vulkan projection");
    std::cout<<"PASS complete PC particle projection, authored constants, ten changed-descriptor negative controls\n";
}
}
int main(int argc,char** argv)try {
    check(argc==3,"Supply prepared MCC map and owned JSON report path");
    if(!*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    check(cache.is_mcc(),"Particle shader routing requires actual MCC content");
    signature_controls();
    projection_controls();
    nlohmann::json report{{"map",cache.name()},{"passed",false},
        {"scope","Actual breakable particle resource admission; no simulation or framebuffer result"},
        {"particles",nlohmann::json::array()}};
    const auto* native=cache.query_tag({.group=tag_groups::vertex_shader,
        .name="rasterizer\\vertex_shaders_xbox\\particle",.require_address=true});
    const auto* pc=cache.query_tag({.group=tag_groups::vertex_shader,
        .name="rasterizer\\vertex_shaders_dx9\\particle",.require_address=true});
    const bool source_tag=native && cache.is_preserved_native_shader(*native);
    const auto signature=compiled_particle_vertex_signature(7);
    const auto native_code=source_tag?cache.load_vertex_shader_code(native->datum,7):
        std::vector<std::byte>(signature.begin(),signature.end());
    if(source_tag)for(unsigned classification=7;classification<=9;++classification) {
        const auto code=cache.load_vertex_shader_code(native->datum,classification);
        check(std::ranges::equal(code,compiled_particle_vertex_signature(classification)),
            "Built-in particle signature differs from the existing admitted native contract");
    }
    if(cache.name()=="coagulation")check(source_tag && pc,
        "Reported map lost the distinct compiled and PC particle identities");
    bool rejected=false;
    if(pc) {
        try {(void)cache.load_vertex_shader_code(pc->datum,7);}
        catch(const FormatError& error) {rejected=std::string_view(error.what())=="vertex shader is not original Xbox format";}
        check(rejected,"PC particle shader entered the original reference decoder");
    }
    report["pc_vertex_present"]=pc!=nullptr;report["pc_vertex_rejected"]=rejected;
    bool passed=true;unsigned checked{};bool glass{},ice{};
    for(const auto& tag:cache.select_tags({.group=TagGroup::from_string("prt3"),.require_address=true})) {
        const auto payload=cache.tag_payload(tag.handle());const Bytes root(payload);
        if(root.size()<0x94 || !root.u32(0x8c))continue;
        const Bytes definition(cache.read_tag_bytes(root.u32(0x90),124));
        const auto* shader_template=cache.query_tag({.handle=TagHandle{definition.u32(0)},
            .group=TagGroup::from_string("stem"),.require_address=true});
        if(!shader_template || shader_template->name!=
            "shaders\\shader_templates\\transparent\\particle_alpha_multiply_add")continue;
        nlohmann::json row{{"name",tag.name},{"datum",tag.datum},{"passed",false}};
        ++checked;glass|=tag.name=="effects\\generic\\solids\\glass_breakable";
        ice|=tag.name=="effects\\generic\\solids\\ice";
        try {
            // SourceWorldParticleDrawsTu15::load_resource selects this exact
            // layer/pass/implementation after a breakable object is damaged.
            const auto material=cache.load_particle_material(tag.datum,15,0,0);
            check(material.vertex.classification==7 &&
                material.vertex.vertex_datum==(source_tag?native->datum:0xffffffffu) &&
                material.vertex.program==native_code,"Breakable particle selected a different vertex contract");
            std::uint64_t hash=14695981039346656037ull;
            for(const auto byte:material.vertex.program)
                hash=(hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
            check(hash==0x2b67638b8d57aa48ull,"Breakable particle differs from the custom Vulkan class7 vertex contract");
            check(!native_fragment_kernel_path("particle",material.pass.program.words).empty(),
                "Breakable particle has no complete custom Vulkan fragment contract");
            check(material.textures[0] && !material.textures[0]->pixels.empty() && material.bitmaps[0],
                "Breakable particle lost its authored primary bitmap binding");
            check(root.u32(0x14)==1,"Breakable particle's authored parameter bank changed");
            const Bytes parameter(cache.read_tag_bytes(root.u32(0x18),40));
            check(cache.string_id(parameter.u32(0))=="alpha_map_texture" &&
                parameter.u32(8)==tag_groups::bitmap.value && material.bitmaps[0]->datum==parameter.u32(12) &&
                material.bitmaps[1] && material.bitmaps[1]->datum==material.bitmaps[0]->datum,
                "Breakable particle projection replaced its imported alpha-map identity");
            if(source_tag) {
                const auto declaration=cache.load_source_vertex_declaration(material.vertex.vertex_datum,7);
                check(!declaration.semantics.empty(),"Breakable particle lost its vertex declaration");
            } else check(material.pass.vertex_shader=="custom_vulkan\\particle",
                "Built-in particle program claims an unavailable source shader tag");
            row["source_vertex_tag_present"]=source_tag;
            row["vertex_datum"]=material.vertex.vertex_datum;
            row["bitmap_datum"]=material.bitmaps[0]->datum;
            row["fragment"]=native_fragment_kernel_path("particle",material.pass.program.words);
            row["passed"]=true;
            std::cout<<"PASS "<<tag.name<<'\n';
        }catch(const std::exception& error) {
            row["error"]=error.what();passed=false;
            std::cerr<<"FAIL "<<tag.name<<": "<<error.what()<<'\n';
        }
        report["particles"].push_back(std::move(row));
    }
    check(checked>0,"Actual cache contains no alpha-multiply-add particle coverage");
    if(cache.name()=="coagulation")check(checked>=2 && glass && ice,
        "Reported map no longer covers both breakable particle owners");
    report["checked"]=checked;report["passed"]=passed;
    std::ofstream output(argv[2]);check(bool(output),"Unable to save particle routing report");
    output<<report.dump(2)<<'\n';output.close();check(bool(output),"Unable to finish particle routing report");
    return passed?0:1;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
